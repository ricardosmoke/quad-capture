#include <os/log.h>
#include <string.h>

#include <DriverKit/IOLib.h>

#include "UA55UsbStream.h"
#include "UA55AudioDriver.h"

extern "C" void UA55AudioDevicePublishTimestamp(void* device, uint64_t sampleTime, uint64_t hostTime);

namespace {

uint8_t InterfaceNumber(IOUSBHostInterface* iface)
{
    const IOUSBConfigurationDescriptor* config = iface->CopyConfigurationDescriptor();
    if (config == nullptr) {
        return 0xFF;
    }
    const IOUSBInterfaceDescriptor* descriptor = iface->GetInterfaceDescriptor(config);
    const uint8_t number = descriptor != nullptr ? descriptor->bInterfaceNumber : 0xFF;
    IOUSBHostFreeDescriptor(config);
    return number;
}

kern_return_t CollectInterfaces(
    IOUSBHostDevice* device,
    IOUSBHostInterface** playbackOut,
    IOUSBHostInterface** captureOut,
    IOUSBHostInterface** midiOut,
    IOUSBHostInterface** statusAOut,
    IOUSBHostInterface** statusBOut)
{
    *playbackOut = nullptr;
    *captureOut = nullptr;
    *midiOut = nullptr;
    *statusAOut = nullptr;
    *statusBOut = nullptr;

    uintptr_t iterator = 0;
    kern_return_t result = device->CreateInterfaceIterator(&iterator);
    if (result != kIOReturnSuccess) {
        return result;
    }

    for (;;) {
        IOUSBHostInterface* iface = nullptr;
        result = device->CopyInterface(iterator, &iface);
        if (result != kIOReturnSuccess) {
            break;
        }
        if (iface == nullptr) {
            result = kIOReturnSuccess;
            break;
        }

        const uint8_t number = InterfaceNumber(iface);
        if (number == kUA55PlaybackInterface && *playbackOut == nullptr) {
            *playbackOut = iface;
            continue;
        }
        if (number == kUA55CaptureInterface && *captureOut == nullptr) {
            *captureOut = iface;
            continue;
        }
        if (number == kUA55MidiInterface && *midiOut == nullptr) {
            *midiOut = iface;
            continue;
        }
        if (number == kUA55StatusAInterface && *statusAOut == nullptr) {
            *statusAOut = iface;
            continue;
        }
        if (number == kUA55StatusBInterface && *statusBOut == nullptr) {
            *statusBOut = iface;
            continue;
        }
        OSSafeReleaseNULL(iface);
    }

    device->DestroyInterfaceIterator(iterator);

    if (*playbackOut == nullptr || *captureOut == nullptr) {
        OSSafeReleaseNULL(*playbackOut);
        OSSafeReleaseNULL(*captureOut);
        OSSafeReleaseNULL(*midiOut);
        OSSafeReleaseNULL(*statusAOut);
        OSSafeReleaseNULL(*statusBOut);
        return kIOReturnNotFound;
    }
    return kIOReturnSuccess;
}

} // namespace

UA55UsbStream::UA55UsbStream() = default;

UA55UsbStream::~UA55UsbStream()
{
    TearDown(client_ != nullptr ? client_ : nullptr);
    FreeBridges();
}

bool UA55UsbStream::EnsureBridges()
{
    if (playbackBridge_ != nullptr && captureBridge_ != nullptr) {
        return true;
    }
    FreeBridges();
    const size_t playBytes = (size_t)bridgeFrames_ * kUA55OutputChannels * sizeof(int32_t);
    const size_t capBytes = (size_t)bridgeFrames_ * kUA55InputChannels * sizeof(int32_t);
    playbackBridge_ = (int32_t*)IOMalloc(playBytes);
    captureBridge_ = (int32_t*)IOMalloc(capBytes);
    if (playbackBridge_ == nullptr || captureBridge_ == nullptr) {
        FreeBridges();
        return false;
    }
    memset(playbackBridge_, 0, playBytes);
    memset(captureBridge_, 0, capBytes);
    return true;
}

void UA55UsbStream::FreeBridges()
{
    if (playbackBridge_ != nullptr) {
        IOFree(playbackBridge_, (size_t)bridgeFrames_ * kUA55OutputChannels * sizeof(int32_t));
        playbackBridge_ = nullptr;
    }
    if (captureBridge_ != nullptr) {
        IOFree(captureBridge_, (size_t)bridgeFrames_ * kUA55InputChannels * sizeof(int32_t));
        captureBridge_ = nullptr;
    }
}

void UA55UsbStream::FreeIsochSlot(IsochSlot* slot)
{
    OSSafeReleaseNULL(slot->action);
    OSSafeReleaseNULL(slot->frameListBuffer);
    OSSafeReleaseNULL(slot->dataBuffer);
    slot->slotIndex = 0;
}

void UA55UsbStream::FreeStatusSlot(StatusSlot* slot)
{
    OSSafeReleaseNULL(slot->action);
    OSSafeReleaseNULL(slot->dataBuffer);
    slot->pipeIndex = 0;
    slot->slotIndex = 0;
}

