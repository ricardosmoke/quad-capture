#include <os/log.h>

#include <DriverKit/DriverKit.h>

#include "UA55AudioDriver.h"
#include "UA55SensClient.h"
#include "UA55USBConstants.h"

kern_return_t UA55SensClient::ExternalMethod(uint64_t selector,
                                                   IOUserClientMethodArguments* arguments,
                                                   const IOUserClientMethodDispatch* dispatch,
                                                   OSObject* target,
                                                   void* reference)
{
    (void)dispatch;
    (void)target;
    (void)reference;
    if (selector != 0 || arguments == nullptr || arguments->scalarOutput == nullptr
        || arguments->scalarOutputCount < 1) {
        return kIOReturnBadArgument;
    }

    uint8_t left = 255;
    uint8_t right = 255;
    auto* driver = OSDynamicCast(UA55AudioDriver, GetProvider());
    if (driver != nullptr) {
        driver->CopyHardwareSens(&left, &right);
    }
    arguments->scalarOutput[0] = (uint64_t)left | ((uint64_t)right << 8);
    arguments->scalarOutputCount = 1;
    return kIOReturnSuccess;
}
