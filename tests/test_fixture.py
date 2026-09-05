# SPDX-License-Identifier: GPL-3.0-or-later

import json
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parent.parent


def test_verified_fixture_manifest_matches_signal():
    manifest_path = REPOSITORY / "fixtures" / "BinRAW_weather.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    signal = (manifest_path.parent / manifest["signal_file"]).read_text(encoding="utf-8")

    assert manifest["schema_version"] == 1
    assert manifest["verified_by_second_receiver"] is True
    assert manifest["captured"]["frequency_hz"] == 433_920_000
    assert manifest["captured"]["flipper_protocol"] == "BinRAW"
    assert manifest["expected"]["sensor_model"] == "THGR122N"
    assert "Frequency: 433920000" in signal
    assert "Protocol: BinRAW" in signal
    assert "Bit_RAW: 955" in signal