kern_return_t UA55UsbStream::PrepareStatusSlot(StatusSlot* slot, uint32_t pipeIndex, uint32_t slotIndex)
{
    FreeStatusSlot(slot);
    slot->pipeIndex = pipeIndex;
    slot->slotIndex = slotIndex;

    kern_return_t result = IOBufferMemoryDescriptor::Create(
        kIOMemoryDirectionIn, kUA55StatusMaxPacket, 0, &slot->dataBuffer);
    if (result != kIOReturnSuccess || slot->dataBuffer == nullptr) {
        FreeStatusSlot(slot);
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    result = slot->dataBuffer->SetLength(kUA55StatusMaxPacket);
    if (result != kIOReturnSuccess) {
        FreeStatusSlot(slot);
        return result;
    }
    return kIOReturnSuccess;
}

void UA55UsbStream::FreeMidiSlot(MidiSlot* slot)
{
    OSSafeReleaseNULL(slot->action);
    OSSafeReleaseNULL(slot->dataBuffer);
    slot->slotIndex = 0;
}

kern_return_t UA55UsbStream::PrepareMidiSlot(MidiSlot* slot, uint32_t slotIndex)
{
    FreeMidiSlot(slot);
    slot->slotIndex = slotIndex;

    kern_return_t result = IOBufferMemoryDescriptor::Create(
        kIOMemoryDirectionIn, kUA55MidiMaxPacket, 0, &slot->dataBuffer);
    if (result != kIOReturnSuccess || slot->dataBuffer == nullptr) {
        FreeMidiSlot(slot);
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    result = slot->dataBuffer->SetLength(kUA55MidiMaxPacket);
    if (result != kIOReturnSuccess) {
        FreeMidiSlot(slot);
        return result;
    }
    return kIOReturnSuccess;
}

kern_return_t UA55UsbStream::PrepareIsochSlot(
    IsochSlot* slot,
    bool directionIn,
    uint16_t maxPacket,
    uint32_t slotIndex)
{
    FreeIsochSlot(slot);
    slot->slotIndex = slotIndex;

    const uint64_t dataCapacity = (uint64_t)maxPacket * (uint64_t)kUA55IsochFramesPerTransfer;
    const uint64_t frameListCapacity = sizeof(IOUSBIsochronousFrame) * (uint64_t)kUA55IsochFramesPerTransfer;
    const uint64_t bufferDirection = directionIn ? kIOMemoryDirectionIn : kIOMemoryDirectionOut;

    kern_return_t result = IOBufferMemoryDescriptor::Create(bufferDirection, dataCapacity, 0, &slot->dataBuffer);
    if (result != kIOReturnSuccess || slot->dataBuffer == nullptr) {
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    result = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, frameListCapacity, 0, &slot->frameListBuffer);
    if (result != kIOReturnSuccess || slot->frameListBuffer == nullptr) {
        FreeIsochSlot(slot);
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }

    result = slot->dataBuffer->SetLength(dataCapacity);
    if (result != kIOReturnSuccess) {
        FreeIsochSlot(slot);
        return result;
    }
    result = slot->frameListBuffer->SetLength(frameListCapacity);
    if (result != kIOReturnSuccess) {
        FreeIsochSlot(slot);
        return result;
    }

    IOAddressSegment dataRange = {};
    result = slot->dataBuffer->GetAddressRange(&dataRange);
    if (result != kIOReturnSuccess || dataRange.address == 0) {
        FreeIsochSlot(slot);
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    memset(reinterpret_cast<void*>(dataRange.address), 0, (size_t)dataCapacity);
    return kIOReturnSuccess;
}

kern_return_t UA55UsbStream::FillFrameList(IsochSlot* slot, uint16_t maxPacket)
{
    IOAddressSegment frameRange = {};
    const kern_return_t result = slot->frameListBuffer->GetAddressRange(&frameRange);
    if (result != kIOReturnSuccess) {
        return result;
    }

    IOUSBIsochronousFrame* frames = reinterpret_cast<IOUSBIsochronousFrame*>(frameRange.address);
    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        frames[index].status = (IOReturn)kIOReturnInvalid;
        frames[index].requestCount = maxPacket;
        frames[index].completeCount = 0;
        frames[index].reserved = 0;
        frames[index].timeStamp = 0;
    }
    return kIOReturnSuccess;
}

uint32_t UA55UsbStream::SumCompleteBytes(IsochSlot* slot)
{
    IOAddressSegment frameRange = {};
    if (slot->frameListBuffer->GetAddressRange(&frameRange) != kIOReturnSuccess) {
        return 0;
    }
    IOUSBIsochronousFrame* frames = reinterpret_cast<IOUSBIsochronousFrame*>(frameRange.address);
    uint32_t total = 0;
    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        total += frames[index].completeCount;
    }
    return total;
}

uint32_t UA55UsbStream::UnpackCaptureToBridge(IsochSlot* slot)
{
    // IN isoc → anel S32 (o HAL lê este anel no IOOperationHandler BeginRead).
    if (captureBridge_ == nullptr || bridgeFrames_ == 0) {
        return 0;
    }

    IOAddressSegment frameRange = {};
    IOAddressSegment dataRange = {};
    if (slot->frameListBuffer->GetAddressRange(&frameRange) != kIOReturnSuccess ||
        slot->dataBuffer->GetAddressRange(&dataRange) != kIOReturnSuccess ||
        frameRange.address == 0 || dataRange.address == 0) {
        return 0;
    }

    IOUSBIsochronousFrame* frames = reinterpret_cast<IOUSBIsochronousFrame*>(frameRange.address);
    const uint8_t* data = reinterpret_cast<const uint8_t*>(dataRange.address);
    const uint32_t bytesPerAudioFrame = kUA55InputChannels * kUA55BytesPerSample;
    uint32_t byteOffset = 0;
    uint32_t totalAudioFrames = 0;

    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        const uint32_t complete = frames[index].completeCount;
        const uint32_t request = frames[index].requestCount != 0
            ? frames[index].requestCount
            : kUA55CaptureMaxPacketAlt1;

        if (complete >= bytesPerAudioFrame && (complete % bytesPerAudioFrame) == 0) {
            const uint32_t nFrames = complete / bytesPerAudioFrame;
            const int32_t* src = reinterpret_cast<const int32_t*>(data + byteOffset);
            for (uint32_t frame = 0; frame < nFrames; frame++) {
                const uint32_t ringIndex =
                    (uint32_t)((captureWriteSample_ + totalAudioFrames + frame) % bridgeFrames_);
                int32_t* dst = &captureBridge_[ringIndex * kUA55InputChannels];
                const int32_t* frameSrc = &src[frame * kUA55InputChannels];
                for (uint32_t ch = 0; ch < kUA55InputChannels; ch++) {
                    dst[ch] = frameSrc[ch];
                }
            }
            totalAudioFrames += nFrames;
        }

        byteOffset += request;
    }

    captureWriteSample_ += totalAudioFrames;
    if (totalAudioFrames >= 40u && totalAudioFrames <= 48u) {
        lastCaptureAudioFrames_ = totalAudioFrames;
    }
    MaybePublishZts();
    return totalAudioFrames;
}

void UA55UsbStream::MaybePublishZts()
{
    if (!streaming_ || sampleTime_ == nullptr || timestampTarget_ == nullptr) {
        return;
    }
    if (captureCompletions_ < kUA55ZtsWarmupCompletions) {
        return;
    }
    if (captureWriteSample_ <= kUA55CapturePublishSlackFrames) {
        return;
    }

    // Publicar atrás da ponta USB para o BeginRead não correr com o IsochIO.
    const uint64_t available = captureWriteSample_ - kUA55CapturePublishSlackFrames;
    const uint64_t aligned =
        (available / kUA55ZeroTimestampPeriod) * kUA55ZeroTimestampPeriod;
    if (aligned == 0 || aligned <= lastPublishedZts_) {
        return;
    }

    lastPublishedZts_ = aligned;
    *sampleTime_ = aligned;
    lastHostTime_ = mach_absolute_time();
    if (aligned == kUA55ZeroTimestampPeriod ||
        (aligned % (kUA55ZeroTimestampPeriod * 64)) == 0) {
        os_log(OS_LOG_DEFAULT,
               "[UA55] build=%u ZTS sample=%llu usbWrite=%llu capUnderrun=%llu capOverrun=%llu playUnderrun=%llu",
               kUA55DriverBuild,
               aligned,
               captureWriteSample_,
               captureUnderruns_,
               captureOverruns_,
               underruns_);
    }
    UA55AudioDevicePublishTimestamp(timestampTarget_, aligned, lastHostTime_);
}

