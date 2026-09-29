/* iig(DriverKit-440 May 22 2026 10:59:06) generated from UA55UsbStream.iig */

#undef	IIG_IMPLEMENTATION
#define	IIG_IMPLEMENTATION 	UA55UsbStream.iig

#if KERNEL
#include <libkern/c++/OSString.h>
#else
#include <DriverKit/DriverKit.h>
#endif /* KERNEL */
#include <DriverKit/IOReturn.h>
#include "UA55UsbStream.h"


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
extern OSMetaClass * gIODispatchQueueMetaClass;
extern OSMetaClass * gIOUserClientMetaClass;
extern OSMetaClass * gIOServiceStateNotificationDispatchSourceMetaClass;
extern OSMetaClass * gOSStringMetaClass;
extern OSMetaClass * gIOMemoryMapMetaClass;
extern OSMetaClass * gIOUSBHostInterfaceMetaClass;
extern OSMetaClass * gOSAction_UA55UsbStream_CaptureIsochCompleteMetaClass;
extern OSMetaClass * gOSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass;
#endif /* !KERNEL */

#if !KERNEL

#define UA55UsbStream_QueueNames  ""

#define UA55UsbStream_MethodNames  ""

#define UA55UsbStreamMetaClass_MethodNames  ""

struct OSClassDescription_UA55UsbStream_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(UA55UsbStream_QueueNames)];
    char               methodNames[sizeof(UA55UsbStream_MethodNames)];
    char               metaMethodNames[sizeof(UA55UsbStreamMetaClass_MethodNames)];
};

const struct OSClassDescription_UA55UsbStream_t
OSClassDescription_UA55UsbStream =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_UA55UsbStream_t),
        .name                    = "UA55UsbStream",
        .superName               = "IOService",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_UA55UsbStream_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_UA55UsbStream_t, metaMethodOptions),
        .queueNamesSize       = sizeof(UA55UsbStream_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_UA55UsbStream_t, queueNames),
        .methodNamesSize         = sizeof(UA55UsbStream_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_UA55UsbStream_t, methodNames),
        .metaMethodNamesSize     = sizeof(UA55UsbStreamMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_UA55UsbStream_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = UA55UsbStream_QueueNames,
    .methodNames     = UA55UsbStream_MethodNames,
    .metaMethodNames = UA55UsbStreamMetaClass_MethodNames,
};

OSMetaClass * gUA55UsbStreamMetaClass;

static kern_return_t
UA55UsbStream_New(OSMetaClass * instance);

const OSClassLoadInformation
UA55UsbStream_Class = 
{
    .description       = &OSClassDescription_UA55UsbStream.base,
    .metaPointer       = &gUA55UsbStreamMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(UA55UsbStream),

    .resv2             = {0},

    .New               = &UA55UsbStream_New,
    .resv3             = {0},

};

extern const void * const
gUA55UsbStream_Declaration;
const void * const
gUA55UsbStream_Declaration
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &UA55UsbStream_Class;

