#include <os/log.h>

#include <DriverKit/DriverKit.h>
#include <DriverKit/IOLib.h>
#include <USBDriverKit/USBDriverKit.h>
#include <AudioDriverKit/AudioDriverKit.h>

#include "UA55AudioDriver.h"
#include "UA55AudioDevice.h"
#include "UA55UsbStream.h"
#include "UA55USBConstants.h"

using namespace AudioDriverKit;

struct UA55AudioDriver_IVars {
    IOUSBHostDevice* device;
    UA55UsbStream* usbStream;
    OSSharedPtr<UA55AudioDevice> audioDevice;
    OSSharedPtr<IODispatchQueue> workQueue;
    bool deviceOpened;
};

bool UA55AudioDriver::init(void)
{
    if (!super::init()) {
        return false;
    }
    ivars = IONewZero(UA55AudioDriver_IVars, 1);
    return ivars != nullptr;
}

void UA55AudioDriver::free(void)
{
    if (ivars != nullptr) {
        if (ivars->audioDevice.get() != nullptr) {
            ivars->audioDevice->ConfigureHardware(0);
        }
        if (ivars->usbStream != nullptr) {
            ivars->usbStream->TearDown(this);
            delete ivars->usbStream;
            ivars->usbStream = nullptr;
        }
        ivars->audioDevice.reset();
        ivars->workQueue.reset();
        if (ivars->device != nullptr && ivars->deviceOpened) {
            ivars->deviceOpened = false;
            ivars->device->Close(this, 0);
        }
        OSSafeReleaseNULL(ivars->device);
    }
    IOSafeDeleteNULL(ivars, UA55AudioDriver_IVars, 1);
    super::free();
}

void UA55AudioDriver::CaptureIsochComplete_Impl(OSAction* action, IOReturn status)
{
    if (ivars == nullptr || ivars->usbStream == nullptr || action == nullptr) {
        return;
    }
    const uint32_t slotIndex = *reinterpret_cast<uint32_t*>(action->GetReference());
    ivars->usbStream->OnCaptureComplete(slotIndex, status);
}

void UA55AudioDriver::PlaybackIsochComplete_Impl(OSAction* action, IOReturn status)
{
    if (ivars == nullptr || ivars->usbStream == nullptr || action == nullptr) {
        return;
    }
    const uint32_t slotIndex = *reinterpret_cast<uint32_t*>(action->GetReference());
    ivars->usbStream->OnPlaybackComplete(slotIndex, status);
}

void UA55AudioDriver::StatusInterruptComplete_Impl(OSAction* action,
                                                   IOReturn status,
                                                   uint32_t actualByteCount,
                                                   uint64_t completionTimestamp)
{
    (void)completionTimestamp;
    if (ivars == nullptr || ivars->usbStream == nullptr || action == nullptr) {
        return;
    }
    const uint32_t* ref = reinterpret_cast<const uint32_t*>(action->GetReference());
    ivars->usbStream->OnStatusComplete(ref[0], ref[1], status, actualByteCount);
}

void UA55AudioDriver::MidiInComplete_Impl(OSAction* action,
                                          IOReturn status,
                                          uint32_t actualByteCount,
                                          uint64_t completionTimestamp)
{
    (void)completionTimestamp;
    if (ivars == nullptr || ivars->usbStream == nullptr || action == nullptr) {
        return;
    }
    const uint32_t slotIndex = *reinterpret_cast<const uint32_t*>(action->GetReference());
    ivars->usbStream->OnMidiInComplete(slotIndex, status, actualByteCount);
}

