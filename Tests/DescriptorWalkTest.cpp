#include "UA55DescriptorWalk.h"

#include <stdio.h>
#include <string.h>

static int gFailures = 0;

#define EXPECT(condition)                                                                 \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #condition);                  \
            gFailures++;                                                                  \
        }                                                                                 \
    } while (0)

static void AppendByte(uint8_t* buffer, size_t* length, size_t capacity, uint8_t value)
{
    EXPECT(*length < capacity);
    if (*length < capacity) {
        buffer[(*length)++] = value;
    }
}

static void AppendLE16(uint8_t* buffer, size_t* length, size_t capacity, uint16_t value)
{
    AppendByte(buffer, length, capacity, (uint8_t)(value & 0xFF));
    AppendByte(buffer, length, capacity, (uint8_t)((value >> 8) & 0xFF));
}

struct CollectedEvents {
    UA55DescriptorEvent events[16];
    int count;
};

static void CollectEvent(const UA55DescriptorEvent* event, void* context)
{
    CollectedEvents* collected = (CollectedEvents*)context;
    if (collected->count < 16) {
        collected->events[collected->count++] = *event;
    }
}

static void PatchTotalLength(uint8_t* buffer, uint16_t totalLength)
{
    buffer[2] = (uint8_t)(totalLength & 0xFF);
    buffer[3] = (uint8_t)((totalLength >> 8) & 0xFF);
}

static void TestTwoInterfaces()
{
    uint8_t buffer[128];
    size_t length = 0;
    memset(buffer, 0, sizeof(buffer));

    AppendByte(buffer, &length, sizeof(buffer), 9);
    AppendByte(buffer, &length, sizeof(buffer), 0x02);
    AppendLE16(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 2);
    AppendByte(buffer, &length, sizeof(buffer), 1);
    AppendByte(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 0x80);
    AppendByte(buffer, &length, sizeof(buffer), 50);

    AppendByte(buffer, &length, sizeof(buffer), 9);
    AppendByte(buffer, &length, sizeof(buffer), 0x04);
    AppendByte(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 0x01);
    AppendByte(buffer, &length, sizeof(buffer), 0x01);
    AppendByte(buffer, &length, sizeof(buffer), 0x00);
    AppendByte(buffer, &length, sizeof(buffer), 0);

    AppendByte(buffer, &length, sizeof(buffer), 9);
    AppendByte(buffer, &length, sizeof(buffer), 0x04);
    AppendByte(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 1);
    AppendByte(buffer, &length, sizeof(buffer), 1);
    AppendByte(buffer, &length, sizeof(buffer), 0x01);
    AppendByte(buffer, &length, sizeof(buffer), 0x02);
    AppendByte(buffer, &length, sizeof(buffer), 0x00);
    AppendByte(buffer, &length, sizeof(buffer), 0);

    AppendByte(buffer, &length, sizeof(buffer), 7);
    AppendByte(buffer, &length, sizeof(buffer), 0x05);
    AppendByte(buffer, &length, sizeof(buffer), 0x01);
    AppendByte(buffer, &length, sizeof(buffer), 0x05);
    AppendLE16(buffer, &length, sizeof(buffer), (uint16_t)((2 << 11) | 192));
    AppendByte(buffer, &length, sizeof(buffer), 4);

    AppendByte(buffer, &length, sizeof(buffer), 3);
    AppendByte(buffer, &length, sizeof(buffer), 0x24);
    AppendByte(buffer, &length, sizeof(buffer), 0x01);

    AppendByte(buffer, &length, sizeof(buffer), 9);
    AppendByte(buffer, &length, sizeof(buffer), 0x04);
    AppendByte(buffer, &length, sizeof(buffer), 1);
    AppendByte(buffer, &length, sizeof(buffer), 0);
    AppendByte(buffer, &length, sizeof(buffer), 1);
    AppendByte(buffer, &length, sizeof(buffer), 0xFF);
    AppendByte(buffer, &length, sizeof(buffer), 0x02);
    AppendByte(buffer, &length, sizeof(buffer), 0x00);
    AppendByte(buffer, &length, sizeof(buffer), 0);

    AppendByte(buffer, &length, sizeof(buffer), 7);
    AppendByte(buffer, &length, sizeof(buffer), 0x05);
    AppendByte(buffer, &length, sizeof(buffer), 0x82);
    AppendByte(buffer, &length, sizeof(buffer), 0x02);
    AppendLE16(buffer, &length, sizeof(buffer), 64);
    AppendByte(buffer, &length, sizeof(buffer), 0);

    PatchTotalLength(buffer, (uint16_t)length);

    CollectedEvents collected = {};
    UA55WalkResult result = UA55WalkConfigurationDescriptor(buffer, length, CollectEvent, &collected);

    EXPECT(result.valid);
    EXPECT(result.error == 0);
    EXPECT(result.wTotalLength == length);
    EXPECT(result.bConfigurationValue == 1);
    EXPECT(result.bNumInterfaces == 2);
    EXPECT(result.bmAttributes == 0x80);
    EXPECT(result.bMaxPower == 50);
    EXPECT(result.interfaceDescriptorCount == 3);
    EXPECT(result.endpointDescriptorCount == 2);
    EXPECT(result.otherDescriptorCount == 1);
    EXPECT(collected.count == 6);

    EXPECT(collected.events[0].kind == kUA55DescriptorInterface);
    EXPECT(collected.events[0].interfaceNumber == 0);
    EXPECT(collected.events[0].alternateSetting == 0);
    EXPECT(collected.events[0].interfaceClass == 0x01);
    EXPECT(collected.events[0].numEndpoints == 0);

    EXPECT(collected.events[1].alternateSetting == 1);
    EXPECT(collected.events[1].interfaceSubClass == 0x02);

    EXPECT(collected.events[2].kind == kUA55DescriptorEndpoint);
    EXPECT(collected.events[2].interfaceNumber == 0);
    EXPECT(collected.events[2].alternateSetting == 1);
    EXPECT(collected.events[2].endpointAddress == 0x01);
    EXPECT(collected.events[2].bmAttributes == 0x05);
    EXPECT(collected.events[2].packetSize == 192);
    EXPECT(collected.events[2].additionalTransactions == 2);
    EXPECT(collected.events[2].bInterval == 4);

    EXPECT(collected.events[3].kind == kUA55DescriptorOther);
    EXPECT(collected.events[3].descriptorType == 0x24);
    EXPECT(collected.events[3].preview[2] == 0x01);

    EXPECT(collected.events[4].interfaceNumber == 1);
    EXPECT(collected.events[4].interfaceClass == 0xFF);

    EXPECT(collected.events[5].endpointAddress == 0x82);
    EXPECT(collected.events[5].packetSize == 64);
    EXPECT(collected.events[5].interfaceNumber == 1);
}

