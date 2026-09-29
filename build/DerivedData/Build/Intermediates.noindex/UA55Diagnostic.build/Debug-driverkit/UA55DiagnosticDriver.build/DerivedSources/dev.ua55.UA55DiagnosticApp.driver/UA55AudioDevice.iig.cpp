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
extern OSMetaClass * gOSBooleanMetaClass;
extern OSMetaClass * gOSDictionaryMetaClass;
extern OSMetaClass * gOSArrayMetaClass;
extern OSMetaClass * gOSSetMetaClass;
extern OSMetaClass * gOSOrderedSetMetaClass;
extern OSMetaClass * gOSStringMetaClass;
extern OSMetaClass * gIOMemoryDescriptorMetaClass;
extern OSMetaClass * gIOBufferMemoryDescriptorMetaClass;
extern OSMetaClass * gIOUserClientMetaClass;
extern OSMetaClass * gIOServiceStateNotificationDispatchSourceMetaClass;
extern OSMetaClass * gIOUserAudioCustomPropertyMetaClass;
extern OSMetaClass * gIOUserAudioDriverMetaClass;
extern OSMetaClass * gIOUserAudioStreamMetaClass;
extern OSMetaClass * gIOUserAudioControlMetaClass;
extern OSMetaClass * gOSAction_UA55AudioDevice_TimerOccurredMetaClass;
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
        case UA55AudioDevice_TimerOccurred_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOTimerDispatchSource::TimerOccurred_Invoke(rpc, self, SimpleMemberFunctionCast(IOTimerDispatchSource::TimerOccurred_Handler, *self, &UA55AudioDevice::TimerOccurred_Impl), OSTypeID(OSAction_UA55AudioDevice_TimerOccurred));
            break;
        }

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

kern_return_t
UA55AudioDevice::CreateActionTimerOccurred(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55AudioDevice_TimerOccurred");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55AudioDevice_TimerOccurred::CreateWithTypeName(this,
                           UA55AudioDevice_TimerOccurred_ID,
                           IOTimerDispatchSource_TimerOccurred_ID,
                           referenceSize,
#if defined(IOKIT_ENABLE_SHARED_PTR)
                           typeName.get(),
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
                           typeName,
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
                           action);

#if !defined(IOKIT_ENABLE_SHARED_PTR)
    typeName->release();
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    return (ret);
}

#if KERNEL
OSDefineMetaClassAndStructors(OSAction_UA55AudioDevice_TimerOccurred, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55AudioDevice_TimerOccurred_QueueNames  ""

#define OSAction_UA55AudioDevice_TimerOccurred_MethodNames  ""

#define OSAction_UA55AudioDevice_TimerOccurredMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55AudioDevice_TimerOccurred_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55AudioDevice_TimerOccurred_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55AudioDevice_TimerOccurredMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t
OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t),
        .name                    = "OSAction_UA55AudioDevice_TimerOccurred",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55AudioDevice_TimerOccurred_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55AudioDevice_TimerOccurred_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55AudioDevice_TimerOccurredMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55AudioDevice_TimerOccurred_QueueNames,
    .methodNames     = OSAction_UA55AudioDevice_TimerOccurred_MethodNames,
    .metaMethodNames = OSAction_UA55AudioDevice_TimerOccurredMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55AudioDevice_TimerOccurredMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDevice_TimerOccurred_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55AudioDevice_TimerOccurred_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55AudioDevice_TimerOccurred.base,
    .metaPointer       = &gOSAction_UA55AudioDevice_TimerOccurredMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55AudioDevice_TimerOccurred),

    .resv2             = {0},

    .New               = &OSAction_UA55AudioDevice_TimerOccurred_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55AudioDevice_TimerOccurred_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55AudioDevice_TimerOccurred_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55AudioDevice_TimerOccurred_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDevice_TimerOccurred_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55AudioDevice_TimerOccurredMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55AudioDevice_TimerOccurredMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55AudioDevice_TimerOccurred) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55AudioDevice_TimerOccurred::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55AudioDevice_TimerOccurred::_Dispatch(OSAction_UA55AudioDevice_TimerOccurred * self, const IORPC rpc)
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
            ret = OSAction::_Dispatch(self, rpc);
            break;
    }

    return (ret);
}

#if KERNEL
kern_return_t
OSAction_UA55AudioDevice_TimerOccurred::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55AudioDevice_TimerOccurredMetaClass::Dispatch(const IORPC rpc)
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



