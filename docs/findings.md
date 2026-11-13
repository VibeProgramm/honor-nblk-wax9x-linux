# Verified findings and remaining questions

## Platform and evidence

HONOR NBLK-WAX9X, NBLK-WAX9X-PCB M1010, BIOS 3.07 (2021-09-14),
Ryzen 5 3500U / Picasso. Local investigation: 2026-11-14 (MSK).
Raw firmware, keys, per-device identifiers and detailed system logs are excluded
from this public repository.

The original DSDT and ten SSDT files match the running firmware. All eleven
tables were disassembled in individual iASL processes with the other ten as
external symbol inputs. Some references remain unresolved: a USB-C method and
optional AMD graphics/SATA/library paths. An unresolved declaration is not by
itself evidence that Linux executes a failing path.

## Monitoring

| ACPI getter | WMI command | Response | Verified use |
| --- | --- | --- | --- |
| `GTMP` | `0x0202` | status byte 0, temperature byte 2 | Six EC temperatures |
| `GFNS` | `0x0802` | status byte 0, little-endian RPM bytes 1–2 | First fan tachometer |
| `GCVA` | `0x0902` | Battery electrical telemetry | Already available through standard battery support |

The eight-byte input buffer carries the selector in byte 2. Verified temperature
selectors are `00`, `05`, `07`, `08`, `0b`, `0e`: CTMP, TSLO, TNTC, CNTC, DNTC,
BTMP respectively. Physical placement of the four middle sensors is unknown.
Fan selector 1 returned zero; a second physical fan was not established and
is not exported by this model-specific module.

Battery `current_now` and `voltage_now` work on battery power and while charging
from AC. The standard BAT1 hwmon device also exposes current and voltage.
AC connection alone does not imply zero current: charging state matters.

The firmware's WMAA dispatcher executes every second invocation, using a global
`WMIC` counter. The new module invokes the audited GTMP/GFNS getters directly,
so its regular polling does not consume or interleave WMI dispatcher calls.

### Checks completed

- Module loaded and registered six temperatures and one RPM channel.
- Readings matched independent reads through the stock WMI debugfs transport.
- 896 concurrent reads of the new sensor attributes passed per test run.
- Existing Fn-lock and charge-threshold reads passed before/after the tests.
- Unload/reload and loading the installed DKMS build passed.
- DKMS builds/installations passed for `7.3.3-1-cachyos`, `7.3.4-arch1-1`,
  and `6.18.63-2-lts`. Runtime testing used `7.3.3-1-cachyos` only.
- Kernel checkpatch `--strict`: no errors, warnings or checks.

Example readings: CPU EC 54 °C, TSLO 45 °C, TNTC 50 °C, CNTC 54 °C,
DNTC 49 °C, battery 37 °C, fan 3511 RPM. These are observations, not limits.

### Separate stock-driver concurrency finding

Concurrent reads of `huawei-wmi/charge_control_thresholds` returned `ENODEV`:
one failure in 256 reads with the new module loaded, and one in 256 reads
after unloading it. The serial baseline had no failures. The new sensor
attributes passed concurrent reads.

Reproducer (only reads stock getter attributes):

```sh
python3 tools/check-stock-wmi.py --workers 1
python3 tools/check-stock-wmi.py --workers 8
```

This is a timing-dependent finding, so an individual run may pass. The global
two-call firmware protocol and the stock driver's per-invocation lock make
interleaving a concrete candidate cause. It is not claimed fixed here.
An upstream fix should first reproduce it against the exact target kernel,
serialize complete logical commands, and cover debugfs/other call paths.

## CPU performance

The firmware advertises legacy P-states 2100/1700/1400 MHz. `acpi-cpufreq` is
active and boost is enabled. Those three entries do not describe a Turbo ceiling.
There is no `_CPC` in the supplied tables; adding a name cannot implement CPPC.

EC event handlers transfer power and temperature policy through AMD ALIB.
Observed EC policy fields and actual active SMU limits must be distinguished.
Detailed local power evidence lives in `analysis/power/` and is not published
as raw logs.

## Firmware and coreboot

The Picasso SoC is supported in coreboot, but no upstream HONOR NBLK-WAX9X
board port was found. ACPI contributes OS-facing resource/EC/power descriptions,
not early memory initialization or complete EC firmware.

The VFCT VBIOS image was extracted and validated locally: 54,272 bytes,
`55 AA` ROM signature, PCIR/ATOM headers, length and checksum. The source VFCT
also matched the live table. This is a graphics image, not a full system BIOS.

Internal flashrom 1.8.0 could not obtain a full SPI image: it reported
`SpiAccessMacRomEn=0` and `SpiHostAccessRomEn=0`, then no flash chip. Access
protections were not modified. The exact flash/EC chip models, PSP/APCB contents
and hardware PSB policy remain unknown. This result does not prove that coreboot
is impossible or that PSB is enabled.

Further port evaluation needs a verified full SPI backup, appropriate AMD
initialization components/configuration, EC and board information, and an
independent determination of BIOS signature enforcement.

## Primary references

- [Linux Huawei WMI driver](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/huawei-wmi.c)
- [Linux hwmon API](https://docs.kernel.org/hwmon/hwmon-kernel-api.html)
- [Huawei-WMI development driver](https://github.com/aymanbagabas/Huawei-WMI)
- [AMD P-state requirements](https://docs.kernel.org/admin-guide/pm/amd-pstate.html)
- [RyzenAdj supported models](https://github.com/FlyGoat/RyzenAdj/wiki/Supported-Models)
- [coreboot AMD Family 17h boot flow](https://doc.coreboot.org/soc/amd/family17h.html)
