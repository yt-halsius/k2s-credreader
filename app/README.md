# K2S Credential Reader v0.4.0

First cleanup/refactor after successful live RCO/LEB decoding.

## Changes

- generic `Credential` model
- Sub-GHz acquisition separated from RCO/LEB decoding
- normal user view no longer shows diagnostic counters
- OK toggles a diagnostic view
- latest credential stays on screen
- no raw RF persistence and no transmit/replay path

## Architecture

```text
CC1101 / Sub-GHz
       |
       v
subghz_reader.c
       |
       v
rco_leb_decoder.c
       |
       v
rco_leb.c
       |
       v
Credential
       |
       v
UI
```

The LEB decoder accumulates only the first 40 decoded bits (`FE + Factory24 + function8`). The final 24 rolling bits are timing-validated but never retained.

## UI

Normal view keeps the last valid result visible. Press **OK** to toggle the debug view and **Back** to exit.

## Build

```bash
./fbt build APPSRC=applications_user/k2s_credential_reader_flipper
```

## Next

Add host-side regression tests and CI for the known RCO vectors, then start the Aptus radio adapter.
