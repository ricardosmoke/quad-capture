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
extern OSMetaClass * gOSBooleanMetaClass;
extern OSMetaClass * gOSDictionaryMetaClass;
extern OSMetaClass * gOSArrayMetaClass;
extern OSMetaClass * gOSSetMetaClass;
extern OSMetaClass * gOSOrderedSetMetaClass;
extern OSMetaClass * gIODispatchQueueMetaClass;
extern OSMetaClass * gIOBufferMemoryDescriptorMetaClass;
extern OSMetaClass * gIOUserClientMetaClass;
extern OSMetaClass * gIOServiceStateNotificationDispatchSourceMetaClass;
extern OSMetaClass * gIOUserAudioObjectMetaClass;
extern OSMetaClass * gIOUserAudioDeviceMetaClass;
extern OSMetaClass * gIOUserAudioCustomPropertyMetaClass;
extern OSMetaClass * gOSStringMetaClass;
extern OSMetaClass * gIOMemoryMapMetaClass;
extern OSMetaClass * gOSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass;
extern OSMetaClass * gOSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass;
extern OSMetaClass * gOSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass;
extern OSMetaClass * gOSAction_UA55AudioDriver_MidiInCompleteMetaClass;
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
        case UA55AudioDriver_CaptureIsochComplete_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOUSBHostPipe::CompleteAsyncIsochIO_Invoke(rpc, self, SimpleMemberFunctionCast(IOUSBHostPipe::CompleteAsyncIsochIO_Handler, *self, &UA55AudioDriver::CaptureIsochComplete_Impl), OSTypeID(OSAction_UA55AudioDriver_CaptureIsochComplete));
            break;
        }
        case UA55AudioDriver_PlaybackIsochComplete_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOUSBHostPipe::CompleteAsyncIsochIO_Invoke(rpc, self, SimpleMemberFunctionCast(IOUSBHostPipe::CompleteAsyncIsochIO_Handler, *self, &UA55AudioDriver::PlaybackIsochComplete_Impl), OSTypeID(OSAction_UA55AudioDriver_PlaybackIsochComplete));
            break;
        }
        case UA55AudioDriver_StatusInterruptComplete_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOUSBHostPipe::CompleteAsyncIO_Invoke(rpc, self, SimpleMemberFunctionCast(IOUSBHostPipe::CompleteAsyncIO_Handler, *self, &UA55AudioDriver::StatusInterruptComplete_Impl), OSTypeID(OSAction_UA55AudioDriver_StatusInterruptComplete));
            break;
        }
        case UA55AudioDriver_MidiInComplete_ID:
#if !KERNEL
        if (self->IsRemote())
        {
            ret = self->OSMetaClassBase::Dispatch(rpc);
            break;
        }
        else
