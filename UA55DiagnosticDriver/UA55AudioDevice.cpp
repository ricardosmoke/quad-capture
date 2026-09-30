#include <os/log.h>
#include <string.h>

#include <DriverKit/DriverKit.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOTimerDispatchSource.h>
#include <AudioDriverKit/AudioDriverKit.h>

#include "UA55AudioDevice.h"
#include "UA55MuteControl.h"
#include "UA55UsbStream.h"
#include "UA55VolumeControl.h"
#include "UA55USBConstants.h"

using namespace AudioDriverKit;

struct UA55AudioDevice_IVars {
    OSSharedPtr<IODispatchQueue> workQueue;
    OSSharedPtr<IODispatchQueue> rateQueue;
    OSSharedPtr<IOUserAudioStream> outputStream;
    OSSharedPtr<IOUserAudioStream> inputStream;
    OSSharedPtr<IOBufferMemoryDescriptor> outputBuffer;
    OSSharedPtr<IOBufferMemoryDescriptor> inputBuffer;
    OSSharedPtr<IOMemoryMap> outputMap;
    OSSharedPtr<IOMemoryMap> inputMap;
    OSSharedPtr<IOTimerDispatchSource> ztsTimer;
    OSSharedPtr<OSAction> ztsAction;
    UA55UsbStream* usbStream;
    OSSharedPtr<UA55VolumeControl> volumeMain;
    OSSharedPtr<UA55VolumeControl> volumeLeft;
    OSSharedPtr<UA55VolumeControl> volumeRight;
    OSSharedPtr<UA55MuteControl> muteMain;
    OSSharedPtr<UA55MuteControl> muteLeft;
    OSSharedPtr<UA55MuteControl> muteRight;
    volatile uint64_t sampleTime;
    uint64_t lastPublishedZts;
    uint64_t hostTicksPerPeriod;
    uint64_t ticksPerMs;
    uint32_t ringFrames;
    uint32_t currentRateInt;
    double pendingRate;
    bool rateChangeInFlight;
    bool ioRunning;
};

