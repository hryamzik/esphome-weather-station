<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Supported protocols

Confidence means:

- **Verified** — production decoder passes a real capture independently decoded
  by a second receiver, plus native and ESPHome compile tests.
- **Experimental** — decoder and real capture exist, but independent RF
  verification is incomplete.
- **Planned** — documented target with no implementation.

| Protocol | Sensor/model | Frequency | Data | Confidence |
| --- | --- | --- | --- | --- |
| Oregon2 | Oregon Scientific THGR122N | 433.92 MHz | Temperature, humidity, channel, rolling code, battery | Verified |
| Oregon v1 | Unspecified | 433.92 MHz | — | Planned |
| Oregon3 | Unspecified | 433.92 MHz | — | Planned |
| Solight | Unspecified | 433.92 MHz | — | Planned; out of Milestone 2 |

“Verified” applies to the committed THGR122N frame and the tested GeekMagic
SmallTV Ultra ESP8266 240×240 receiver path. It is not a claim that every
Oregon2 model or radio frontend has been validated.

Only `oregon2` is currently accepted in the ESPHome `protocols` list. Its
adapter declares temperature, humidity, channel, rolling-code, and battery
capabilities to the protocol-neutral router. Planned protocols must provide the
same normalized identity/capability boundary; they must not add
protocol-specific matching logic to the ESPHome entity layer.
