#include <os/log.h>
#include <stdio.h>
#include <string.h>

#include <DriverKit/IOLib.h>

#include "UA55UsbStream.h"
#include "UA55AudioDriver.h"
#include "UA55Quirks.h"

// Sobrevive a um rematch USB no mesmo processo da dext.
static uint32_t gUA55DesiredRateHz = kUA55SampleRateInt;

namespace {

static const uint8_t kUA55ClockRequest = 3;
static const uint16_t kUA55ClockReadValue = 0x0001;
static const uint16_t kUA55ClockWriteValue = 0x0008;
static const uint8_t kUA55ClockWritePrefix = 0x40;
static const uint8_t kUA55ClockRequestIn = 0xC0;
static const uint8_t kUA55ClockRequestOut = 0x40;
static const uint32_t kUA55ClockPollAttempts = 40;
static const uint32_t kUA55ClockPollMs = 25;

} // namespace

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
    const uint32_t inCh = rate_.inputChannels != 0 ? rate_.inputChannels : kUA55InputChannels;
    const uint32_t bytesPerAudioFrame = inCh * kUA55BytesPerSample;
    uint32_t byteOffset = 0;
    uint32_t totalAudioFrames = 0;
    uint8_t uframeSamples[kUA55IsochFramesPerTransfer];
    bool patternOk = true;

    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        const uint32_t complete = frames[index].completeCount;
        const uint32_t request = frames[index].requestCount != 0
            ? frames[index].requestCount
            : rate_.captureMaxPacket;

        uint32_t nFrames = 0;
        if (complete >= bytesPerAudioFrame && (complete % bytesPerAudioFrame) == 0) {
            nFrames = complete / bytesPerAudioFrame;
            const int32_t* src = reinterpret_cast<const int32_t*>(data + byteOffset);
            for (uint32_t frame = 0; frame < nFrames; frame++) {
                const uint32_t ringIndex =
                    (uint32_t)((captureWriteSample_ + totalAudioFrames + frame) % bridgeFrames_);
                int32_t* dst = &captureBridge_[ringIndex * inCh];
                const int32_t* frameSrc = &src[frame * inCh];
                for (uint32_t ch = 0; ch < inCh; ch++) {
                    dst[ch] = frameSrc[ch];
                }
            }
            totalAudioFrames += nFrames;
        }
        if (nFrames < rate_.samplesPerUframeMin || nFrames > rate_.samplesPerUframeMax) {
            patternOk = false;
        }
        uframeSamples[index] = (uint8_t)nFrames;
        byteOffset += request;
    }

    captureWriteSample_ += totalAudioFrames;
    const uint32_t minTransfer = rate_.samplesPerUframeMin * kUA55IsochFramesPerTransfer;
    const uint32_t maxTransfer = rate_.samplesPerUframeMax * kUA55IsochFramesPerTransfer;
    if (totalAudioFrames >= minTransfer && totalAudioFrames <= maxTransfer) {
        lastCaptureAudioFrames_ = totalAudioFrames;
        if (patternOk) {
            for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
                captureUframeSamples_[index] = uframeSamples[index];
            }
            capturePatternValid_ = true;
        }
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

    const uint32_t inCh = rate_.inputChannels != 0 ? rate_.inputChannels : kUA55InputChannels;
    const uint64_t writeTip = captureWriteSample_;
    for (uint32_t i = 0; i < frameCount; i++) {
        const uint64_t absSample = sampleTime + i;
        const uint32_t dstIndex = (uint32_t)((sampleTime + i) % ringFrames);
        float* dst = &inputRing[dstIndex * inCh];

        // Ainda não escrito pelo USB (ou dentro da margem de corrida).
        if (absSample + 16ull >= writeTip) {
            captureUnderruns_++;
            for (uint32_t ch = 0; ch < inCh; ch++) {
                dst[ch] = 0.0f;
            }
            continue;
        }
        // Já reescrito no anel (HAL atrasado demais).
        if (writeTip - absSample > bridgeFrames_) {
            captureOverruns_++;
            for (uint32_t ch = 0; ch < inCh; ch++) {
                dst[ch] = 0.0f;
            }
            continue;
        }

        const uint32_t srcIndex = (uint32_t)(absSample % bridgeFrames_);
        const int32_t* src = &captureBridge_[srcIndex * inCh];
        for (uint32_t ch = 0; ch < inCh; ch++) {
            dst[ch] = S24In32ToFloat(src[ch]);
        }
    }
}