namespace {

// 1, 2 e 3 são IOUserAudioReservedConfigChangeAction (SampleRate,
// RingBufferFrameSize, StreamFormat). Usar 1 faz o Perform do HAL
// reaplicar a taxa antiga e o CoreAudio religa o áudio antes da troca.
static const uint64_t kUA55ConfigChangeSampleRate = 100;

IOUserAudioStreamBasicDescription MakeFloatFormat(uint32_t channels, double sampleRate)
{
    IOUserAudioStreamBasicDescription format = {};
    format.mSampleRate = sampleRate;
    format.mFormatID = IOUserAudioFormatID::LinearPCM;
    format.mFormatFlags = static_cast<IOUserAudioFormatFlags>(
        LinearPCMFormatFlagIsFloat | LinearPCMFormatFlagIsPacked);
    format.mBytesPerPacket = channels * sizeof(float);
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = channels * sizeof(float);
    format.mChannelsPerFrame = channels;
    format.mBitsPerChannel = 32;
    format.mReserved = 0;
    return format;
}

void PublishChannelLayout(UA55AudioDevice* device, uint32_t outCh, uint32_t inCh)
{
    IOUserAudioChannelLabel outLabels[kUA55OutputChannels] = {
        IOUserAudioChannelLabel::Left,
        IOUserAudioChannelLabel::Right,
        IOUserAudioChannelLabel::LeftSurround,
        IOUserAudioChannelLabel::RightSurround,
    };
    IOUserAudioChannelLabel inLabels[kUA55InputChannels] = {
        IOUserAudioChannelLabel::Left,
        IOUserAudioChannelLabel::Right,
        IOUserAudioChannelLabel::Center,
        IOUserAudioChannelLabel::LeftSurround,
        IOUserAudioChannelLabel::RightSurround,
        IOUserAudioChannelLabel::Unknown,
    };
    if (outCh == 0 || outCh > kUA55OutputChannels) {
        outCh = kUA55OutputChannels;
    }
    if (inCh == 0 || inCh > kUA55InputChannels) {
        inCh = kUA55InputChannels;
    }
    device->SetPreferredOutputChannelLayout(outLabels, outCh);
    device->SetPreferredInputChannelLayout(inLabels, inCh);
}

void PublishStreamFormats(UA55AudioDevice* device, UA55AudioDevice_IVars* ivars, double sampleRate)
{
    IOUserAudioStreamBasicDescription outFormats[kUA55RateCount];
    IOUserAudioStreamBasicDescription inFormats[kUA55RateCount];
    uint32_t current = 0;
    for (uint32_t index = 0; index < kUA55RateCount; index++) {
        outFormats[index] = MakeFloatFormat(kUA55Rates[index].outputChannels, kUA55Rates[index].rate);
        inFormats[index] = MakeFloatFormat(kUA55Rates[index].inputChannels, kUA55Rates[index].rate);
        if (kUA55Rates[index].rateInt == (uint32_t)sampleRate) {
            current = index;
        }
    }
    ivars->outputStream->SetAvailableStreamFormats(outFormats, kUA55RateCount);
    ivars->inputStream->SetAvailableStreamFormats(inFormats, kUA55RateCount);
    ivars->outputStream->SetCurrentStreamFormat(&outFormats[current]);
    ivars->inputStream->SetCurrentStreamFormat(&inFormats[current]);
    PublishChannelLayout(device, kUA55Rates[current].outputChannels, kUA55Rates[current].inputChannels);

    // O anel do HAL é buffer/bytesPorFrame. Em 192 kHz o formato é estéreo;
    // encolher o comprimento mantém os 512 frames, senão o host vê o dobro.
    const uint64_t outBytes =
        (uint64_t)ivars->ringFrames * kUA55Rates[current].outputChannels * sizeof(float);
    const uint64_t inBytes =
        (uint64_t)ivars->ringFrames * kUA55Rates[current].inputChannels * sizeof(float);
    uint64_t currentOut = 0;
    uint64_t currentIn = 0;
    if (ivars->outputBuffer.get() != nullptr &&
        ivars->outputBuffer->GetLength(&currentOut) == kIOReturnSuccess &&
        currentOut != outBytes &&
        ivars->outputBuffer->SetLength(outBytes) == kIOReturnSuccess) {
        ivars->outputStream->SetIOMemoryDescriptor(ivars->outputBuffer.get());
    }
    if (ivars->inputBuffer.get() != nullptr &&
        ivars->inputBuffer->GetLength(&currentIn) == kIOReturnSuccess &&
        currentIn != inBytes &&
        ivars->inputBuffer->SetLength(inBytes) == kIOReturnSuccess) {
        ivars->inputStream->SetIOMemoryDescriptor(ivars->inputBuffer.get());
    }
}

void UpdateTimebase(UA55AudioDevice_IVars* ivars, double sampleRate)
{
    const uint64_t ticksPerMs = ivars->ticksPerMs != 0 ? ivars->ticksPerMs : 1000000ull;
    ivars->hostTicksPerPeriod =
        (ticksPerMs * 1000ull * (uint64_t)kUA55ZeroTimestampPeriod) / (uint64_t)sampleRate;
    if (ivars->hostTicksPerPeriod < 1000ull) {
        ivars->hostTicksPerPeriod =
            (uint64_t)((double)kUA55ZeroTimestampPeriod * 1.0e9 / sampleRate);
    }
}

bool AddOutputVolume(UA55AudioDevice* device,
                     IOUserAudioDriver* driver,
                     uint32_t element,
                     const char* name,
                     OSSharedPtr<UA55VolumeControl>& slot)
{
    const IOUserAudioLevelControlRange range = { -96.0f, 0.0f };
    auto control = OSSharedPtr(OSTypeAlloc(UA55VolumeControl), OSNoRetain);
    if (control.get() == nullptr) {
        return false;
    }
    if (!control->init(driver,
                       true,
                       0.0f,
                       range,
                       element,
                       IOUserAudioObjectPropertyScope::Output,
                       IOUserAudioClassID::VolumeControl)) {
        return false;
    }
    auto label = OSSharedPtr(OSString::withCString(name), OSNoRetain);
    control->SetName(label.get());
    if (device->AddControl(control.get()) != kIOReturnSuccess) {
        return false;
    }
    slot = control;
    return true;
}

bool AddOutputMute(UA55AudioDevice* device,
                   IOUserAudioDriver* driver,
                   uint32_t element,
                   const char* name,
                   OSSharedPtr<UA55MuteControl>& slot)
{
    auto control = OSSharedPtr(OSTypeAlloc(UA55MuteControl), OSNoRetain);
    if (control.get() == nullptr) {
        return false;
    }
    if (!control->init(driver,
                       true,
                       false,
                       element,
                       IOUserAudioObjectPropertyScope::Output,
                       IOUserAudioClassID::MuteControl)) {
        return false;
    }
    auto label = OSSharedPtr(OSString::withCString(name), OSNoRetain);
    control->SetName(label.get());
    if (device->AddControl(control.get()) != kIOReturnSuccess) {
        return false;
    }
    slot = control;
    return true;
}

} // namespace