void UA55UsbStream::HalReadInput(float* inputRing,
                                 uint32_t ringFrames,
                                 uint64_t sampleTime,
                                 uint32_t frameCount)
{
    if (captureBridge_ == nullptr || inputRing == nullptr || ringFrames == 0 || frameCount == 0) {
        return;
    }

    const uint64_t writeTip = captureWriteSample_;
    for (uint32_t i = 0; i < frameCount; i++) {
        const uint64_t absSample = sampleTime + i;
        const uint32_t dstIndex = (uint32_t)((sampleTime + i) % ringFrames);
        float* dst = &inputRing[dstIndex * kUA55InputChannels];

        // Ainda não escrito pelo USB (ou dentro da margem de corrida).
        if (absSample + 16ull >= writeTip) {
            captureUnderruns_++;
            for (uint32_t ch = 0; ch < kUA55InputChannels; ch++) {
                dst[ch] = 0.0f;
            }
            continue;
        }
        // Já reescrito no anel (HAL atrasado demais).
        if (writeTip - absSample > bridgeFrames_) {
            captureOverruns_++;
            for (uint32_t ch = 0; ch < kUA55InputChannels; ch++) {
                dst[ch] = 0.0f;
            }
            continue;
        }

        const uint32_t srcIndex = (uint32_t)(absSample % bridgeFrames_);
        const int32_t* src = &captureBridge_[srcIndex * kUA55InputChannels];
        for (uint32_t ch = 0; ch < kUA55InputChannels; ch++) {
            dst[ch] = S24In32ToFloat(src[ch]);
        }
    }
}

uint32_t UA55UsbStream::FillPlaybackTransfer(IsochSlot* slot)
{
    IOAddressSegment frameRange = {};
    IOAddressSegment dataRange = {};
    if (slot->frameListBuffer->GetAddressRange(&frameRange) != kIOReturnSuccess ||
        slot->dataBuffer->GetAddressRange(&dataRange) != kIOReturnSuccess ||
        frameRange.address == 0 || dataRange.address == 0) {
        return 0;
    }

    IOUSBIsochronousFrame* frames = reinterpret_cast<IOUSBIsochronousFrame*>(frameRange.address);
    int32_t* data = reinterpret_cast<int32_t*>(dataRange.address);
    uint32_t totalFrames = 0;
    const uint32_t bytesPerFrame = kUA55OutputChannels * kUA55BytesPerSample;

    int64_t drift = 0;
    // Alinhar OUT à ponta do HAL (não ao capture): o WriteEnd é a fonte da verdade.
    if (halWriteSample_ > kUA55PlaybackReadSlackFrames) {
        const uint64_t target = halWriteSample_ - kUA55PlaybackReadSlackFrames;
        drift = (int64_t)playbackReadSample_ - (int64_t)target;
        lastLoggedDrift_ = drift;
        if (drift > (int64_t)kUA55PlaybackResyncThreshold ||
            drift < -(int64_t)kUA55PlaybackResyncThreshold) {
            playbackReadSample_ = target;
            playbackResyncs_++;
            drift = 0;
            lastLoggedDrift_ = 0;
        }
    } else if (captureWriteSample_ > kUA55PlaybackReadSlackFrames) {
        const uint64_t target = captureWriteSample_ - kUA55PlaybackReadSlackFrames;
        drift = (int64_t)playbackReadSample_ - (int64_t)target;
        lastLoggedDrift_ = drift;
    }

    uint32_t remaining = 0;
    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        playbackPhase_ += kUA55SampleRateInt;
        uint32_t samples = playbackPhase_ / kUA55HighSpeedUframesPerSecond;
        playbackPhase_ %= kUA55HighSpeedUframesPerSecond;
        if (samples < 5u) {
            samples = 5u;
        } else if (samples > 6u) {
            samples = 6u;
        }
        remaining += samples;
    }
    if (lastCaptureAudioFrames_ >= 40u && lastCaptureAudioFrames_ <= 48u) {
        remaining = lastCaptureAudioFrames_;
    }
    if (drift < -16) {
        remaining += 1u;
    } else if (drift > 16 && remaining > 40u) {
        remaining -= 1u;
    }
    if (remaining < 40u) {
        remaining = 40u;
    } else if (remaining > 48u) {
        remaining = 48u;
    }

    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        const uint32_t uframesLeft = kUA55IsochFramesPerTransfer - index;
        uint32_t samples = remaining / uframesLeft;
        if (samples < 5u) {
            samples = 5u;
        } else if (samples > 6u) {
            samples = 6u;
        }
        if (samples > remaining) {
            samples = remaining;
        }
        remaining -= samples;

        frames[index].status = (IOReturn)kIOReturnInvalid;
        frames[index].requestCount = (uint16_t)(samples * bytesPerFrame);
        frames[index].completeCount = 0;
        frames[index].reserved = 0;
        frames[index].timeStamp = 0;

        for (uint32_t frame = 0; frame < samples; frame++) {
            int32_t* dst = &data[(totalFrames + frame) * kUA55OutputChannels];
            if (playbackBridge_ == nullptr || bridgeFrames_ == 0) {
                for (uint32_t ch = 0; ch < kUA55OutputChannels; ch++) {
                    dst[ch] = 0;
                }
                continue;
            }
            // Sem HAL ativo (pause) ou ainda sem WriteEnd: silêncio, sem contar underrun.
            const uint64_t absSample = playbackReadSample_ + totalFrames + frame;
            if (timestampTarget_ == nullptr || halWriteSample_ == 0) {
                for (uint32_t ch = 0; ch < kUA55OutputChannels; ch++) {
                    dst[ch] = 0;
                }
                continue;
            }
            if (absSample >= halWriteSample_) {
                underruns_++;
                for (uint32_t ch = 0; ch < kUA55OutputChannels; ch++) {
                    dst[ch] = 0;
                }
                continue;
            }
            const uint32_t ringIndex = (uint32_t)(absSample % bridgeFrames_);
            const int32_t* src = &playbackBridge_[ringIndex * kUA55OutputChannels];
            for (uint32_t ch = 0; ch < kUA55OutputChannels; ch++) {
                dst[ch] = src[ch];
            }
        }
        totalFrames += samples;
    }

    playbackReadSample_ += totalFrames;
    return totalFrames;
}

