#include <os/log.h>
#include <string.h>

#include <DriverKit/DriverKit.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOTimerDispatchSource.h>
#include <AudioDriverKit/AudioDriverKit.h>

#include "UA55AudioDevice.h"
#include "UA55UsbStream.h"
#include "UA55USBConstants.h"

using namespace AudioDriverKit;

struct UA55AudioDevice_IVars {
    OSSharedPtr<IODispatchQueue> workQueue;
    OSSharedPtr<IOUserAudioStream> outputStream;
    OSSharedPtr<IOUserAudioStream> inputStream;
    OSSharedPtr<IOBufferMemoryDescriptor> outputBuffer;
    OSSharedPtr<IOBufferMemoryDescriptor> inputBuffer;
    OSSharedPtr<IOMemoryMap> outputMap;
    OSSharedPtr<IOMemoryMap> inputMap;
    OSSharedPtr<IOTimerDispatchSource> ztsTimer;
    OSSharedPtr<OSAction> ztsAction;
    UA55UsbStream* usbStream;
    volatile uint64_t sampleTime;
    uint64_t lastPublishedZts;
    uint64_t hostTicksPerPeriod;
    uint32_t ringFrames;
    bool ioRunning;
};

namespace {

IOUserAudioStreamBasicDescription MakeFloatFormat(uint32_t channels)
{
    IOUserAudioStreamBasicDescription format = {};
    format.mSampleRate = kUA55SampleRate;
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
    ivars->ringFrames = kUA55HalRingFrames;
    // Calibração grosseira do timebase (IOSleep 5 ms).
    {
        const uint64_t t0 = mach_absolute_time();
        IOSleep(5);
        const uint64_t t1 = mach_absolute_time();
        const uint64_t ticksPerMs = (t1 > t0) ? ((t1 - t0) / 5ull) : 1000000ull;
        ivars->hostTicksPerPeriod =
            (ticksPerMs * 1000ull * (uint64_t)kUA55ZeroTimestampPeriod) / (uint64_t)kUA55SampleRateInt;
        if (ivars->hostTicksPerPeriod < 1000ull) {
            ivars->hostTicksPerPeriod =
                (uint64_t)((double)kUA55ZeroTimestampPeriod * 1.0e9 / kUA55SampleRate);
        }
        os_log(OS_LOG_DEFAULT,
               "[UA55] timebase ticks/ms=%llu hostTicks/period=%llu",
               ticksPerMs,
               ivars->hostTicksPerPeriod);
    }

    const double rates[] = { kUA55SampleRate };
    SetAvailableSampleRates(rates, 1);
    SetSampleRate(kUA55SampleRate);
    SetTransportType(IOUserAudioTransportType::USB);
    SetCanBeDefaultInputDevice(true);
    SetCanBeDefaultOutputDevice(true);
    SetCanBeDefaultSystemOutputDevice(true);
    SetPreferredChannelsForStereo(1, 2);

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
    SetPreferredOutputChannelLayout(outLabels, kUA55OutputChannels);
    SetPreferredInputChannelLayout(inLabels, kUA55InputChannels);

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
    const IOUserAudioStreamBasicDescription outFormat = MakeFloatFormat(kUA55OutputChannels);
    const IOUserAudioStreamBasicDescription inFormat = MakeFloatFormat(kUA55InputChannels);

    ivars->outputStream = IOUserAudioStream::Create(in_driver, IOUserAudioStreamDirection::Output, ivars->outputBuffer.get());
    if (ivars->outputStream.get() == nullptr) {
        return false;
    }
    ivars->outputStream->SetName(outName.get());
    ivars->outputStream->SetAvailableStreamFormats(&outFormat, 1);
    ivars->outputStream->SetCurrentStreamFormat(&outFormat);

    ivars->inputStream = IOUserAudioStream::Create(in_driver, IOUserAudioStreamDirection::Input, ivars->inputBuffer.get());
    if (ivars->inputStream.get() == nullptr) {
        return false;
    }
    ivars->inputStream->SetName(inName.get());
    ivars->inputStream->SetAvailableStreamFormats(&inFormat, 1);
    ivars->inputStream->SetCurrentStreamFormat(&inFormat);

    error = AddStream(ivars->outputStream.get());
    if (error != kIOReturnSuccess) {
        return false;
    }
    error = AddStream(ivars->inputStream.get());
    if (error != kIOReturnSuccess) {
        return false;
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
           "[UA55] build=%u audio device configured 4out/6in @ 44.1 kHz (HAL ring=ZTS period)",
           kUA55DriverBuild);
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
    }
    IOSafeDeleteNULL(ivars, UA55AudioDevice_IVars, 1);
    super::free();
}