extern "C" void UA55AudioDevicePublishTimestamp(void* device, uint64_t sampleTime, uint64_t hostTime)
{
    if (device == nullptr) {
        return;
    }
    // Mantido por compatibilidade; o ZTS oficial passa pelo timer na work queue.
    static_cast<UA55AudioDevice*>(device)->PublishUsbTimestamp(sampleTime, hostTime);
}

bool UA55AudioDevice::init(IOUserAudioDriver* in_driver,
                           bool in_supports_prewarming,
                           OSString* in_device_uid,
                           OSString* in_model_uid,
                           OSString* in_manufacturer_uid,
                           uint32_t in_zero_timestamp_period)
{
    if (!super::init(in_driver,
                     in_supports_prewarming,
                     in_device_uid,
                     in_model_uid,
                     in_manufacturer_uid,
                     in_zero_timestamp_period)) {
        return false;
    }

    ivars = IONewZero(UA55AudioDevice_IVars, 1);
    if (ivars == nullptr) {
        return false;
    }

    ivars->workQueue = in_driver->GetWorkQueue();
    IODispatchQueue::Create("UA55Rate", 0, 0, ivars->rateQueue.attach());
    ivars->ringFrames = kUA55HalRingFrames;
    // Calibração grosseira do timebase (IOSleep 5 ms).
    {
        const uint64_t t0 = mach_absolute_time();
        IOSleep(5);
        const uint64_t t1 = mach_absolute_time();
        ivars->ticksPerMs = (t1 > t0) ? ((t1 - t0) / 5ull) : 1000000ull;
        UpdateTimebase(ivars, kUA55SampleRate);
        os_log(OS_LOG_DEFAULT,
               "[UA55] timebase ticks/ms=%llu hostTicks/period=%llu",
               ivars->ticksPerMs,
               ivars->hostTicksPerPeriod);
    }

    double rates[kUA55RateCount];
    for (uint32_t index = 0; index < kUA55RateCount; index++) {
        rates[index] = kUA55Rates[index].rate;
    }
    SetAvailableSampleRates(rates, kUA55RateCount);
    SetSampleRate(kUA55SampleRate);
    ivars->currentRateInt = kUA55SampleRateInt;
    ivars->pendingRate = kUA55SampleRate;
    SetTransportType(IOUserAudioTransportType::USB);
    SetCanBeDefaultInputDevice(true);
    SetCanBeDefaultOutputDevice(true);
    SetCanBeDefaultSystemOutputDevice(true);
    SetPreferredChannelsForStereo(1, 2);

    PublishChannelLayout(this, kUA55OutputChannels, kUA55InputChannels);

    const uint64_t outBytes = (uint64_t)ivars->ringFrames * kUA55OutputChannels * sizeof(float);
    const uint64_t inBytes = (uint64_t)ivars->ringFrames * kUA55InputChannels * sizeof(float);

    kern_return_t error = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, outBytes, 0, ivars->outputBuffer.attach());
    if (error != kIOReturnSuccess) {
        return false;
    }
    error = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, inBytes, 0, ivars->inputBuffer.attach());
    if (error != kIOReturnSuccess) {
        return false;
    }

    auto outName = OSSharedPtr(OSString::withCString("Output"), OSNoRetain);
    auto inName = OSSharedPtr(OSString::withCString("Input"), OSNoRetain);
    const IOUserAudioStreamBasicDescription outFormat = MakeFloatFormat(kUA55OutputChannels, kUA55SampleRate);
    const IOUserAudioStreamBasicDescription inFormat = MakeFloatFormat(kUA55InputChannels, kUA55SampleRate);

    ivars->outputStream = IOUserAudioStream::Create(in_driver, IOUserAudioStreamDirection::Output, ivars->outputBuffer.get());
    if (ivars->outputStream.get() == nullptr) {
        return false;
    }
    ivars->outputStream->SetName(outName.get());
    ivars->outputStream->SetCurrentStreamFormat(&outFormat);

    ivars->inputStream = IOUserAudioStream::Create(in_driver, IOUserAudioStreamDirection::Input, ivars->inputBuffer.get());
    if (ivars->inputStream.get() == nullptr) {
        return false;
    }
    ivars->inputStream->SetName(inName.get());
    ivars->inputStream->SetCurrentStreamFormat(&inFormat);
    PublishStreamFormats(this, ivars, kUA55SampleRate);

    error = AddStream(ivars->outputStream.get());
    if (error != kIOReturnSuccess) {
        return false;
    }
    error = AddStream(ivars->inputStream.get());
    if (error != kIOReturnSuccess) {
        return false;
    }

    // Elemento 0 = main; 1 e 2 = par estéreo preferido (slider do macOS).
    const bool volumesOk =
        AddOutputVolume(this, in_driver, 0, "Volume", ivars->volumeMain) &&
        AddOutputVolume(this, in_driver, 1, "Volume Left", ivars->volumeLeft) &&
        AddOutputVolume(this, in_driver, 2, "Volume Right", ivars->volumeRight) &&
        AddOutputMute(this, in_driver, 0, "Mute", ivars->muteMain) &&
        AddOutputMute(this, in_driver, 1, "Mute Left", ivars->muteLeft) &&
        AddOutputMute(this, in_driver, 2, "Mute Right", ivars->muteRight);
    if (!volumesOk) {
        os_log(OS_LOG_DEFAULT, "[UA55] output volume controls failed — level stays full scale");
    }

    error = SetIOOperationHandler(^kern_return_t(IOUserAudioObjectID,
                                                 IOUserAudioIOOperation operation,
                                                 uint32_t frameCount,
                                                 uint64_t sampleTime,
                                                 uint64_t hostTime) {
        (void)hostTime;
        if (ivars == nullptr || !ivars->ioRunning || ivars->usbStream == nullptr) {
            return kIOReturnSuccess;
        }
        float* outAddr = nullptr;
        float* inAddr = nullptr;
        if (ivars->outputMap.get() != nullptr) {
            outAddr = reinterpret_cast<float*>(ivars->outputMap->GetAddress());
        }
        if (ivars->inputMap.get() != nullptr) {
            inAddr = reinterpret_cast<float*>(ivars->inputMap->GetAddress());
        }
        if (operation == IOUserAudioIOOperationWriteEnd && outAddr != nullptr) {
            ivars->usbStream->HalWriteOutput(outAddr, ivars->ringFrames, sampleTime, frameCount);
        } else if (operation == IOUserAudioIOOperationBeginRead && inAddr != nullptr) {
            ivars->usbStream->HalReadInput(inAddr, ivars->ringFrames, sampleTime, frameCount);
        }
        return kIOReturnSuccess;
    });
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] SetIOOperationHandler=FAILED 0x%08x", (unsigned int)error);
        return false;
    }

    // Timer só como fallback de wake; o ZTS vem do USB (PublishUsbTimestamp).
    error = IOTimerDispatchSource::Create(ivars->workQueue.get(), ivars->ztsTimer.attach());
    if (error != kIOReturnSuccess || ivars->ztsTimer.get() == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] ZTS timer create failed 0x%08x", (unsigned int)error);
        return false;
    }
    error = CreateActionTimerOccurred(sizeof(void*), ivars->ztsAction.attach());
    if (error != kIOReturnSuccess || ivars->ztsAction.get() == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] ZTS action create failed 0x%08x", (unsigned int)error);
        return false;
    }
    error = ivars->ztsTimer->SetHandler(ivars->ztsAction.get());
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] ZTS SetHandler failed 0x%08x", (unsigned int)error);
        return false;
    }

    os_log(OS_LOG_DEFAULT,
           "[UA55] build=%u audio device configured 4out/6in @ 44.1/48/96 kHz, 2out/2in @ 192 kHz volume=%d",
           kUA55DriverBuild,
           volumesOk ? 1 : 0);
    return true;
}