void UA55UsbStream::SetOutputPairGain(uint32_t pair, float linearGain)
{
    if (pair > 1) {
        return;
    }
    if (linearGain < 0.0f) {
        linearGain = 0.0f;
    } else if (linearGain > 1.0f) {
        linearGain = 1.0f;
    }
    uint32_t bits = 0;
    memcpy(&bits, &linearGain, sizeof(bits));
    __atomic_store_n(&outputGainBits_[pair], bits, __ATOMIC_RELAXED);
}

void UA55UsbStream::SetOutputPairMuted(uint32_t pair, bool muted)
{
    if (pair > 1) {
        return;
    }
    __atomic_store_n(&outputMutePair_[pair], muted ? 1u : 0u, __ATOMIC_RELAXED);
}

void UA55UsbStream::SetOutputMasterMuted(bool muted)
{
    __atomic_store_n(&outputMuteMaster_, muted ? 1u : 0u, __ATOMIC_RELAXED);
}

float UA55UsbStream::LoadOutputPairGain(uint32_t pair) const
{
    if (pair > 1) {
        return 1.0f;
    }
    const uint32_t bits = __atomic_load_n(&outputGainBits_[pair], __ATOMIC_RELAXED);
    float gain = 1.0f;
    memcpy(&gain, &bits, sizeof(gain));
    return gain;
}

void UA55UsbStream::HalWriteOutput(const float* outputRing,
                                   uint32_t ringFrames,
                                   uint64_t sampleTime,
                                   uint32_t frameCount)
{
    if (playbackBridge_ == nullptr || outputRing == nullptr || ringFrames == 0 || frameCount == 0) {
        return;
    }
    const bool masterMuted = __atomic_load_n(&outputMuteMaster_, __ATOMIC_RELAXED) != 0;
    const bool muteL = masterMuted || __atomic_load_n(&outputMutePair_[0], __ATOMIC_RELAXED) != 0;
    const bool muteR = masterMuted || __atomic_load_n(&outputMutePair_[1], __ATOMIC_RELAXED) != 0;
    const float gainL = muteL ? 0.0f : LoadOutputPairGain(0);
    const float gainR = muteR ? 0.0f : LoadOutputPairGain(1);
    const float channelGain[kUA55OutputChannels] = { gainL, gainR, gainL, gainR };
    for (uint32_t i = 0; i < frameCount; i++) {
        const uint32_t srcIndex = (uint32_t)((sampleTime + i) % ringFrames);
        const uint32_t dstIndex = (uint32_t)((sampleTime + i) % bridgeFrames_);
        const float* src = &outputRing[srcIndex * kUA55OutputChannels];
        int32_t* dst = &playbackBridge_[dstIndex * kUA55OutputChannels];
        for (uint32_t ch = 0; ch < kUA55OutputChannels; ch++) {
            dst[ch] = FloatToS24In32(src[ch] * channelGain[ch]);
        }
    }
    const uint64_t end = sampleTime + frameCount;
    if (end > halWriteSample_) {
        halWriteSample_ = end;
    }
}

void UA55UsbStream::SetTimestampTarget(volatile uint64_t* sampleTime, void* timestampTarget)
{
    sampleTime_ = sampleTime;
    timestampTarget_ = timestampTarget;
    // Re-sincroniza OUT à ponta do HAL (ou ao capture se ainda não houve WriteEnd).
    if (halWriteSample_ > kUA55PlaybackReadSlackFrames) {
        playbackReadSample_ = halWriteSample_ - kUA55PlaybackReadSlackFrames;
    } else if (captureWriteSample_ > kUA55PlaybackReadSlackFrames) {
        playbackReadSample_ = captureWriteSample_ - kUA55PlaybackReadSlackFrames;
    }
    underruns_ = 0;
}

void UA55UsbStream::ClearTimestampTarget()
{
    sampleTime_ = nullptr;
    timestampTarget_ = nullptr;
}

