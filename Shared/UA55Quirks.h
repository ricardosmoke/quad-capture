#pragma once

// Marco 3: claim IF0/IF1 + SelectAlternateSetting(1) + IsochIO duplex async.
// Nenhuma request vendor-specific é emitida.
//
// Endpoints e maxPacket do alt 1 vieram do dump [UA55] neste Mac.
// Capture sync @ 44.1 kHz confirmado. Playback sync trava; usar async.
//
// OCTA-CAPTURE (0582:0120) é outro produto e não é reconhecida aqui.

static const bool kUA55VendorRequestsEnabled = false;
