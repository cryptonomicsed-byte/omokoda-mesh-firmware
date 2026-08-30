#include "NostrCryptoEngine.h"
#include "configuration.h"
#include "meshUtils.h"
#include "nostr/bip32_derivation.h"
#include "nostr/master_seed.h"
#include <Curve25519.h>

#if !(MESHTASTIC_EXCLUDE_PKI)
#if !(MESHTASTIC_EXCLUDE_XEDDSA)
#include "XEdDSA.h"
#endif
#endif

NostrCryptoEngine::NostrCryptoEngine(omokoda::MasterSeedSource *seedSource, uint32_t nodeIndex)
    : seedSource(seedSource), nodeIndex(nodeIndex)
{
}

#if !(MESHTASTIC_EXCLUDE_PKI)
#if !(MESHTASTIC_EXCLUDE_PKI_KEYGEN)

bool NostrCryptoEngine::deriveTransportKeySeed(uint8_t out[32])
{
    if (!seedSource)
        return false;

    uint8_t masterSeed[64];
    if (!seedSource->getSeed(masterSeed))
        return false;

    // m/44'/20000'/<node_index>'/3' — see NostrCryptoEngine.h class
    // comment for the full path allocation table.
    const uint32_t path[] = {44, 20000, nodeIndex, kTransportKeyPathIndex};
    omokoda::Bip32Key derived;
    bool ok = omokoda::bip32DeriveHardenedPath(masterSeed, sizeof(masterSeed), path, 4, derived);

    // Best-effort scrub of the master seed copy on the stack.
    memset(masterSeed, 0, sizeof(masterSeed));

    if (!ok)
        return false;

    memcpy(out, derived.key, 32);
    memset(derived.key, 0, sizeof(derived.key));
    memset(derived.chainCode, 0, sizeof(derived.chainCode));
    return true;
}

void NostrCryptoEngine::generateKeyPair(uint8_t *pubKey, uint8_t *privKey)
{
    uint8_t derivedSeed[32];
    if (deriveTransportKeySeed(derivedSeed)) {
        LOG_INFO("Deriving Curve25519 transport key from Nostr master seed (m/44'/20000'/%u'/%u')", nodeIndex,
                 kTransportKeyPathIndex);

        // Curve25519 clamps the low/high bits of the private scalar
        // itself inside Curve25519::eval / dh1's underlying field ops
        // (RFC 7748 decodeScalar25519), so the raw derived 32 bytes can
        // be used directly as the private scalar input, same as upstream
        // treats a caller-supplied private key in regeneratePublicKey().
        memcpy(private_key, derivedSeed, 32);
        Curve25519::eval(public_key, private_key, 0);
        if (Curve25519::isWeakPoint(public_key)) {
            // Astronomically unlikely for a real derived scalar, but
            // never silently hand out a weak key: fall back to upstream
            // random generation rather than use it.
            LOG_ERROR("Derived Curve25519 transport key is a weak point; falling back to random keypair");
            memset(derivedSeed, 0, sizeof(derivedSeed));
            CryptoEngine::generateKeyPair(pubKey, privKey);
            return;
        }

        memcpy(pubKey, public_key, sizeof(public_key));
        memcpy(privKey, private_key, sizeof(private_key));
#if !(MESHTASTIC_EXCLUDE_XEDDSA)
        XEdDSA::priv_curve_to_ed_keys(private_key, xeddsa_private_key, xeddsa_public_key);
#endif
        memset(derivedSeed, 0, sizeof(derivedSeed));
        return;
    }

    // No master seed provisioned yet (node not yet birthed into the
    // Nostr identity hierarchy). Never brick boot on a missing seed —
    // fall back to upstream's random Curve25519 keypair, matching the
    // "random-key fallback" tier main.cpp already uses elsewhere in this
    // ecosystem (see omokoda-mesh main.cpp boot path). The node can be
    // re-provisioned with a real seed and its identity re-derived later.
    LOG_WARN("No Nostr master seed provisioned; falling back to random Curve25519 keypair");
    CryptoEngine::generateKeyPair(pubKey, privKey);
}

bool NostrCryptoEngine::regeneratePublicKey(uint8_t *pubKey, uint8_t *privKey)
{
    // Deterministic identity: ignore the supplied privKey and always
    // re-derive from the master seed, rather than trusting a possibly
    // stale/foreign stored key. If no seed is available yet, fall back
    // to upstream's behavior (treat the supplied privKey as authoritative)
    // so existing stored security config still works before this node is
    // birthed.
    uint8_t derivedSeed[32];
    if (deriveTransportKeySeed(derivedSeed)) {
        generateKeyPair(pubKey, privKey);
        memset(derivedSeed, 0, sizeof(derivedSeed));
        return true;
    }
    return CryptoEngine::regeneratePublicKey(pubKey, privKey);
}

bool NostrCryptoEngine::ensurePkiKeys(meshtastic_Config_SecurityConfig &security, meshtastic_User &user)
{
    if (user.is_licensed) {
        return false;
    }

    uint8_t derivedSeed[32];
    bool haveDerivedIdentity = deriveTransportKeySeed(derivedSeed);
    memset(derivedSeed, 0, sizeof(derivedSeed));

    bool keygenSuccess = false;
    if (haveDerivedIdentity) {
        // Always (re)derive rather than trust whatever is currently
        // stored in security config — this node's identity is defined by
        // the master seed, not by NVS contents that might predate
        // provisioning or come from a factory-random fallback key.
        generateKeyPair(security.public_key.bytes, security.private_key.bytes);
        keygenSuccess = true;
    } else if (security.private_key.size == 32) {
        keygenSuccess = regeneratePublicKey(security.public_key.bytes, security.private_key.bytes);
    } else {
        LOG_INFO("Generate new PKI keys (no Nostr master seed provisioned)");
        generateKeyPair(security.public_key.bytes, security.private_key.bytes);
        keygenSuccess = true;
    }

    if (keygenSuccess) {
        security.public_key.size = 32;
        security.private_key.size = 32;
        user.public_key.size = 32;
        memcpy(user.public_key.bytes, security.public_key.bytes, 32);
    }

    return keygenSuccess;
}

#endif
#endif
