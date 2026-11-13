#!/usr/bin/env python3
"""Reproduce concurrent reads of stock Huawei WMI getters without setting controls."""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--workers', type=int, default=8)
parser.add_argument('--iterations', type=int, default=32)
args = parser.parse_args()
if not 1 <= args.workers <= 16 or not 1 <= args.iterations <= 128:
    parser.error('workers must be 1..16 and iterations 1..128')
base = Path('/sys/devices/platform/huawei-wmi')
paths = [base / n for n in ['fn_lock_state', 'charge_control_thresholds']]
if not all(p.exists() for p in paths):
    raise SystemExit('Required stock huawei-wmi attributes are absent')


def reader(_):
    errors = []
    for _ in range(args.iterations):
        for path in paths:
            try:
                path.read_text()
            except OSError as error:
                errors.append(f'{path.name}: errno {error.errno}')
    return errors


with ThreadPoolExecutor(max_workers=args.workers) as pool:
    failures = Counter(error for batch in pool.map(reader, range(args.workers))
                       for error in batch)
print(json.dumps({
    'new_hwmon_loaded': Path('/sys/module/honor_acpi_hwmon').exists(),
    'reads': args.workers * args.iterations * len(paths),
    'workers': args.workers,
    'failures': dict(failures),
}, indent=2))
sys.exit(1 if failures else 0)
