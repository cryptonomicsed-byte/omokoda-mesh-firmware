#ifndef OMOKODA_SECP256K1_CONFIG_H
#define OMOKODA_SECP256K1_CONFIG_H

// Copied/adapted from ~/omokoda-mesh (lib/crypto_secp256k1/libsecp256k1-config.h).
// This fork only needs base secp256k1 (secp256k1_ec_seckey_tweak_add,
// used by src/mesh/nostr/bip32_derivation.cpp for BIP-32 child key
// derivation) — NOT Schnorr/extrakeys, since this repo doesn't do Nostr
// event signing itself, only re-derives a Curve25519 transport key from
// the same master seed. Kept as its own file (not shared verbatim with
// omokoda-mesh's copy) since the module set differs.
//
// Must be the ONE libsecp256k1-config.h visible on the build's include
// path. Do NOT add vendor/secp256k1-embedded/src/ (the wrapper's own src
// dir, which has a competing copy of this filename) to any include path.

// Field/scalar backend + build settings, same choices the vendored
// wrapper uses (portable, no external asm/GMP — required for ESP32/Xtensa
// and other embedded targets alike).
#define USE_NUM_NONE 1
#define USE_FIELD_INV_BUILTIN 1
#define USE_SCALAR_INV_BUILTIN 1
#define USE_FIELD_10X26 1
#define USE_SCALAR_8X32 1

#define USE_ECMULT_STATIC_PRECOMPUTATION 1
#define ECMULT_GEN_PREC_BITS 4
#define ECMULT_WINDOW_SIZE 4

#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

#endif // OMOKODA_SECP256K1_CONFIG_H
