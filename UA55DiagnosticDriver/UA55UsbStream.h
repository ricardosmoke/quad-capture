#pragma once

#include <DriverKit/DriverKit.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <USBDriverKit/USBDriverKit.h>

#include "UA55USBConstants.h"

class UA55AudioDriver;

// Anel isoc duplex (OUT 0x05 / IN 0x85) + anéis S32 ponte HAL↔USB.
// Drain contínuo de IF3/IF4 interrupt status (0x82/0x81) — sem isto a MCU
// da UA-55 trava os knobs/botões enquanto o áudio continua.
// O HAL NÃO é lido/escrito no thread USB — só via IOOperationHandler.
class UA55UsbStream
{
public:
    UA55UsbStream();
    ~UA55UsbStream();

    kern_return_t Prepare(IOUSBHostDevice* device, IOService* client, UA55AudioDriver* actionOwner);
    void TearDown(IOService* client);
    // Para o isoc se estiver a correr, escolhe o alt USB e reabre os pipes.
    kern_return_t ApplySampleRate(uint32_t rateInt);
    uint32_t CurrentRate() const { return rate_.rateInt; }
    kern_return_t StartStreaming();
    void StopStreaming();
    // streaming_=false sem esperar. A espera fica noutra fila, para as
    // completions poderem correr.
    void BeginRateChangeQuiesce();
    bool FinishRateChangeDrain();
    // 96 ou 192 kHz → 44.1, e 44.1 → 192: transmite silêncio em 48 kHz fora
    // do Perform, depois deixa o USB já na taxa final. Dentro do Perform
    // as completions não correm.
    bool Warm44100Via48000(uint32_t finalRate);

    void SetTimestampTarget(volatile uint64_t* sampleTime, void* timestampTarget);
    void ClearTimestampTarget();
    bool IsStreaming() const { return streaming_; }
    uint64_t CurrentCaptureSample() const { return captureWriteSample_; }

    // Chamados do IOOperationHandler (contexto tempo-real do HAL).
    void HalWriteOutput(const float* outputRing, uint32_t ringFrames, uint64_t sampleTime, uint32_t frameCount);
    void HalReadInput(float* inputRing, uint32_t ringFrames, uint64_t sampleTime, uint32_t frameCount);

    // Ganho digital da saída (slider / mute do macOS). pair 0 = L, 1 = R.
    // Aplicado aos canais 1/3 (L) e 2/4 (R) antes do USB. Não mexe no knob da placa.
    void SetOutputPairGain(uint32_t pair, float linearGain);
    void SetOutputPairMuted(uint32_t pair, bool muted);
    void SetOutputMasterMuted(bool muted);

    kern_return_t SubmitCaptureSlot(uint32_t slotIndex);
    kern_return_t SubmitPlaybackSlot(uint32_t slotIndex);
    void OnCaptureComplete(uint32_t slotIndex, IOReturn status);
    void OnPlaybackComplete(uint32_t slotIndex, IOReturn status);
    void OnStatusComplete(uint32_t pipeIndex, uint32_t slotIndex, IOReturn status, uint32_t actualByteCount);
    void OnMidiInComplete(uint32_t slotIndex, IOReturn status, uint32_t actualByteCount);

private:
    struct IsochSlot {
        IOBufferMemoryDescriptor* dataBuffer = nullptr;
        IOBufferMemoryDescriptor* frameListBuffer = nullptr;
        OSAction* action = nullptr;
        uint32_t slotIndex = 0;
    };

    struct StatusSlot {
        IOBufferMemoryDescriptor* dataBuffer = nullptr;
        OSAction* action = nullptr;
        uint32_t pipeIndex = 0;
        uint32_t slotIndex = 0;
    };

    struct StatusPipe {
        IOUSBHostInterface* iface = nullptr;
        IOUSBHostPipe* pipe = nullptr;
        bool opened = false;
        uint8_t interfaceNumber = 0;
        uint8_t endpointAddress = 0;
        StatusSlot slots[kUA55StatusSlotsPerPipe];
        uint64_t completions = 0;
        uint64_t bytes = 0;
        uint64_t errors = 0;
    };

    struct MidiSlot {
        IOBufferMemoryDescriptor* dataBuffer = nullptr;
        OSAction* action = nullptr;
        uint32_t slotIndex = 0;
    };

