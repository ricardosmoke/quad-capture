/* iig(DriverKit-440) generated from UA55AudioDriver.iig */

/* UA55AudioDriver.iig:1-8 */
#ifndef UA55AudioDriver_h
#define UA55AudioDriver_h

#include <Availability.h>
#include <DriverKit/OSSharedPtr.h>
#include <AudioDriverKit/IOUserAudioDriver.h>  /* .iig include */
#include <USBDriverKit/IOUSBHostPipe.h>  /* .iig include */

/* source class UA55AudioDriver UA55AudioDriver.iig:9-30 */

#if __DOCUMENTATION__
#define KERNEL IIG_KERNEL

class UA55AudioDriver: public IOUserAudioDriver
{
public:
    virtual bool init(void) override;
    virtual void free(void) override;
    virtual kern_return_t Start(IOService* provider) override;
    virtual kern_return_t Stop(IOService* provider) override;

    virtual void CaptureIsochComplete(OSAction* action, IOReturn status)
        TYPE(IOUSBHostPipe::CompleteAsyncIsochIO);
    virtual void PlaybackIsochComplete(OSAction* action, IOReturn status)
        TYPE(IOUSBHostPipe::CompleteAsyncIsochIO);
    virtual void StatusInterruptComplete(OSAction* action,
                                         IOReturn status,
                                         uint32_t actualByteCount,
                                         uint64_t completionTimestamp)
        TYPE(IOUSBHostPipe::CompleteAsyncIO);
    virtual void MidiInComplete(OSAction* action,
                                IOReturn status,
                                uint32_t actualByteCount,
                                uint64_t completionTimestamp)
        TYPE(IOUSBHostPipe::CompleteAsyncIO);
};

#undef KERNEL
#else /* __DOCUMENTATION__ */

/* generated class UA55AudioDriver UA55AudioDriver.iig:9-30 */

#define UA55AudioDriver_CaptureIsochComplete_ID            0x5175ff1a77a56567ULL
#define UA55AudioDriver_PlaybackIsochComplete_ID            0x49da0d6bb928295cULL
#define UA55AudioDriver_StatusInterruptComplete_ID            0x4b269818d883569bULL
#define UA55AudioDriver_MidiInComplete_ID            0x7cba7213692078c0ULL

#define UA55AudioDriver_Start_Args \
        IOService * provider

#define UA55AudioDriver_Stop_Args \
        IOService * provider

#define UA55AudioDriver_CaptureIsochComplete_Args \
        OSAction * action, \
        IOReturn status

#define UA55AudioDriver_PlaybackIsochComplete_Args \
        OSAction * action, \
        IOReturn status

#define UA55AudioDriver_StatusInterruptComplete_Args \
        OSAction * action, \
        IOReturn status, \
        uint32_t actualByteCount, \
        uint64_t completionTimestamp

#define UA55AudioDriver_MidiInComplete_Args \
        OSAction * action, \
        IOReturn status, \
        uint32_t actualByteCount, \
        uint64_t completionTimestamp

#define UA55AudioDriver_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(UA55AudioDriver * self, const IORPC rpc);\
\
    kern_return_t\
    CreateActionCaptureIsochComplete(size_t referenceSize, OSAction ** action);\
\
    kern_return_t\
    CreateActionPlaybackIsochComplete(size_t referenceSize, OSAction ** action);\
\
    kern_return_t\
    CreateActionStatusInterruptComplete(size_t referenceSize, OSAction ** action);\
\
    kern_return_t\
    CreateActionMidiInComplete(size_t referenceSize, OSAction ** action);\
\
\
protected:\
    /* _Impl methods */\
\
    kern_return_t\
    Start_Impl(IOService_Start_Args);\
\
    kern_return_t\
    Stop_Impl(IOService_Stop_Args);\
\
    void\
    CaptureIsochComplete_Impl(UA55AudioDriver_CaptureIsochComplete_Args);\
\
    void\
    PlaybackIsochComplete_Impl(UA55AudioDriver_PlaybackIsochComplete_Args);\
\
    void\
    StatusInterruptComplete_Impl(UA55AudioDriver_StatusInterruptComplete_Args);\