// Mantém a mesma folga em tempo (~12 ms / ~46 ms) em todas as taxas.
// Os constantes são o valor que trava 44.1 kHz; em frames fixos, 192 kHz
// recentra a cada poucos ms e o áudio não volta.
static uint32_t ScaledFrom44100(uint32_t framesAt44100, uint32_t rateInt)
{
    const uint32_t rate = rateInt != 0 ? rateInt : 44100;
    const uint32_t scaled = (uint32_t)(((uint64_t)framesAt44100 * rate) / 44100u);
    return scaled < framesAt44100 ? framesAt44100 : scaled;
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
    const uint32_t outCh = rate_.outputChannels != 0 ? rate_.outputChannels : kUA55OutputChannels;
    const uint32_t bytesPerFrame = outCh * kUA55BytesPerSample;

    int64_t drift = 0;
    const uint32_t slack = ScaledFrom44100(kUA55PlaybackReadSlackFrames, rate_.rateInt);
    const uint32_t resyncAt = ScaledFrom44100(kUA55PlaybackResyncThreshold, rate_.rateInt);
    // Alinhar OUT à ponta do HAL (não ao capture): o WriteEnd é a fonte da verdade.
    if (halWriteSample_ > slack) {
        const uint64_t target = halWriteSample_ - slack;
        drift = (int64_t)playbackReadSample_ - (int64_t)target;
        lastLoggedDrift_ = drift;
        if (drift > (int64_t)slack ||
            drift < -(int64_t)resyncAt) {
            playbackReadSample_ = target;
            playbackResyncs_++;
            drift = 0;
            lastLoggedDrift_ = 0;
        }
    } else if (captureWriteSample_ > slack) {
        const uint64_t target = captureWriteSample_ - slack;
        drift = (int64_t)playbackReadSample_ - (int64_t)target;
        lastLoggedDrift_ = drift;
    }

    const uint32_t minSamples = rate_.samplesPerUframeMin;
    const uint32_t maxSamples = rate_.samplesPerUframeMax;

    // 44.1 kHz: copiar o tamanho de cada microframe da captura. Somar e
    // redistribuir (e o ±1 de drift) desalinha o DAC e enfia zeros no OUT.
    uint32_t planned[kUA55IsochFramesPerTransfer];
    if (minSamples != maxSamples && capturePatternValid_) {
        for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
            planned[index] = captureUframeSamples_[index];
        }
    } else if (minSamples == maxSamples) {
        for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
            planned[index] = minSamples;
        }
    } else {
        for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
            playbackPhase_ += rate_.rateInt;
            uint32_t samples = playbackPhase_ / kUA55HighSpeedUframesPerSecond;
            playbackPhase_ %= kUA55HighSpeedUframesPerSecond;
            if (samples < minSamples) {
                samples = minSamples;
            } else if (samples > maxSamples) {
                samples = maxSamples;
            }
            planned[index] = samples;
        }
    }

    uint32_t playbackConsumed = 0;
    for (uint32_t index = 0; index < kUA55IsochFramesPerTransfer; index++) {
        const uint32_t samples = planned[index];

        frames[index].status = (IOReturn)kIOReturnInvalid;
        frames[index].requestCount = (uint16_t)(samples * bytesPerFrame);
        frames[index].completeCount = 0;
        frames[index].reserved = 0;
        frames[index].timeStamp = 0;

        uint32_t consumedHere = 0;
        for (uint32_t frame = 0; frame < samples; frame++) {
            int32_t* dst = &data[(totalFrames + frame) * outCh];
            if (playbackBridge_ == nullptr || bridgeFrames_ == 0) {
                for (uint32_t ch = 0; ch < outCh; ch++) {
                    dst[ch] = 0;
                }
                consumedHere++;
                continue;
            }
            // Pause: silêncio e avança, para não repetir o áudio antigo.
            // Sem WriteEnd ainda: silêncio sem avançar, senão a leitura
            // larga na frente do HAL e o pacote seguinte sai zerado.
            if (timestampTarget_ == nullptr) {
                for (uint32_t ch = 0; ch < outCh; ch++) {
                    dst[ch] = 0;
                }
                consumedHere++;
                continue;
            }
            if (halWriteSample_ == 0) {
                for (uint32_t ch = 0; ch < outCh; ch++) {
                    dst[ch] = 0;
                }
                continue;
            }
            const uint64_t absSample = playbackReadSample_ + playbackConsumed + consumedHere;
            if (absSample >= halWriteSample_) {
                // Não avança o ponteiro: senão a leitura fica na frente do HAL
                // e cada pacote seguinte sai zerado (ruído / velocidade errada).
                underruns_++;
                for (uint32_t ch = 0; ch < outCh; ch++) {
                    dst[ch] = 0;
                }
                continue;
            }
            const uint32_t ringIndex = (uint32_t)(absSample % bridgeFrames_);
            const int32_t* src = &playbackBridge_[ringIndex * outCh];
            for (uint32_t ch = 0; ch < outCh; ch++) {
                dst[ch] = src[ch];
            }
            consumedHere++;
        }
        totalFrames += samples;
        playbackConsumed += consumedHere;
    }

    playbackReadSample_ += playbackConsumed;
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
    const uint32_t outCh = rate_.outputChannels != 0 ? rate_.outputChannels : kUA55OutputChannels;
    float channelGain[kUA55OutputChannels];
    for (uint32_t ch = 0; ch < outCh; ch++) {
        channelGain[ch] = (ch % 2u) == 0u ? gainL : gainR;
    }
    for (uint32_t i = 0; i < frameCount; i++) {
        const uint32_t srcIndex = (uint32_t)((sampleTime + i) % ringFrames);
        const uint32_t dstIndex = (uint32_t)((sampleTime + i) % bridgeFrames_);
        const float* src = &outputRing[srcIndex * outCh];
        int32_t* dst = &playbackBridge_[dstIndex * outCh];
        for (uint32_t ch = 0; ch < outCh; ch++) {
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
    const uint32_t slack = ScaledFrom44100(kUA55PlaybackReadSlackFrames, rate_.rateInt);
    if (halWriteSample_ > slack) {
        playbackReadSample_ = halWriteSample_ - slack;
    } else if (captureWriteSample_ > slack) {
        playbackReadSample_ = captureWriteSample_ - slack;
    }
    underruns_ = 0;
}

void UA55UsbStream::ClearTimestampTarget()
{
    sampleTime_ = nullptr;
    timestampTarget_ = nullptr;
}

kern_return_t UA55UsbStream::OpenAudioPipes(uint8_t alternate)
{
    if (playbackInterface_ == nullptr || captureInterface_ == nullptr) {
        return kIOReturnOffline;
    }

    OSSafeReleaseNULL(playbackPipe_);
    OSSafeReleaseNULL(capturePipe_);

    kern_return_t result = kIOReturnSuccess;
    if (streamedSinceOpen_) {
        // A UA-55 reinicia se o alt muda direto de um alt que acabou de
        // transmitir. O ALSA passa por alt 0 antes do alt novo.
        result = playbackInterface_->SelectAlternateSetting(0);
        if (result != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF0 alt 0=FAILED 0x%08x", (unsigned int)result);
            return result;
        }
        result = captureInterface_->SelectAlternateSetting(0);
        if (result != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF1 alt 0=FAILED 0x%08x", (unsigned int)result);
            return result;
        }
        streamedSinceOpen_ = false;
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream idle alt before %u", alternate);
        IOSleep(50);
    }

    result = playbackInterface_->SelectAlternateSetting(alternate);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF0 alt %u=FAILED 0x%08x",
               alternate, (unsigned int)result);
        return result;
    }
    result = captureInterface_->SelectAlternateSetting(alternate);
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream IF1 alt %u=FAILED 0x%08x",
               alternate, (unsigned int)result);
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
    return kIOReturnSuccess;
}

