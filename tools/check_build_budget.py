#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

import argparse
import re
from pathlib import Path


ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")
USAGE = re.compile(r"^(RAM|Flash):\s+\[[^\]]*\]\s+([0-9.]+)%", re.MULTILINE)


def parse_usage(output: str) -> dict[str, float]:
    clean = ANSI_ESCAPE.sub("", output)
    return {name.lower(): float(percent) for name, percent in USAGE.findall(clean)}


def enforce_budget(
    output: str, *, max_ram_percent: float = 55.0, max_flash_percent: float = 65.0
) -> dict[str, float]:
    usage = parse_usage(output)
    missing = {"ram", "flash"} - usage.keys()
    if missing:
        raise ValueError(f"missing PlatformIO usage line(s): {', '.join(sorted(missing))}")
    if usage["ram"] > max_ram_percent:
        raise ValueError(
            f"static RAM {usage['ram']:.1f}% exceeds {max_ram_percent:.1f}% budget"
        )
    if usage["flash"] > max_flash_percent:
        raise ValueError(
            f"flash {usage['flash']:.1f}% exceeds {max_flash_percent:.1f}% budget"
        )
    return usage


def main() -> int:
    parser = argparse.ArgumentParser(description="Enforce ESP8266 firmware budgets")
    parser.add_argument("build_log", type=Path)
    parser.add_argument("--max-ram", type=float, default=55.0)
    parser.add_argument("--max-flash", type=float, default=65.0)
    arguments = parser.parse_args()

    try:
        usage = enforce_budget(
            arguments.build_log.read_text(encoding="utf-8"),
            max_ram_percent=arguments.max_ram,
            max_flash_percent=arguments.max_flash,
        )
    except ValueError as error:
        parser.error(str(error))
    print(
        f"Firmware budget passed: RAM {usage['ram']:.1f}% <= {arguments.max_ram:.1f}%, "
        f"flash {usage['flash']:.1f}% <= {arguments.max_flash:.1f}%"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
