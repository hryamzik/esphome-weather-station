# SPDX-License-Identifier: GPL-3.0-or-later

import os
import subprocess
import tempfile
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parent.parent
COMPONENT = REPOSITORY / "components" / "weather_station_screen"
OUTPUT = REPOSITORY / "docs" / "previews"
SCENARIOS = (
    "startup",
    "day",
    "night",
    "long-age",
    "secondary-cycle-a",
    "secondary-cycle-b",
    "wifi-unavailable",
)


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as directory:
        executable = Path(directory) / "preview_renderer"
        subprocess.run(
            [
                os.environ.get("CXX", "c++"),
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                f"-I{COMPONENT}",
                str(REPOSITORY / "tools" / "preview_renderer.cpp"),
                str(COMPONENT / "display_layout.cpp"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        for scenario in SCENARIOS:
            subprocess.run(
                [str(executable), scenario, str(OUTPUT / f"{scenario}.svg")],
                check=True,
            )
            print(OUTPUT / f"{scenario}.svg")


if __name__ == "__main__":
    main()
