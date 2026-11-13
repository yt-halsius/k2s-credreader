# v0.6.1-rc1

Mini-sprint focused on feedback consistency and UI polish.

## Morse feedback

R/A/E/M use the same 1 kHz tone and the same 40/120 ms dot/dash timing as before.
A 120 ms (3-unit) inter-character silence is now appended to each completed
Morse letter.

This is most noticeable for contactless credentials: an EM read followed quickly
by a MIFARE read now sounds like two distinct letters (`E`, then `M`) rather than
one merged sequence.

## UI

The `LEFT: Radio` / `LEFT: Contactless` inline header text has been removed.
Mode switching now uses `elements_button_left()`, matching the small bottom-edge
button labels used by native Flipper/Momentum applications.
