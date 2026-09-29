/* iig(DriverKit-440) generated from UA55AudioDevice.iig */

/* UA55AudioDevice.iig:1-8 */
#ifndef UA55AudioDevice_h
#define UA55AudioDevice_h

#include <Availability.h>
#include <DriverKit/OSSharedPtr.h>
#include <DriverKit/IOTimerDispatchSource.h>  /* .iig include */
#include <AudioDriverKit/IOUserAudioDevice.h>  /* .iig include */

/* source class UA55AudioDevice UA55AudioDevice.iig:9-28 */

#if __DOCUMENTATION__
#define KERNEL IIG_KERNEL

class UA55AudioDevice: public IOUserAudioDevice
{
public:
    virtual bool init(IOUserAudioDriver* in_driver,
                      bool in_supports_prewarming,
                      OSString* in_device_uid,
                      OSString* in_model_uid,
                      OSString* in_manufacturer_uid,
                      uint32_t in_zero_timestamp_period) override;
    virtual void free(void) override;

    virtual kern_return_t ConfigureHardware(uint64_t usbStreamAddr) LOCALONLY;
    virtual void PublishUsbTimestamp(uint64_t sampleTime, uint64_t hostTime) LOCALONLY;
    virtual void UpdatePreampSensReadDb(uint32_t channel, uint32_t db) LOCALONLY;

    virtual kern_return_t StartIO(IOUserAudioStartStopFlags in_flags) override;
    virtual kern_return_t StopIO(IOUserAudioStartStopFlags in_flags) override;

    virtual void TimerOccurred(OSAction* action, uint64_t time)
        TYPE(IOTimerDispatchSource::TimerOccurred);
};

#undef KERNEL
#else /* __DOCUMENTATION__ */

/* generated class UA55AudioDevice UA55AudioDevice.iig:9-28 */

#define UA55AudioDevice_TimerOccurred_ID            0x1267fc6e9974e8d8ULL

#define UA55AudioDevice_TimerOccurred_Args \
        OSAction * action, \
        uint64_t time

#define UA55AudioDevice_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(UA55AudioDevice * self, const IORPC rpc);\
\
    kern_return_t\
    CreateActionTimerOccurred(size_t referenceSize, OSAction ** action);\
\
\
protected:\
    /* _Impl methods */\
\
    void\
    TimerOccurred_Impl(UA55AudioDevice_TimerOccurred_Args);\
\
\
public:\
    /* _Invoke methods */\
\


#define UA55AudioDevice_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define UA55AudioDevice_VirtualMethods \
\
public:\
\
    virtual bool\
    init(\
        IOUserAudioDriver * in_driver,\
        bool in_supports_prewarming,\
        OSString * in_device_uid,\
        OSString * in_model_uid,\
        OSString * in_manufacturer_uid,\
        uint32_t in_zero_timestamp_period) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    free(\
) APPLE_KEXT_OVERRIDE;\
\
    virtual kern_return_t\
    ConfigureHardware(\
        uint64_t usbStreamAddr) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    PublishUsbTimestamp(\
        uint64_t sampleTime,\
        uint64_t hostTime) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    UpdatePreampSensReadDb(\
        uint32_t channel,\
        uint32_t db) APPLE_KEXT_OVERRIDE;\
\
    virtual kern_return_t\
    StartIO(\
        IOUserAudioStartStopFlags in_flags) APPLE_KEXT_OVERRIDE;\
\
    virtual kern_return_t\
    StopIO(\
        IOUserAudioStartStopFlags in_flags) APPLE_KEXT_OVERRIDE;\
\


#if !KERNEL

extern OSMetaClass          * gUA55AudioDeviceMetaClass;
extern const OSClassLoadInformation UA55AudioDevice_Class;

class UA55AudioDeviceMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

#if !KERNEL

class  UA55AudioDeviceInterface : public OSInterface
{
public:
    virtual kern_return_t
    ConfigureHardware(uint64_t usbStreamAddr) = 0;

    virtual void
    PublishUsbTimestamp(uint64_t sampleTime,
        uint64_t hostTime) = 0;

    virtual void
    UpdatePreampSensReadDb(uint32_t channel,
        uint32_t db) = 0;

    kern_return_t
    ConfigureHardware_Call(uint64_t usbStreamAddr)  { return ConfigureHardware(usbStreamAddr); };\

    void
    PublishUsbTimestamp_Call(uint64_t sampleTime,
        uint64_t hostTime)  { return PublishUsbTimestamp(sampleTime, hostTime); };\

