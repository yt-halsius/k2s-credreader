# v0.6.0

First K2S release with all four target credential inputs available in one app.

## Added

- Native Momentum EM4100 reading.
- Native Momentum ISO14443-A reading for 4-byte MIFARE UIDs.
- Serialized contactless scanner: EM first, then MIFARE.
- `Left` mode toggle between contactless and Sub-GHz radio.
- Automatic suppression of repeated credentials while present.
- Morse acknowledgement for EM (`E`) and MIFARE (`M`).

## Existing radio support

- RCO/LEB live receive and credential normalization.
- Aptus live receive through Momentum's native KeeLoq decoder.
- Morse acknowledgement for RCO (`R`) and Aptus (`A`).

## Important design constraint

K2S remains reader-only: no raw-frame persistence, replay/TX path, rolling-code
storage, NFC sector dumping or key storage.
