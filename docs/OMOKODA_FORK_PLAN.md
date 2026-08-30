# omokoda-mesh-firmware — fork scope & plan

Fork of `meshtastic/firmware` at `cryptonomicsed-byte/omokoda-mesh-firmware`,
tracking upstream `main` (currently at `7239fe8`). Part of the
`omokoda-mesh` ecosystem — see that repo's `docs/ARCHITECTURE.md` for the
full layering decision (Nostr core identity / Reticulum optional bridge /
Meshtastic wrapped-then-forked / Arch foundation / OSOVM tie-in).

## Why fork instead of wrap

Wrapping (treat upstream Meshtastic as an unmodified black box, only
passively sniff its broadcast frames) was tried first as a submodule
reference. Rejected because it can never let an omokoda node **originate**
a packet that real Meshtastic nodes will route — sniffing is one-way.
Forking lets us swap the identity layer at the source and still be a
first-class participant on existing Meshtastic meshes.

## What we change

**`src/mesh/CryptoEngine.{h,cpp}`** — the entire identity/crypto seam.
Confirmed it's a `virtual class CryptoEngine` doing X25519 key exchange +
XEdDSA signing over Curve25519, already isolated from routing logic. Plan:
subclass it (`NostrCryptoEngine`) so a node's `public_key`/private key pair
is the same NIP-06-derived secp256k1 key the rest of the ecosystem uses,
instead of Meshtastic's own generated Curve25519 keypair. This is the one
required change — everything else stays upstream behavior.

**`src/mesh/generated/` protobufs** — extend (not replace) with the tags
needed to carry a Nostr pubkey/signature alongside Meshtastic's native
`meshtastic_User`/`NodeInfo` fields, so a node is addressable both ways
during the transition.

## What we deliberately keep unmodified (upstream behavior, don't touch)

- Mesh routing / flooding algorithm (`src/mesh/` router, `FloodingRouter`,
  etc.) — this is the actual interop value; changing it breaks
  compatibility with the public Meshtastic mesh.
- Radio HAL / board support (`variants/`, `boards/`) — upstream already
  supports far more boards than `omokoda-mesh`'s own HAL does today; no
  reason to duplicate that work.
- Protobuf wire format for existing message types — only additive changes.

## Relationship to `omokoda-mesh` (the other, non-forked repo)

`omokoda-mesh` remains the from-scratch universal firmware (any ESP32,
`Envelope` packet format, direct Nostr bridge) for nodes that don't need
Meshtastic-mesh interop at all. `omokoda-mesh-firmware` (this fork) is for
nodes that specifically need to be first-class Meshtastic-mesh
participants while rooting identity in Nostr. Both converge on the same
Nostr-identity rule; they differ in which existing mesh (if any) a given
deployment needs to interoperate with.

## Upstream sync policy

Track upstream deliberately, not automatically:
```
git remote add upstream https://github.com/meshtastic/firmware.git
git fetch upstream
git merge upstream/main   # review before merging; CryptoEngine subclass
                           # should minimize merge conflicts by staying
                           # additive rather than editing upstream files
                           # in place wherever possible
```

## Status

Just forked (2026-08-29). No code changes yet — this doc is the scope
before the first commit. `Meshtastic-Android` was left as a vendored
reference submodule (not forked) since the identity swap is a
firmware/protocol-level change; the companion app can point at a forked
firmware's existing wire format without needing its own fork, at least
until/unless the protobuf extension requires app-side changes too.

**2026-08-29, `NostrCryptoEngine` first implementation.** Added
`src/mesh/NostrCryptoEngine.{h,cpp}`, a `CryptoEngine` subclass
(`src/mesh/CryptoEngine.h`) that overrides `generateKeyPair`,
`regeneratePublicKey`, and `ensurePkiKeys` to derive the Curve25519
transport key from the ecosystem's Nostr master seed instead of
generating a random one.

Curve mismatch (secp256k1 Nostr identity vs. Curve25519 Meshtastic PKI)
is resolved the same way `omokoda-mesh`'s Reticulum bridge resolves it
for `RNS::Identity` (`~/omokoda-mesh/lib/reticulum_bridge/reticulum_bridge.cpp`,
`deriveRnsIdentity()`): derive a dedicated BIP-32 hardened child from the
same master seed, don't reuse the secp256k1 key bit-for-bit. Path
allocation at `m/44'/20000'/<node_index>'/N'` (recorded in
`NostrCryptoEngine.h` too):
- N=0 — Nostr identity (secp256k1), `omokoda-mesh`
- N=1 — Reticulum X25519, `omokoda-mesh`
- N=2 — Reticulum Ed25519, `omokoda-mesh`
- N=3 — **this fork's** Curve25519 transport key (new)

The Nostr key (N=0) stays the ecosystem's sole root of trust; this
fork's derived key is transport-layer only, same status as
`RNS::Identity`.

Ported (cited in-file as copied/adapted, not reinvented) into
`src/mesh/nostr/`: `bip32_derivation.{h,cpp}` (BIP-32 hardened
derivation, matches BIPON39's math), `master_seed.{h,cpp}` (NVS +
serial-injection seed storage, same NVS namespace/key as
`omokoda-mesh` so a seed is portable across both firmwares), and
`node_index.{h,cpp}` (ESP32 eFuse-MAC-derived per-node index, no
external registry). Vendored `secp256k1-embedded` as a git submodule at
`vendor/secp256k1-embedded` (same upstream as `omokoda-mesh`, added to
`.gitmodules`), plus `lib/crypto_secp256k1/{libsecp256k1-config.h,
secp256k1_bundle.c}` — this fork's config only enables the base
module (no Schnorr/extrakeys; this repo doesn't sign Nostr events
itself, only derives a key). `platformio.ini`'s `[env]` build_flags gained
the two `-I` include paths for this.

Not done yet:
- Not wired into boot/`main.cpp` — `crypto` global still defaults to the
  platform engines (`ESP32CryptoEngine`, etc). Constructing a
  `NostrCryptoEngine` at boot needs a `MasterSeedSource*` and node
  index available before `crypto` is assigned, which touches the same
  boot-ordering questions `omokoda-mesh`'s own `main.cpp` wiring
  already solved (see that repo's commit `fac6729`) — deliberately left
  for a follow-up pass so this stays a small, reviewable diff.
  `SerialSeedProvisioner`/first-boot provisioning flow is ported but
  also unwired for the same reason.
- **Build-verified 2026-08-29** — `pio run -e heltec-v3` (PlatformIO
  6.1.19 under a Python 3.12 venv) completed `SUCCESS`, full
  `firmware-heltec-v3-2.8.0.92c00fc.factory.bin` produced (RAM 38.8%,
  Flash 68.4%). Zero compile errors; zero warnings on
  `NostrCryptoEngine.cpp.o`, `bip32_derivation.cpp.o`,
  `master_seed.cpp.o`, `node_index.cpp.o` specifically. The flagged
  `meshtastic/Crypto` `SHA512.h` `resetHMAC`/`finalizeHMAC` API risk did
  not materialize — `Crypto/SHA512.cpp.o` compiled clean against the
  same calls `bip32_derivation.cpp` makes.
- Protobuf extension for carrying a Nostr pubkey alongside
  `meshtastic_User`/`NodeInfo` (see "What we change" above) — not
  started.