    void
    UpdatePreampSensReadDb_Call(uint32_t channel,
        uint32_t db)  { return UpdatePreampSensReadDb(channel, db); };\

};

struct UA55AudioDevice_IVars;
struct UA55AudioDevice_LocalIVars;

class UA55AudioDevice : public IOUserAudioDevice, public UA55AudioDeviceInterface
{
#if !KERNEL
    friend class UA55AudioDeviceMetaClass;
#endif /* !KERNEL */

#if !KERNEL
public:
#ifdef UA55AudioDevice_DECLARE_IVARS
UA55AudioDevice_DECLARE_IVARS
#else /* UA55AudioDevice_DECLARE_IVARS */
    union
    {
        UA55AudioDevice_IVars * ivars;
        UA55AudioDevice_LocalIVars * lvars;
    };
#endif /* UA55AudioDevice_DECLARE_IVARS */
#endif /* !KERNEL */

#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gUA55AudioDeviceMetaClass; };
#endif /* KERNEL */

    using super = IOUserAudioDevice;

#if !KERNEL
    UA55AudioDevice_Methods
    UA55AudioDevice_VirtualMethods
#endif /* !KERNEL */

};
#endif /* !KERNEL */


#define OSAction_UA55AudioDevice_TimerOccurred_Methods \
\
public:\
\
    virtual kern_return_t\
    Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;\
\
    static kern_return_t\
    _Dispatch(OSAction_UA55AudioDevice_TimerOccurred * self, const IORPC rpc);\
\
\
protected:\
    /* _Impl methods */\
\
\
public:\
    /* _Invoke methods */\
\


#define OSAction_UA55AudioDevice_TimerOccurred_KernelMethods \
\
protected:\
    /* _Impl methods */\
\


#define OSAction_UA55AudioDevice_TimerOccurred_VirtualMethods \
\
public:\
\


#if !KERNEL

extern OSMetaClass          * gOSAction_UA55AudioDevice_TimerOccurredMetaClass;
extern const OSClassLoadInformation OSAction_UA55AudioDevice_TimerOccurred_Class;

class OSAction_UA55AudioDevice_TimerOccurredMetaClass : public OSMetaClass
{
public:
    virtual kern_return_t
    New(OSObject * instance) override;
    virtual kern_return_t
    Dispatch(const IORPC rpc) override;
};

#endif /* !KERNEL */

class  __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDevice_TimerOccurredInterface : public OSInterface
{
public:
};

struct OSAction_UA55AudioDevice_TimerOccurred_IVars;
struct OSAction_UA55AudioDevice_TimerOccurred_LocalIVars;

class __attribute__((availability(driverkit,introduced=20,message="Type-safe OSAction factory methods are available in DriverKit 20 and newer"))) OSAction_UA55AudioDevice_TimerOccurred : public OSAction, public OSAction_UA55AudioDevice_TimerOccurredInterface
{
#if KERNEL
    OSDeclareDefaultStructorsWithDispatch(OSAction_UA55AudioDevice_TimerOccurred);
#endif /* KERNEL */

#if !KERNEL
    friend class OSAction_UA55AudioDevice_TimerOccurredMetaClass;
#endif /* !KERNEL */

public:
#ifdef OSAction_UA55AudioDevice_TimerOccurred_DECLARE_IVARS
OSAction_UA55AudioDevice_TimerOccurred_DECLARE_IVARS
#else /* OSAction_UA55AudioDevice_TimerOccurred_DECLARE_IVARS */
    union
    {
        OSAction_UA55AudioDevice_TimerOccurred_IVars * ivars;
        OSAction_UA55AudioDevice_TimerOccurred_LocalIVars * lvars;
    };
#endif /* OSAction_UA55AudioDevice_TimerOccurred_DECLARE_IVARS */
#if !KERNEL
    static OSMetaClass *
    sGetMetaClass() { return gOSAction_UA55AudioDevice_TimerOccurredMetaClass; };
    virtual const OSMetaClass *
    getMetaClass() const APPLE_KEXT_OVERRIDE { return gOSAction_UA55AudioDevice_TimerOccurredMetaClass; };
#endif /* KERNEL */

    using super = OSAction;

#if !KERNEL
    OSAction_UA55AudioDevice_TimerOccurred_Methods
#endif /* !KERNEL */

    OSAction_UA55AudioDevice_TimerOccurred_VirtualMethods
};

#endif /* !__DOCUMENTATION__ */

/* UA55AudioDevice.iig:30- */

#endif
