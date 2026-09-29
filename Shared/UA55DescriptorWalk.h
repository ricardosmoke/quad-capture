#pragma once

#include <stddef.h>
#include <stdint.h>

// Caminhada do configuration descriptor no layout do USB 2.0.
// Não atribui função de áudio, MIDI ou controle a nenhuma interface.

enum UA55DescriptorKind {
    kUA55DescriptorInterface = 1,
    kUA55DescriptorEndpoint = 2,
    kUA55DescriptorOther = 3
};

struct UA55DescriptorEvent {
    uint8_t kind;
    uint8_t interfaceNumber;
    uint8_t alternateSetting;
    uint8_t interfaceClass;
    uint8_t interfaceSubClass;
    uint8_t interfaceProtocol;
    uint8_t numEndpoints;
    uint8_t endpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint16_t packetSize;
    uint8_t additionalTransactions;
    uint8_t bInterval;
    uint8_t descriptorType;
    uint8_t descriptorLength;
    uint32_t offset;
    uint8_t preview[8];
    uint8_t previewLength;
};

struct UA55WalkResult {
    bool valid;
    bool truncated;
    uint16_t wTotalLength;
    uint8_t bConfigurationValue;
    uint8_t bNumInterfaces;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
    uint32_t interfaceDescriptorCount;
    uint32_t endpointDescriptorCount;
    uint32_t otherDescriptorCount;
    const char* error;
};

typedef void (*UA55DescriptorEventFn)(const UA55DescriptorEvent* event, void* context);

// `readableBytes` é o máximo que a função pode ler. Se wTotalLength
// passar desse limite, a caminhada falha e não lê além do cabeçalho.
UA55WalkResult UA55WalkConfigurationDescriptor(
    const uint8_t* bytes,
    size_t readableBytes,
    UA55DescriptorEventFn callback,
    void* context);
