/* iig(DriverKit-440) generated from UA55AudioDevice.iig */

/* UA55AudioDevice.iig:1-8 */
#ifndef UA55AudioDevice_h
#define UA55AudioDevice_h

#include <Availability.h>
#include <DriverKit/IOService.h>  /* .iig include */
#include <DriverKit/OSSharedPtr.h>
#include <AudioDriverKit/IOUserAudioDevice.h>  /* .iig include */

/* source class UA55AudioDevice UA55AudioDevice.iig:9-24 */

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

    virtual kern_return_t ConfigureHardware(IOService* usbStream) LOCALONLY;
    virtual void PublishUsbTimestamp(uint64_t sampleTime, uint64_t hostTime) LOCALONLY;

    virtual kern_return_t StartIO(IOUserAudioStartStopFlags in_flags) override;
    virtual kern_return_t StopIO(IOUserAudioStartStopFlags in_flags) override;
};

#undef KERNEL
#else /* __DOCUMENTATION__ */

/* generated class UA55AudioDevice UA55AudioDevice.iig:9-24 */


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
\
protected:\
    /* _Impl methods */\
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
        IOService * usbStream) APPLE_KEXT_OVERRIDE;\
\
    virtual void\
    PublishUsbTimestamp(\
        uint64_t sampleTime,\
        uint64_t hostTime) APPLE_KEXT_OVERRIDE;\
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
    ConfigureHardware(IOService * usbStream) = 0;

    virtual void
    PublishUsbTimestamp(uint64_t sampleTime,
        uint64_t hostTime) = 0;

    kern_return_t
    ConfigureHardware_Call(IOService * usbStream)  { return ConfigureHardware(usbStream); };\

    void
    PublishUsbTimestamp_Call(uint64_t sampleTime,
        uint64_t hostTime)  { return PublishUsbTimestamp(sampleTime, hostTime); };\

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


#endif /* !__DOCUMENTATION__ */

/* UA55AudioDevice.iig:26- */

#endif