kern_return_t UA55UsbStream::Prepare(IOUSBHostDevice* device, IOService* client, UA55AudioDriver* actionOwner)
{
    if (device == nullptr || client == nullptr || actionOwner == nullptr) {
        return kIOReturnBadArgument;
    }

    TearDown(client);

    device_ = device;
    device_->retain();
    client_ = client;
    actionOwner_ = actionOwner;

    IOSleep(100);

    statusPipes_[0].interfaceNumber = kUA55StatusAInterface;
    statusPipes_[0].endpointAddress = kUA55StatusAEndpointAddress;
    statusPipes_[1].interfaceNumber = kUA55StatusBInterface;
    statusPipes_[1].endpointAddress = kUA55StatusBEndpointAddress;

    kern_return_t result = CollectInterfaces(
        device,
        &playbackInterface_,
        &captureInterface_,
        &midiInterface_,
        &statusPipes_[0].iface,
        &statusPipes_[1].iface);
    if (result != kIOReturnSuccess) {
        IOSleep(200);
        result = CollectInterfaces(
            device,
            &playbackInterface_,
            &captureInterface_,
            &midiInterface_,
            &statusPipes_[0].iface,
            &statusPipes_[1].iface);
    }
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream interfaces not found 0x%08x", (unsigned int)result);
        return result;
    }

    result = playbackInterface_->Open(client, 0, nullptr);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF0 Open=FAILED 0x%08x", (unsigned int)result);
        return result;
    }
    playbackOpened_ = true;

    result = captureInterface_->Open(client, 0, nullptr);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF1 Open=FAILED 0x%08x", (unsigned int)result);
        return result;
    }
    captureOpened_ = true;

    result = playbackInterface_->SelectAlternateSetting(kUA55StreamingAlternate);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF0 alt=FAILED 0x%08x", (unsigned int)result);
        return result;
    }
    result = captureInterface_->SelectAlternateSetting(kUA55StreamingAlternate);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF1 alt=FAILED 0x%08x", (unsigned int)result);
        return result;
    }

    result = playbackInterface_->CopyPipe(kUA55PlaybackEndpointAddress, &playbackPipe_);
    if (result != kIOReturnSuccess || playbackPipe_ == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream OUT pipe=FAILED 0x%08x", (unsigned int)result);
        return result != kIOReturnSuccess ? result : kIOReturnNoDevice;
    }
    result = captureInterface_->CopyPipe(kUA55CaptureEndpointAddress, &capturePipe_);
    if (result != kIOReturnSuccess || capturePipe_ == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IN pipe=FAILED 0x%08x", (unsigned int)result);
        return result != kIOReturnSuccess ? result : kIOReturnNoDevice;
    }

    // MIDI IF2: drenar bulk IN (painel/mixer SysEx). Best-effort.
    if (kUA55MidiDrainEnabled) {
        const kern_return_t midiResult = PrepareMidiPipe();
        if (midiResult == kIOReturnSuccess) {
            const kern_return_t pollResult = StartMidiPolling();
            if (pollResult != kIOReturnSuccess) {
                os_log(OS_LOG_DEFAULT, "[UA55] midi polling start=FAILED 0x%08x (audio ok)",
                       (unsigned int)pollResult);
            }
        } else {
            os_log(OS_LOG_DEFAULT, "[UA55] midi pipe prepare=FAILED 0x%08x (audio ok)",
                   (unsigned int)midiResult);
        }
    } else if (midiInterface_ != nullptr) {
        OSSafeReleaseNULL(midiInterface_);
    }

    // IF3/IF4: desligado por defeito (Linux ignora; IF4 flood + não destravou painel).
    if (kUA55StatusInterruptDrainEnabled) {
        const kern_return_t statusResult = PrepareStatusPipes();
        if (statusResult == kIOReturnSuccess) {
            const kern_return_t pollResult = StartStatusPolling();
            if (pollResult != kIOReturnSuccess) {
                os_log(OS_LOG_DEFAULT, "[UA55] status polling start=FAILED 0x%08x (audio ok)",
                       (unsigned int)pollResult);
            }
        } else {
            os_log(OS_LOG_DEFAULT, "[UA55] status pipes prepare=FAILED 0x%08x (audio ok)",
                   (unsigned int)statusResult);
        }
    } else {
        OSSafeReleaseNULL(statusPipes_[0].iface);
        OSSafeReleaseNULL(statusPipes_[1].iface);
        os_log(OS_LOG_DEFAULT, "[UA55] status IF3/IF4 ignored (midi=%d)",
               kUA55MidiDrainEnabled ? 1 : 0);
    }

    os_log(OS_LOG_DEFAULT, "[UA55] UsbStream prepared OUT=0x%02x IN=0x%02x MIDI_IN=0x%02x",
           kUA55PlaybackEndpointAddress,
           kUA55CaptureEndpointAddress,
           kUA55MidiInEndpointAddress);
    return kIOReturnSuccess;
}

kern_return_t UA55UsbStream::PrepareStatusPipes()
{
    if (client_ == nullptr || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }

    for (uint32_t pipeIndex = 0; pipeIndex < kUA55StatusPipeCount; pipeIndex++) {
        StatusPipe* status = &statusPipes_[pipeIndex];
        if (status->iface == nullptr) {
            os_log(OS_LOG_DEFAULT, "[UA55] status IF%u missing", status->interfaceNumber);
            return kIOReturnNotFound;
        }

        kern_return_t result = status->iface->Open(client_, 0, nullptr);
        if (result != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[UA55] status IF%u Open=FAILED 0x%08x",
                   status->interfaceNumber, (unsigned int)result);
            return result;
        }
        status->opened = true;

        result = status->iface->SelectAlternateSetting(kUA55StreamingAlternate);
        if (result != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[UA55] status IF%u alt=FAILED 0x%08x",
                   status->interfaceNumber, (unsigned int)result);
            return result;
        }

        result = status->iface->CopyPipe(status->endpointAddress, &status->pipe);
        if (result != kIOReturnSuccess || status->pipe == nullptr) {
            os_log(OS_LOG_DEFAULT, "[UA55] status IF%u EP 0x%02x pipe=FAILED 0x%08x",
                   status->interfaceNumber, status->endpointAddress, (unsigned int)result);
            return result != kIOReturnSuccess ? result : kIOReturnNoDevice;
        }

        for (uint32_t slotIndex = 0; slotIndex < kUA55StatusSlotsPerPipe; slotIndex++) {
            result = PrepareStatusSlot(&status->slots[slotIndex], pipeIndex, slotIndex);
            if (result != kIOReturnSuccess) {
                return result;
            }
            result = actionOwner_->CreateActionStatusInterruptComplete(
                sizeof(uint32_t) * 2, &status->slots[slotIndex].action);
            if (result != kIOReturnSuccess || status->slots[slotIndex].action == nullptr) {
                return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
            }
            uint32_t* ref = reinterpret_cast<uint32_t*>(status->slots[slotIndex].action->GetReference());
            ref[0] = pipeIndex;
            ref[1] = slotIndex;
        }

        os_log(OS_LOG_DEFAULT, "[UA55] status IF%u EP 0x%02x ready",
               status->interfaceNumber, status->endpointAddress);
    }
    return kIOReturnSuccess;
}

