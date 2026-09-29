/* iig(DriverKit-440 May 22 2026 10:59:06) generated from UA55AudioDevice.iig */

#undef	IIG_IMPLEMENTATION
#define	IIG_IMPLEMENTATION 	UA55AudioDevice.iig

#if KERNEL
#include <libkern/c++/OSString.h>
#else
#include <DriverKit/DriverKit.h>
#endif /* KERNEL */
#include <DriverKit/IOReturn.h>
#include "UA55AudioDevice.h"


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
extern OSMetaClass * gIOMemoryDescriptorMetaClass;
extern OSMetaClass * gIOBufferMemoryDescriptorMetaClass;
extern OSMetaClass * gIOUserClientMetaClass;
extern OSMetaClass * gOSActionMetaClass;
extern OSMetaClass * gIOServiceStateNotificationDispatchSourceMetaClass;
extern OSMetaClass * gIOUserAudioCustomPropertyMetaClass;
extern OSMetaClass * gIOUserAudioDriverMetaClass;
extern OSMetaClass * gIODispatchQueueMetaClass;
extern OSMetaClass * gIOUserAudioStreamMetaClass;
extern OSMetaClass * gIOUserAudioControlMetaClass;
#endif /* !KERNEL */

#if !KERNEL

#define UA55AudioDevice_QueueNames  ""

#define UA55AudioDevice_MethodNames  ""

#define UA55AudioDeviceMetaClass_MethodNames  ""

struct OSClassDescription_UA55AudioDevice_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(UA55AudioDevice_QueueNames)];
    char               methodNames[sizeof(UA55AudioDevice_MethodNames)];
    char               metaMethodNames[sizeof(UA55AudioDeviceMetaClass_MethodNames)];
};

const struct OSClassDescription_UA55AudioDevice_t
OSClassDescription_UA55AudioDevice =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_UA55AudioDevice_t),
        .name                    = "UA55AudioDevice",
        .superName               = "IOUserAudioDevice",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_UA55AudioDevice_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_UA55AudioDevice_t, metaMethodOptions),
        .queueNamesSize       = sizeof(UA55AudioDevice_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_UA55AudioDevice_t, queueNames),
        .methodNamesSize         = sizeof(UA55AudioDevice_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_UA55AudioDevice_t, methodNames),
        .metaMethodNamesSize     = sizeof(UA55AudioDeviceMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_UA55AudioDevice_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = UA55AudioDevice_QueueNames,
    .methodNames     = UA55AudioDevice_MethodNames,
    .metaMethodNames = UA55AudioDeviceMetaClass_MethodNames,
};

OSMetaClass * gUA55AudioDeviceMetaClass;

static kern_return_t
UA55AudioDevice_New(OSMetaClass * instance);

const OSClassLoadInformation
UA55AudioDevice_Class = 
{
    .description       = &OSClassDescription_UA55AudioDevice.base,
    .metaPointer       = &gUA55AudioDeviceMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(UA55AudioDevice),

    .resv2             = {0},

    .New               = &UA55AudioDevice_New,
    .resv3             = {0},

};

extern const void * const
gUA55AudioDevice_Declaration;
const void * const
gUA55AudioDevice_Declaration
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &UA55AudioDevice_Class;

static kern_return_t
UA55AudioDevice_New(OSMetaClass * instance)
{
    if (!new(instance) UA55AudioDeviceMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

kern_return_t
UA55AudioDeviceMetaClass::New(OSObject * instance)
{
    if (!new(instance) UA55AudioDevice) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
UA55AudioDevice::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
UA55AudioDevice::_Dispatch(UA55AudioDevice * self, const IORPC rpc)
{
    kern_return_t ret = kIOReturnUnsupported;
#ifdef KERNEL
    IORPCMessage * msg = rpc.kernelContent;
#else /* KERNEL */
    IORPCMessage * msg = IORPCMessageFromMach(rpc.message, false);
#endif /* KERNEL */

    switch (msg->msgid)
    {

        default:
            ret = IOUserAudioDevice::_Dispatch(self, rpc);
            break;
    }

    return (ret);
}

#if KERNEL
kern_return_t
UA55AudioDevice::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
UA55AudioDeviceMetaClass::Dispatch(const IORPC rpc)
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