#endif /* !KERNEL */
        {
            ret = IOUSBHostPipe::CompleteAsyncIO_Invoke(rpc, self, SimpleMemberFunctionCast(IOUSBHostPipe::CompleteAsyncIO_Handler, *self, &UA55AudioDriver::MidiInComplete_Impl), OSTypeID(OSAction_UA55AudioDriver_MidiInComplete));
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

kern_return_t
UA55AudioDriver::CreateActionCaptureIsochComplete(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55AudioDriver_CaptureIsochComplete");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55AudioDriver_CaptureIsochComplete::CreateWithTypeName(this,
                           UA55AudioDriver_CaptureIsochComplete_ID,
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
UA55AudioDriver::CreateActionPlaybackIsochComplete(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55AudioDriver_PlaybackIsochComplete");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55AudioDriver_PlaybackIsochComplete::CreateWithTypeName(this,
                           UA55AudioDriver_PlaybackIsochComplete_ID,
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
UA55AudioDriver::CreateActionStatusInterruptComplete(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55AudioDriver_StatusInterruptComplete");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55AudioDriver_StatusInterruptComplete::CreateWithTypeName(this,
                           UA55AudioDriver_StatusInterruptComplete_ID,
                           IOUSBHostPipe_CompleteAsyncIO_ID,
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
UA55AudioDriver::CreateActionMidiInComplete(size_t referenceSize, OSAction ** action)
{
    kern_return_t ret;

#if defined(IOKIT_ENABLE_SHARED_PTR)
    OSSharedPtr<OSString>
#else /* defined(IOKIT_ENABLE_SHARED_PTR) */
    OSString *
#endif /* !defined(IOKIT_ENABLE_SHARED_PTR) */
    typeName = OSString::withCString("OSAction_UA55AudioDriver_MidiInComplete");
    if (!typeName) {
        return kIOReturnNoMemory;
    }
    ret = OSAction_UA55AudioDriver_MidiInComplete::CreateWithTypeName(this,
                           UA55AudioDriver_MidiInComplete_ID,
                           IOUSBHostPipe_CompleteAsyncIO_ID,
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
OSDefineMetaClassAndStructors(OSAction_UA55AudioDriver_CaptureIsochComplete, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55AudioDriver_CaptureIsochComplete_QueueNames  ""

#define OSAction_UA55AudioDriver_CaptureIsochComplete_MethodNames  ""

#define OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55AudioDriver_CaptureIsochComplete_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55AudioDriver_CaptureIsochComplete_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t
OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t),
        .name                    = "OSAction_UA55AudioDriver_CaptureIsochComplete",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55AudioDriver_CaptureIsochComplete_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55AudioDriver_CaptureIsochComplete_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55AudioDriver_CaptureIsochComplete_QueueNames,
    .methodNames     = OSAction_UA55AudioDriver_CaptureIsochComplete_MethodNames,
    .metaMethodNames = OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_CaptureIsochComplete_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55AudioDriver_CaptureIsochComplete_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55AudioDriver_CaptureIsochComplete.base,
    .metaPointer       = &gOSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55AudioDriver_CaptureIsochComplete),

    .resv2             = {0},

    .New               = &OSAction_UA55AudioDriver_CaptureIsochComplete_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55AudioDriver_CaptureIsochComplete_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55AudioDriver_CaptureIsochComplete_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55AudioDriver_CaptureIsochComplete_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_CaptureIsochComplete_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_CaptureIsochComplete) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55AudioDriver_CaptureIsochComplete::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55AudioDriver_CaptureIsochComplete::_Dispatch(OSAction_UA55AudioDriver_CaptureIsochComplete * self, const IORPC rpc)
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
OSAction_UA55AudioDriver_CaptureIsochComplete::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass::Dispatch(const IORPC rpc)
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
OSDefineMetaClassAndStructors(OSAction_UA55AudioDriver_PlaybackIsochComplete, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55AudioDriver_PlaybackIsochComplete_QueueNames  ""

#define OSAction_UA55AudioDriver_PlaybackIsochComplete_MethodNames  ""

#define OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55AudioDriver_PlaybackIsochComplete_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55AudioDriver_PlaybackIsochComplete_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t
OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t),
        .name                    = "OSAction_UA55AudioDriver_PlaybackIsochComplete",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55AudioDriver_PlaybackIsochComplete_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55AudioDriver_PlaybackIsochComplete_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55AudioDriver_PlaybackIsochComplete_QueueNames,
    .methodNames     = OSAction_UA55AudioDriver_PlaybackIsochComplete_MethodNames,
    .metaMethodNames = OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_PlaybackIsochComplete_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55AudioDriver_PlaybackIsochComplete_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55AudioDriver_PlaybackIsochComplete.base,
    .metaPointer       = &gOSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55AudioDriver_PlaybackIsochComplete),

    .resv2             = {0},

    .New               = &OSAction_UA55AudioDriver_PlaybackIsochComplete_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55AudioDriver_PlaybackIsochComplete_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55AudioDriver_PlaybackIsochComplete_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55AudioDriver_PlaybackIsochComplete_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_PlaybackIsochComplete_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_PlaybackIsochComplete) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55AudioDriver_PlaybackIsochComplete::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55AudioDriver_PlaybackIsochComplete::_Dispatch(OSAction_UA55AudioDriver_PlaybackIsochComplete * self, const IORPC rpc)
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
OSAction_UA55AudioDriver_PlaybackIsochComplete::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass::Dispatch(const IORPC rpc)
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
OSDefineMetaClassAndStructors(OSAction_UA55AudioDriver_StatusInterruptComplete, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55AudioDriver_StatusInterruptComplete_QueueNames  ""

#define OSAction_UA55AudioDriver_StatusInterruptComplete_MethodNames  ""

#define OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55AudioDriver_StatusInterruptComplete_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55AudioDriver_StatusInterruptComplete_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t
OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t),
        .name                    = "OSAction_UA55AudioDriver_StatusInterruptComplete",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55AudioDriver_StatusInterruptComplete_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55AudioDriver_StatusInterruptComplete_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55AudioDriver_StatusInterruptComplete_QueueNames,
    .methodNames     = OSAction_UA55AudioDriver_StatusInterruptComplete_MethodNames,
    .metaMethodNames = OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_StatusInterruptComplete_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55AudioDriver_StatusInterruptComplete_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55AudioDriver_StatusInterruptComplete.base,
    .metaPointer       = &gOSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55AudioDriver_StatusInterruptComplete),

    .resv2             = {0},

    .New               = &OSAction_UA55AudioDriver_StatusInterruptComplete_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55AudioDriver_StatusInterruptComplete_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55AudioDriver_StatusInterruptComplete_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55AudioDriver_StatusInterruptComplete_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_StatusInterruptComplete_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_StatusInterruptComplete) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55AudioDriver_StatusInterruptComplete::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55AudioDriver_StatusInterruptComplete::_Dispatch(OSAction_UA55AudioDriver_StatusInterruptComplete * self, const IORPC rpc)
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
OSAction_UA55AudioDriver_StatusInterruptComplete::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass::Dispatch(const IORPC rpc)
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
OSDefineMetaClassAndStructors(OSAction_UA55AudioDriver_MidiInComplete, OSAction);
#endif /* KERNEL */