void UA55AudioDevice::free(void)
{
    if (ivars != nullptr) {
        ivars->ioRunning = false;
        if (ivars->ztsTimer.get() != nullptr) {
            ivars->ztsTimer->SetEnable(false);
            ivars->ztsTimer->Cancel(^() {});
        }
        ivars->ztsTimer.reset();
        ivars->ztsAction.reset();
        if (ivars->usbStream != nullptr) {
            ivars->usbStream->StopStreaming();
            ivars->usbStream = nullptr;
        }
        ivars->outputMap.reset();
        ivars->inputMap.reset();
        ivars->outputStream.reset();
        ivars->inputStream.reset();
        ivars->outputBuffer.reset();
        ivars->inputBuffer.reset();
        ivars->workQueue.reset();
        ivars->rateQueue.reset();
    }
    IOSafeDeleteNULL(ivars, UA55AudioDevice_IVars, 1);
    super::free();
}

kern_return_t UA55AudioDevice::ConfigureHardware(uint64_t usbStreamAddr)
{
    if (ivars == nullptr) {
        return kIOReturnBadArgument;
    }
    ivars->usbStream = reinterpret_cast<UA55UsbStream*>(usbStreamAddr);
    if (ivars->volumeMain.get() != nullptr) {
        ivars->volumeMain->BindStream(usbStreamAddr, 2);
    }
    if (ivars->volumeLeft.get() != nullptr) {
        ivars->volumeLeft->BindStream(usbStreamAddr, 0);
    }
    if (ivars->volumeRight.get() != nullptr) {
        ivars->volumeRight->BindStream(usbStreamAddr, 1);
    }
    if (ivars->muteMain.get() != nullptr) {
        ivars->muteMain->BindStream(usbStreamAddr, 2);
    }
    if (ivars->muteLeft.get() != nullptr) {
        ivars->muteLeft->BindStream(usbStreamAddr, 0);
    }
    if (ivars->muteRight.get() != nullptr) {
        ivars->muteRight->BindStream(usbStreamAddr, 1);
    }
    if (ivars->usbStream != nullptr) {
        const UA55RateConfig* hardware = UA55RateForHz((double)ivars->usbStream->CurrentRate());
        if (hardware != nullptr && hardware->rateInt != ivars->currentRateInt) {
            ivars->currentRateInt = hardware->rateInt;
            ivars->pendingRate = hardware->rate;
            UpdateTimebase(ivars, hardware->rate);
            SetSampleRate(hardware->rate);
            if (ivars->outputStream.get() != nullptr && ivars->inputStream.get() != nullptr) {
                PublishStreamFormats(this, ivars, hardware->rate);
            }
            os_log(OS_LOG_DEFAULT, "[UA55] adopted hardware rate %u Hz", hardware->rateInt);
        }
    }
    return kIOReturnSuccess;
}

