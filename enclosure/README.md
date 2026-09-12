# Clip-on enclosure

The enclosure is a two-piece parametric OpenSCAD model for a standard Arduino
Uno and common 2.4-inch MCUFRIEND TFT shield.

## Files

- `uno_24_tft_clip_case.scad`: editable source
- `uno_24_tft_base.stl`: printable base
- `uno_24_tft_lid.stl`: printable clip-on lid
- `assembly_preview.png`: rendered assembly preview

## Features

- Four snap points
- 50 x 38 mm default touchscreen opening
- USB type-B and barrel-power cutouts
- Uno locating posts
- Bottom and sensor ventilation
- Lid arranged bezel-down for support-free printing

## Printing

Recommended starting settings:

- 0.2 mm layer height
- Three perimeters
- 20% infill
- PLA or PETG
- No supports

The default snap clearance is 0.30 mm per side. Increase `fit` to 0.40-0.50 mm
if your printer produces tight parts.

## Customization

Edit the variables at the top of the SCAD file. In particular, verify
`screen_x`, `screen_y`, connector locations, total stack height, and snap
clearance against the physical assembly. Set `part` to `base`, `lid`, `print`,
or `assembly`, render with F6, and export as STL.
