#pragma once
#include <cstdint>
#include <cstddef>

// Copied/adapted from ~/omokoda-mesh (cryptonomicsed-byte/omokoda-mesh,
// include/nostr/bip32_derivation.h + lib/crypto_secp256k1/bip32_derivation.cpp)
// verbatim, not reinvented. BIP-32 hardened derivation over secp256k1,
// matching BIPON39's Bip32Mode exactly (cryptonomicsed-byte/BIPON39,
// src/derivation.rs): master key = HMAC-SHA512("Bitcoin seed", seed),
// child = (IL + parent) mod n via HMAC-SHA512(parent_chain_code, 0x00 ||
// parent_key || index_be32) for hardened indices.
//
// Only hardened derivation is implemented, matching omokoda-mesh's own
// path (m/44'/20000'/<node_index>'/N') and this fork's own transport-key
// child (see NostrCryptoEngine.h for which N).

namespace omokoda {

struct Bip32Key {
    uint8_t key[32];
    uint8_t chainCode[32];
};

// Derives the master key from a >=64-byte seed (e.g. a BIP-39 seed —
// producing that seed from a mnemonic is out of scope here, same
// boundary as in omokoda-mesh).
bool bip32MasterFromSeed(const uint8_t *seed, size_t seedLen, Bip32Key &out);

// Derives one hardened child: index is the unhardened index
// (0..2^31-1), this function ORs in the hardened bit (0x80000000) itself.
bool bip32DeriveHardenedChild(const Bip32Key &parent, uint32_t index, Bip32Key &out);

// Derives a full path of hardened indices in one call, e.g.
// {44, 20000, nodeIndex, 3} for m/44'/20000'/<nodeIndex>'/3'.
bool bip32DeriveHardenedPath(const uint8_t *seed, size_t seedLen, const uint32_t *indices, size_t indexCount, Bip32Key &out);

} // namespace omokoda
