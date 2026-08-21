# K2S Aptus RX v0.5.4 — FAP-safe native receiver callback

v0.5.3 reached `APPCHK` but failed because K2S called
`subghz_protocol_decoder_base_set_decoder_callback()` directly. That symbol
exists in firmware but is deliberately disabled for external FAPs.

Momentum API 87.1 exposes the correct higher-level workaround:

```text
subghz_receiver_set_rx_callback()
```

That function is exported to FAPs. Inside firmware, `SubGhzReceiver` performs
the private callback hookup for every registered decoder and forwards a
successfully decoded frame to the application-level receiver callback.

So v0.5.4 uses:

```text
CC1101
  ↓
SubGhzWorker
  ↓
K2S pair dispatcher
  ├── K2S RCO/LEB decoder
  │
  └── SubGhzReceiver
         ↓
     protocol registry
         ↓
    native KeeLoq
         ↓
subghz_receiver_set_rx_callback()
         ↓
  protocol == KeeLoq?
         ↓
 serialize transiently in RAM
         ↓
      fixed32
         ↓
  Aptus normalization
```

The FAP no longer calls the disabled per-decoder callback API.

## Reader, not recorder

No `.sub` file is created, no storage object is opened, and there is no TX
path. Momentum's native decoder necessarily reconstructs the current 64-bit
frame transiently. K2S immediately extracts only `fixed32`, wipes the temporary
8-byte key buffer, frees the in-memory `FlipperFormat`, and allocates a clean
one for the next frame.

## Test

Build:

```bash
./fbt build APPSRC=applications_user/k2s_credential_reader_flipper
```

If `APPCHK` is clean, launch and test the Aptus transmitter.

Press OK for debug view. A successful native KeeLoq decode should increment
the `KLQ` count and show the extracted fixed32.
