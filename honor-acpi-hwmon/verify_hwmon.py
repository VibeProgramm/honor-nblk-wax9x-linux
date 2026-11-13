#!/usr/bin/env python3
"""Hardware integration checks; --compare-wmi uses only audited getter commands."""
import argparse
import ast
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import json
from pathlib import Path
import stat
import time

SENSORS = {
    'temp1_input': (0x00, 'CPU (EC)'),
    'temp2_input': (0x05, 'TSLO (EC)'),
    'temp3_input': (0x07, 'TNTC (EC)'),
    'temp4_input': (0x08, 'CNTC (EC)'),
    'temp5_input': (0x0B, 'DNTC (EC)'),
    'temp6_input': (0x0E, 'Battery (EC)'),
    'fan1_input': (0x00, 'CPU fan (EC)'),
}


def locate():
    matches = [p for p in Path('/sys/class/hwmon').iterdir()
               if (p / 'name').read_text().strip() == 'honor_acpi_hwmon']
    assert len(matches) == 1, f'Expected one hwmon device, got {matches}'
    return matches[0]


def snapshot(device):
    values = {}
    for name in SENSORS:
        value = int((device / name).read_text())
        limit = 254000 if name.startswith('temp') else 65534
        assert 0 <= value <= limit, (name, value)
        if name.startswith('temp'):
            assert value % 1000 == 0, (name, value)
        values[name] = value
    return values


def existing_features():
    base = Path('/sys/devices/platform/huawei-wmi')
    return {name: (base / name).read_text().strip()
            for name in ['fn_lock_state', 'charge_control_thresholds']}


def compare_wmi(device):
    base = Path('/sys/kernel/debug/huawei-wmi')
    saved = (base / 'arg').read_text()
    comparisons = []
    try:
        for name, (selector, _) in SENSORS.items():
            fan = name.startswith('fan')
            command = (0x0802 if fan else 0x0202) | (selector << 16)
            data = None
            for _ in range(2):
                (base / 'arg').write_text(hex(command))
                raw = (base / 'call').read_text()
                obj = ast.literal_eval(raw.replace('{', '[').replace('}', ']'))
                data = obj[1] if isinstance(obj[0], list) else obj[4:]
                assert len(data) >= 3
                if data[0] == 0:
                    break
            assert data[0] == 0, (name, data[:3])
            reference = data[1] | (data[2] << 8) if fan else data[2] * 1000
            actual = int((device / name).read_text())
            # Independent transports can straddle EC updates and the 1 s cache.
            tolerance = 500 if fan else 5000
            assert abs(reference - actual) <= tolerance, (name, reference, actual)
            comparisons.append({'sensor': name, 'wmi': reference, 'hwmon': actual})
    finally:
        (base / 'arg').write_text(saved)
    return comparisons


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compare-wmi', action='store_true', help='Requires root/debugfs')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    device = locate()
    started = datetime.now(timezone.utc).isoformat()
    before = existing_features()
    assert set(p.name for p in device.glob('*_input')) == set(SENSORS)
    for name, (_, label) in SENSORS.items():
        assert (device / name.replace('_input', '_label')).read_text().strip() == label
        assert stat.S_IMODE((device / name).stat().st_mode) == 0o444
    assert (device / 'update_interval').read_text().strip() == '1000'
    samples = []
    for _ in range(3):
        samples.append(snapshot(device))
        time.sleep(1.05)
    comparisons = compare_wmi(device) if args.compare_wmi else []

    def read_batch(_):
        for _ in range(16):
            snapshot(device)

    with ThreadPoolExecutor(max_workers=8) as pool:
        list(pool.map(read_batch, range(8)))
    assert existing_features() == before, 'Existing controls changed during read tests'
    battery = Path('/sys/class/power_supply/BAT1')
    result = {
        'result': 'PASS', 'started': started, 'hwmon': str(device),
        'samples': samples, 'wmi_comparison': comparisons,
        'concurrent_sensor_reads': 8 * 16 * len(SENSORS),
        'existing_features': before,
        'battery': {n: (battery / n).read_text().strip()
                    for n in ['status', 'current_now', 'voltage_now']},
        'ac_online': Path('/sys/class/power_supply/ACAD/online').read_text().strip(),
    }
    text = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.write_text(text)
    print(text, end='')


if __name__ == '__main__':
    main()
