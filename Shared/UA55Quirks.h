#pragma once

// Clock da QUAD-CAPTURE: request vendor 3, igual ao quirk ALSA
// (OCTA/QUAD). Sem isto o alt USB muda e o hardware continua em 44.1 kHz:
// a placa reinicia ou a captura fica em silêncio.
// Leitura: 0xC0 / wValue 0x0001 → 3 bytes LE com a taxa.
// Escrita: 0x40 / wValue 0x0008 → 0x40 + taxa em 24 bits LE.

static const bool kUA55VendorRequestsEnabled = true;
