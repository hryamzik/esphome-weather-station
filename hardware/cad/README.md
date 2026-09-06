<!-- SPDX-License-Identifier: CERN-OHL-S-2.0 -->

# GeekMagic radio holder

The radio holder secures an SRX882S receiver inside the GeekMagic SmallTV Ultra
enclosure while keeping its antenna and wiring clear of the ESP8266 board.
Installed examples are shown in [`hardware/README.md`](../README.md).

Available formats:

- `GeekMagicRadioHolder.FCStd` — editable FreeCAD source.
- `GeekMagicRadioHolder.3mf` — ready for import into a slicer.
- `GeekMagicRadioHolder.stl` — portable mesh.

Check clearances against your own device revision before printing or soldering:
GeekMagic board and enclosure layouts may vary. Keep the antenna away from
conductive parts, insulate exposed receiver contacts, and provide strain relief
for wires so the holder does not carry solder-joint loads.

These hardware design files are licensed under `CERN-OHL-S-2.0`; see
[`LICENSE`](LICENSE). Software and documentation elsewhere in the repository
remain under `GPL-3.0-or-later`.
