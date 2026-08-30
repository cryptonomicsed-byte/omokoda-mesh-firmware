#pragma once
#include <cstdint>
#include <cstddef>

// Copied/adapted from ~/omokoda-mesh (include/nostr/master_seed.h +
// lib/crypto_secp256k1/master_seed.cpp), not reinvented. See
// omokoda-mesh's docs/SEED_PROVISIONING.md for the full three-tier
// design this is one piece of. Uses the SAME NVS namespace/key
// ("omokoda_seed" / "seed64") as omokoda-mesh so a node's master seed
// (and therefore its Nostr identity) is shareable across both firmwares
// if a single physical chip ever needs to run either.

class Stream; // matches Arduino's global ::Stream

namespace omokoda {

class MasterSeedSource {
  public:
    virtual ~MasterSeedSource() = default;
    // Fills out[64] with the master seed. Returns false if none is
    // provisioned yet or on storage error.
    virtual bool getSeed(uint8_t out[64]) = 0;
};

// Persisted seed storage (ESP32 NVS via Arduino Preferences). Write-once:
// provisionIfAbsent() never overwrites an existing seed.
class NvsMasterSeedSource : public MasterSeedSource {
  public:
    bool getSeed(uint8_t out[64]) override;

    // Persists a seed if none exists yet. Returns false if a seed was
    // already present (does not overwrite) or on storage error.
    bool provisionIfAbsent(const uint8_t seed[64]);
};

// Tier 2 (see omokoda-mesh docs/SEED_PROVISIONING.md): one-shot seed
// injection over serial from a trusted provisioning host. Protocol:
// waits for a 4-byte magic header, then reads exactly 64 bytes within a
// timeout. Never overwrites an existing seed.
class SerialSeedProvisioner {
  public:
    static bool provisionFromStream(::Stream &stream, uint32_t timeoutMs, NvsMasterSeedSource &store);
};

} // namespace omokoda