void UA55AudioDevice::PublishUsbTimestamp(uint64_t sampleTime, uint64_t hostTime)
{
    if (ivars == nullptr || !ivars->ioRunning) {
        return;
    }
    if (sampleTime <= ivars->lastPublishedZts) {
        return;
    }
    // Direto (sem DispatchAsync): menor atraso → menos underrun no bridge.
    ivars->lastPublishedZts = sampleTime;
    ivars->sampleTime = sampleTime;
    UpdateCurrentZeroTimestamp(sampleTime, hostTime);
}

void UA55AudioDevice::TimerOccurred_Impl(OSAction* action, uint64_t time)
{
    (void)action;
    (void)time;
    // ZTS é publicado pelo USB; o timer não mexe no relógio do HAL.
    if (ivars == nullptr || !ivars->ioRunning || ivars->ztsTimer.get() == nullptr) {
        return;
    }
    const uint64_t wake = mach_absolute_time() + ivars->hostTicksPerPeriod;
    ivars->ztsTimer->WakeAtTime(kIOTimerClockMachAbsoluteTime, wake, 0);
}

kern_return_t UA55AudioDevice::HandleChangeSampleRate(double in_sample_rate)
{
    const UA55RateConfig* mode = UA55RateForHz(in_sample_rate);
    if (mode == nullptr || ivars == nullptr) {
        return kIOReturnUnsupported;
    }
    if (mode->rateInt == ivars->currentRateInt && !ivars->rateChangeInFlight) {
        return kIOReturnSuccess;
    }
    ivars->pendingRate = mode->rate;
    os_log(OS_LOG_DEFAULT, "[UA55] sample rate request %u Hz", mode->rateInt);
    if (ivars->rateChangeInFlight) {
        return kIOReturnSuccess;
    }
    ivars->rateChangeInFlight = true;
    if (ivars->usbStream != nullptr &&
        ivars->usbStream->IsStreaming() &&
        ivars->rateQueue.get() != nullptr) {
        ivars->usbStream->BeginRateChangeQuiesce();
        UA55AudioDevice* device = this;
        ivars->rateQueue->DispatchAsync(^{
            if (device->ivars == nullptr || device->ivars->usbStream == nullptr) {
                return;
            }
            if (!device->ivars->usbStream->FinishRateChangeDrain()) {
                device->ivars->rateChangeInFlight = false;
                return;
            }
            const UA55RateConfig* pending = UA55RateForHz(device->ivars->pendingRate);
            const uint32_t currentHz = device->ivars->usbStream->CurrentRate();
            const bool warm441 =
                pending != nullptr &&
                ((pending->rateInt == 44100 && (currentHz == 96000 || currentHz == 192000)) ||
                 (currentHz == 44100 && pending->rateInt == 192000));
            if (warm441 && !device->ivars->usbStream->Warm44100Via48000(pending->rateInt)) {
                device->ivars->rateChangeInFlight = false;
                return;
            }
            const kern_return_t queued =
                device->RequestDeviceConfigurationChange(kUA55ConfigChangeSampleRate, nullptr);
            if (queued != kIOReturnSuccess) {
                device->ivars->rateChangeInFlight = false;
            }
        });
        return kIOReturnSuccess;
    }
    const kern_return_t result = RequestDeviceConfigurationChange(kUA55ConfigChangeSampleRate, nullptr);
    if (result != kIOReturnSuccess) {
        ivars->rateChangeInFlight = false;
    }
    return result;
}

