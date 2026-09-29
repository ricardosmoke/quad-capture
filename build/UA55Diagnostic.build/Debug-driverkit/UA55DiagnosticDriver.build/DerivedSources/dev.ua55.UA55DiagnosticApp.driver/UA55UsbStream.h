/* iig(DriverKit-440) generated from UA55UsbStream.iig */

/* UA55UsbStream.iig:1-9 */
#ifndef UA55UsbStream_h
#define UA55UsbStream_h

#include <Availability.h>
#include <DriverKit/IOService.h>  /* .iig include */
#include <USBDriverKit/IOUSBHostDevice.h>  /* .iig include */
#include <USBDriverKit/IOUSBHostPipe.h>  /* .iig include */

// Anel isoc duplex reutilizável (OUT 0x05 / IN 0x85). Completions reinserem transfers.
/* source class UA55UsbStream UA55UsbStream.iig:10-33 */

#if __DOCUMENTATION__
#define KERNEL IIG_KERNEL

class UA55UsbStream: public IOService
{
public:
    virtual bool init(void) override;
    virtual void free(void) override;

    virtual kern_return_t Prepare(IOUSBHostDevice* device, IOService* client) LOCALONLY;
    virtual void TearDown(IOService* client) LOCALONLY;
    virtual kern_return_t StartStreaming(void) LOCALONLY;
    virtual void StopStreaming(void) LOCALONLY;

    virtual void SetAudioBridge(uint64_t playbackRingAddr,
                                uint64_t captureRingAddr,
                                uint32_t ringFrames,
                                uint64_t sampleTimeAddr,
                                uint64_t timestampTargetAddr) LOCALONLY;

    virtual kern_return_t SubmitCaptureSlot(uint32_t slotIndex) LOCALONLY;
    virtual kern_return_t SubmitPlaybackSlot(uint32_t slotIndex) LOCALONLY;

    virtual void CaptureIsochComplete(OSAction* action, IOReturn status)
        TYPE(IOUSBHostPipe::CompleteAsyncIsochIO);
    virtual void PlaybackIsochComplete(OSAction* action, IOReturn status)
        TYPE(IOUSBHostPipe::CompleteAsyncIsochIO);
};

#undef KERNEL
#else /* __DOCUMENTATION__ */

/* generated class UA55UsbStream UA55UsbStream.iig:10-33 */

#define UA55UsbStream_CaptureIsochComplete_ID            0x7c23b0e0efb2f36eULL
#define UA55UsbStream_PlaybackIsochComplete_ID            0x207099f16367d4b4ULL

#define UA55UsbStream_CaptureIsochComplete_Args \
        OSAction * action, \
        IOReturn status

#define UA55UsbStream_PlaybackIsochComplete_Args \
        OSAction * action, \
        IOReturn status

#define UA55UsbStream_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(UA55UsbStream * self, const IORPC rpc);\
\
    kern_return_t\
    CreateActionCaptureIsochComplete(size_t referenceSize, OSAction ** action);\
\
    kern_return_t\
    CreateActionPlaybackIsochComplete(size_t referenceSize, OSAction ** action);\
\
\
protected:\
    /* _Impl methods */\
\
    void\
    CaptureIsochComplete_Impl(UA55UsbStream_CaptureIsochComplete_Args);\
\
    void\
    PlaybackIsochComplete_Impl(UA55UsbStream_PlaybackIsochComplete_Args);\
\
\
public:\
    /* _Invoke methods */\
\


#define UA55UsbStream_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define UA55UsbStream_VirtualMethods \
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
    virtual kern_return_t\
    Prepare(\
        IOUSBHostDevice * device,\
        IOService * client) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    TearDown(\
        IOService * client) APPLE_KEXT_OVERRIDE;\
\
    virtual kern_return_t\
    StartStreaming(\
) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    StopStreaming(\
) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    SetAudioBridge(\
        uint64_t playbackRingAddr,\
        uint64_t captureRingAddr,\
        uint32_t ringFrames,\
        uint64_t sampleTimeAddr,\
        uint64_t timestampTargetAddr) APPLE_KEXT_OVERRIDE;\
\
    virtual kern_return_t\
    SubmitCaptureSlot(\
        uint32_t slotIndex) APPLE_KEXT_OVERRIDE;\
\
    virtual kern_return_t\
    SubmitPlaybackSlot(\
        uint32_t slotIndex) APPLE_KEXT_OVERRIDE;\
\


#if !KERNEL

extern OSMetaClass          * gUA55UsbStreamMetaClass;
extern const OSClassLoadInformation UA55UsbStream_Class;

class UA55UsbStreamMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

#if !KERNEL

class  UA55UsbStreamInterface : public OSInterface
{
public:
    virtual kern_return_t
    Prepare(IOUSBHostDevice * device,
        IOService * client) = 0;

    virtual void
    TearDown(IOService * client) = 0;

    virtual kern_return_t
    StartStreaming() = 0;

    virtual void
    StopStreaming() = 0;

    virtual void
    SetAudioBridge(uint64_t playbackRingAddr,
        uint64_t captureRingAddr,
        uint32_t ringFrames,
        uint64_t sampleTimeAddr,
        uint64_t timestampTargetAddr) = 0;

    virtual kern_return_t
    SubmitCaptureSlot(uint32_t slotIndex) = 0;

    virtual kern_return_t
    SubmitPlaybackSlot(uint32_t slotIndex) = 0;

