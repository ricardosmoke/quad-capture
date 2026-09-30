#pragma once

#include <stdint.h>

// Roland QUAD-CAPTURE UA-55. Estes valores também estão na Info.plist
// e no entitlement USB. Os três lugares precisam permanecer iguais.
// 0x0582 = 1410, 0x012F = 303.
static const uint16_t kUA55VendorID = 0x0582;
static const uint16_t kUA55ProductID = 0x012F;

// Deve coincidir com CURRENT_PROJECT_VERSION no Xcode (log ao plugar).
#ifndef UA55_DRIVER_BUILD
#define UA55_DRIVER_BUILD 43
#endif
static const uint32_t kUA55DriverBuild = UA55_DRIVER_BUILD;

// Pedido explícito do primeiro marco. O valor real lido do descritor
// é registrado à parte; este número não substitui essa leitura.
static const uint8_t kUA55RequestedConfiguration = 1;

// Limite de segurança da caminhada do configuration descriptor.
// Não é tamanho medido do aparelho.
static const uint16_t kUA55MaxConfigurationBytes = 4096;

// Medidos no Mac em 2026-09-28 a partir do configuration descriptor
// da UA-55 (VID 0582 PID 012F). Não vieram do quirks-table.h.
static const uint8_t kUA55PlaybackInterface = 0;
static const uint8_t kUA55CaptureInterface = 1;
static const uint8_t kUA55MidiInterface = 2;
static const uint8_t kUA55StatusAInterface = 3; // interrupt IN 0x82 (clock/status)
static const uint8_t kUA55StatusBInterface = 4; // interrupt IN 0x81 (flood telemetria)
static const uint8_t kUA55StreamingAlternate = 1;
static const uint8_t kUA55MidiAlternateBulk = 0;
static const uint8_t kUA55PlaybackEndpointAddress = 0x05;
static const uint8_t kUA55CaptureEndpointAddress = 0x85;
static const uint8_t kUA55MidiOutEndpointAddress = 0x06;
static const uint8_t kUA55MidiInEndpointAddress = 0x86;
static const uint8_t kUA55StatusAEndpointAddress = 0x82;
static const uint8_t kUA55StatusBEndpointAddress = 0x81;
static const uint16_t kUA55PlaybackMaxPacketAlt1 = 112;
static const uint16_t kUA55CaptureMaxPacketAlt1 = 168;
static const uint16_t kUA55MidiMaxPacket = 512;
static const uint16_t kUA55StatusMaxPacket = 8;
static const uint32_t kUA55StatusPipeCount = 2;
static const uint32_t kUA55StatusSlotsPerPipe = 2;
static const uint32_t kUA55MidiInSlotCount = 4;

// Linux ignora IF3/IF4. Abrir IF4 gera flood (~ms) e NÃO destravou o painel.
// O mixer/painel Roland Capture fala por MIDI SysEx no IF2 — drenar 0x86.
static const bool kUA55StatusInterruptDrainEnabled = false;
static const bool kUA55MidiDrainEnabled = true;

// Probe isoc sync (marco 2): 8 microframes HS (~1 ms com bInterval=1).
static const uint32_t kUA55IsochProbeFrameCount = 8;

// Stream duplex async (marco 3): N transfers em voo, cada um com M microframes.
static const uint32_t kUA55IsochRingDepth = 8;
static const uint32_t kUA55IsochFramesPerTransfer = 8;
static const uint32_t kUA55IsochLogEveryCompletions = 250;

// AudioDriverKit: 4 out / 6 in, Float32 no HAL ↔ S32 24-in-32 no USB.
// Alts medidos: 1 = 44.1 kHz, 2 = 48 kHz, 3 = 96 kHz. Alt 4 é 192 kHz em 2 canais
// e não entra nesta lista. Pacotes HS (8000 microframes/s): 44.1 varia 5–6 samples,
// 48 é 6, 96 é 12. O maxPacket cobre +1 sample de folga async.
static const double kUA55SampleRate = 44100.0;
static const uint32_t kUA55SampleRateInt = 44100;

struct UA55RateConfig {
    double rate;
    uint32_t rateInt;
    uint8_t alternate;
    uint16_t playbackMaxPacket;
    uint16_t captureMaxPacket;
    uint32_t samplesPerUframeMin;
    uint32_t samplesPerUframeMax;
    uint32_t nominalFramesPerTransfer;
};

static const uint32_t kUA55RateCount = 3;
static const UA55RateConfig kUA55Rates[kUA55RateCount] = {
    { 44100.0, 44100, 1, 112, 168, 5, 6, 44 },
    { 48000.0, 48000, 2, 112, 168, 6, 6, 48 },
    { 96000.0, 96000, 3, 208, 312, 12, 12, 96 },
};

inline const UA55RateConfig* UA55RateForHz(double hz)
{
    const UA55RateConfig* match = nullptr;
    double best = 2.0;
    for (uint32_t index = 0; index < kUA55RateCount; index++) {
        double delta = hz - kUA55Rates[index].rate;
        if (delta < 0.0) {
            delta = -delta;
        }
        if (delta < best) {
            best = delta;
            match = &kUA55Rates[index];
        }
    }
    return best < 1.0 ? match : nullptr;
}
static const uint32_t kUA55HighSpeedUframesPerSecond = 8000;
static const uint32_t kUA55OutputChannels = 4;
static const uint32_t kUA55InputChannels = 6;
static const uint32_t kUA55BytesPerSample = 4;
static const uint32_t kUA55ZeroTimestampPeriod = 512;
// Tem de ser IGUAL ao ZTS period (modelo Apple). Ring 4096 com period 512
// causava corte periódico a ~93 ms (= 4096/44100) no áudio gravado.
static const uint32_t kUA55HalRingFrames = kUA55ZeroTimestampPeriod;
// Anel S32 USB pode ser maior (vários períodos de folga).
static const uint32_t kUA55BridgeFrames = 4096;
static const uint32_t kUA55ZtsWarmupCompletions = 8;
// Ler o bridge atrás da ponta que o HAL já escreveu (WriteEnd).
// 128 ≈ 3 ms; 256 ≈ 6 ms; 512 ≈ 12 ms. Mais folga = menos ruído, mais latência.
static const uint32_t kUA55PlaybackReadSlackFrames = 512;
static const uint32_t kUA55PlaybackResyncThreshold = 1024;
// Publicar ZTS de captura ATRÁS da ponta USB (captureWriteSample_).
// Sem isto o HAL faz BeginRead na ponta → corrida com IsochIO → clicks na voz.
static const uint32_t kUA55CapturePublishSlackFrames = 512;
