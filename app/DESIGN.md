# K2S Credential Reader design

## Core rule

**Reader, not recorder.**

No raw RF frame persistence, rolling-code storage or transmit/replay path.

## Layering

```text
physical reader / radio
        |
        v
protocol decoder
        |
        v
credential normalization
        |
        v
UI / output
```

Future adapters should produce the same `Credential` structure rather than updating the GUI directly.
