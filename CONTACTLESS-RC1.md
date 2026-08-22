# K2S contactless v0.6.0-rc1

First integration candidate for EM4100 + 4-byte ISO14443-3A/MIFARE CSN reading.

Formatting contract:
- EM4100 12 34 56 78 9A -> 00123456789A
- MIFARE 12 34 56 78 -> 0012345678
- 7/10-byte NFC UIDs are intentionally ignored in this version.

Reader architecture:
- Sub-GHz: existing RCO + Aptus path
- LF: native LFRFIDWorker, standard EM4100 only
- HF: native ISO14443-3A synchronous poll in a dedicated thread
- All successful reads normalize into the common Credential model
- Existing Morse layer emits E for EM and M for MIFARE

No raw captures, sectors, keys, counters or replay material are persisted.

Host normalization tests pass. The Momentum FAP build and simultaneous
Sub-GHz/LF/NFC hardware behavior need the first on-device build/test.