kern_return_t UA55AudioDevice::PerformDeviceConfigurationChange(uint64_t in_change_action,
                                                                OSObject* in_change_info)
{
    if (in_change_action == kUA55ConfigChangeSampleRate && ivars != nullptr) {
        const UA55RateConfig* mode = UA55RateForHz(ivars->pendingRate);
        if (mode == nullptr) {
            ivars->rateChangeInFlight = false;
            return kIOReturnUnsupported;
        }
        if (ivars->usbStream != nullptr) {
            const kern_return_t usbResult = ivars->usbStream->ApplySampleRate(mode->rateInt);
            if (usbResult != kIOReturnSuccess) {
                os_log(OS_LOG_DEFAULT, "[UA55] sample rate USB switch failed 0x%08x",
                       (unsigned int)usbResult);
                ivars->rateChangeInFlight = false;
                return usbResult;
            }
        }
        ivars->currentRateInt = mode->rateInt;
        UpdateTimebase(ivars, mode->rate);
        SetSampleRate(mode->rate);
        if (ivars->outputStream.get() != nullptr && ivars->inputStream.get() != nullptr) {
            PublishStreamFormats(this, ivars, mode->rate);
        }
        os_log(OS_LOG_DEFAULT, "[UA55] sample rate now %u Hz %uout/%uin",
               mode->rateInt, mode->outputChannels, mode->inputChannels);

        ivars->rateChangeInFlight = false;
        const UA55RateConfig* latest = UA55RateForHz(ivars->pendingRate);
        if (latest != nullptr && latest->rateInt != ivars->currentRateInt) {
            ivars->rateChangeInFlight = true;
            const kern_return_t queued =
                RequestDeviceConfigurationChange(kUA55ConfigChangeSampleRate, nullptr);
            if (queued != kIOReturnSuccess) {
                ivars->rateChangeInFlight = false;
            }
        }
    }
    return super::PerformDeviceConfigurationChange(in_change_action, in_change_info);
}

