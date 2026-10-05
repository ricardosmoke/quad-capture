#include <os/log.h>
#include <string.h>

#include <DriverKit/DriverKit.h>

#include "UA55LoCutProperty.h"
#include "UA55UsbStream.h"

struct UA55LoCutProperty_IVars {
    UA55UsbStream* stream;
};

namespace {

int HexNibble(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'A' && digit <= 'F') {
        return digit - 'A' + 10;
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    return -1;
}

bool ParsePacket(const char* text, uint8_t* out, uint32_t capacity, uint32_t* length)
{
    if (text == nullptr || out == nullptr || length == nullptr) {
        return false;
    }
    uint32_t hexLen = 0;
    while (text[hexLen] != '\0') {
        if (hexLen >= capacity * 2) {
            return false;
        }
        hexLen++;
    }
    if (hexLen != 40 && hexLen != 48 && hexLen != 56) {
        return false;
    }
    const uint32_t count = hexLen / 2;
    for (uint32_t index = 0; index < count; index++) {
        const int high = HexNibble(text[index * 2]);
        const int low = HexNibble(text[index * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        out[index] = (uint8_t)((high << 4) | low);
    }
    *length = count;
    return true;
}

} // namespace

bool UA55LoCutProperty::init(IOUserAudioDriver* in_audio_driver,
                             IOUserAudioObjectPropertyAddress in_prop_addr,
                             bool in_is_property_settable,
                             IOUserAudioCustomPropertyDataType in_qualifier_data_type,
                             IOUserAudioCustomPropertyDataType in_data_type)
{
    if (!super::init(in_audio_driver,
                     in_prop_addr,
                     in_is_property_settable,
                     in_qualifier_data_type,
                     in_data_type)) {
        return false;
    }
    ivars = IONewZero(UA55LoCutProperty_IVars, 1);
    return ivars != nullptr;
}

void UA55LoCutProperty::free(void)
{
    IOSafeDeleteNULL(ivars, UA55LoCutProperty_IVars, 1);
    super::free();
}

void UA55LoCutProperty::BindStream(uint64_t usbStreamAddr)
{
    if (ivars == nullptr) {
        return;
    }
    __atomic_store_n(&ivars->stream, reinterpret_cast<UA55UsbStream*>(usbStreamAddr), __ATOMIC_RELEASE);
}

kern_return_t UA55LoCutProperty::HandleChangeCustomPropertyDataValueWithQualifier(OSObject* in_qualifier_data,
                                                                                 OSObject* in_data)
{
    (void)in_qualifier_data;
    auto* text = OSDynamicCast(OSString, in_data);
    uint8_t packet[28];
    uint32_t length = 0;
    if (text == nullptr || !ParsePacket(text->getCStringNoCopy(), packet, 28, &length)) {
        return kIOReturnBadArgument;
    }
    if (ivars == nullptr) {
        return kIOReturnOffline;
    }
    UA55UsbStream* stream = __atomic_load_n(&ivars->stream, __ATOMIC_ACQUIRE);
    if (stream == nullptr) {
        return kIOReturnOffline;
    }
    const kern_return_t sent = stream->SendMidi(packet, length);
    if (sent != kIOReturnSuccess) {
        return sent;
    }
    return super::HandleChangeCustomPropertyDataValueWithQualifier(in_qualifier_data, in_data);
}
