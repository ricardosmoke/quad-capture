/* iig(DriverKit-440) generated from UA55AudioDriver.iig */

/* UA55AudioDriver.iig:1-7 */
#ifndef UA55AudioDriver_h
#define UA55AudioDriver_h

#include <Availability.h>
#include <DriverKit/OSSharedPtr.h>
#include <AudioDriverKit/IOUserAudioDriver.h>  /* .iig include */

/* source class UA55AudioDriver UA55AudioDriver.iig:8-14 */

#if __DOCUMENTATION__
#define KERNEL IIG_KERNEL

class UA55AudioDriver: public IOUserAudioDriver
{
public:
    virtual bool init(void) override;
    virtual void free(void) override;
    virtual kern_return_t Start(IOService* provider) override;
    virtual kern_return_t Stop(IOService* provider) override;
};

#undef KERNEL
#else /* __DOCUMENTATION__ */

/* generated class UA55AudioDriver UA55AudioDriver.iig:8-14 */


#define UA55AudioDriver_Start_Args \
        IOService * provider

#define UA55AudioDriver_Stop_Args \
        IOService * provider

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


#endif /* !__DOCUMENTATION__ */

/* UA55AudioDriver.iig:16- */

#endif
