# SPDX-License-Identifier: GPL-3.0-or-later

import pytest

from tools.check_build_budget import enforce_budget, parse_usage


BUILD_OUTPUT = """
RAM:   [====      ]  48.3% (used 39592 bytes from 81920 bytes)
Flash: [====      ]  36.8% (used 384221 bytes from 1044464 bytes)
"""


def test_parse_platformio_usage():
    assert parse_usage(BUILD_OUTPUT) == {"ram": 48.3, "flash": 36.8}


def test_build_within_budget():
    assert enforce_budget(BUILD_OUTPUT)["ram"] == 48.3


@pytest.mark.parametrize(
    ("output", "message"),
    [
        (BUILD_OUTPUT.replace("48.3%", "55.1%"), "static RAM"),
        (BUILD_OUTPUT.replace("36.8%", "65.1%"), "flash"),
        ("Linking firmware", "missing PlatformIO usage"),
    ],
)
def test_budget_failures_are_explicit(output, message):
    with pytest.raises(ValueError, match=message):
        enforce_budget(output)
