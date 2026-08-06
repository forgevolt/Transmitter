# Housing

The printed enclosure, in two variants: one joystick and two.

| Path | Contents |
|---|---|
| `cad/Remote.f3z` | The Fusion source, as an archive containing both variants |
| `stl/1-joystick.stl`, `stl/2-joysticks.stl` | Print-ready meshes |
| `print-profiles/` | Bambu Studio projects, one per part and material |

## Print settings

Taken from the profiles in `print-profiles/`, all sliced for a **Bambu Lab H2D with a 0.4 mm
nozzle** - except `2-Joystick-CarbonPlatePrinting`, which is an X1 Carbon project.

| | |
|---|---|
| Layer height | 0.12 mm (0.2 mm first layer) |
| Walls | 3-4 for the shells, 1-2 for the light pipes |
| Infill | 15% gyroid, raised to 50% for the parts that carry the joysticks |
| Supports | on for every part except the carbon-plate project |
| Brim | none |

The 50% infill on the top and bottom shells of the two-joystick variant is deliberate: those are
the parts a hand loads when the sticks are pushed to their end stops.

Materials that have been printed successfully: **ABS-GF, PLA-GF, PLA-CF, PETG-CF**. The
glass- and carbon-filled variants are there for stiffness rather than appearance; plain PLA prints
fine but flexes noticeably around the joystick cut-outs.

## Opening the CAD

`Remote.f3z` is a Fusion archive rather than a single `.f3d` part - open it with *File → Open* in
Fusion, which will unpack the design and its components.

There is no STEP export here yet. If you want to modify the housing without a Fusion licence, a
STEP export is the thing to ask for; the STLs are fine for printing but carry no editable
geometry.

## About `print-profiles/`

These are Bambu Studio projects: a full copy of the mesh plus the slicer state, so each is a
couple of megabytes and specific to one printer and one filament. They are here as a reference for
anyone with the same machine. On any other printer, the STLs plus the table above carry the same
information in a form your own slicer can use.
