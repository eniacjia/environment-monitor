/*
  Clip-on enclosure for Arduino Uno + common 2.4-inch MCUFRIEND TFT shield
  Designed for FDM printing. Units: millimetres.

  part = "print"     base and lid arranged for printing
  part = "base"      base only
  part = "lid"       lid only, bezel face down
  part = "assembly"  assembled preview
*/

$fn = 40;
part = "print";

// ---- Fit parameters: measure your particular shield before final printing ----
case_x = 76.0;
case_y = 61.0;
wall = 2.0;
corner_r = 4.0;
floor_t = 2.0;
base_h = 15.0;
lid_h = 18.0;
overlap = 5.0;
fit = 0.30;                 // per-side sliding clearance

uno_x = 68.6;
uno_y = 53.4;
board_x0 = (case_x - uno_x) / 2;
board_y0 = (case_y - uno_y) / 2;
standoff_h = 3.0;
standoff_od = 5.5;
standoff_pin = 2.5;

// Typical visible area for a 2.4-inch 240x320 module.
screen_x = 50.0;
screen_y = 38.0;
screen_center_x = case_x / 2;
screen_center_y = case_y / 2;
screen_clearance = 0.6;

// Connector locations measured from the lower-left outside corner.
usb_center_y = board_y0 + 38.0;
power_center_y = board_y0 + 10.5;

// Standard Uno mounting holes relative to the board lower-left corner.
uno_holes = [
  [2.54, 15.24],
  [17.78, 50.80],
  [66.04, 7.62],
  [66.04, 35.56]
];

module rounded_box(x, y, z, r) {
  linear_extrude(height=z)
    hull()
      for (px=[r, x-r], py=[r, y-r])
        translate([px, py]) circle(r=r);
}

module base_shell() {
  difference() {
    union() {
      rounded_box(case_x, case_y, base_h-overlap, corner_r);
      translate([wall, wall, base_h-overlap-0.2])
        rounded_box(case_x-2*wall, case_y-2*wall, overlap+0.2, corner_r-wall);
    }

    // Main electronics cavity.
    translate([wall, wall, floor_t])
      rounded_box(case_x-2*wall, case_y-2*wall, base_h+1, corner_r-wall);

    // USB type-B opening.
    translate([-1, usb_center_y-7.0, 5.0])
      cube([wall+3, 14.0, 12.0]);

    // DC barrel connector opening.
    translate([-1, power_center_y-5.5, 5.0])
      cube([wall+3, 11.0, 11.0]);

    // Snap pockets in the stepped rim.
    for (x=[case_x*0.28, case_x*0.72]) {
      translate([x-4, wall-1, base_h-3.2]) cube([8, wall+2, 2.0]);
      translate([x-4, case_y-wall-1, base_h-3.2]) cube([8, wall+2, 2.0]);
    }

    // Bottom ventilation.
    for (x=[18:8:58])
      translate([x, 12, -1]) cube([2.2, 37, floor_t+2]);
  }

  // Push-fit board supports. Pins can be clipped off for screw mounting.
  for (h=uno_holes)
    translate([board_x0+h[0], board_y0+h[1], floor_t-0.2]) {
      cylinder(h=standoff_h+0.2, d=standoff_od);
      cylinder(h=standoff_h+2.2, d=standoff_pin);
    }
}

module lid_shell() {
  difference() {
    rounded_box(case_x, case_y, lid_h, corner_r);

    // Open lower cavity; retain a 2 mm bezel roof.
    translate([wall, wall, -1])
      rounded_box(case_x-2*wall, case_y-2*wall,
                  lid_h-wall+1, corner_r-wall);

    // Clearance over the base's stepped rim.
    translate([wall-fit, wall-fit, -1])
      rounded_box(case_x-2*(wall-fit), case_y-2*(wall-fit),
                  overlap+1, corner_r-wall+fit);

    // Touchscreen opening.
    translate([screen_center_x-screen_x/2-screen_clearance,
               screen_center_y-screen_y/2-screen_clearance, lid_h-wall-1])
      cube([screen_x+2*screen_clearance,
            screen_y+2*screen_clearance, wall+2]);

    // Sensor airflow slots beside the screen.
    for (y=[16:5:46])
      translate([case_x-9, y, lid_h-wall-1])
        cube([4.0, 2.0, wall+2]);
  }

  // Four internal snap bumps. Chamfer-like cylinders ease assembly.
  for (x=[case_x*0.28, case_x*0.72]) {
    translate([x, wall+0.3, 2.8]) rotate([90,0,0]) cylinder(h=1.2, d=2.0);
    translate([x, case_y-wall-0.3, 2.8]) rotate([-90,0,0]) cylinder(h=1.2, d=2.0);
  }

  // Bezel ribs prevent the TFT PCB from rattling without touching the glass.
  for (x=[screen_center_x-screen_x/2-2.0,
          screen_center_x+screen_x/2+2.0])
    translate([x-1, screen_center_y-screen_y/2, lid_h-4.2])
      cube([2, screen_y, 2.4]);
}

module printable_lid() {
  // Flip so the flat bezel face is on the print bed; supports normally unnecessary.
  translate([case_x, 0, lid_h]) rotate([0,180,0]) lid_shell();
}

if (part == "base") {
  base_shell();
} else if (part == "lid") {
  printable_lid();
} else if (part == "assembly") {
  color("DimGray") base_shell();
  color("SlateGray", 0.85) translate([0,0,base_h-overlap]) lid_shell();
} else {
  base_shell();
  translate([case_x+10, 0, 0]) printable_lid();
}