kern_return_t UA55UsbStream::ReadHardwareRate(uint32_t* rateOut)
{
    if (rateOut == nullptr || device_ == nullptr || client_ == nullptr) {
        return kIOReturnBadArgument;
    }

    IOBufferMemoryDescriptor* buffer = nullptr;
    kern_return_t result = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionIn, 4, 0, &buffer);
    if (result != kIOReturnSuccess || buffer == nullptr) {
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    result = buffer->SetLength(4);
    if (result != kIOReturnSuccess) {
        OSSafeReleaseNULL(buffer);
        return result;
    }

    uint16_t transferred = 0;
    result = device_->DeviceRequest(client_,
                                     kUA55ClockRequestIn,
                                     kUA55ClockRequest,
                                     kUA55ClockReadValue,
                                     0,
                                     4,
                                     buffer,
                                     &transferred,
                                     1000);
    if (result == kIOReturnSuccess && transferred >= 3) {
        IOAddressSegment range = {};
        if (buffer->GetAddressRange(&range) == kIOReturnSuccess && range.address != 0) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(range.address);
            *rateOut = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16);
        } else {
            result = kIOReturnNoMemory;
        }
    } else if (result == kIOReturnSuccess) {
        result = kIOReturnUnderrun;
    }
    OSSafeReleaseNULL(buffer);
    return result;
}

