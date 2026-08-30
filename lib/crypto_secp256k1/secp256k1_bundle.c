// Copied/adapted from ~/omokoda-mesh (lib/crypto_secp256k1/secp256k1_bundle.c).
// Single-translation-unit build of libsecp256k1, sized for embedded
// targets (no external asm/GMP, static ecmult precomputation). Compiles
// the real upstream secp256k1.c directly from the pinned submodule at
// vendor/secp256k1-embedded/secp256k1/ — deliberately not the wrapper
// repo's own src/secp256k1_bundle.c (different module set). See
// libsecp256k1-config.h in this directory for the module/backend flags
// this fork actually needs (base module only, no Schnorr/extrakeys).

#define SECP256K1_BUILD

#include "libsecp256k1-config.h"
#include "../../vendor/secp256k1-embedded/secp256k1/src/secp256k1.c"