#if !KERNEL

#define OSAction_UA55AudioDriver_MidiInComplete_QueueNames  ""

#define OSAction_UA55AudioDriver_MidiInComplete_MethodNames  ""

#define OSAction_UA55AudioDriver_MidiInCompleteMetaClass_MethodNames  ""

struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t
{
    OSClassDescription base;
    uint64_t           methodOptions[2 * 0];
    uint64_t           metaMethodOptions[2 * 0];
    char               queueNames[sizeof(OSAction_UA55AudioDriver_MidiInComplete_QueueNames)];
    char               methodNames[sizeof(OSAction_UA55AudioDriver_MidiInComplete_MethodNames)];
    char               metaMethodNames[sizeof(OSAction_UA55AudioDriver_MidiInCompleteMetaClass_MethodNames)];
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t
OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete =
{
    .base =
    {
        .descriptionSize         = sizeof(OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t),
        .name                    = "OSAction_UA55AudioDriver_MidiInComplete",
        .superName               = "OSAction",
        .methodOptionsSize       = 2 * sizeof(uint64_t) * 0,
        .methodOptionsOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t, methodOptions),
        .metaMethodOptionsSize   = 2 * sizeof(uint64_t) * 0,
        .metaMethodOptionsOffset = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t, metaMethodOptions),
        .queueNamesSize       = sizeof(OSAction_UA55AudioDriver_MidiInComplete_QueueNames),
        .queueNamesOffset     = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t, queueNames),
        .methodNamesSize         = sizeof(OSAction_UA55AudioDriver_MidiInComplete_MethodNames),
        .methodNamesOffset       = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t, methodNames),
        .metaMethodNamesSize     = sizeof(OSAction_UA55AudioDriver_MidiInCompleteMetaClass_MethodNames),
        .metaMethodNamesOffset   = __builtin_offsetof(struct OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete_t, metaMethodNames),
        .flags                   = 0*kOSClassCanRemote,
        .resv1                   = {0},
    },
    .methodOptions =
    {
    },
    .metaMethodOptions =
    {
    },
    .queueNames      = OSAction_UA55AudioDriver_MidiInComplete_QueueNames,
    .methodNames     = OSAction_UA55AudioDriver_MidiInComplete_MethodNames,
    .metaMethodNames = OSAction_UA55AudioDriver_MidiInCompleteMetaClass_MethodNames,
};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
OSMetaClass * gOSAction_UA55AudioDriver_MidiInCompleteMetaClass;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_MidiInComplete_New(OSMetaClass * instance);

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const OSClassLoadInformation
OSAction_UA55AudioDriver_MidiInComplete_Class = 
{
    .description       = &OSClassDescription_OSAction_UA55AudioDriver_MidiInComplete.base,
    .metaPointer       = &gOSAction_UA55AudioDriver_MidiInCompleteMetaClass,
    .version           = 1,
    .instanceSize      = sizeof(OSAction_UA55AudioDriver_MidiInComplete),

    .resv2             = {0},

    .New               = &OSAction_UA55AudioDriver_MidiInComplete_New,
    .resv3             = {0},

};

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
extern const void * const
gOSAction_UA55AudioDriver_MidiInComplete_Declaration;
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
const void * const
gOSAction_UA55AudioDriver_MidiInComplete_Declaration
 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
__attribute__((used,visibility("hidden"),section("__DATA_CONST,__osclassinfo,regular,no_dead_strip"),no_sanitize("address")))
    = &OSAction_UA55AudioDriver_MidiInComplete_Class;

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
static kern_return_t
OSAction_UA55AudioDriver_MidiInComplete_New(OSMetaClass * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_MidiInCompleteMetaClass) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

 __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer")))
kern_return_t
OSAction_UA55AudioDriver_MidiInCompleteMetaClass::New(OSObject * instance)
{
    if (!new(instance) OSAction_UA55AudioDriver_MidiInComplete) return (kIOReturnNoMemory);
    return (kIOReturnSuccess);
}

#endif /* !KERNEL */

#ifdef KERNEL
#define MESSAGE_CONTENT(__field) (messageContent->__field)
#else /* KERNEL */
#define MESSAGE_CONTENT(__field) (message->content.__field)
#endif /* KERNEL */

kern_return_t
OSAction_UA55AudioDriver_MidiInComplete::Dispatch(const IORPC rpc)
{
    return _Dispatch(this, rpc);
}

kern_return_t
OSAction_UA55AudioDriver_MidiInComplete::_Dispatch(OSAction_UA55AudioDriver_MidiInComplete * self, const IORPC rpc)
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
OSAction_UA55AudioDriver_MidiInComplete::MetaClass::Dispatch(const IORPC rpc)
{
#else /* KERNEL */
kern_return_t
OSAction_UA55AudioDriver_MidiInCompleteMetaClass::Dispatch(const IORPC rpc)
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



