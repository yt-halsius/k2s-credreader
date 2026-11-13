# v0.7.0-rc1

## USB HID keyboard output

- HID is ON by default at application startup.
- Each accepted normalized credential is typed followed by Enter.
- Right toggles HID ON/OFF.
- The previous USB configuration is restored when HID is disabled or K2S exits.
- Output is connected to the same accepted-credential event used by display and
  Morse, so duplicate suppression happens before keyboard output.

## Footer/layout

- Bottom screen area is now reserved for native-style button flags.
- Left flag switches Contactless/Radio mode.
- Right flag displays/toggles HID ON/OFF.
- Credential and scanner/status text has been moved upward so it no longer
  collides with the button flags.

Expected keyboard output:
- RCO: 9 decimal digits + Enter
- Aptus: decimal credential + Enter
- EM: 12 uppercase hex characters + Enter
- MIFARE: 10 uppercase hex characters + Enter