    kern_return_t
    Prepare_Call(IOUSBHostDevice * device,
        IOService * client)  { return Prepare(device, client); };\

    void
    TearDown_Call(IOService * client)  { return TearDown(client); };\

    kern_return_t
    StartStreaming_Call()  { return StartStreaming(); };\

    void
    StopStreaming_Call()  { return StopStreaming(); };\

    void
    SetAudioBridge_Call(uint64_t playbackRingAddr,
        uint64_t captureRingAddr,
        uint32_t ringFrames,
        uint64_t sampleTimeAddr,
        uint64_t timestampTargetAddr)  { return SetAudioBridge(playbackRingAddr, captureRingAddr, ringFrames, sampleTimeAddr, timestampTargetAddr); };\

    kern_return_t
    SubmitCaptureSlot_Call(uint32_t slotIndex)  { return SubmitCaptureSlot(slotIndex); };\

    kern_return_t
    SubmitPlaybackSlot_Call(uint32_t slotIndex)  { return SubmitPlaybackSlot(slotIndex); };\

};

struct UA55UsbStream_IVars;
struct UA55UsbStream_LocalIVars;

class UA55UsbStream : public IOService, public UA55UsbStreamInterface
{
#if !KERNEL
    friend class UA55UsbStreamMetaClass;
#endif /* !KERNEL */

#if !KERNEL
public:
#ifdef UA55UsbStream_DECLARE_IVARS
UA55UsbStream_DECLARE_IVARS
#else /* UA55UsbStream_DECLARE_IVARS */
    union
    {
        UA55UsbStream_IVars * ivars;
        UA55UsbStream_LocalIVars * lvars;
    };
#endif /* UA55UsbStream_DECLARE_IVARS */
#endif /* !KERNEL */

#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gUA55UsbStreamMetaClass; };
#endif /* KERNEL */

    using super = IOService;

#if !KERNEL
    UA55UsbStream_Methods
    UA55UsbStream_VirtualMethods
#endif /* !KERNEL */

};
#endif /* !KERNEL */


#define OSAction_UA55UsbStream_CaptureIsochComplete_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55UsbStream_CaptureIsochComplete * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55UsbStream_CaptureIsochComplete_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55UsbStream_CaptureIsochComplete_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55UsbStream_CaptureIsochCompleteMetaClass;
extern const OSClassLoadInformation OSAction_UA55UsbStream_CaptureIsochComplete_Class;

class OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55UsbStream_CaptureIsochCompleteInterface : public OSInterface
{
public:
};

struct OSAction_UA55UsbStream_CaptureIsochComplete_IVars;
struct OSAction_UA55UsbStream_CaptureIsochComplete_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55UsbStream_CaptureIsochComplete : public OSAction, public OSAction_UA55UsbStream_CaptureIsochCompleteInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55UsbStream_CaptureIsochComplete);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55UsbStream_CaptureIsochCompleteMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55UsbStream_CaptureIsochComplete_DECLARE_IVARS
OSAction_UA55UsbStream_CaptureIsochComplete_DECLARE_IVARS
#else /* OSAction_UA55UsbStream_CaptureIsochComplete_DECLARE_IVARS */
    union
    {
        OSAction_UA55UsbStream_CaptureIsochComplete_IVars * ivars;
        OSAction_UA55UsbStream_CaptureIsochComplete_LocalIVars * lvars;
    };
#endif /* OSAction_UA55UsbStream_CaptureIsochComplete_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55UsbStream_CaptureIsochCompleteMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55UsbStream_CaptureIsochCompleteMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55UsbStream_CaptureIsochComplete_Methods
#endif /* !KERNEL */

    OSAction_UA55UsbStream_CaptureIsochComplete_VirtualMethods
};

#define OSAction_UA55UsbStream_PlaybackIsochComplete_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55UsbStream_PlaybackIsochComplete * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55UsbStream_PlaybackIsochComplete_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55UsbStream_PlaybackIsochComplete_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass;
extern const OSClassLoadInformation OSAction_UA55UsbStream_PlaybackIsochComplete_Class;

class OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55UsbStream_PlaybackIsochCompleteInterface : public OSInterface
{
public:
};

struct OSAction_UA55UsbStream_PlaybackIsochComplete_IVars;
struct OSAction_UA55UsbStream_PlaybackIsochComplete_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55UsbStream_PlaybackIsochComplete : public OSAction, public OSAction_UA55UsbStream_PlaybackIsochCompleteInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55UsbStream_PlaybackIsochComplete);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55UsbStream_PlaybackIsochComplete_DECLARE_IVARS
OSAction_UA55UsbStream_PlaybackIsochComplete_DECLARE_IVARS
#else /* OSAction_UA55UsbStream_PlaybackIsochComplete_DECLARE_IVARS */
    union
    {
        OSAction_UA55UsbStream_PlaybackIsochComplete_IVars * ivars;
        OSAction_UA55UsbStream_PlaybackIsochComplete_LocalIVars * lvars;
    };
#endif /* OSAction_UA55UsbStream_PlaybackIsochComplete_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55UsbStream_PlaybackIsochCompleteMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55UsbStream_PlaybackIsochComplete_Methods
#endif /* !KERNEL */

    OSAction_UA55UsbStream_PlaybackIsochComplete_VirtualMethods
};

#endif /* !__DOCUMENTATION__ */

/* UA55UsbStream.iig:35- */

#endif
