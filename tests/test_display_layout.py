# SPDX-License-Identifier: GPL-3.0-or-later

import os
import subprocess
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parent.parent
COMPONENT = REPOSITORY / "components" / "weather_station_screen"
LAYOUT = COMPONENT / "display_layout.cpp"


def _compile(output, source):
    subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{COMPONENT}",
            str(source),
            str(LAYOUT),
            "-o",
            str(output),
        ],
        check=True,
    )


def test_display_formatting_and_layout_helpers(tmp_path):
    executable = tmp_path / "display_layout_test"
    _compile(executable, REPOSITORY / "tests" / "native_display_main.cpp")
    subprocess.run([str(executable)], check=True)


def test_committed_previews_match_production_layout(tmp_path):
    executable = tmp_path / "preview_renderer"
    _compile(executable, REPOSITORY / "tools" / "preview_renderer.cpp")

    for scenario in (
        "startup",
        "day",
        "night",
        "long-age",
        "secondary-cycle-a",
        "secondary-cycle-b",
        "wifi-unavailable",
    ):
        rendered = tmp_path / f"{scenario}.svg"
        subprocess.run([str(executable), scenario, str(rendered)], check=True)
        committed = REPOSITORY / "docs" / "previews" / f"{scenario}.svg"
        assert rendered.read_text(encoding="utf-8") == committed.read_text(
            encoding="utf-8"
        )

        if scenario == "day":
            svg = rendered.read_text(encoding="utf-8")
            weather_temperature = next(
                line for line in svg.splitlines() if ">18.4°C</text>" in line
            )
            clock = next(
                line for line in svg.splitlines() if ">10:08</text>" in line
            )
            primary_values = next(
                line
                for line in svg.splitlines()
                if ">21.3°C  47%</text>" in line
            )
            assert 'font-size="24"' in weather_temperature
            assert 'font-weight="500"' in weather_temperature
            assert 'font-weight="700"' in clock
            assert 'font-weight="700"' in primary_values