static void TestRejectsUnsafeLengths()
{
    uint8_t tiny[4] = {9, 0x02, 0, 0};
    UA55WalkResult tooSmall = UA55WalkConfigurationDescriptor(tiny, sizeof(tiny), 0, 0);
    EXPECT(!tooSmall.valid);

    uint8_t zeroLength[9] = {0, 0x02, 9, 0, 0, 1, 0, 0x80, 50};
    UA55WalkResult zero = UA55WalkConfigurationDescriptor(zeroLength, sizeof(zeroLength), 0, 0);
    EXPECT(!zero.valid);

    uint8_t shortTotal[9] = {9, 0x02, 4, 0, 1, 1, 0, 0x80, 50};
    UA55WalkResult shortTotalResult = UA55WalkConfigurationDescriptor(shortTotal, sizeof(shortTotal), 0, 0);
    EXPECT(!shortTotalResult.valid);

    uint8_t claimsTooMuch[9] = {9, 0x02, 100, 0, 1, 1, 0, 0x80, 50};
    UA55WalkResult truncated = UA55WalkConfigurationDescriptor(claimsTooMuch, sizeof(claimsTooMuch), 0, 0);
    EXPECT(!truncated.valid);
    EXPECT(truncated.truncated);
    EXPECT(truncated.wTotalLength == 100);

    UA55WalkResult missing = UA55WalkConfigurationDescriptor(0, 9, 0, 0);
    EXPECT(!missing.valid);
}

static void TestInteriorTruncation()
{
    uint8_t buffer[16] = {
        9, 0x02, 12, 0, 1, 1, 0, 0x80, 50,
        0, 0x04, 0
    };
    UA55WalkResult result = UA55WalkConfigurationDescriptor(buffer, sizeof(buffer), 0, 0);
    EXPECT(!result.valid);
    EXPECT(result.truncated);
}

int main()
{
    TestTwoInterfaces();
    TestRejectsUnsafeLengths();
    TestInteriorTruncation();
    if (gFailures != 0) {
        printf("%d failure(s)\n", gFailures);
        return 1;
    }
    printf("descriptor walk tests passed\n");
    return 0;
}
