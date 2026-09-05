# SPDX-License-Identifier: GPL-3.0-or-later

import subprocess
import sys
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parent.parent
COMPONENTS = REPOSITORY / "components"


def _config(component_body):
    return f"""\
esphome:
  name: schema-test
esp8266:
  board: esp01_1m
external_components:
  - source:
      type: local
      path: {COMPONENTS}
logger:
  baud_rate: 0
remote_receiver:
  id: receiver
  pin: GPIO3
weather_station:
  receiver_id: receiver
{component_body}
"""


def _validate(tmp_path, component_body):
    path = tmp_path / "config.yaml"
    path.write_text(_config(component_body), encoding="utf-8")
    return subprocess.run(
        [sys.executable, "-m", "esphome", "config", str(path)],
        check=False,
        capture_output=True,
        text=True,
    )


def test_unsupported_protocol_is_rejected_cleanly(tmp_path):
    result = _validate(
        tmp_path,
        """\
  protocols: [solight]
  stations:
    - id: outside
      name: Outside
      selector:
        protocol: solight
        model: 0x1D20
        channel: 1
""",
    )

    assert result.returncode != 0
    output = result.stdout + result.stderr
    assert "solight" in output
    assert "oregon2" in output


def test_overlapping_station_selectors_are_rejected(tmp_path):
    result = _validate(
        tmp_path,
        """\
  protocols: [oregon2]
  stations:
    - id: any_code
      name: Any rolling code
      selector:
        protocol: oregon2
        model: 0x1D20
        channel: 1
    - id: exact_code
      name: Exact rolling code
      selector:
        protocol: oregon2
        model: 0x1D20
        channel: 1
        rolling_code: 0x8B
""",
    )

    assert result.returncode != 0
    output = result.stdout + result.stderr
    assert "overlap" in output
    assert "any_code" in output
    assert "exact_code" in output
