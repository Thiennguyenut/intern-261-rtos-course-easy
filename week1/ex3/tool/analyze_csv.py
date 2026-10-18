#!/usr/bin/env python3
"""Measure toggle intervals from a PulseView CSV; use Time column, dedup off."""

import argparse
import csv
from decimal import Decimal
from pathlib import Path


MS_PER_UNIT = {
    "seconds": Decimal("1000"),
    "milliseconds": Decimal("1"),
    "microseconds": Decimal("0.001"),
    "nanoseconds": Decimal("0.000001"),
}


def analyze(path, samples=800, period_ms=Decimal("7"), channel=None, unit=None):
    if samples < 1 or not period_ms.is_finite() or period_ms <= 0:
        raise ValueError("Samples and target period must be positive.")

    edges = []
    previous_level = None
    previous_time = None
    with Path(path).open(newline="", encoding="utf-8-sig") as source:
        lines = (line for line in source
                 if line.strip() and not line.lstrip().startswith((";", "#")))
        reader = csv.reader(lines)
        header = [name.strip() for name in next(reader, [])]
        if len(header) < 2:
            raise ValueError("CSV needs a time column and a logic column.")
        time_unit = unit or header[0].lower()
        if time_unit not in MS_PER_UNIT:
            raise ValueError("Unknown time unit; specify --unit explicitly.")
        if channel is None:
            if len(header) != 2:
                raise ValueError("Multiple channels; specify --channel D0.")
            channel_index = 1
        else:
            if channel not in header[1:]:
                raise ValueError(f"Channel {channel!r} not found in {header[1:]}.")
            channel_index = header.index(channel)

        for row in reader:
            if len(row) != len(header):
                raise ValueError(f"Unexpected column count at CSV row {reader.line_num}.")
            time_ms = Decimal(row[0].strip()) * MS_PER_UNIT[time_unit]
            level = row[channel_index].strip()
            if level not in ("0", "1"):
                raise ValueError("Logic levels must be 0 or 1.")
            if not time_ms.is_finite():
                raise ValueError("Timestamps must be finite.")
            if previous_time is not None and time_ms <= previous_time:
                raise ValueError("Timestamps must be strictly increasing.")

            # The first row is an initial level, not a captured transition.
            if previous_level is not None and level != previous_level:
                edges.append(time_ms)
            previous_level, previous_time = level, time_ms
            if len(edges) == samples + 1:
                break

    if len(edges) < samples + 1:
        raise ValueError(
            f"Need {samples + 1} transitions for {samples} intervals; "
            f"found {len(edges)}."
        )

    intervals = [end - start for start, end in zip(edges, edges[1:])]
    elapsed = edges[-1] - edges[0]
    return {
        "samples": samples,
        "channel": header[channel_index],
        "unit": time_unit,
        "start_ms": edges[0],
        "end_ms": edges[-1],
        "min_ms": min(intervals),
        "max_ms": max(intervals),
        "mean_ms": elapsed / samples,
        "jitter_us": (max(intervals) - min(intervals)) * 1000,
        "elapsed_ms": elapsed,
        "drift_ms": elapsed - samples * period_ms,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", type=Path)
    parser.add_argument("--samples", type=int, default=800)
    parser.add_argument("--period-ms", type=Decimal, default=Decimal("7"))
    parser.add_argument("--channel", help="CSV column name, e.g. D0 or logic")
    parser.add_argument("--unit", choices=MS_PER_UNIT,
                        help="Time unit, required if the CSV header is just Time")
    args = parser.parse_args()
    try:
        result = analyze(args.csv_file, args.samples, args.period_ms,
                         args.channel, args.unit)
    except (OSError, ValueError, ArithmeticError, csv.Error) as error:
        parser.exit(1, f"Error: {error}\n")

    print(f"File: {args.csv_file}")
    print(f"Channel: {result['channel']}; input unit: {result['unit']}")
    print(f"Intervals: {result['samples']} ({result['samples'] + 1} transitions)")
    print(f"Window: {result['start_ms']:.6f} .. {result['end_ms']:.6f} ms")
    print(f"Target period: {args.period_ms} ms")
    for label, key, unit in (
        ("Min", "min_ms", "ms"),
        ("Max", "max_ms", "ms"),
        ("Mean", "mean_ms", "ms"),
        ("Jitter", "jitter_us", "us"),
        ("Drift", "drift_ms", "ms"),
        ("Elapsed", "elapsed_ms", "ms"),
    ):
        print(f"{label}: {result[key]:.6f} {unit}")


if __name__ == "__main__":
    main()
