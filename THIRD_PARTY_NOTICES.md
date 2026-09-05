<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Third-party notices

The example and compile-test configurations request **Roboto** through
ESPHome's `gfonts://` integration. The font is not stored in this repository.

- Copyright 2011 The Roboto Project Authors
- License: SIL Open Font License 1.1 (`OFL-1.1`)
- Source and license:
  <https://github.com/google/fonts/tree/main/ofl/roboto>

ESPHome downloads the font at build time and embeds the selected glyphs; the
source and license are linked above. The display's sun, moon, cloud,
precipitation, and lightning glyphs are original geometric primitives
implemented in `display_layout.cpp`; no third-party weather icon assets are
used.
