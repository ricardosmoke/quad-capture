/* iig(DriverKit-440 May 22 2026 10:59:06) generated from UA55AudioDriver.iig */

#undef	IIG_IMPLEMENTATION
#define	IIG_IMPLEMENTATION 	UA55AudioDriver.iig

#if KERNEL
#include <libkern/c++/OSString.h>
#else
#include <DriverKit/DriverKit.h>
#endif /* KERNEL */
#include <DriverKit/IOReturn.h>
#include "UA55AudioDriver.h"


#if __has_builtin(__builtin_load_member_function_pointer)
#define SimpleMemberFunctionCast(cfnty, self, func) (cfnty)__builtin_load_member_function_pointer(self, func)
#else
#define SimpleMemberFunctionCast(cfnty, self, func) ({ union { typeof(func) memfun; cfnty cfun; } pair; pair.memfun = func; pair.cfun; })
#endif


#if !KERNEL
extern OSMetaClass * gOSContainerMetaClass;
extern OSMetaClass * gOSDataMetaClass;
extern OSMetaClass * gOSNumberMetaClass;
extern OSMetaClass * gOSStringMetaClass;
extern OSMetaClass * gOSBooleanMetaClass;
extern OSMetaClass * gOSDictionaryMetaClass;
extern OSMetaClass * gOSArrayMetaClass;
extern OSMetaClass * gOSSetMetaClass;
extern OSMetaClass * gOSOrderedSetMetaClass;
extern OSMetaClass * gIODispatchQueueMetaClass;
extern OSMetaClass * gIOMemoryDescriptorMetaClass;
extern OSMetaClass * gIOBufferMemoryDescriptorMetaClass;
extern OSMetaClass * gIOUserClientMetaClass;
extern OSMetaClass * gOSActionMetaClass;
extern OSMetaClass * gIOServiceStateNotificationDispatchSourceMetaClass;
extern OSMetaClass * gIOUserAudioObjectMetaClass;
extern OSMetaClass * gIOUserAudioDeviceMetaClass;
extern OSMetaClass * gIOUserAudioCustomPropertyMetaClass;
#endif /* !KERNEL */

#if !KERNEL

#define UA55AudioDriver_QueueNames  ""

#define UA55AudioDriver_MethodNames  ""

#define UA55AudioDriverMetaClass_MethodNames  ""

struct OSClassDescription_UA55AudioDriver_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(UA55AudioDriver_QueueNames)];
    char               methodNames[sizeof(UA55AudioDriver_MethodNames)];
    char               metaMethodNames[sizeof(UA55AudioDriverMetaClass_MethodNames)];
};

const struct OSClassDescription_UA55AudioDriver_t
OSClassDescription_UA55AudioDriver =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_UA55AudioDriver_t),
        .name                    = "UA55AudioDriver",
        .superName               = "IOUserAudioDriver",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_UA55AudioDriver_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_UA55AudioDriver_t, metaMethodOptions),
        .queueNamesSize       = sizeof(UA55AudioDriver_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_UA55AudioDriver_t, queueNames),
        .methodNamesSize         = sizeof(UA55AudioDriver_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_UA55AudioDriver_t, methodNames),
        .metaMethodNamesSize     = sizeof(UA55AudioDriverMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_UA55AudioDriver_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = UA55AudioDriver_QueueNames,
    .methodNames     = UA55AudioDriver_MethodNames,
    .metaMethodNames = UA55AudioDriverMetaClass_MethodNames,
};

OSMetaClass * gUA55AudioDriverMetaClass;

static kern_return_t
UA55AudioDriver_New(OSMetaClass * instance);

const OSClassLoadInformation
UA55AudioDriver_Class = 
{
    .description       = &OSClassDescription_UA55AudioDriver.base,
    .metaPointer       = &gUA55AudioDriverMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(UA55AudioDriver),

    .resv2             = {0},

    .New               = &UA55AudioDriver_New,
    .resv3             = {0},

};

extern const void * const
gUA55AudioDriver_Declaration;
const void * const
gUA55AudioDriver_Declaration
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &UA55AudioDriver_Class;

static kern_return_t
UA55AudioDriver_New(OSMetaClass * instance)
{
    if (!new(instance) UA55AudioDriverMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

kern_return_t
UA55AudioDriverMetaClass::New(OSObject * instance)
{
    if (!new(instance) UA55AudioDriver) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
UA55AudioDriver::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
UA55AudioDriver::_Dispatch(UA55AudioDriver * self, const IORPC rpc)
{
    kern_return_t ret = kIOReturnUnsupported;
#ifdef KERNEL
    IORPCMessage * msg = rpc.kernelContent;
#else /* KERNEL */
    IORPCMessage * msg = IORPCMessageFromMach(rpc.message, false);
#endif /* KERNEL */

    switch (msg->msgid)
    {
        case IOService_Start_ID:
        {
            ret = IOService::Start_Invoke(rpc, self, SimpleMemberFunctionCast(IOService::Start_Handler, *self, &UA55AudioDriver::Start_Impl));
            break;
        }
        case IOService_Stop_ID:
        {
            ret = IOService::Stop_Invoke(rpc, self, SimpleMemberFunctionCast(IOService::Stop_Handler, *self, &UA55AudioDriver::Stop_Impl));
            break;
        }

        default:
            ret = IOUserAudioDriver::_Dispatch(self, rpc);
            break;
    }

    return (ret);
}

#if KERNEL
kern_return_t
UA55AudioDriver::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
UA55AudioDriverMetaClass::Dispatch(const IORPC rpc)
{
#endif /* !KERNEL */

    kern_return_t ret = kIOReturnUnsupported;
#ifdef KERNEL
    IORPCMessage * msg = rpc.kernelContent;
#else /* KERNEL */
    IORPCMessage * msg = IORPCMessageFromMach(rpc.message, false);
#endif /* KERNEL */

    switch (msg->msgid)
    {

        default:
            ret = OSMetaClassBase::Dispatch(rpc);
            break;
    }

    return (ret);
}



