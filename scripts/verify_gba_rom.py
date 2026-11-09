#!/usr/bin/env python3
"""Sanity-check a .gba ROM header.

Usage: verify_gba_rom.py ROM.gba [--title TITLE] [--code CODE]
Checks: size, Nintendo logo start, fixed byte 0x96, header complement checksum,
and optionally the 12-byte title / 4-byte game code.
"""
import argparse
import hashlib
import sys
from pathlib import Path

MAX_ROM = 32 * 1024 * 1024
LOGO_START = bytes.fromhex("24FFAE51699AA221")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("--title")
    ap.add_argument("--code")
    args = ap.parse_args()

    data = Path(args.rom).read_bytes()
    problems = []

    if len(data) < 0xC0:
        print(f"::error::ROM is only {len(data)} bytes; header is incomplete")
        return 1
    if len(data) > MAX_ROM:
        problems.append(f"ROM is {len(data)} bytes, larger than the 32 MiB GBA limit")
    if data[0x04:0x0C] != LOGO_START:
        problems.append("Nintendo logo bytes at 0x04 are missing or wrong (header not fixed?)")
    if data[0xB2] != 0x96:
        problems.append(f"Fixed byte at 0xB2 is {data[0xB2]:#04x}, expected 0x96")

    expected = (-(sum(data[0xA0:0xBD]) + 0x19)) & 0xFF
    if data[0xBD] != expected:
        problems.append(f"Header checksum is {data[0xBD]:#04x}, expected {expected:#04x}")

    title = data[0xA0:0xAC].rstrip(b"\0").decode("ascii", "replace")
    code = data[0xAC:0xB0].decode("ascii", "replace")
    if args.title is not None and title != args.title:
        problems.append(f"Title is '{title}', expected '{args.title}'")
    if args.code is not None and code != args.code:
        problems.append(f"Game code is '{code}', expected '{args.code}'")

    print(f"File     : {args.rom}")
    print(f"Size     : {len(data)} bytes ({len(data) / 1024:.1f} KiB)")
    print(f"Title    : {title}")
    print(f"Game code: {code}")
    print(f"SHA-256  : {hashlib.sha256(data).hexdigest()}")

    for p in problems:
        print(f"::error::{p}")
    print("ROM header OK" if not problems else f"{len(problems)} problem(s) found")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
