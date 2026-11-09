#!/usr/bin/env python3
"""Validate roster/roster-plan.csv and cross-check it against src/main.cpp.

Errors (exit 1): bad header, wrong row count, bad part numbers, duplicate names,
CSV/code roster size or per-part counts out of sync.
Warnings (exit 0): playable-status differences between the CSV and the code.
"""
import csv
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV_PATH = ROOT / "roster" / "roster-plan.csv"
CPP_PATH = ROOT / "src" / "main.cpp"
EXPECTED_ROWS = 50
EXPECTED_HEADER = ["part", "character", "prototype_status"]

errors, warnings = [], []


def err(msg):
    errors.append(msg)
    print(f"::error::{msg}")


def warn(msg):
    warnings.append(msg)
    print(f"::warning::{msg}")


def load_csv():
    with CSV_PATH.open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader, [])
        if header != EXPECTED_HEADER:
            err(f"CSV header is {header}, expected {EXPECTED_HEADER}")
        rows = [r for r in reader if r]
    parsed = []
    for n, r in enumerate(rows, start=2):
        if len(r) != 3:
            err(f"CSV line {n}: expected 3 columns, got {len(r)}")
            continue
        part, name, status = (c.strip() for c in r)
        if not part.isdigit() or not 1 <= int(part) <= 8:
            err(f"CSV line {n}: part '{part}' must be 1-8")
            continue
        parsed.append((int(part), name, status))
    return parsed


def load_cpp():
    text = CPP_PATH.read_text(encoding="utf-8")
    m = re.search(r"FighterInfo\s+roster\[\]\s*=\s*\{(.*?)\n\s*\};", text, re.S)
    if not m:
        err("Could not find 'FighterInfo roster[]' in src/main.cpp")
        return []
    entries = re.findall(
        r'\{\s*"([^"]+)"\s*,\s*"PART\s+(\d)"\s*,\s*[A-Za-z_0-9]+\s*,\s*(true|false)\s*\}',
        m.group(1),
    )
    return [(int(p), n, pl == "true") for n, p, pl in entries]


def main():
    for p in (CSV_PATH, CPP_PATH):
        if not p.is_file():
            err(f"Missing file: {p.relative_to(ROOT)}")
    if errors:
        return 1

    csv_rows = load_csv()
    cpp_rows = load_cpp()

    if len(csv_rows) != EXPECTED_ROWS:
        err(f"CSV has {len(csv_rows)} characters, expected {EXPECTED_ROWS}")
    if len(cpp_rows) != EXPECTED_ROWS:
        err(f"main.cpp roster has {len(cpp_rows)} entries, expected {EXPECTED_ROWS}")

    dupes = [n for n, c in Counter(n for _, n, _ in csv_rows).items() if c > 1]
    if dupes:
        err(f"Duplicate character names in CSV: {', '.join(dupes)}")

    csv_parts = Counter(p for p, _, _ in csv_rows)
    cpp_parts = Counter(p for p, _, _ in cpp_rows)
    if csv_parts != cpp_parts:
        err(f"Per-part counts differ: CSV={dict(sorted(csv_parts.items()))} "
            f"main.cpp={dict(sorted(cpp_parts.items()))}")

    csv_playable = sum(1 for _, _, s in csv_rows if s.lower().startswith("playable"))
    cpp_playable = sum(1 for _, _, pl in cpp_rows if pl)
    if csv_playable != cpp_playable:
        warn(f"Playable fighters differ: CSV marks {csv_playable}, main.cpp enables "
             f"{cpp_playable}. Update roster-plan.csv or the 'playable' flags.")

    print(f"Roster: {len(csv_rows)} CSV rows, {len(cpp_rows)} code entries, "
          f"{len(errors)} error(s), {len(warnings)} warning(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
