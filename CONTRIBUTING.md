<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Contributing

Keep protocol decoders independent of ESPHome in `components/weather_station/`;
the ESPHome wrapper should only translate receiver pulses and publish entities.
Changes must retain C++17 native buildability and carry SPDX identifiers.

## Adding a protocol

1. Add a focused decoder and reading type beside `oregon2_decoder.*`.
2. Add native tests for valid, inverted, noisy, truncated, and out-of-range
   input where applicable.
3. Wire the decoder into `WeatherStationComponent` without changing existing
   entity behavior unexpectedly.
4. Extend the ESPHome compile fixture.
5. Update the supported-protocol matrix and describe any model limitations.

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
