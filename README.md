<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# ESPHome Weather Station

An ESPHome external component for receiving 433.92 MHz weather sensors on the
GeekMagic SmallTV Ultra. Milestone 1 extracts a tested Oregon2 decoder and its
reproducible fixture-based verification into a standalone public project.

The tested target is the **GeekMagic SmallTV Ultra ESP8266, 240×240 display
variant**, using its existing OOK receiver path. The component currently
publishes temperature, humidity, channel, rolling code, and battery-low state
from an Oregon Scientific THGR122N.

Future CC1101 hardware support is intentionally planned only in this public
repository; it is not implemented in Milestone 1. Multi-station UI, Solight
decoding, display integration, and CAD are also out of scope for this baseline.

## Use as an external component

```yaml
external_components:
  - source: github://hryamzik/esphome-weather-station@main
    components: [weather_station]

remote_receiver:
  id: rf_receiver
  pin: GPIO3
  tolerance: 50%
  filter: 250us
  idle: 4ms

weather_station:
  receiver_id: rf_receiver
  temperature:
    name: Outdoor Temperature
  humidity:
    name: Outdoor Humidity
  channel:
    name: Weather Station Channel
  rolling_code:
    name: Weather Station Rolling Code
  battery_low:
    name: Weather Station Battery Low
```

Receiver pins vary between device revisions; verify your board before flashing.
The complete compile fixture is in
`tests/esphome/geekmagic-smalltv-ultra.yaml`.

## Development

Python 3.11+ and a C++17 compiler are required.

```sh
make setup
make test
make esphome-config
make esphome-compile
```

`make test` compiles the production decoder natively and decodes the committed,
second-receiver-verified Flipper BinRAW capture. See
`docs/supported-protocols.md` for support confidence and `CONTRIBUTING.md` for
the fixture ladder required for new protocols.

## Attribution and licensing

Oregon2 behavior was compared with the Weather Station application from
[flipperdevices/flipperzero-good-faps at commit 55328b486971e49078a955a7c086e4463fa6843b](https://github.com/flipperdevices/flipperzero-good-faps/tree/55328b486971e49078a955a7c086e4463fa6843b/weather_station).
At extraction time this was the repository's current latest commit and the
repository published no tags. Its nominal Oregon2 timings and decoded behavior
served as a reference; this decoder was independently implemented against
published protocol behavior and the verified capture. See `NOTICE.md`.

Software is licensed under `GPL-3.0-or-later`. Future hardware design assets
under `hardware/cad/` are reserved for `CERN-OHL-S-2.0`; no CAD is included yet.
