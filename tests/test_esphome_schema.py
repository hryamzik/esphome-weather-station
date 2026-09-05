# SPDX-License-Identifier: GPL-3.0-or-later

import subprocess
import sys
from pathlib import Path

import pytest


REPOSITORY = Path(__file__).resolve().parent.parent
COMPONENTS = REPOSITORY / "components"
SCREEN_FIXTURE = REPOSITORY / "tests" / "esphome" / "geekmagic-smalltv-ultra.yaml"


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


def _validate_screen_option(tmp_path, old_option, new_option):
    config = SCREEN_FIXTURE.read_text(encoding="utf-8")
    config = config.replace("path: ../../components", f"path: {COMPONENTS}")
    config = config.replace(old_option, new_option)
    path = tmp_path / "screen-config.yaml"
    path.write_text(config, encoding="utf-8")
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


@pytest.mark.parametrize(
    ("replacement", "message"),
    (
        ("  stale_after: 0s", "greater than zero"),
        ("  stale_after: 4294967296s", "too large"),
    ),
)
def test_stale_after_rejects_non_positive_or_unsafe_values(
    tmp_path, replacement, message
):
    result = _validate_screen_option(
        tmp_path, "  stale_after: 5min", replacement
    )

    assert result.returncode != 0
    assert message in result.stdout + result.stderr


def test_removed_show_seconds_option_is_rejected(tmp_path):
    result = _validate_screen_option(
        tmp_path,
        "  show_am_pm: true",
        "  show_am_pm: true\n  show_seconds: true",
    )

    assert result.returncode != 0
    assert "show_seconds" in result.stdout + result.stderr
