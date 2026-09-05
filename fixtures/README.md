<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Signal fixtures

Each Flipper `.sub` capture has a same-basename JSON manifest containing its
capture metadata, expected decoded values, and verification status.

`BinRAW_weather.sub` is a real 433.92 MHz Oregon2 THGR122N transmission. A
second receiver independently confirmed the decoded protocol and values.
Weather telemetry is acceptable fixture data; do not add credentials or
captures from rolling-code access-control devices.

See `CONTRIBUTING.md` for the required fixture confidence ladder.