kern_return_t UA55UsbStream::StartStatusPolling()
{
    if (actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    statusPolling_ = true;
    for (uint32_t pipeIndex = 0; pipeIndex < kUA55StatusPipeCount; pipeIndex++) {
        if (statusPipes_[pipeIndex].pipe == nullptr) {
            continue;
        }
        for (uint32_t slotIndex = 0; slotIndex < kUA55StatusSlotsPerPipe; slotIndex++) {
            const kern_return_t result = SubmitStatusSlot(pipeIndex, slotIndex);
            if (result != kIOReturnSuccess) {
                os_log(OS_LOG_DEFAULT, "[UA55] status submit IF%u slot%u=FAILED 0x%08x",
                       statusPipes_[pipeIndex].interfaceNumber,
                       slotIndex,
                       (unsigned int)result);
                statusPolling_ = false;
                return result;
            }
        }
    }
    os_log(OS_LOG_DEFAULT, "[UA55] status interrupt drain started");
    return kIOReturnSuccess;
}

void UA55UsbStream::StopStatusPolling()
{
    statusPolling_ = false;
}

kern_return_t UA55UsbStream::SubmitStatusSlot(uint32_t pipeIndex, uint32_t slotIndex)
{
    if (!statusPolling_ || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    if (pipeIndex >= kUA55StatusPipeCount || slotIndex >= kUA55StatusSlotsPerPipe) {
        return kIOReturnBadArgument;
    }

    StatusPipe* status = &statusPipes_[pipeIndex];
    if (status->pipe == nullptr) {
        return kIOReturnOffline;
    }

    StatusSlot* slot = &status->slots[slotIndex];
    if (slot->dataBuffer == nullptr || slot->action == nullptr) {
        return kIOReturnNoMemory;
    }

    // completionTimeoutMs MUST be 0 for interrupt endpoints.
    return status->pipe->AsyncIO(slot->dataBuffer, kUA55StatusMaxPacket, slot->action, 0);
}

void UA55UsbStream::OnStatusComplete(uint32_t pipeIndex, uint32_t slotIndex, IOReturn statusCode, uint32_t actualByteCount)
{
    if (pipeIndex >= kUA55StatusPipeCount || slotIndex >= kUA55StatusSlotsPerPipe) {
        return;
    }

    StatusPipe* status = &statusPipes_[pipeIndex];
    status->completions++;

    if (statusCode != kIOReturnSuccess) {
        status->errors++;
    } else {
        status->bytes += actualByteCount;
        // IF3 raro; IF4 é flood — só summary abaixo.
        if (status->interfaceNumber == kUA55StatusAInterface &&
            actualByteCount > 0 &&
            (status->completions % 200) == 1) {
            StatusSlot* slot = &status->slots[slotIndex];
            IOAddressSegment range = {};
            if (slot->dataBuffer != nullptr &&
                slot->dataBuffer->GetAddressRange(&range) == kIOReturnSuccess &&
                range.address != 0) {
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(range.address);
                os_log(OS_LOG_DEFAULT,
                       "[UA55] status IF3 EP 0x82 len=%u bytes=%02x %02x %02x %02x %02x %02x %02x %02x",
                       actualByteCount,
                       actualByteCount > 0 ? bytes[0] : 0,
                       actualByteCount > 1 ? bytes[1] : 0,
                       actualByteCount > 2 ? bytes[2] : 0,
                       actualByteCount > 3 ? bytes[3] : 0,
                       actualByteCount > 4 ? bytes[4] : 0,
                       actualByteCount > 5 ? bytes[5] : 0,
                       actualByteCount > 6 ? bytes[6] : 0,
                       actualByteCount > 7 ? bytes[7] : 0);
            }
        }
    }

    if ((status->completions % 500) == 0) {
        os_log(OS_LOG_DEFAULT,
               "[UA55] status IF%u completions=%llu bytes=%llu errors=%llu last=0x%08x",
               status->interfaceNumber,
               status->completions,
               status->bytes,
               status->errors,
               (unsigned int)statusCode);
    }

    if (statusPolling_) {
        SubmitStatusSlot(pipeIndex, slotIndex);
    }
}

kern_return_t UA55UsbStream::PrepareMidiPipe()
{
    if (client_ == nullptr || actionOwner_ == nullptr || midiInterface_ == nullptr) {
        return kIOReturnOffline;
    }

    kern_return_t result = midiInterface_->Open(client_, 0, nullptr);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] MIDI IF2 Open=FAILED 0x%08x", (unsigned int)result);
        return result;
    }
    midiOpened_ = true;

    // Alt 0 = bulk 0x06/0x86 (MIDI). Alt 1 = interrupt — não usar.
    result = midiInterface_->SelectAlternateSetting(kUA55MidiAlternateBulk);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] MIDI IF2 alt0=FAILED 0x%08x", (unsigned int)result);
        return result;
    }

    result = midiInterface_->CopyPipe(kUA55MidiInEndpointAddress, &midiInPipe_);
    if (result != kIOReturnSuccess || midiInPipe_ == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] MIDI IN 0x86 pipe=FAILED 0x%08x", (unsigned int)result);
        return result != kIOReturnSuccess ? result : kIOReturnNoDevice;
    }

    for (uint32_t slotIndex = 0; slotIndex < kUA55MidiInSlotCount; slotIndex++) {
        result = PrepareMidiSlot(&midiInSlots_[slotIndex], slotIndex);
        if (result != kIOReturnSuccess) {
            return result;
        }
        result = actionOwner_->CreateActionMidiInComplete(sizeof(uint32_t), &midiInSlots_[slotIndex].action);
        if (result != kIOReturnSuccess || midiInSlots_[slotIndex].action == nullptr) {
            return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
        }
        *reinterpret_cast<uint32_t*>(midiInSlots_[slotIndex].action->GetReference()) = slotIndex;
    }

    os_log(OS_LOG_DEFAULT, "[UA55] MIDI IF2 EP 0x86 ready");
    return kIOReturnSuccess;
}

kern_return_t UA55UsbStream::StartMidiPolling()
{
    if (midiInPipe_ == nullptr || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    midiPolling_ = true;
    midiCompletions_ = 0;
    midiBytes_ = 0;
    midiErrors_ = 0;
    for (uint32_t slotIndex = 0; slotIndex < kUA55MidiInSlotCount; slotIndex++) {
        const kern_return_t result = SubmitMidiInSlot(slotIndex);
        if (result != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[UA55] MIDI submit slot%u=FAILED 0x%08x",
                   slotIndex, (unsigned int)result);
            midiPolling_ = false;
            return result;
        }
    }
    os_log(OS_LOG_DEFAULT, "[UA55] MIDI bulk IN drain started");
    return kIOReturnSuccess;
}

void UA55UsbStream::StopMidiPolling()
{
    midiPolling_ = false;
}

