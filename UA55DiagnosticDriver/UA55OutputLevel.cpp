#include <math.h>
#include <string.h>

#include <os/log.h>
#include <DriverKit/DriverKit.h>

#include "UA55MuteControl.h"
#include "UA55VolumeControl.h"
#include "UA55UsbStream.h"

using namespace AudioDriverKit;

namespace {

// Curva do slider do macOS (transfer 1/3): amplitude = scalar^3.
// Metade do slider fica em cerca de -18 dB, como as colunas internas.
float GainFromScalar(float scalar)
{
    if (scalar <= 0.0f) {
        return 0.0f;
    }
    if (scalar >= 1.0f) {
        return 1.0f;
    }
    return scalar * scalar * scalar;
}

float GainFromDecibels(float db)
{
    if (db <= -96.0f) {
        return 0.0f;
    }
    if (db >= 0.0f) {
        return 1.0f;
    }
    return powf(10.0f, db / 20.0f);
}

} // namespace

struct UA55VolumeControl_IVars {
    UA55UsbStream* stream;
    uint32_t pairIndex;
    bool applying;
};

struct UA55MuteControl_IVars {
    UA55UsbStream* stream;
    uint32_t pairIndex;
    bool applying;
};

static void ApplyGain(UA55UsbStream* stream, uint32_t pairIndex, float gain)
{
    if (stream == nullptr) {
        return;
    }
    if (pairIndex >= 2) {
        stream->SetOutputPairGain(0, gain);
        stream->SetOutputPairGain(1, gain);
        return;
    }
    stream->SetOutputPairGain(pairIndex, gain);
}

static void ApplyMute(UA55UsbStream* stream, uint32_t pairIndex, bool muted)
{
    if (stream == nullptr) {
        return;
    }
    if (pairIndex >= 2) {
        stream->SetOutputMasterMuted(muted);
        return;
    }
    stream->SetOutputPairMuted(pairIndex, muted);
}

bool UA55VolumeControl::init(IOUserAudioDriver* in_driver,
                             bool in_is_settable,
                             float in_decibel_value,
                             IOUserAudioLevelControlRange in_decibel_range,
                             IOUserAudioObjectPropertyElement in_control_element,
                             IOUserAudioObjectPropertyScope in_control_scope,
                             IOUserAudioClassID in_control_class_id)
{
    if (!super::init(in_driver,
                     in_is_settable,
                     in_decibel_value,
                     in_decibel_range,
                     in_control_element,
                     in_control_scope,
                     in_control_class_id)) {
        return false;
    }

    ivars = IONewZero(UA55VolumeControl_IVars, 1);
    if (ivars == nullptr) {
        return false;
    }
    ivars->stream = nullptr;
    ivars->pairIndex = 0;
    return true;
}

void UA55VolumeControl::free(void)
{
    IOSafeDeleteNULL(ivars, UA55VolumeControl_IVars, 1);
    super::free();
}

void UA55VolumeControl::BindStream(uint64_t usbStreamAddr, uint32_t pairIndex)
{
    if (ivars == nullptr) {
        return;
    }
    ivars->pairIndex = pairIndex;
    UA55UsbStream* stream = reinterpret_cast<UA55UsbStream*>(usbStreamAddr);
    __atomic_store_n(&ivars->stream, stream, __ATOMIC_RELEASE);
}

kern_return_t UA55VolumeControl::HandleChangeDecibelValue(float in_decibel_value)
{
    if (ivars == nullptr) {
        return kIOReturnNotReady;
    }
    if (ivars->applying) {
        return super::HandleChangeDecibelValue(in_decibel_value);
    }
    ivars->applying = true;
    const kern_return_t result = super::HandleChangeDecibelValue(in_decibel_value);
    if (result == kIOReturnSuccess) {
        UA55UsbStream* stream = __atomic_load_n(&ivars->stream, __ATOMIC_ACQUIRE);
        ApplyGain(stream, ivars->pairIndex, GainFromDecibels(in_decibel_value));
    }
    ivars->applying = false;
    return result;
}

kern_return_t UA55VolumeControl::HandleChangeScalarValue(float in_scalar_value)
{
    if (ivars == nullptr) {
        return kIOReturnNotReady;
    }
    if (ivars->applying) {
        return super::HandleChangeScalarValue(in_scalar_value);
    }
    ivars->applying = true;
    const kern_return_t result = super::HandleChangeScalarValue(in_scalar_value);
    if (result == kIOReturnSuccess) {
        UA55UsbStream* stream = __atomic_load_n(&ivars->stream, __ATOMIC_ACQUIRE);
        ApplyGain(stream, ivars->pairIndex, GainFromScalar(in_scalar_value));
    }
    ivars->applying = false;
    return result;
}

bool UA55MuteControl::init(IOUserAudioDriver* in_driver,
                           bool in_is_settable,
                           bool in_control_value,
                           IOUserAudioObjectPropertyElement in_control_element,
                           IOUserAudioObjectPropertyScope in_control_scope,
                           IOUserAudioClassID in_control_class_id)
{
    if (!super::init(in_driver,
                     in_is_settable,
                     in_control_value,
                     in_control_element,
                     in_control_scope,
                     in_control_class_id)) {
        return false;
    }

    ivars = IONewZero(UA55MuteControl_IVars, 1);
    if (ivars == nullptr) {
        return false;
    }
    ivars->stream = nullptr;
    ivars->pairIndex = 0;
    return true;
}

void UA55MuteControl::free(void)
{
    IOSafeDeleteNULL(ivars, UA55MuteControl_IVars, 1);
    super::free();
}

void UA55MuteControl::BindStream(uint64_t usbStreamAddr, uint32_t pairIndex)
{
    if (ivars == nullptr) {
        return;
    }
    ivars->pairIndex = pairIndex;
    UA55UsbStream* stream = reinterpret_cast<UA55UsbStream*>(usbStreamAddr);
    __atomic_store_n(&ivars->stream, stream, __ATOMIC_RELEASE);
}

kern_return_t UA55MuteControl::HandleChangeControlValue(bool in_control_value)
{
    if (ivars == nullptr) {
        return kIOReturnNotReady;
    }
    if (ivars->applying) {
        return super::HandleChangeControlValue(in_control_value);
    }
    ivars->applying = true;
    const kern_return_t result = super::HandleChangeControlValue(in_control_value);
    if (result == kIOReturnSuccess) {
        UA55UsbStream* stream = __atomic_load_n(&ivars->stream, __ATOMIC_ACQUIRE);
        ApplyMute(stream, ivars->pairIndex, in_control_value);
    }
    ivars->applying = false;
    return result;
}
