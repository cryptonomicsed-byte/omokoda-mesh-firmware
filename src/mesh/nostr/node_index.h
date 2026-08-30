#pragma once
#include <cstdint>

// Copied/adapted from ~/omokoda-mesh (include/nostr/node_index.h +
// lib/crypto_secp256k1/node_index.cpp), not reinvented. Node-index
// assignment for BIP-32 derivation (m/44'/20000'/<node_index>'/...).
// Answers the requirement that two nodes sharing a master seed must not
// derive the same key: each needs a distinct index, without requiring an
// external registry mapping chips to indices.

namespace omokoda {

// Derives a node index from the ESP32's factory-burned eFuse MAC —
// globally unique per chip, permanent, requires no external bookkeeping.
// Not a secret on its own; only job is keeping sibling nodes on the same
// master seed from colliding on the same BIP-32 child.
//
// Returns 0 on non-ESP32 builds (host-side tests, etc).
uint32_t deriveNodeIndexFromHardwareId();

} // namespace omokoda
