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