\
    void\
    MidiInComplete_Impl(UA55AudioDriver_MidiInComplete_Args);\
\
\
public:\
    /* _Invoke methods */\
\


#define UA55AudioDriver_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define UA55AudioDriver_VirtualMethods \
\
public:\
\
    virtual bool\
    init(\
) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    free(\
) APPLE_KEXT_OVERRIDE;\
\


#if !KERNEL

extern OSMetaClass          * gUA55AudioDriverMetaClass;
extern const OSClassLoadInformation UA55AudioDriver_Class;

class UA55AudioDriverMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

#if !KERNEL

class  UA55AudioDriverInterface : public OSInterface
{
public:
};

struct UA55AudioDriver_IVars;
struct UA55AudioDriver_LocalIVars;

class UA55AudioDriver : public IOUserAudioDriver, public UA55AudioDriverInterface
{
#if !KERNEL
    friend class UA55AudioDriverMetaClass;
#endif /* !KERNEL */

#if !KERNEL
public:
#ifdef UA55AudioDriver_DECLARE_IVARS
UA55AudioDriver_DECLARE_IVARS
#else /* UA55AudioDriver_DECLARE_IVARS */
    union
    {
        UA55AudioDriver_IVars * ivars;
        UA55AudioDriver_LocalIVars * lvars;
    };
#endif /* UA55AudioDriver_DECLARE_IVARS */
#endif /* !KERNEL */

#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gUA55AudioDriverMetaClass; };
#endif /* KERNEL */

    using super = IOUserAudioDriver;

#if !KERNEL
    UA55AudioDriver_Methods
    UA55AudioDriver_VirtualMethods
#endif /* !KERNEL */

};
#endif /* !KERNEL */


#define OSAction_UA55AudioDriver_CaptureIsochComplete_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55AudioDriver_CaptureIsochComplete * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55AudioDriver_CaptureIsochComplete_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55AudioDriver_CaptureIsochComplete_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass;
extern const OSClassLoadInformation OSAction_UA55AudioDriver_CaptureIsochComplete_Class;

class OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_CaptureIsochCompleteInterface : public OSInterface
{
public:
};

struct OSAction_UA55AudioDriver_CaptureIsochComplete_IVars;
struct OSAction_UA55AudioDriver_CaptureIsochComplete_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_CaptureIsochComplete : public OSAction, public OSAction_UA55AudioDriver_CaptureIsochCompleteInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55AudioDriver_CaptureIsochComplete);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55AudioDriver_CaptureIsochComplete_DECLARE_IVARS
OSAction_UA55AudioDriver_CaptureIsochComplete_DECLARE_IVARS
#else /* OSAction_UA55AudioDriver_CaptureIsochComplete_DECLARE_IVARS */
    union
    {
        OSAction_UA55AudioDriver_CaptureIsochComplete_IVars * ivars;
        OSAction_UA55AudioDriver_CaptureIsochComplete_LocalIVars * lvars;
    };
#endif /* OSAction_UA55AudioDriver_CaptureIsochComplete_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55AudioDriver_CaptureIsochCompleteMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55AudioDriver_CaptureIsochComplete_Methods
#endif /* !KERNEL */

    OSAction_UA55AudioDriver_CaptureIsochComplete_VirtualMethods
};

#define OSAction_UA55AudioDriver_PlaybackIsochComplete_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55AudioDriver_PlaybackIsochComplete * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55AudioDriver_PlaybackIsochComplete_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55AudioDriver_PlaybackIsochComplete_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass;
extern const OSClassLoadInformation OSAction_UA55AudioDriver_PlaybackIsochComplete_Class;

class OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_PlaybackIsochCompleteInterface : public OSInterface
{
public:
};

