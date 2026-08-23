# v0.7.0-rc2

UI-only fix after hardware screenshots.

## Footer labels disappearing after radio read

Cause:
`draw_radio()` switches the Canvas to `FontBigNumbers` to draw the credential.
The footer is drawn afterwards, and `elements_button_left/right()` inherits the
current Canvas font. Before a credential is read the Canvas is still using the
small font, so labels are visible; after a read they are rendered with the big
font and clip out of the small button labels.

Fix:
`draw_footer()` now explicitly sets `FontSecondary` before drawing the native
button labels.

## Footer collision

The normal contactless view no longer draws the `EM window ...` /
`MIFARE window ...` status line above/through the footer. Scan phase remains
available in the debug view.

HID button semantics are unchanged:
- HID enabled -> button says `HID OFF` (action)
- HID disabled -> button says `HID ON` (action)
