<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Notices and provenance

The Oregon2 decoder behavior and timing assumptions were checked against:

- Project: `flipperdevices/flipperzero-good-faps`
- Application: `weather_station`
- Commit: `55328b486971e49078a955a7c086e4463fa6843b`
- Source: <https://github.com/flipperdevices/flipperzero-good-faps/tree/55328b486971e49078a955a7c086e4463fa6843b/weather_station>
- Upstream license: GPL-3.0

That immutable commit was upstream's current latest commit when this project
was extracted; upstream had no published tags. The Flipper application is a
behavioral reference, particularly for Oregon protocol variants and nominal
500/1000 µs Oregon2 timing. This repository's decoder was independently
implemented using published protocol behavior and the verified
`fixtures/BinRAW_weather.sub` capture (TE 478 µs).

The fixture was captured in Flipper BinRAW format and independently decoded by
a second receiver as Oregon2 THGR122N.
