# K2S Credential Reader

Flipper Zero / Momentum reader for normalizing several physical access-control
credential types into one common representation.

Current v0.6 functionality:

| Source | Native / decoder path | Normalized output | Audible acknowledgement |
| --- | --- | --- | --- |
| RCO/LEB radio | K2S LEB decoder | 9-digit decimal credential | `R` (`.-.`) |
| Aptus radio | Momentum native KeeLoq + K2S normalization | decimal Aptus credential | `A` (`.-`) |
| EM4100 | Momentum native LF RFID decoder | 12 uppercase hex characters | `E` (`.`) |
| MIFARE / ISO14443-A | Momentum native NFC poller, 4-byte UID | `00` + 8 uppercase hex characters | `M` (`--`) |

## Contactless mode

Contactless mode is the default.

K2S intentionally does not keep LF, NFC and Sub-GHz active at the same time.
The reader alternates between:

```text
EM / LF       1200 ms
MIFARE / NFC   500 ms
```

EM gets the first scan window. If a valid EM credential is found, K2S switches
to NFC immediately instead of waiting for the rest of the EM window.

The same credential is suppressed while it remains present. A changed
credential is accepted immediately, while repeated missed scan windows re-arm
the corresponding reader.

Examples:

```text
EM payload:      12 34 56 78 9A
K2S credential:  00123456789A

MIFARE UID:      12 34 56 78
K2S credential:  0012345678
```

Only 4-byte ISO14443-A UIDs are accepted in v0.6. Longer UIDs are intentionally
left for a later version.

## Radio mode

Press **Left** to temporarily switch from contactless mode to Sub-GHz radio
mode. Contactless readers are fully torn down before the CC1101 receiver is
started.

Radio mode supports RCO/LEB and Aptus. Press **Left** again to stop Sub-GHz and
return to contactless mode, beginning with EM scanning.

Press **OK** to toggle the debug view and **Back** to exit.

## Reader, not recorder

K2S is deliberately a reader, not a recorder or transmitter.

It does not persist raw RF frames, rolling-code material, counters, NFC card
contents, sectors, keys or replay-capable captures. Protocol-specific transient
data is kept only as long as necessary to derive the normalized credential.

## Morse acknowledgement

Successful credentials generate short 1 kHz Morse acknowledgements:

```text
RCO      R  .-.
Aptus    A  .-
MIFARE   M  --
EM       E  .
```

Dot timing is 40 ms, dash timing 120 ms and the intra-character gap is 40 ms.

## Build against Momentum

Copy `app/` into the Momentum tree as:

```text
applications_user/k2s_credential_reader_flipper/
```

Then build:

```bash
./fbt build APPSRC=applications_user/k2s_credential_reader_flipper
```

or launch directly:

```bash
./fbt launch APPSRC=applications_user/k2s_credential_reader_flipper
```

## Tests

Host-side regression tests cover:

- RCO credential conversion and known 34-bit vectors
- Aptus fixed32 -> credential conversion
- EM4100 formatting
- 4-byte MIFARE UID formatting
- invalid contactless input lengths

Run locally with:

```bash
make -C tests test
```

CI also builds the FAP against a deliberately pinned Momentum revision.


## v0.6.1 UI/feedback refinement

- Mode switching is shown with Flipper's native bottom-left button hint instead
  of inline `LEFT:` text.
- Morse letters now include the standard 3-unit inter-character silence.
  This matters especially in contactless mode where EM (`E`) can be followed
  immediately by MIFARE (`M`); without a character gap the two acknowledgements
  can perceptually run together even though the element timing is correct.
