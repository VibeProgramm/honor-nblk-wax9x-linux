#!/usr/bin/env python3
"""Check the Git index before publishing; never print matching secret values."""
from pathlib import PurePosixPath
import re
import subprocess

ALLOWED = {
    '.gitignore', 'LICENSE', 'README.md',
    'honor-acpi-hwmon/Makefile', 'honor-acpi-hwmon/dkms.conf',
    'honor-acpi-hwmon/honor_acpi_hwmon.c', 'honor-acpi-hwmon/verify_hwmon.py',
    'tools/check-public-tree.py',
}
PATTERNS = {
    'OEM product key': rb'\b(?:[A-Z0-9]{5}-){4}[A-Z0-9]{5}\b',
    'GitHub token': rb'\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,})',
    'private key': rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',
    'local home directory': rb'/ho' + rb'me/[^/\s]+/',
    'device serial assignment': rb'(?im)^\s*(?:POWER_SUPPLY_SERIAL_NUMBER|ID_SERIAL|product_serial|board_serial)\s*[:=]',
}

files = subprocess.check_output(['git', 'ls-files', '-z']).decode().split('\0')
errors = []
count = 0
for name in filter(None, files):
    path = PurePosixPath(name)
    if name not in ALLOWED and not (path.parts[0] == 'docs' and path.suffix == '.md'):
        errors.append(f'{name}: not in the public source/documentation allowlist')
        continue
    data = subprocess.check_output(['git', 'show', ':' + name])
    if b'\0' in data or len(data) > 256 * 1024:
        errors.append(f'{name}: binary or unexpectedly large file')
    for label, pattern in PATTERNS.items():
        if re.search(pattern, data):
            errors.append(f'{name}: possible {label}; value withheld')
    count += 1

if errors:
    raise SystemExit('\n'.join(errors))
print(f'PASS: {count} indexed source/documentation files; no flagged content.')