kern_return_t UA55AudioDevice::StartIO(IOUserAudioStartStopFlags in_flags)
{
    if (ivars == nullptr || ivars->workQueue.get() == nullptr) {
        return kIOReturnNotReady;
    }
    // StartIO do segundo cliente (painel) chega já nesta fila. DispatchSync nela
    // mesma não retorna: a música para no StopIO anterior e o painel fica preso.
    if (!ivars->workQueue->OnQueue()) {
        __block kern_return_t hopped = kIOReturnSuccess;
        ivars->workQueue->DispatchSync(^{
            hopped = StartIO(in_flags);
        });
        return hopped;
    }

    __block kern_return_t error = kIOReturnSuccess;
    __block OSSharedPtr<IOMemoryDescriptor> outputMD;
    __block OSSharedPtr<IOMemoryDescriptor> inputMD;
    __block bool startedUsbThisCall = false;

    {
        float* outAddr = nullptr;
        float* inAddr = nullptr;
        uint32_t outCh = kUA55OutputChannels;
        uint32_t inCh = kUA55InputChannels;
        const UA55RateConfig* playing = nullptr;
        const bool alreadyStreaming =
            (ivars->usbStream != nullptr && ivars->usbStream->IsStreaming());

        error = super::StartIO(in_flags);
        if (error != kIOReturnSuccess) {
            return error;
        }

        outputMD = ivars->outputStream->GetIOMemoryDescriptor();
        if (outputMD.get() == nullptr) {
            error = kIOReturnNoMemory;
            goto Failure;
        }
        error = outputMD->CreateMapping(0, 0, 0, 0, 0, ivars->outputMap.attach());
        if (error != kIOReturnSuccess) {
            goto Failure;
        }

        inputMD = ivars->inputStream->GetIOMemoryDescriptor();
        if (inputMD.get() == nullptr) {
            error = kIOReturnNoMemory;
            goto Failure;
        }
        error = inputMD->CreateMapping(0, 0, 0, 0, 0, ivars->inputMap.attach());
        if (error != kIOReturnSuccess) {
            goto Failure;
        }

        outAddr = reinterpret_cast<float*>(ivars->outputMap->GetAddress());
        inAddr = reinterpret_cast<float*>(ivars->inputMap->GetAddress());
        if (outAddr == nullptr || inAddr == nullptr) {
            error = kIOReturnNoMemory;
            goto Failure;
        }

        playing = UA55RateForHz((double)ivars->currentRateInt);
        if (playing != nullptr && playing->outputChannels != 0) {
            outCh = playing->outputChannels;
        }
        if (playing != nullptr && playing->inputChannels != 0) {
            inCh = playing->inputChannels;
        }
        if (!alreadyStreaming) {
            memset(outAddr, 0, (size_t)ivars->ringFrames * outCh * sizeof(float));
            memset(inAddr, 0, (size_t)ivars->ringFrames * inCh * sizeof(float));
            ivars->sampleTime = 0;
            ivars->lastPublishedZts = 0;
            UpdateCurrentZeroTimestamp(0, mach_absolute_time());
        }

        if (ivars->rateChangeInFlight) {
            ivars->ioRunning = true;
            os_log(OS_LOG_DEFAULT, "[UA55] StartIO during rate change — USB stays idle");
            return kIOReturnSuccess;
        }

        if (ivars->usbStream != nullptr) {
            ivars->usbStream->SetTimestampTarget(&ivars->sampleTime, this);
            error = ivars->usbStream->StartStreaming();
            if (error != kIOReturnSuccess) {
                goto Failure;
            }
            startedUsbThisCall = !alreadyStreaming;
            // USB já no ar: não republicar o ZTS daqui. O HAL ainda está dentro
            // deste StartIO, e UpdateCurrentZeroTimestamp espera o HAL.
        }

        ivars->ioRunning = true;

        os_log(OS_LOG_DEFAULT,
               "[UA55] build=%u StartIO success (ring=%u ZTS=%u, already=%d)",
               kUA55DriverBuild,
               kUA55HalRingFrames,
               kUA55ZeroTimestampPeriod,
               alreadyStreaming ? 1 : 0);
        return error;

    Failure:
        ivars->ioRunning = false;
        if (ivars->usbStream != nullptr) {
            ivars->usbStream->ClearTimestampTarget();
            if (startedUsbThisCall) {
                ivars->usbStream->StopStreaming();
            }
        }
        ivars->outputMap.reset();
        ivars->inputMap.reset();
        super::StopIO(in_flags);
        os_log(OS_LOG_DEFAULT, "[UA55] StartIO failed 0x%08x", (unsigned int)error);
        return error;
    }
}

kern_return_t UA55AudioDevice::StopIO(IOUserAudioStartStopFlags in_flags)
{
    if (ivars == nullptr || ivars->workQueue.get() == nullptr) {
        return kIOReturnNotReady;
    }
    if (!ivars->workQueue->OnQueue()) {
        __block kern_return_t hopped = kIOReturnSuccess;
        ivars->workQueue->DispatchSync(^{
            hopped = StopIO(in_flags);
        });
        return hopped;
    }

    ivars->ioRunning = false;
    if (ivars->usbStream != nullptr) {
        ivars->usbStream->ClearTimestampTarget();
    }
    ivars->outputMap.reset();
    ivars->inputMap.reset();
    const kern_return_t error = super::StopIO(in_flags);
    os_log(OS_LOG_DEFAULT, "[UA55] StopIO (HAL only, USB kept) status=0x%08x",
           (unsigned int)error);
    return error;
}