kern_return_t UA55UsbStream::SubmitMidiInSlot(uint32_t slotIndex)
{
    if (!midiPolling_ || midiInPipe_ == nullptr || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    if (slotIndex >= kUA55MidiInSlotCount) {
        return kIOReturnBadArgument;
    }

    MidiSlot* slot = &midiInSlots_[slotIndex];
    if (slot->dataBuffer == nullptr || slot->action == nullptr) {
        return kIOReturnNoMemory;
    }

    return midiInPipe_->AsyncIO(slot->dataBuffer, kUA55MidiMaxPacket, slot->action, 0);
}

void UA55UsbStream::OnMidiInComplete(uint32_t slotIndex, IOReturn statusCode, uint32_t actualByteCount)
{
    if (slotIndex >= kUA55MidiInSlotCount) {
        return;
    }

    midiCompletions_++;
    if (statusCode != kIOReturnSuccess) {
        midiErrors_++;
    } else {
        midiBytes_ += actualByteCount;
        if (actualByteCount > 0) {
            MidiSlot* slot = &midiInSlots_[slotIndex];
            IOAddressSegment range = {};
            if (slot->dataBuffer != nullptr &&
                slot->dataBuffer->GetAddressRange(&range) == kIOReturnSuccess &&
                range.address != 0) {
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(range.address);
                // Log primeiros pacotes e depois esparsos — útil ao girar knobs.
                if (midiCompletions_ <= 30 || (midiCompletions_ % 25) == 0) {
                    os_log(OS_LOG_DEFAULT,
                           "[UA55] MIDI IN len=%u bytes=%02x %02x %02x %02x %02x %02x %02x %02x",
                           actualByteCount,
                           actualByteCount > 0 ? bytes[0] : 0,
                           actualByteCount > 1 ? bytes[1] : 0,
                           actualByteCount > 2 ? bytes[2] : 0,
                           actualByteCount > 3 ? bytes[3] : 0,
                           actualByteCount > 4 ? bytes[4] : 0,
                           actualByteCount > 5 ? bytes[5] : 0,
                           actualByteCount > 6 ? bytes[6] : 0,
                           actualByteCount > 7 ? bytes[7] : 0);
                }
            }
        }
    }

    if ((midiCompletions_ % 100) == 0) {
        os_log(OS_LOG_DEFAULT,
               "[UA55] MIDI completions=%llu bytes=%llu errors=%llu last=0x%08x",
               midiCompletions_, midiBytes_, midiErrors_, (unsigned int)statusCode);
    }

    if (midiPolling_) {
        SubmitMidiInSlot(slotIndex);
    }
}

void UA55UsbStream::TearDownMidiPipe(IOService* closer)
{
    StopMidiPolling();
    if (midiInPipe_ != nullptr && closer != nullptr) {
        midiInPipe_->Abort(0, kIOReturnAborted, closer);
    }
    IOSleep(20);

    for (uint32_t slotIndex = 0; slotIndex < kUA55MidiInSlotCount; slotIndex++) {
        FreeMidiSlot(&midiInSlots_[slotIndex]);
    }
    OSSafeReleaseNULL(midiInPipe_);

    if (midiInterface_ != nullptr && midiOpened_ && closer != nullptr) {
        midiInterface_->SelectAlternateSetting(0);
        midiInterface_->Close(closer, 0);
        midiOpened_ = false;
    }
    OSSafeReleaseNULL(midiInterface_);
    midiCompletions_ = 0;
    midiBytes_ = 0;
    midiErrors_ = 0;
}

void UA55UsbStream::TearDownStatusPipes(IOService* closer)
{
    StopStatusPolling();

    for (uint32_t pipeIndex = 0; pipeIndex < kUA55StatusPipeCount; pipeIndex++) {
        StatusPipe* status = &statusPipes_[pipeIndex];
        if (status->pipe != nullptr && closer != nullptr) {
            status->pipe->Abort(0, kIOReturnAborted, closer);
        }
    }
    IOSleep(30);

    for (uint32_t pipeIndex = 0; pipeIndex < kUA55StatusPipeCount; pipeIndex++) {
        StatusPipe* status = &statusPipes_[pipeIndex];
        for (uint32_t slotIndex = 0; slotIndex < kUA55StatusSlotsPerPipe; slotIndex++) {
            FreeStatusSlot(&status->slots[slotIndex]);
        }
        OSSafeReleaseNULL(status->pipe);

        if (status->iface != nullptr && status->opened && closer != nullptr) {
            status->iface->SelectAlternateSetting(0);
            status->iface->Close(closer, 0);
            status->opened = false;
        }
        OSSafeReleaseNULL(status->iface);
        status->completions = 0;
        status->bytes = 0;
        status->errors = 0;
    }
}

void UA55UsbStream::TearDown(IOService* client)
{
    // Para o anel sem Abort primeiro; Abort só no teardown final do dext.
    streaming_ = false;
    StopStatusPolling();
    StopMidiPolling();
    IOSleep(50);

    IOService* closer = client != nullptr ? client : client_;
    if (closer == nullptr) {
        for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
            FreeIsochSlot(&captureSlots_[index]);
            FreeIsochSlot(&playbackSlots_[index]);
        }
        TearDownMidiPipe(nullptr);
        TearDownStatusPipes(nullptr);
        return;
    }

    if (capturePipe_ != nullptr) {
        capturePipe_->Abort(0, kIOReturnAborted, closer);
    }
    if (playbackPipe_ != nullptr) {
        playbackPipe_->Abort(0, kIOReturnAborted, closer);
    }
    TearDownMidiPipe(closer);
    TearDownStatusPipes(closer);
    IOSleep(50);

    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        FreeIsochSlot(&captureSlots_[index]);
        FreeIsochSlot(&playbackSlots_[index]);
    }

    OSSafeReleaseNULL(capturePipe_);
    OSSafeReleaseNULL(playbackPipe_);

    if (captureInterface_ != nullptr && captureOpened_) {
        captureInterface_->SelectAlternateSetting(0);
        captureInterface_->Close(closer, 0);
        captureOpened_ = false;
    }
    if (playbackInterface_ != nullptr && playbackOpened_) {
        playbackInterface_->SelectAlternateSetting(0);
        playbackInterface_->Close(closer, 0);
        playbackOpened_ = false;
    }

    OSSafeReleaseNULL(captureInterface_);
    OSSafeReleaseNULL(playbackInterface_);
    OSSafeReleaseNULL(device_);
    client_ = nullptr;
    actionOwner_ = nullptr;
}