kern_return_t UA55AudioDevice::ConfigureHardware(uint64_t usbStreamAddr)
{
    if (ivars == nullptr || usbStreamAddr == 0) {
        return kIOReturnBadArgument;
    }
    ivars->usbStream = reinterpret_cast<UA55UsbStream*>(usbStreamAddr);
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

kern_return_t UA55AudioDevice::StartIO(IOUserAudioStartStopFlags in_flags)
{
    __block kern_return_t error = kIOReturnSuccess;
    __block OSSharedPtr<IOMemoryDescriptor> outputMD;
    __block OSSharedPtr<IOMemoryDescriptor> inputMD;
    __block bool startedUsbThisCall = false;

    ivars->workQueue->DispatchSync(^() {
        float* outAddr = nullptr;
        float* inAddr = nullptr;
        const bool alreadyStreaming =
            (ivars->usbStream != nullptr && ivars->usbStream->IsStreaming());

        error = super::StartIO(in_flags);
        if (error != kIOReturnSuccess) {
            return;
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

        // NÃO limpar o ring se o USB já está a correr — apagar mid-stream causa picote.
        if (!alreadyStreaming) {
            memset(outAddr, 0, (size_t)ivars->ringFrames * kUA55OutputChannels * sizeof(float));
            memset(inAddr, 0, (size_t)ivars->ringFrames * kUA55InputChannels * sizeof(float));
            ivars->sampleTime = 0;
            ivars->lastPublishedZts = 0;
            UpdateCurrentZeroTimestamp(0, mach_absolute_time());
        }

        if (ivars->usbStream != nullptr) {
            ivars->usbStream->SetTimestampTarget(&ivars->sampleTime, this);
            error = ivars->usbStream->StartStreaming();
            if (error != kIOReturnSuccess) {
                goto Failure;
            }
            startedUsbThisCall = !alreadyStreaming;

            if (alreadyStreaming) {
                const uint64_t aligned =
                    (ivars->usbStream->CurrentCaptureSample() / kUA55ZeroTimestampPeriod)
                    * kUA55ZeroTimestampPeriod;
                if (aligned > 0) {
                    ivars->sampleTime = aligned;
                    ivars->lastPublishedZts = aligned;
                    UpdateCurrentZeroTimestamp(aligned, mach_absolute_time());
                }
            }
        }

        ivars->ioRunning = true;

        os_log(OS_LOG_DEFAULT,
               "[UA55] build=%u StartIO success (ring=%u ZTS=%u, already=%d)",
               kUA55DriverBuild,
               kUA55HalRingFrames,
               kUA55ZeroTimestampPeriod,
               alreadyStreaming ? 1 : 0);
        return;

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
    });

    return error;
}

kern_return_t UA55AudioDevice::StopIO(IOUserAudioStartStopFlags in_flags)
{
    __block kern_return_t error = kIOReturnSuccess;

    ivars->workQueue->DispatchSync(^() {
        ivars->ioRunning = false;
        if (ivars->usbStream != nullptr) {
            ivars->usbStream->ClearTimestampTarget();
        }
        ivars->outputMap.reset();
        ivars->inputMap.reset();
        error = super::StopIO(in_flags);
        os_log(OS_LOG_DEFAULT, "[UA55] StopIO (HAL only, USB kept) status=0x%08x", (unsigned int)error);
    });

    return error;
}
