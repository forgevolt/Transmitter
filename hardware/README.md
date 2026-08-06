# PCB

Everything needed to have the transmitter's board made, and to change it.

| Path | Contents |
|---|---|
| `easyeda/ProDoc_Remote_R2_2026-08-06.epro2` | The editable EasyEDA Pro project - schematic and layout |
| `gerber/Gerber_RemotePCB_R2_2026-08-06.zip` | Fabrication files for revision R2, zipped as a fab expects them |
| `schematic.pdf` | The schematic as a document, readable without EasyEDA |
| `bom.csv` | Bill of materials, with LCSC part numbers |

The board here is **revision R2**. The Gerber archive and the EasyEDA project carry the revision
and the export date in their filenames, so a later revision can sit beside this one rather than
silently replacing it.

## Ordering a board

Upload `gerber/Gerber_RemotePCB_R2_2026-08-06.zip` as it is - it is a standard Gerber and drill
set. The BOM lists LCSC part numbers, which suits JLCPCB assembly, but nothing in the design
depends on a particular fab.

If you revise the layout, tag this repository when you order from it. A year later, "which files
made the board in my hand" then has an answer the tree alone cannot give.

## A wiring constraint that outlives the layout

**S3 sits on GPIO 12, and no external pull-up may be fitted to it.** GPIO 12 is the MTDI strapping
pin: its level is latched at reset and selects the flash regulator voltage, high meaning 1.8 V. A
WROOM-32E has 3.3 V flash and will not boot if the pin is high at reset. The switch may only pull
the line down - open, the pin sees the chip's internal pull-down; closed, it sees ground.

Fitting a pull-up to S3 is an otherwise reasonable thing to do, and it would stop the board
booting. `docs/README.md` states the same constraint from the wiring side, for anyone building
without this PCB.
