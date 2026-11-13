# HONOR NBLK-WAX9X Linux support

Read-only platform telemetry for the HONOR NBLK-WAX9X (Ryzen 5 3500U,
Picasso), board NBLK-WAX9X-PCB, BIOS 3.07.

## Current implementation

`honor-acpi-hwmon/` contains an external GPL-2.0-only kernel module exposing
six EC temperatures and the confirmed fan tachometer through standard hwmon.
It calls the existing ACPI `GTMP` and `GFNS` getters. The firmware's WMI
dispatcher forwards the corresponding commands to these same methods.

The module is restricted to the verified DMI model and BIOS version. It caches
each reading or error for one second, serializes its requests, validates firmware
responses, and invalidates the cache on resume. All seven channels remain exposed
when a reading fails; the next read after cache expiry retries the getter. Every
sensor attribute is read-only. Temperature labels retain the firmware's names
where physical sensor placement is not established.

The existing in-tree `huawei-wmi` driver provides battery charge thresholds,
Fn-lock, hotkeys and mic-mute LED support. Standard ACPI battery support already
exposes battery current and voltage, including via hwmon. The new module adds
the missing temperature/fan channels alongside those existing drivers.

This is a hardware-validated development project, not an upstream kernel
submission. A future upstream change should evaluate integrating monitoring
with the existing HONOR hwmon or Huawei platform driver and supporting other
verified firmware layouts.

See [verified findings](docs/findings.md) and [installation/removal](docs/install.md).
The [upstream-readiness review](docs/upstream.md) records the relevant existing
Linux driver, remaining integration decisions and submission requirements.

## Build and verify

Install headers for the running kernel, a matching compiler, make and DKMS.
On Arch/CachyOS the required headers must match `uname -r`.

```sh
make -C honor-acpi-hwmon
sudo insmod honor-acpi-hwmon/honor_acpi_hwmon.ko
sensors honor_acpi_hwmon-isa-0000
python3 honor-acpi-hwmon/verify_hwmon.py
```

The Makefile selects LLVM for kernels built with Clang. For another kernel:

```sh
make -C honor-acpi-hwmon KDIR=/lib/modules/KERNEL_VERSION/build
```

`verify_hwmon.py --compare-wmi` additionally compares sensor readings with the
stock driver's debugfs getter interface and requires root. It temporarily
selects only known read commands in debugfs and restores the previous selector.

## Firmware research

The local ACPI collection contains DSDT and ten SSDTs. Their analysis identified
the EC getters, WMI dispatch, AMD power-limit callbacks and a VFCT VBIOS image.
Generated analysis and firmware images are local artifacts outside Git.
The project records reproducible findings and source code rather than OEM
license data, per-device identifiers or firmware binaries.

Before each publication, stage only the intended sources and run:

```sh
python3 tools/check-public-tree.py
git diff --cached --check
git diff --cached
```

The allowlist/content check supplements manual review; it is not a guarantee
that arbitrary additions are free of private data.

## License

GPL-2.0-only; see `LICENSE`.
