<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Contributing

Keep protocol decoders and `station_router.*` independent of ESPHome in
`components/weather_station/`. The ESPHome wrapper should only translate
receiver pulses, configure routing, and publish entities. Changes must retain
C++17 native buildability and carry SPDX identifiers.

## Adding a protocol

1. Add a focused decoder and reading type beside `oregon2_decoder.*`.
2. Add native tests for valid, inverted, noisy, truncated, and out-of-range
   input where applicable.
3. Normalize its output into a `DecodedReading` with protocol-neutral identity,
   capability flags, values, and last-seen behavior.
4. Add the protocol name to the compile-time schema allow-list and dispatcher.
5. Add native routing tests plus valid and invalid ESPHome schema cases.
6. Extend the ESPHome compile fixture and update the protocol matrix.

Selectors must remain deterministic. Test exact rolling-code selectors,
rolling-code wildcards, overlap rejection, ignore precedence, unknown
discovery, and primary fallback. Never resolve overlapping configured stations
by declaration order.

Do not copy protocol code from a reference project unless its provenance and
license are compatible and explicitly documented. Pin behavioral references
to immutable commits.

## Fixture confidence ladder

Each `.sub` capture must have a same-basename JSON manifest and contain no
credentials or access-control rolling codes.

1. **Captured** — real sensor capture, frequency and receiver recorded.
2. **Decoded** — expected fields recorded and asserted by the production native
   decoder test.
3. **Independently verified** — a second receiver/decoder confirms protocol and
   values; set `verified_by_second_receiver` to `true`.
4. **Hardware verified** — the target GeekMagic hardware receives the replayed
   RF frame and publishes matching ESPHome entities.

New protocol support starts as Experimental until levels 1–3 are committed.
Move it to Verified only after level 4 is documented. Synthetic fixtures are
useful for edge cases but cannot raise protocol confidence by themselves.

Run `make test`, `make esphome-config`, and, when toolchain downloads are
available, `make esphome-compile` before opening a pull request.

## Display changes

Keep formatting and geometry in the ESPHome-independent
`weather_station_screen/display_layout.*` scene builder. The firmware renderer
and `tools/preview_renderer.cpp` must consume those same commands; do not create
a separate mock layout for screenshots.

Run `make preview`, review every SVG in `docs/previews/`, then run `make test`.
Tests require committed previews to match the deterministic production layout.
The production `build_scene` path must remain allocation-free after setup.
Use fixed-capacity snapshot/scene storage, preserve explicit truncation and
overflow behavior, and extend the allocation-counting native test whenever the
render data model changes. Keep the pre-association startup path bounded and
allocation-free; it must not construct or emit the full scene until the
latched Wi-Fi gate opens.
Weather glyphs should be original primitives where practical. Document the
source, copyright, and license of any imported font, icon, or image in
`THIRD_PARTY_NOTICES.md`.
