#include "UA55DescriptorWalk.h"

static uint16_t UA55ReadLE16(const uint8_t* bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static UA55WalkResult UA55InvalidWalk(const char* error)
{
    UA55WalkResult result = {};
    result.valid = false;
    result.error = error;
    return result;
}

static void UA55FillPreview(UA55DescriptorEvent* event, const uint8_t* descriptor)
{
    uint8_t count = event->descriptorLength;
    if (count > 8) {
        count = 8;
    }
    event->previewLength = count;
    for (uint8_t index = 0; index < 8; index++) {
        event->preview[index] = (index < count) ? descriptor[index] : 0;
    }
}

UA55WalkResult UA55WalkConfigurationDescriptor(
    const uint8_t* bytes,
    size_t readableBytes,
    UA55DescriptorEventFn callback,
    void* context)
{
    if (bytes == 0 || readableBytes < 9) {
        return UA55InvalidWalk("configuration descriptor too small");
    }

    const uint8_t headerLength = bytes[0];
    const uint8_t headerType = bytes[1];
    if (headerLength < 9 || headerType != 0x02) {
        return UA55InvalidWalk("buffer is not a configuration descriptor");
    }
    if (headerLength > readableBytes) {
        return UA55InvalidWalk("configuration bLength exceeds readable bytes");
    }

    const uint16_t totalLength = UA55ReadLE16(bytes + 2);
    if (totalLength < headerLength) {
        return UA55InvalidWalk("wTotalLength smaller than bLength");
    }
    if ((size_t)totalLength > readableBytes) {
        UA55WalkResult rejected = UA55InvalidWalk("wTotalLength exceeds readable bytes");
        rejected.truncated = true;
        rejected.wTotalLength = totalLength;
        rejected.bNumInterfaces = bytes[4];
        rejected.bConfigurationValue = bytes[5];
        rejected.bmAttributes = bytes[7];
        rejected.bMaxPower = bytes[8];
        return rejected;
    }

    UA55WalkResult result = {};
    result.valid = true;
    result.error = 0;
    result.wTotalLength = totalLength;
    result.bNumInterfaces = bytes[4];
    result.bConfigurationValue = bytes[5];
    result.bmAttributes = bytes[7];
    result.bMaxPower = bytes[8];

    bool haveInterface = false;
    uint8_t currentInterface = 0;
    uint8_t currentAlternate = 0;
    uint32_t offset = 0;

    while (offset < totalLength) {
        if ((size_t)offset + 2 > (size_t)totalLength) {
            result.valid = false;
            result.truncated = true;
            result.error = "descriptor header truncated";
            break;
        }

        const uint8_t descriptorLength = bytes[offset];
        const uint8_t descriptorType = bytes[offset + 1];
        if (descriptorLength < 2 || (size_t)offset + descriptorLength > (size_t)totalLength) {
            result.valid = false;
            result.truncated = true;
            result.error = "descriptor length walks outside wTotalLength";
            break;
        }

        const uint8_t* descriptor = bytes + offset;
        UA55DescriptorEvent event = {};
        event.descriptorType = descriptorType;
        event.descriptorLength = descriptorLength;
        event.offset = offset;
        event.interfaceNumber = haveInterface ? currentInterface : 0xFF;
        event.alternateSetting = haveInterface ? currentAlternate : 0xFF;
        UA55FillPreview(&event, descriptor);

        if (descriptorType == 0x04) {
            if (descriptorLength < 9) {
                result.valid = false;
                result.truncated = true;
                result.error = "interface descriptor shorter than 9 bytes";
                break;
            }
            event.kind = kUA55DescriptorInterface;
            event.interfaceNumber = descriptor[2];
            event.alternateSetting = descriptor[3];
            event.numEndpoints = descriptor[4];
            event.interfaceClass = descriptor[5];
            event.interfaceSubClass = descriptor[6];
            event.interfaceProtocol = descriptor[7];
            currentInterface = event.interfaceNumber;
            currentAlternate = event.alternateSetting;
            haveInterface = true;
            result.interfaceDescriptorCount++;
        } else if (descriptorType == 0x05) {
            if (descriptorLength < 7) {
                result.valid = false;
                result.truncated = true;
                result.error = "endpoint descriptor shorter than 7 bytes";
                break;
            }
            event.kind = kUA55DescriptorEndpoint;
            event.endpointAddress = descriptor[2];
            event.bmAttributes = descriptor[3];
            event.wMaxPacketSize = UA55ReadLE16(descriptor + 4);
            event.packetSize = (uint16_t)(event.wMaxPacketSize & 0x07FF);
            event.additionalTransactions = (uint8_t)((event.wMaxPacketSize >> 11) & 0x3);
            event.bInterval = descriptor[6];
            result.endpointDescriptorCount++;
        } else if (offset != 0) {
            event.kind = kUA55DescriptorOther;
            result.otherDescriptorCount++;
        }

        if (offset != 0 && callback != 0) {
            callback(&event, context);
        }

        offset += descriptorLength;
    }

    return result;
}