static kern_return_t
UA55UsbStream_New(OSMetaClass * instance)
{
    if (!new(instance) UA55UsbStreamMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

kern_return_t
UA55UsbStreamMetaClass::New(OSObject * instance)
{
    if (!new(instance) UA55UsbStream) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
UA55UsbStream::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
UA55UsbStream::_Dispatch(UA55UsbStream * self, const IORPC rpc)
{
    kern_return_t ret = kIOReturnUnsupported;
#ifdef KERNEL
    IORPCMessage * msg = rpc.kernelContent;
#else /* KERNEL */
    IORPCMessage * msg = IORPCMessageFromMach(rpc.message, false);
#endif /* KERNEL */

    switch (msg->msgid)
    {
        case UA55UsbStream_CaptureIsochComplete_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOUSBHostPipe::CompleteAsyncIsochIO_Invoke(rpc, self, SimpleMemberFunctionCast(IOUSBHostPipe::CompleteAsyncIsochIO_Handler, *self, &UA55UsbStream::CaptureIsochComplete_Impl), OSTypeID(OSAction_UA55UsbStream_CaptureIsochComplete));
            break;
        }
        case UA55UsbStream_PlaybackIsochComplete_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOUSBHostPipe::CompleteAsyncIsochIO_Invoke(rpc, self, SimpleMemberFunctionCast(IOUSBHostPipe::CompleteAsyncIsochIO_Handler, *self, &UA55UsbStream::PlaybackIsochComplete_Impl), OSTypeID(OSAction_UA55UsbStream_PlaybackIsochComplete));
            break;
        }

        default:
            ret = IOService::_Dispatch(self, rpc);
            break;
    }

    return (ret);
}

#if KERNEL
kern_return_t
UA55UsbStream::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
UA55UsbStreamMetaClass::Dispatch(const IORPC rpc)
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
UA55UsbStream::CreateActionCaptureIsochComplete(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55UsbStream_CaptureIsochComplete");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55UsbStream_CaptureIsochComplete::CreateWithTypeName(this,
                           UA55UsbStream_CaptureIsochComplete_ID,
                           IOUSBHostPipe_CompleteAsyncIsochIO_ID,
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

kern_return_t
UA55UsbStream::CreateActionPlaybackIsochComplete(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55UsbStream_PlaybackIsochComplete");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55UsbStream_PlaybackIsochComplete::CreateWithTypeName(this,
                           UA55UsbStream_PlaybackIsochComplete_ID,
                           IOUSBHostPipe_CompleteAsyncIsochIO_ID,
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
OSDefineMetaClassAndStructors(OSAction_UA55UsbStream_CaptureIsochComplete, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55UsbStream_CaptureIsochComplete_QueueNames  ""

#define OSAction_UA55UsbStream_CaptureIsochComplete_MethodNames  ""

#define OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55UsbStream_CaptureIsochComplete_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55UsbStream_CaptureIsochComplete_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t
OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t),
        .name                    = "OSAction_UA55UsbStream_CaptureIsochComplete",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55UsbStream_CaptureIsochComplete_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55UsbStream_CaptureIsochComplete_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55UsbStream_CaptureIsochComplete_QueueNames,
    .methodNames     = OSAction_UA55UsbStream_CaptureIsochComplete_MethodNames,
    .metaMethodNames = OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55UsbStream_CaptureIsochCompleteMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55UsbStream_CaptureIsochComplete_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55UsbStream_CaptureIsochComplete_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55UsbStream_CaptureIsochComplete.base,
    .metaPointer       = &gOSAction_UA55UsbStream_CaptureIsochCompleteMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55UsbStream_CaptureIsochComplete),

    .resv2             = {0},

    .New               = &OSAction_UA55UsbStream_CaptureIsochComplete_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55UsbStream_CaptureIsochComplete_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55UsbStream_CaptureIsochComplete_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55UsbStream_CaptureIsochComplete_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55UsbStream_CaptureIsochComplete_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55UsbStream_CaptureIsochComplete) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55UsbStream_CaptureIsochComplete::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55UsbStream_CaptureIsochComplete::_Dispatch(OSAction_UA55UsbStream_CaptureIsochComplete * self, const IORPC rpc)
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
OSAction_UA55UsbStream_CaptureIsochComplete::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass::Dispatch(const IORPC rpc)
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

#if KERNEL
OSDefineMetaClassAndStructors(OSAction_UA55UsbStream_PlaybackIsochComplete, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55UsbStream_PlaybackIsochComplete_QueueNames  ""

#define OSAction_UA55UsbStream_PlaybackIsochComplete_MethodNames  ""

#define OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55UsbStream_PlaybackIsochComplete_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55UsbStream_PlaybackIsochComplete_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t
OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t),
        .name                    = "OSAction_UA55UsbStream_PlaybackIsochComplete",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55UsbStream_PlaybackIsochComplete_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55UsbStream_PlaybackIsochComplete_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55UsbStream_PlaybackIsochComplete_QueueNames,
    .methodNames     = OSAction_UA55UsbStream_PlaybackIsochComplete_MethodNames,
    .metaMethodNames = OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55UsbStream_PlaybackIsochComplete_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55UsbStream_PlaybackIsochComplete_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55UsbStream_PlaybackIsochComplete.base,
    .metaPointer       = &gOSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55UsbStream_PlaybackIsochComplete),

    .resv2             = {0},

    .New               = &OSAction_UA55UsbStream_PlaybackIsochComplete_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55UsbStream_PlaybackIsochComplete_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55UsbStream_PlaybackIsochComplete_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55UsbStream_PlaybackIsochComplete_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55UsbStream_PlaybackIsochComplete_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55UsbStream_PlaybackIsochComplete) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55UsbStream_PlaybackIsochComplete::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55UsbStream_PlaybackIsochComplete::_Dispatch(OSAction_UA55UsbStream_PlaybackIsochComplete * self, const IORPC rpc)
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
OSAction_UA55UsbStream_PlaybackIsochComplete::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass::Dispatch(const IORPC rpc)
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



