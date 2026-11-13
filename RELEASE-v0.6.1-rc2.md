# v0.6.1-rc2

Fixes contactless event semantics.

EM and MIFARE now keep fully independent duplicate state:

- New EM -> display/update EM and emit `E`.
- Same EM again -> no new `E`.
- New MIFARE -> display/update MIFARE and emit `M`.
- Same MIFARE again -> no new `M`.
- Discovering/changing EM does not clear or re-arm MIFARE.
- Discovering/changing MIFARE does not clear or re-arm EM.
- Each technology is re-armed only by its own release/change logic.

This makes event order symmetrical:
EM -> MIFARE -> same EM is `E`, `M`, silence.
MIFARE -> EM -> same MIFARE is `M`, `E`, silence.