kern_return_t UA55UsbStream::SetHardwareClock(uint32_t rateInt)
{
    if (!kUA55VendorRequestsEnabled) {
        hardwareRate_ = rateInt;
        return kIOReturnSuccess;
    }
    if (device_ == nullptr || client_ == nullptr) {
        return kIOReturnOffline;
    }

    uint32_t current = 0;
    kern_return_t result = ReadHardwareRate(&current);
    if (result == kIOReturnSuccess && current == rateInt) {
        hardwareRate_ = rateInt;
        os_log(OS_LOG_DEFAULT, "[UA55] clock already %u Hz", rateInt);
        return kIOReturnSuccess;
    }
    if (result != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] clock read=FAILED 0x%08x", (unsigned int)result);
    } else {
        os_log(OS_LOG_DEFAULT, "[UA55] clock read %u Hz", current);
    }

    IOBufferMemoryDescriptor* buffer = nullptr;
    result = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, 4, 0, &buffer);
    if (result != kIOReturnSuccess || buffer == nullptr) {
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    result = buffer->SetLength(4);
    if (result != kIOReturnSuccess) {
        OSSafeReleaseNULL(buffer);
        return result;
    }
    IOAddressSegment range = {};
    result = buffer->GetAddressRange(&range);
    if (result != kIOReturnSuccess || range.address == 0) {
        OSSafeReleaseNULL(buffer);
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    uint8_t* data = reinterpret_cast<uint8_t*>(range.address);
    data[0] = kUA55ClockWritePrefix;
    data[1] = (uint8_t)(rateInt & 0xFF);
    data[2] = (uint8_t)((rateInt >> 8) & 0xFF);
    data[3] = (uint8_t)((rateInt >> 16) & 0xFF);

    uint16_t transferred = 0;
    result = device_->DeviceRequest(client_,
                                     kUA55ClockRequestOut,
                                     kUA55ClockRequest,
                                     kUA55ClockWriteValue,
                                     0,
                                     4,
                                     buffer,
                                     &transferred,
                                     1000);
    OSSafeReleaseNULL(buffer);
    if (result != kIOReturnSuccess || transferred != 4) {
        os_log(OS_LOG_DEFAULT, "[UA55] clock write %u Hz=FAILED 0x%08x transferred=%u",
               rateInt,
               (unsigned int)result,
               transferred);
        return result != kIOReturnSuccess ? result : kIOReturnUnderrun;
    }

    for (uint32_t attempt = 0; attempt < kUA55ClockPollAttempts; attempt++) {
        current = 0;
        result = ReadHardwareRate(&current);
        if (result == kIOReturnSuccess && current == rateInt) {
            hardwareRate_ = rateInt;
            os_log(OS_LOG_DEFAULT, "[UA55] clock now %u Hz", rateInt);
            return kIOReturnSuccess;
        }
        IOSleep(kUA55ClockPollMs);
    }

    os_log(OS_LOG_DEFAULT, "[UA55] clock did not reach %u Hz (last=%u)", rateInt, current);
    return kIOReturnTimeout;
}

kern_return_t UA55UsbStream::ApplySampleRate(uint32_t rateInt)
{
    const UA55RateConfig* mode = UA55RateForHz((double)rateInt);
    if (mode == nullptr) {
        return kIOReturnUnsupported;
    }
    gUA55DesiredRateHz = mode->rateInt;
    if (rate_.rateInt == mode->rateInt &&
        hardwareRate_ == mode->rateInt &&
        playbackPipe_ != nullptr &&
        capturePipe_ != nullptr) {
        return kIOReturnSuccess;
    }
    if (playbackInterface_ == nullptr || captureInterface_ == nullptr) {
        return kIOReturnOffline;
    }

    const UA55RateConfig previous = rate_;
    const uint32_t previousHardware = hardwareRate_;
    if (streaming_) {
        StopStreaming();
    }
    if (__atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE) != 0) {
        os_log(OS_LOG_DEFAULT, "[UA55] sample rate change aborted, isoch still in flight");
        return kIOReturnTimeout;
    }

    kern_return_t result = OpenAudioPipes(mode->alternate);
    if (result != kIOReturnSuccess) {
        if (previous.rateInt != 0) {
            OpenAudioPipes(previous.alternate);
            rate_ = previous;
        }
        return result;
    }

    result = SetHardwareClock(mode->rateInt);
    if (result != kIOReturnSuccess) {
        if (previous.rateInt != 0 && previous.alternate != mode->alternate) {
            OpenAudioPipes(previous.alternate);
            rate_ = previous;
            hardwareRate_ = previousHardware;
        }
        return result;
    }

    rate_ = *mode;
    playbackPhase_ = 0;
    lastCaptureAudioFrames_ = mode->nominalFramesPerTransfer;
    capturePatternValid_ = false;
    os_log(OS_LOG_DEFAULT, "[UA55] sample rate %u Hz alt=%u packets out=%u in=%u",
           mode->rateInt,
           mode->alternate,
           mode->playbackMaxPacket,
           mode->captureMaxPacket);
    return kIOReturnSuccess;
}

void UA55UsbStream::PrimeStreamingSilently(uint32_t milliseconds)
{
    if (StartStreaming() != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] sample rate prime start failed");
        return;
    }
    IOSleep(milliseconds);
    StopStreaming();
    os_log(OS_LOG_DEFAULT, "[UA55] sample rate prime stopped inFlight=%u",
           __atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE));
}