kern_return_t UA55AudioDriver::Start_Impl(IOService* provider)
{
    kern_return_t error = Start(provider, SUPERDISPATCH);
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] Start(super)=FAILED 0x%08x", (unsigned int)error);
        return error;
    }

    IOUSBHostDevice* device = OSDynamicCast(IOUSBHostDevice, provider);
    if (device == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] provider is not IOUSBHostDevice");
        return kIOReturnNoDevice;
    }

    device->retain();
    ivars->device = device;
    os_log(OS_LOG_DEFAULT, "[UA55] ========== BUILD %u loaded (device matched) ==========", kUA55DriverBuild);
    os_log(OS_LOG_DEFAULT, "[UA55] device matched");

    const IOUSBDeviceDescriptor* deviceDescriptor = device->CopyDeviceDescriptor();
    if (deviceDescriptor == nullptr || deviceDescriptor->bLength < 18) {
        os_log(OS_LOG_DEFAULT, "[UA55] device descriptor missing");
        OSSafeReleaseNULL(ivars->device);
        return kIOReturnError;
    }

    {
        const uint8_t* rawDevice = reinterpret_cast<const uint8_t*>(deviceDescriptor);
        const uint16_t vendorID = (uint16_t)(rawDevice[8] | (rawDevice[9] << 8));
        const uint16_t productID = (uint16_t)(rawDevice[10] | (rawDevice[11] << 8));
        os_log(OS_LOG_DEFAULT, "[UA55] VID=0x%04X PID=0x%04X", vendorID, productID);
        if (vendorID != kUA55VendorID || productID != kUA55ProductID) {
            IOUSBHostFreeDescriptor(deviceDescriptor);
            OSSafeReleaseNULL(ivars->device);
            return kIOReturnUnsupported;
        }
    }
    IOUSBHostFreeDescriptor(deviceDescriptor);

    error = device->Open(this, 0, NULL);
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] Open=FAILED 0x%08x", (unsigned int)error);
        OSSafeReleaseNULL(ivars->device);
        return error;
    }
    ivars->deviceOpened = true;

    // matchInterfaces=true necessário para as interfaces de áudio aparecerem.
    error = device->SetConfiguration(kUA55RequestedConfiguration, true);
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] SetConfiguration(1,true)=FAILED 0x%08x", (unsigned int)error);
        ivars->deviceOpened = false;
        device->Close(this, 0);
        OSSafeReleaseNULL(ivars->device);
        return error;
    }
    os_log(OS_LOG_DEFAULT, "[UA55] SetConfiguration(1,true)=SUCCESS");

    ivars->workQueue = GetWorkQueue();
    if (ivars->workQueue.get() == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] GetWorkQueue failed");
        return kIOReturnInvalid;
    }

    auto driverName = OSSharedPtr(OSString::withCString("QUAD-CAPTURE UA-55"), OSNoRetain);
    SetName(driverName.get());
    SetTransportType(IOUserAudioTransportType::USB);

    ivars->usbStream = new UA55UsbStream();
    if (ivars->usbStream == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream alloc failed");
        return kIOReturnNoMemory;
    }

    error = ivars->usbStream->Prepare(device, this, this);
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] UsbStream Prepare=FAILED 0x%08x", (unsigned int)error);
        return error;
    }

    auto deviceUID = OSSharedPtr(OSString::withCString("QUAD-CAPTURE-UA-55"), OSNoRetain);
    auto modelUID = OSSharedPtr(OSString::withCString("UA-55"), OSNoRetain);
    auto manufacturerUID = OSSharedPtr(OSString::withCString("Roland"), OSNoRetain);
    auto deviceName = OSSharedPtr(OSString::withCString("QUAD-CAPTURE UA-55"), OSNoRetain);

    ivars->audioDevice = OSSharedPtr(OSTypeAlloc(UA55AudioDevice), OSNoRetain);
    if (ivars->audioDevice.get() == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] AudioDevice alloc failed");
        return kIOReturnNoMemory;
    }

    const bool ok = ivars->audioDevice->init(this,
                                             false,
                                             deviceUID.get(),
                                             modelUID.get(),
                                             manufacturerUID.get(),
                                             kUA55ZeroTimestampPeriod);
    if (!ok) {
        os_log(OS_LOG_DEFAULT, "[UA55] AudioDevice init failed");
        ivars->audioDevice.reset();
        return kIOReturnNoMemory;
    }

    ivars->audioDevice->SetName(deviceName.get());
    error = ivars->audioDevice->ConfigureHardware(reinterpret_cast<uint64_t>(ivars->usbStream));
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] ConfigureHardware=FAILED 0x%08x", (unsigned int)error);
        return error;
    }

    error = AddObject(ivars->audioDevice.get());
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] AddObject=FAILED 0x%08x", (unsigned int)error);
        return error;
    }

    error = RegisterService();
    if (error != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[UA55] RegisterService=FAILED 0x%08x", (unsigned int)error);
        return error;
    }

    os_log(OS_LOG_DEFAULT, "[UA55] audio driver resident — check Audio MIDI Setup");
    return kIOReturnSuccess;
}

kern_return_t UA55AudioDriver::NewUserClient_Impl(uint32_t type, IOUserClient** userClient)
{
    if (type != kUA55SensUserClientType) {
        return NewUserClient(type, userClient, SUPERDISPATCH);
    }
    if (userClient == nullptr) {
        return kIOReturnBadArgument;
    }
    IOService* service = nullptr;
    const kern_return_t created = Create(this, "UA55SensUserClientProperties", &service);
    if (created != kIOReturnSuccess || service == nullptr) {
        os_log(OS_LOG_DEFAULT, "[UA55] sens user client create failed 0x%08x", (unsigned int)created);
        OSSafeReleaseNULL(service);
        return created != kIOReturnSuccess ? created : kIOReturnNoMemory;
    }
    *userClient = OSDynamicCast(IOUserClient, service);
    if (*userClient == nullptr) {
        OSSafeReleaseNULL(service);
        return kIOReturnUnsupported;
    }
    os_log(OS_LOG_DEFAULT, "[UA55] sens user client opened");
    return kIOReturnSuccess;
}

void UA55AudioDriver::CopyHardwareSens(uint8_t* left, uint8_t* right)
{
    if (ivars != nullptr && ivars->usbStream != nullptr) {
        ivars->usbStream->CopySens(left, right);
        return;
    }
    if (left != nullptr) {
        *left = 255;
    }
    if (right != nullptr) {
        *right = 255;
    }
}

kern_return_t UA55AudioDriver::Stop_Impl(IOService* provider)
{
    os_log(OS_LOG_DEFAULT, "[UA55] Stop");

    if (ivars != nullptr) {
        if (ivars->audioDevice.get() != nullptr) {
            ivars->audioDevice->ConfigureHardware(0);
        }
        if (ivars->usbStream != nullptr) {
            ivars->usbStream->TearDown(this);
            delete ivars->usbStream;
            ivars->usbStream = nullptr;
        }
        if (ivars->audioDevice.get() != nullptr) {
            RemoveObject(ivars->audioDevice.get());
            ivars->audioDevice.reset();
        }
        if (ivars->device != nullptr && ivars->deviceOpened) {
            ivars->deviceOpened = false;
            const kern_return_t closeResult = ivars->device->Close(this, 0);
            os_log(OS_LOG_DEFAULT, "[UA55] Close=0x%08x", (unsigned int)closeResult);
        }
        OSSafeReleaseNULL(ivars->device);
        ivars->workQueue.reset();
    }

    return Stop(provider, SUPERDISPATCH);
}