struct OSAction_UA55AudioDriver_PlaybackIsochComplete_IVars;
struct OSAction_UA55AudioDriver_PlaybackIsochComplete_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_PlaybackIsochComplete : public OSAction, public OSAction_UA55AudioDriver_PlaybackIsochCompleteInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55AudioDriver_PlaybackIsochComplete);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55AudioDriver_PlaybackIsochComplete_DECLARE_IVARS
OSAction_UA55AudioDriver_PlaybackIsochComplete_DECLARE_IVARS
#else /* OSAction_UA55AudioDriver_PlaybackIsochComplete_DECLARE_IVARS */
    union
    {
        OSAction_UA55AudioDriver_PlaybackIsochComplete_IVars * ivars;
        OSAction_UA55AudioDriver_PlaybackIsochComplete_LocalIVars * lvars;
    };
#endif /* OSAction_UA55AudioDriver_PlaybackIsochComplete_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55AudioDriver_PlaybackIsochCompleteMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55AudioDriver_PlaybackIsochComplete_Methods
#endif /* !KERNEL */

    OSAction_UA55AudioDriver_PlaybackIsochComplete_VirtualMethods
};

#define OSAction_UA55AudioDriver_StatusInterruptComplete_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55AudioDriver_StatusInterruptComplete * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55AudioDriver_StatusInterruptComplete_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55AudioDriver_StatusInterruptComplete_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass;
extern const OSClassLoadInformation OSAction_UA55AudioDriver_StatusInterruptComplete_Class;

class OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_StatusInterruptCompleteInterface : public OSInterface
{
public:
};

struct OSAction_UA55AudioDriver_StatusInterruptComplete_IVars;
struct OSAction_UA55AudioDriver_StatusInterruptComplete_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_StatusInterruptComplete : public OSAction, public OSAction_UA55AudioDriver_StatusInterruptCompleteInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55AudioDriver_StatusInterruptComplete);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55AudioDriver_StatusInterruptComplete_DECLARE_IVARS
OSAction_UA55AudioDriver_StatusInterruptComplete_DECLARE_IVARS
#else /* OSAction_UA55AudioDriver_StatusInterruptComplete_DECLARE_IVARS */
    union
    {
        OSAction_UA55AudioDriver_StatusInterruptComplete_IVars * ivars;
        OSAction_UA55AudioDriver_StatusInterruptComplete_LocalIVars * lvars;
    };
#endif /* OSAction_UA55AudioDriver_StatusInterruptComplete_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55AudioDriver_StatusInterruptCompleteMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55AudioDriver_StatusInterruptComplete_Methods
#endif /* !KERNEL */

    OSAction_UA55AudioDriver_StatusInterruptComplete_VirtualMethods
};

#define OSAction_UA55AudioDriver_MidiInComplete_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55AudioDriver_MidiInComplete * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55AudioDriver_MidiInComplete_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55AudioDriver_MidiInComplete_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55AudioDriver_MidiInCompleteMetaClass;
extern const OSClassLoadInformation OSAction_UA55AudioDriver_MidiInComplete_Class;

class OSAction_UA55AudioDriver_MidiInCompleteMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_MidiInCompleteInterface : public OSInterface
{
public:
};

struct OSAction_UA55AudioDriver_MidiInComplete_IVars;
struct OSAction_UA55AudioDriver_MidiInComplete_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDriver_MidiInComplete : public OSAction, public OSAction_UA55AudioDriver_MidiInCompleteInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55AudioDriver_MidiInComplete);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55AudioDriver_MidiInCompleteMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55AudioDriver_MidiInComplete_DECLARE_IVARS
OSAction_UA55AudioDriver_MidiInComplete_DECLARE_IVARS
#else /* OSAction_UA55AudioDriver_MidiInComplete_DECLARE_IVARS */
    union
    {
        OSAction_UA55AudioDriver_MidiInComplete_IVars * ivars;
        OSAction_UA55AudioDriver_MidiInComplete_LocalIVars * lvars;
    };
#endif /* OSAction_UA55AudioDriver_MidiInComplete_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55AudioDriver_MidiInCompleteMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55AudioDriver_MidiInCompleteMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55AudioDriver_MidiInComplete_Methods
#endif /* !KERNEL */

    OSAction_UA55AudioDriver_MidiInComplete_VirtualMethods
};

#endif /* !__DOCUMENTATION__ */

/* UA55AudioDriver.iig:32- */

#endif