kern_return_t UA55UsbStream::SubmitCaptureSlot(uint32_t slotIndex)
{
    if (!streaming_ || capturePipe_ == nullptr || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    if (slotIndex >= kUA55IsochRingDepth) {
        return kIOReturnBadArgument;
    }

    IsochSlot* slot = &captureSlots_[slotIndex];
    kern_return_t result = FillFrameList(slot, kUA55CaptureMaxPacketAlt1);
    if (result != kIOReturnSuccess) {
        return result;
    }

    if (slot->action == nullptr) {
        result = actionOwner_->CreateActionCaptureIsochComplete(sizeof(uint32_t), &slot->action);
        if (result != kIOReturnSuccess || slot->action == nullptr) {
            return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
        }
        *reinterpret_cast<uint32_t*>(slot->action->GetReference()) = slotIndex;
    }

    return capturePipe_->IsochIO(slot->dataBuffer, slot->frameListBuffer, 0, slot->action);
}

kern_return_t UA55UsbStream::SubmitPlaybackSlot(uint32_t slotIndex)
{
    if (!streaming_ || playbackPipe_ == nullptr || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    if (slotIndex >= kUA55IsochRingDepth) {
        return kIOReturnBadArgument;
    }

    IsochSlot* slot = &playbackSlots_[slotIndex];
    if (FillPlaybackTransfer(slot) == 0) {
        return kIOReturnNoMemory;
    }

    kern_return_t result = kIOReturnSuccess;
    if (slot->action == nullptr) {
        result = actionOwner_->CreateActionPlaybackIsochComplete(sizeof(uint32_t), &slot->action);
        if (result != kIOReturnSuccess || slot->action == nullptr) {
            return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
        }
        *reinterpret_cast<uint32_t*>(slot->action->GetReference()) = slotIndex;
    }

    return playbackPipe_->IsochIO(slot->dataBuffer, slot->frameListBuffer, 0, slot->action);
}

void UA55UsbStream::OnCaptureComplete(uint32_t slotIndex, IOReturn status)
{
    // Soft-stop: sai cedo se já não estamos em streaming (evita tocar slots a serem libertados).
    if (!streaming_) {
        return;
    }
    if (slotIndex >= kUA55IsochRingDepth) {
        return;
    }

    captureCompletions_++;
    IsochSlot* slot = &captureSlots_[slotIndex];

    if (status != kIOReturnSuccess) {
        captureErrors_++;
    } else {
        captureBytes_ += SumCompleteBytes(slot);
        UnpackCaptureToBridge(slot);
    }

    if ((captureCompletions_ % kUA55IsochLogEveryCompletions) == 0) {
        os_log(OS_LOG_DEFAULT,
               "[UA55] capture async completions=%llu bytes=%llu errors=%llu lastStatus=0x%08x",
               captureCompletions_,
               captureBytes_,
               captureErrors_,
               (unsigned int)status);
    }

    if (streaming_) {
        SubmitCaptureSlot(slotIndex);
    }
}

void UA55UsbStream::OnPlaybackComplete(uint32_t slotIndex, IOReturn status)
{
    if (!streaming_) {
        return;
    }
    if (slotIndex >= kUA55IsochRingDepth) {
        return;
    }

    playbackCompletions_++;
    if (status != kIOReturnSuccess) {
        playbackErrors_++;
    } else {
        playbackBytes_ += SumCompleteBytes(&playbackSlots_[slotIndex]);
    }

    if ((playbackCompletions_ % kUA55IsochLogEveryCompletions) == 0) {
        os_log(OS_LOG_DEFAULT,
               "[UA55] playback async completions=%llu bytes=%llu errors=%llu lastStatus=0x%08x drift=%lld resyncs=%llu inFrames=%u underruns=%llu",
               playbackCompletions_,
               playbackBytes_,
               playbackErrors_,
               (unsigned int)status,
               lastLoggedDrift_,
               playbackResyncs_,
               lastCaptureAudioFrames_,
               underruns_);
    }

    if (streaming_) {
        SubmitPlaybackSlot(slotIndex);
    }
}

kern_return_t UA55UsbStream::StartStreaming()
{
    if (streaming_) {
        // Já a correr (ex.: pause→play). Não reinicia o USB nem o relógio.
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream already streaming — keep isoc alive");
        return kIOReturnSuccess;
    }

    if (capturePipe_ == nullptr || playbackPipe_ == nullptr || actionOwner_ == nullptr) {
        return kIOReturnOffline;
    }
    if (!EnsureBridges()) {
        return kIOReturnNoMemory;
    }

    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        kern_return_t result = PrepareIsochSlot(&captureSlots_[index], true, kUA55CaptureMaxPacketAlt1, index);
        if (result != kIOReturnSuccess) {
            return result;
        }
        result = PrepareIsochSlot(&playbackSlots_[index], false, kUA55PlaybackMaxPacketAlt1, index);
        if (result != kIOReturnSuccess) {
            return result;
        }

        result = actionOwner_->CreateActionCaptureIsochComplete(sizeof(uint32_t), &captureSlots_[index].action);
        if (result != kIOReturnSuccess || captureSlots_[index].action == nullptr) {
            return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
        }
        *reinterpret_cast<uint32_t*>(captureSlots_[index].action->GetReference()) = index;

        result = actionOwner_->CreateActionPlaybackIsochComplete(sizeof(uint32_t), &playbackSlots_[index].action);
        if (result != kIOReturnSuccess || playbackSlots_[index].action == nullptr) {
            return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
        }
        *reinterpret_cast<uint32_t*>(playbackSlots_[index].action->GetReference()) = index;
    }

    if (sampleTime_ != nullptr) {
        *sampleTime_ = 0;
    }
    playbackPhase_ = 0;
    lastCaptureAudioFrames_ = 44;
    playbackReadSample_ = 0;
    captureWriteSample_ = 0;
    lastPublishedZts_ = 0;
    lastHostTime_ = 0;
    playbackResyncs_ = 0;
    lastLoggedDrift_ = 0;
    halWriteSample_ = 0;
    underruns_ = 0;
    captureUnderruns_ = 0;
    captureOverruns_ = 0;
    captureCompletions_ = 0;
    playbackCompletions_ = 0;
    captureBytes_ = 0;
    playbackBytes_ = 0;
    captureErrors_ = 0;
    playbackErrors_ = 0;
    streaming_ = true;

    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        const kern_return_t result = SubmitCaptureSlot(index);
        if (result != kIOReturnSuccess) {
            streaming_ = false;
            os_log(OS_LOG_DEFAULT, "[UA55] UsbStream capture submit abort slot %u", index);
            return result;
        }
    }
    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        const kern_return_t result = SubmitPlaybackSlot(index);
        if (result != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[UA55] UsbStream playback submit failed slot %u 0x%08x",
                   index, (unsigned int)result);
        }
    }

    os_log(OS_LOG_DEFAULT, "[UA55] UsbStream duplex started depth=%u", kUA55IsochRingDepth);
    return kIOReturnSuccess;
}

void UA55UsbStream::StopStreaming()
{
    if (!streaming_) {
        return;
    }

    const uint64_t caps = captureCompletions_;
    const uint64_t plays = playbackCompletions_;

    // Soft-stop: NÃO usar Pipe::Abort. Abort no StopIO faz a UA-55 reenumerar
    // (~2 s de I/O ≈ 2048 completions a 1 ms → device matched em loop no AMS).
    // Preferir ClearTimestampTarget no StopIO e só chamar isto no teardown do device.
    streaming_ = false;
    ClearTimestampTarget();
    IOSleep(150);

    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        FreeIsochSlot(&captureSlots_[index]);
        FreeIsochSlot(&playbackSlots_[index]);
    }

    os_log(OS_LOG_DEFAULT,
           "[UA55] UsbStream soft-stopped captureCompletions=%llu playbackCompletions=%llu",
           caps,
           plays);
}
