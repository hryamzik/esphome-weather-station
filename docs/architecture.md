<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Architecture

The component has three layers:

1. Protocol decoders turn pulse trains into protocol-specific readings.
   `oregon2_decoder.*` has no ESPHome dependency.
2. Protocol adapters normalize readings into `DecodedReading`. The normalized
   identity is protocol/model/channel/rolling code; capability flags describe
   which values are meaningful.
3. `StationRouter` applies ignores, assigns configured stations, records
   per-station state, chooses the primary, and tracks unknown discovery. It is
   host-testable C++ and exposes immutable station state for a future display.
   `WeatherStationComponent` owns ESPHome sensors and publishes routed results.
4. `weather_station_screen` reads immutable router state and converts it plus
   Home Assistant/local diagnostics into a `ScreenSnapshot`.
   `display_layout.*` turns that snapshot into device-neutral drawing commands.
   Both the ESPHome renderer and host SVG preview consume the same scene, so
   preview snapshots exercise production formatting, layout, icon, and
   secondary-cycling logic.

Routing invariants:

- Ignore selectors are evaluated first.
- At most one configured selector can match a reading.
- A selector without rolling code is a wildcard, so it overlaps every selector
  with the same protocol/model/channel.
- Unknown readings never publish into configured station entities.
- Primary fallback is selected only once, when the first configured station is
  heard.
- Unsigned millisecond subtraction preserves age/window behavior across timer
  wraparound.

Adding Solight should therefore add a decoder and adapter, extend the
compile-time protocol dispatcher/schema, and leave station/entity/display
routing unchanged.
