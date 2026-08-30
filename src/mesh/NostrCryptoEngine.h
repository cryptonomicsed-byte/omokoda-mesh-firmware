#pragma once
#include "CryptoEngine.h"

// NostrCryptoEngine — a CryptoEngine subclass whose key material is
// rooted in the same NIP-06/BIP-32-derived secp256k1 master key the rest
// of the omokoda-mesh ecosystem uses, instead of Meshtastic's own
// randomly-generated Curve25519 keypair. See
// docs/OMOKODA_FORK_PLAN.md "What we change" for why this is the one
// required seam change in this fork.
//
// Curve mismatch: Meshtastic's CryptoEngine does X25519 ECDH + XEdDSA
// signing over Curve25519 — a different curve family from secp256k1.
// omokoda-mesh already solved this exact multi-curve problem for its
// Reticulum bridge (see ~/omokoda-mesh/include/reticulum/reticulum_bridge.h,
// deriveRnsIdentity()): rather than reusing the secp256k1 key bit-for-bit
// (which doesn't even make sense across curve families), it derives a
// SEPARATE BIP-32 hardened child from the same master seed and feeds
// THAT into the other curve's key material. This class follows the same
// pattern.
//
// Path allocation at m/44'/20000'/<node_index>'/N' (docs/EVENT_KINDS.md
// "Key derivation" is the canonical registry of N; recorded here too so
// this fork doesn't collide with the others):
//   N=0  Secp256k1NostrSigner   — the canonical Nostr identity (npub)
//   N=1  RNS::Identity X25519   — Reticulum bridge, omokoda-mesh
//   N=2  RNS::Identity Ed25519  — Reticulum bridge, omokoda-mesh
//   N=3  THIS CLASS'S Curve25519 transport key (Meshtastic-mesh PKI/DM
//        encryption + XEdDSA signing) — chosen because 0-2 are already
//        taken by omokoda-mesh's own derivations; using a fresh index
//        keeps this fork's transport key cryptographically independent
//        of the Reticulum bridge key even though both are Curve25519-family,
//        so compromise of one doesn't imply compromise of the other.
//
// The Nostr secp256k1 key (N=0) remains the ecosystem's sole root of
// trust / canonical public identity. This class's derived Curve25519 key
// is purely transport-layer for Meshtastic-mesh compatibility, exactly
// analogous to RNS::Identity — not a second root of trust, and not
// itself published as the node's "identity" anywhere outside this mesh's
// wire format.
namespace omokoda {
class MasterSeedSource;
}

class NostrCryptoEngine : public CryptoEngine
{
  public:
    // seedSource must outlive this engine (crypto is a process-lifetime
    // singleton in practice, same as the upstream `crypto` global).
    // nodeIndex comes from omokoda::deriveNodeIndexFromHardwareId()
    // (src/mesh/nostr/node_index.h) — see that file for why it's
    // hardware-derived rather than externally assigned.
    NostrCryptoEngine(omokoda::MasterSeedSource *seedSource, uint32_t nodeIndex);

#if !(MESHTASTIC_EXCLUDE_PKI)
#if !(MESHTASTIC_EXCLUDE_PKI_KEYGEN)
    // Overridden to DERIVE rather than randomize: pulls the 64-byte
    // master seed from seedSource, derives the BIP-32 hardened child at
    // m/44'/20000'/<nodeIndex>'/3', and uses that 32-byte result as the
    // Curve25519 private key (via Curve25519::eval, same as upstream's
    // regeneratePublicKey path) instead of calling Curve25519::dh1() on
    // fresh randomness. Falls back to the upstream random-keypair
    // behavior if no seed is provisioned yet (mirrors main.cpp's
    // existing NvsMasterSeedSource fallback tiers in omokoda-mesh: no
    // seed provisioned yet should never brick a node — a node should be
    // able to boot and be reprovisioned later, not deadlock at keygen).
    void generateKeyPair(uint8_t *pubKey, uint8_t *privKey) override;

    // Deterministic — always re-derives the same key from the master
    // seed, so this simply calls generateKeyPair() and ignores the
    // supplied privKey input (unlike upstream, which treats privKey as
    // caller-supplied). This preserves the invariant that this node's
    // identity is always the one true derivation, never an
    // externally-injected Curve25519 key.
    bool regeneratePublicKey(uint8_t *pubKey, uint8_t *privKey) override;

    // Same as upstream except it calls this class's generateKeyPair()
    // instead of CryptoEngine's when no valid stored key is present, so
    // freshly-provisioned security config gets the derived key, not a
    // random one.
    bool ensurePkiKeys(meshtastic_Config_SecurityConfig &security, meshtastic_User &user) override;
#endif
#endif

  private:
    omokoda::MasterSeedSource *seedSource;
    uint32_t nodeIndex;

    // BIP-32 path segment for this fork's Curve25519 transport key. See
    // the class comment above for why 3 (0-2 already used elsewhere in
    // the ecosystem for the same master seed).
    static constexpr uint32_t kTransportKeyPathIndex = 3;

    // Derives the 32-byte Curve25519 seed material. Returns false (and
    // leaves out untouched) if no master seed is provisioned yet.
    bool deriveTransportKeySeed(uint8_t out[32]);
};