    IOUSBHostDevice* device_ = nullptr;
    IOService* client_ = nullptr;
    UA55AudioDriver* actionOwner_ = nullptr;
    IOUSBHostInterface* playbackInterface_ = nullptr;
    IOUSBHostInterface* captureInterface_ = nullptr;
    IOUSBHostInterface* midiInterface_ = nullptr;
    IOUSBHostPipe* playbackPipe_ = nullptr;
    IOUSBHostPipe* capturePipe_ = nullptr;
    IOUSBHostPipe* midiInPipe_ = nullptr;
    IsochSlot captureSlots_[kUA55IsochRingDepth];
    IsochSlot playbackSlots_[kUA55IsochRingDepth];
    StatusPipe statusPipes_[kUA55StatusPipeCount];
    MidiSlot midiInSlots_[kUA55MidiInSlotCount];
    bool playbackOpened_ = false;
    bool captureOpened_ = false;
    bool midiOpened_ = false;
    volatile bool streaming_ = false;
    bool streamedSinceOpen_ = false;
    uint32_t isochInFlight_ = 0;
    volatile bool statusPolling_ = false;
    volatile bool midiPolling_ = false;
    uint64_t midiCompletions_ = 0;
    uint64_t midiBytes_ = 0;
    uint64_t midiErrors_ = 0;

    int32_t* playbackBridge_ = nullptr;
    int32_t* captureBridge_ = nullptr;
    uint32_t bridgeFrames_ = kUA55BridgeFrames;

    volatile uint64_t* sampleTime_ = nullptr;
    void* timestampTarget_ = nullptr;

    uint64_t captureCompletions_ = 0;
    uint64_t playbackCompletions_ = 0;
    uint64_t captureBytes_ = 0;
    uint64_t playbackBytes_ = 0;
    uint64_t captureErrors_ = 0;
    uint64_t playbackErrors_ = 0;
    UA55RateConfig rate_ = kUA55Rates[0];
    uint32_t hardwareRate_ = 0;
    uint32_t playbackPhase_ = 0;
    uint32_t lastCaptureAudioFrames_ = 44;
    uint8_t captureUframeSamples_[kUA55IsochFramesPerTransfer] = {};
    bool capturePatternValid_ = false;
    uint64_t playbackReadSample_ = 0;
    uint64_t captureWriteSample_ = 0;
    uint64_t lastPublishedZts_ = 0;
    uint64_t lastHostTime_ = 0;
    uint64_t playbackResyncs_ = 0;
    int64_t lastLoggedDrift_ = 0;
    uint64_t halWriteSample_ = 0;
    uint64_t underruns_ = 0;
    uint64_t captureUnderruns_ = 0;
    uint64_t captureOverruns_ = 0;

    // Bits de float, leitura atómica no thread de IO. 1.0f até o HAL mudar o volume.
    uint32_t outputGainBits_[2] = { 0x3f800000u, 0x3f800000u };
    uint32_t outputMutePair_[2] = { 0u, 0u };
    uint32_t outputMuteMaster_ = 0u;

    float LoadOutputPairGain(uint32_t pair) const;

    static void FreeIsochSlot(IsochSlot* slot);
    static void FreeStatusSlot(StatusSlot* slot);
    static void FreeMidiSlot(MidiSlot* slot);
    static kern_return_t PrepareIsochSlot(IsochSlot* slot, bool directionIn, uint16_t maxPacket, uint32_t slotIndex);
    static kern_return_t PrepareStatusSlot(StatusSlot* slot, uint32_t pipeIndex, uint32_t slotIndex);
    static kern_return_t PrepareMidiSlot(MidiSlot* slot, uint32_t slotIndex);
    static kern_return_t FillFrameList(IsochSlot* slot, uint16_t maxPacket);
    static uint32_t SumCompleteBytes(IsochSlot* slot);
    uint32_t UnpackCaptureToBridge(IsochSlot* slot);
    uint32_t FillPlaybackTransfer(IsochSlot* slot);
    void MaybePublishZts();
    bool EnsureBridges();
    void FreeBridges();
    kern_return_t PrepareStatusPipes();
    void TearDownStatusPipes(IOService* closer);
    kern_return_t StartStatusPolling();
    void StopStatusPolling();
    kern_return_t SubmitStatusSlot(uint32_t pipeIndex, uint32_t slotIndex);
    kern_return_t OpenAudioPipes(uint8_t alternate);
    void PrimeStreamingSilently(uint32_t milliseconds);
    bool DrainIsoch();
    kern_return_t ReadHardwareRate(uint32_t* rateOut);
    kern_return_t SetHardwareClock(uint32_t rateInt);
    kern_return_t PrepareMidiPipe();
    void TearDownMidiPipe(IOService* closer);
    kern_return_t StartMidiPolling();
    void StopMidiPolling();
    kern_return_t SubmitMidiInSlot(uint32_t slotIndex);

    static inline int32_t FloatToS24In32(float sample)
    {
        if (sample > 1.0f) {
            sample = 1.0f;
        } else if (sample < -1.0f) {
            sample = -1.0f;
        }
        return ((int32_t)(sample * 8388607.0f)) << 8;
    }

    static inline float S24In32ToFloat(int32_t sample)
    {
        return (float)(sample >> 8) * (1.0f / 8388607.0f);
    }
};