bool UA55UsbStream::Warm44100Via48000(uint32_t finalRate)
{
    if (rate_.rateInt == 48000 || rate_.rateInt == finalRate) {
        return ApplySampleRate(finalRate) == kIOReturnSuccess;
    }
    os_log(OS_LOG_DEFAULT, "[UA55] sample rate warm 48000 before %u", finalRate);
    if (ApplySampleRate(48000) != kIOReturnSuccess) {
        return false;
    }
    // Nesta fila as completions USB correm. Dentro do Perform, não.
    PrimeStreamingSilently(50);
    if (streaming_ || __atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE) != 0) {
        os_log(OS_LOG_DEFAULT, "[UA55] sample rate warm did not stop inFlight=%u",
               __atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE));
        return false;
    }
    return ApplySampleRate(finalRate) == kIOReturnSuccess;
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

    result = ApplySampleRate(gUA55DesiredRateHz);
    if (result != kIOReturnSuccess) {
        return result;
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

    result = midiInterface_->CopyPipe(kUA55MidiOutEndpointAddress, &midiOutPipe_);
    if (result != kIOReturnSuccess || midiOutPipe_ == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] MIDI OUT 0x06 pipe=FAILED 0x%08x", (unsigned int)result);
        OSSafeReleaseNULL(midiOutPipe_);
    } else {
        os_log(OS_LOG_DEFAULT, "[UA55] MIDI OUT 0x06 ready");
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

namespace {

bool IsLoCutPacket(const uint8_t* bytes, uint32_t length)
{
    if (bytes == nullptr || length != 20) {
        return false;
    }
    if (bytes[0] != 0x14 || bytes[4] != 0x14 || bytes[8] != 0x14 || bytes[12] != 0x14 || bytes[16] != 0x16) {
        return false;
    }
    if (bytes[1] != 0xF0 || bytes[2] != 0x41 || bytes[3] != 0x10) {
        return false;
    }
    if (bytes[5] != 0x00 || bytes[6] != 0x00 || bytes[7] != 0x56) {
        return false;
    }
    if (bytes[9] != 0x12 || bytes[10] != 0x00 || bytes[11] != 0x05) {
        return false;
    }
    if (bytes[18] != 0xF7 || bytes[19] != 0x00) {
        return false;
    }
    const uint8_t channel = bytes[13];
    const uint8_t parameter = bytes[14];
    const uint8_t value = bytes[15];
    if (channel > 1) {
        return false;
    }
    if (parameter == 0x01 || parameter == 0x02) {
        if (value > 1) {
            return false;
        }
    } else if (parameter == 0x04) {
        if (value > 108) {
            return false;
        }
    } else {
        return false;
    }
    const int total = 0x00 + 0x05 + (int)channel + (int)parameter + (int)value;
    const uint8_t sum = (uint8_t)((0 - total) & 0x7F);
    return bytes[17] == sum;
}

bool IsAutoSensPacket(const uint8_t* bytes, uint32_t length)
{
    static const uint8_t packet[20] = {
        0x14, 0xF0, 0x41, 0x10,
        0x14, 0x00, 0x00, 0x56,
        0x14, 0x12, 0x00, 0x02,
        0x14, 0x01, 0x02, 0x01,
        0x16, 0x7A, 0xF7, 0x00
    };
    return bytes != nullptr && length == 20 && memcmp(bytes, packet, 20) == 0;
}

bool IsStateRequestPacket(const uint8_t* bytes, uint32_t length)
{
    static const uint8_t packet[24] = {
        0x14, 0xF0, 0x41, 0x10,
        0x14, 0x00, 0x00, 0x56,
        0x14, 0x11, 0x01, 0x00,
        0x14, 0x00, 0x00, 0x00,
        0x14, 0x00, 0x00, 0x3B,
        0x16, 0x44, 0xF7, 0x00
    };
    return bytes != nullptr && length == 24 && memcmp(bytes, packet, 24) == 0;
}

} // namespace

kern_return_t UA55UsbStream::SendMidi(const uint8_t* bytes, uint32_t length)
{
    const bool loCut = IsLoCutPacket(bytes, length);
    const bool autoSens = IsAutoSensPacket(bytes, length);
    const bool stateRequest = IsStateRequestPacket(bytes, length);
    if (!loCut && !autoSens && !stateRequest) {
        return kIOReturnBadArgument;
    }
    if (midiOutPipe_ == nullptr) {
        return kIOReturnOffline;
    }

    IOBufferMemoryDescriptor* buffer = nullptr;
    kern_return_t result = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, length, 0, &buffer);
    if (result != kIOReturnSuccess || buffer == nullptr) {
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    result = buffer->SetLength(length);
    if (result != kIOReturnSuccess) {
        OSSafeReleaseNULL(buffer);
        return result;
    }
    IOAddressSegment range = {};
    result = buffer->GetAddressRange(&range);
    if (result != kIOReturnSuccess || range.address == 0) {
        OSSafeReleaseNULL(buffer);
        return result != kIOReturnSuccess ? result : kIOReturnNoMemory;
    }
    memcpy(reinterpret_cast<void*>(range.address), bytes, length);

    uint32_t transferred = 0;
    result = midiOutPipe_->IO(buffer, length, &transferred, 1000);
    OSSafeReleaseNULL(buffer);
    if (stateRequest) {
        os_log(OS_LOG_DEFAULT, "[UA55] state rq1 status=0x%08x transferred=%u",
               (unsigned int)result, transferred);
    } else if (autoSens) {
        os_log(OS_LOG_DEFAULT, "[UA55] autosens press status=0x%08x transferred=%u",
               (unsigned int)result, transferred);
    } else {
        os_log(OS_LOG_DEFAULT, "[UA55] preamp out param=%u ch=%u val=%u status=0x%08x transferred=%u",
               bytes[14], bytes[13], bytes[15], (unsigned int)result, transferred);
    }
    if (result == kIOReturnSuccess && transferred != length) {
        return kIOReturnUnderrun;
    }
    return result;
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
                IngestMidiBytes(bytes, actualByteCount);
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

void UA55UsbStream::SetSensListener(void* context, void (*listener)(void* context, uint8_t channel, uint8_t db))
{
    sensContext_ = context;
    sensListener_ = listener;
}

void UA55UsbStream::CopySens(uint8_t* left, uint8_t* right) const
{
    if (left != nullptr) {
        *left = sensDb_[0];
    }
    if (right != nullptr) {
        *right = sensDb_[1];
    }
}

void UA55UsbStream::IngestMidiBytes(const uint8_t* bytes, uint32_t length)
{
    if (bytes == nullptr || length < 4) {
        return;
    }
    for (uint32_t offset = 0; offset + 4 <= length; offset += 4) {
        const uint8_t cin = bytes[offset] & 0x0F;
        const uint8_t* data = bytes + offset + 1;
        uint8_t count = 0;
        if (cin == 0x4) {
            count = 3;
        } else if (cin == 0x5) {
            count = 1;
        } else if (cin == 0x6) {
            count = 2;
        } else if (cin == 0x7) {
            count = 3;
        } else if (cin >= 0x2 && cin <= 0x3) {
            count = (cin == 0x2) ? 2 : 3;
            NoteDeviceMessage("midi", data, count);
            continue;
        } else if (cin >= 0x8 && cin <= 0xE) {
            count = (cin == 0xC || cin == 0xD) ? 2 : 3;
            NoteDeviceMessage("midi", data, count);
            continue;
        } else {
            continue;
        }
        for (uint8_t index = 0; index < count; index++) {
            const uint8_t value = data[index];
            if (value == 0xF0) {
                sysexLen_ = 0;
                sysexOpen_ = true;
            }
            if (!sysexOpen_) {
                continue;
            }
            if (sysexLen_ >= sizeof(sysex_)) {
                sysexOpen_ = false;
                sysexLen_ = 0;
                continue;
            }
            sysex_[sysexLen_++] = value;
            if (value == 0xF7) {
                HandleSysEx(sysex_, sysexLen_);
                sysexOpen_ = false;
                sysexLen_ = 0;
            }
        }
    }
}

void UA55UsbStream::HandleSysEx(const uint8_t* msg, uint32_t length)
{
    // F0 41 <dev> 00 00 56 12 <addr 4> <data...> <csum> F7
    if (msg == nullptr || length < 14 || msg[0] != 0xF0 || msg[length - 1] != 0xF7) {
        return;
    }
    if (msg[1] != 0x41 || msg[3] != 0x00 || msg[4] != 0x00 || msg[5] != 0x56 || msg[6] != 0x12) {
        return;
    }
    uint32_t sum = 0;
    for (uint32_t index = 7; index + 2 < length; index++) {
        sum += msg[index];
    }
    const uint8_t expect = (uint8_t)((0x80 - (sum & 0x7F)) & 0x7F);
    if (msg[length - 2] != expect) {
        return;
    }
    const uint8_t* addr = msg + 7;
    const uint8_t data = msg[11];
    const uint32_t dataBytes = length - 13;
    // RQ1 01 00 00 00 / 59 bytes. O log curto de 8 bytes não chega na tela.
    if (dataBytes == 59 && addr[0] == 0x01 && addr[1] == 0x00 && addr[2] == 0x00 && addr[3] == 0x00) {
        char hex[4 * 2 + 59 * 2 + 1];
        uint32_t used = 0;
        for (uint32_t index = 0; index < 4; index++) {
            used += (uint32_t)snprintf(hex + used, sizeof(hex) - used, "%02x", addr[index]);
        }
        for (uint32_t index = 0; index < 59; index++) {
            used += (uint32_t)snprintf(hex + used, sizeof(hex) - used, "%02x", msg[11 + index]);
        }
        os_log(OS_LOG_DEFAULT, "[UA55] dt1 %{public}s", hex);
        return;
    }
    // 00 05 <canal> 04 = SENS daquele preamp. O byte vai de 0 a 127
    // (máximo de um byte MIDI); acima de 54 continua o mesmo ganho.
    // 00 02 01 03 = AUTO SENS: 02 ligado, 00 desligado. O par 00 02 01 02
    // chega junto e não é outro comando. Cada toque repete os mesmos bytes,
    // então não passa pelo filtro que esconde DT1 já visto.
    const bool sens = dataBytes >= 1 && addr[0] == 0x00 && addr[1] == 0x05 && addr[3] == 0x04
        && addr[2] <= 1 && data <= 127;
    if (dataBytes >= 1 && addr[0] == 0x00 && addr[1] == 0x02 && addr[2] == 0x01 && addr[3] == 0x02
        && (data == 0x01 || data == 0x02)) {
        return;
    }
    if (dataBytes >= 1 && addr[0] == 0x00 && addr[1] == 0x02 && addr[2] == 0x01 && addr[3] == 0x03
        && (data == 0x02 || data == 0x00)) {
        os_log(OS_LOG_DEFAULT, "[UA55] autosens %{public}s", data == 0x02 ? "on" : "off");
        return;
    }
    if (!sens) {
        uint8_t packed[12] = {};
        const uint32_t copyData = dataBytes > 8 ? 8 : dataBytes;
        memcpy(packed, addr, 4);
        if (copyData > 0) {
            memcpy(packed + 4, msg + 11, copyData);
        }
        NoteDeviceMessage("dt1", packed, 4 + copyData);
        return;
    }
    const uint8_t channel = addr[2];
    if (sensDb_[channel] == data) {
        return;
    }
    sensDb_[channel] = data;
    os_log(OS_LOG_DEFAULT, "[UA55] sens %u = %u dB", channel + 1, data);
    if (sensListener_ != nullptr) {
        sensListener_(sensContext_, channel, data);
    }
}

void UA55UsbStream::NoteDeviceMessage(const char* tag, const uint8_t* bytes, uint32_t length)
{
    if (tag == nullptr || bytes == nullptr || length == 0) {
        return;
    }
    if (length > 12) {
        length = 12;
    }
    for (uint32_t index = 0; index < 16; index++) {
        SeenDeviceMessage* slot = &seenMessage_[index];
        if (slot->used && slot->length == length && memcmp(slot->bytes, bytes, length) == 0) {
            return;
        }
    }
    SeenDeviceMessage* stored = &seenMessage_[seenMessageNext_];
    seenMessageNext_ = (uint8_t)((seenMessageNext_ + 1) % 16);
    memset(stored, 0, sizeof(*stored));
    memcpy(stored->bytes, bytes, length);
    stored->length = (uint8_t)length;
    stored->used = true;

    char hex[40];
    uint32_t used = 0;
    for (uint32_t index = 0; index < length && used + 3 < sizeof(hex); index++) {
        const int wrote = snprintf(hex + used, sizeof(hex) - used, "%02x", bytes[index]);
        if (wrote < 0) {
            return;
        }
        used += (uint32_t)wrote;
    }
    os_log(OS_LOG_DEFAULT, "[UA55] %{public}s %{public}s", tag, hex);
}

void UA55UsbStream::TearDownMidiPipe(IOService* closer)
{
    StopMidiPolling();
    if (midiInPipe_ != nullptr && closer != nullptr) {
        midiInPipe_->Abort(0, kIOReturnAborted, closer);
    }
    if (midiOutPipe_ != nullptr && closer != nullptr) {
        midiOutPipe_->Abort(0, kIOReturnAborted, closer);
    }
    IOSleep(20);

    for (uint32_t slotIndex = 0; slotIndex < kUA55MidiInSlotCount; slotIndex++) {
        FreeMidiSlot(&midiInSlots_[slotIndex]);
    }
    OSSafeReleaseNULL(midiInPipe_);
    OSSafeReleaseNULL(midiOutPipe_);

    if (midiInterface_ != nullptr && midiOpened_ && closer != nullptr) {
        midiInterface_->SelectAlternateSetting(0);
        midiInterface_->Close(closer, 0);
        midiOpened_ = false;
    }
    OSSafeReleaseNULL(midiInterface_);
    midiCompletions_ = 0;
    midiBytes_ = 0;
    midiErrors_ = 0;
    sysexOpen_ = false;
    sysexLen_ = 0;
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
    kern_return_t result = FillFrameList(slot, rate_.captureMaxPacket);
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

    __atomic_fetch_add(&isochInFlight_, 1, __ATOMIC_ACQ_REL);
    result = capturePipe_->IsochIO(slot->dataBuffer, slot->frameListBuffer, 0, slot->action);
    if (result != kIOReturnSuccess) {
        __atomic_fetch_sub(&isochInFlight_, 1, __ATOMIC_ACQ_REL);
    }
    return result;
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

    __atomic_fetch_add(&isochInFlight_, 1, __ATOMIC_ACQ_REL);
    result = playbackPipe_->IsochIO(slot->dataBuffer, slot->frameListBuffer, 0, slot->action);
    if (result != kIOReturnSuccess) {
        __atomic_fetch_sub(&isochInFlight_, 1, __ATOMIC_ACQ_REL);
    }
    return result;
}

void UA55UsbStream::OnCaptureComplete(uint32_t slotIndex, IOReturn status)
{
    __atomic_fetch_sub(&isochInFlight_, 1, __ATOMIC_ACQ_REL);
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
    __atomic_fetch_sub(&isochInFlight_, 1, __ATOMIC_ACQ_REL);
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
        kern_return_t result = PrepareIsochSlot(&captureSlots_[index], true, rate_.captureMaxPacket, index);
        if (result != kIOReturnSuccess) {
            return result;
        }
        result = PrepareIsochSlot(&playbackSlots_[index], false, rate_.playbackMaxPacket, index);
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
    lastCaptureAudioFrames_ = rate_.nominalFramesPerTransfer;
    capturePatternValid_ = false;
    if (playbackBridge_ != nullptr) {
        memset(playbackBridge_, 0, (size_t)bridgeFrames_ * kUA55OutputChannels * sizeof(int32_t));
    }
    if (captureBridge_ != nullptr) {
        memset(captureBridge_, 0, (size_t)bridgeFrames_ * kUA55InputChannels * sizeof(int32_t));
    }
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
    streamedSinceOpen_ = true;

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

    os_log(OS_LOG_DEFAULT, "[UA55] UsbStream duplex started rate=%u alt=%u depth=%u",
           rate_.rateInt, rate_.alternate, kUA55IsochRingDepth);
    return kIOReturnSuccess;
}

void UA55UsbStream::StopStreaming()
{
    if (!streaming_) {
        return;
    }

    const uint64_t caps = captureCompletions_;
    const uint64_t plays = playbackCompletions_;

    // Sem Pipe::Abort: Abort faz a UA-55 reenumerar. Espera os isoc
    // em voo terminarem antes de libertar os slots.
    streaming_ = false;
    ClearTimestampTarget();
    const bool drained = DrainIsoch();
    if (!drained) {
        // A fila das completions está bloqueada por esta espera. Devolve
        // o stream para os pacotes em voo se reencaminharem, em vez de
        // abrir outro anel por cima.
        streaming_ = true;
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream isoch drain timeout inFlight=%u",
               __atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE));
        return;
    }

    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        FreeIsochSlot(&captureSlots_[index]);
        FreeIsochSlot(&playbackSlots_[index]);
    }

    os_log(OS_LOG_DEFAULT,
           "[UA55] UsbStream soft-stopped captureCompletions=%llu playbackCompletions=%llu",
           caps,
           plays);
}

bool UA55UsbStream::FinishRateChangeDrain()
{
    const uint64_t caps = captureCompletions_;
    const uint64_t plays = playbackCompletions_;
    if (!DrainIsoch()) {
        streaming_ = true;
        os_log(OS_LOG_DEFAULT, "[UA55] rate change kept isoc, drain timeout inFlight=%u",
               __atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE));
        return false;
    }

    for (uint32_t index = 0; index < kUA55IsochRingDepth; index++) {
        FreeIsochSlot(&captureSlots_[index]);
        FreeIsochSlot(&playbackSlots_[index]);
    }
    os_log(OS_LOG_DEFAULT,
           "[UA55] isoc drained for rate change capture=%llu playback=%llu",
           caps,
           plays);
    return true;
}

void UA55UsbStream::BeginRateChangeQuiesce()
{
    streaming_ = false;
    ClearTimestampTarget();
}

bool UA55UsbStream::DrainIsoch()
{
    for (uint32_t waited = 0; waited < 200; waited++) {
        if (__atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE) == 0) {
            return true;
        }
        IOSleep(1);
    }
    return __atomic_load_n(&isochInFlight_, __ATOMIC_ACQUIRE) == 0;
}
