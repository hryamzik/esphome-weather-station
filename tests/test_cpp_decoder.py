# SPDX-License-Identifier: GPL-3.0-or-later

import os
import subprocess
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parent.parent
COMPONENT = REPOSITORY / "components" / "weather_station"


def test_production_decoder_decodes_verified_binraw_fixture(tmp_path):
    executable = tmp_path / "native_decoder_test"
    compiler = os.environ.get("CXX", "c++")
    subprocess.run(
        [
            compiler,
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{COMPONENT}",
            str(REPOSITORY / "tests" / "native_decoder_main.cpp"),
            str(COMPONENT / "oregon2_decoder.cpp"),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run(
        [str(executable), str(REPOSITORY / "fixtures" / "BinRAW_weather.sub")],
        check=True,
    )
