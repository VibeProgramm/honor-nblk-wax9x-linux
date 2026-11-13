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

The external Huawei-WMI development driver's generic eleven-temperature/two-fan
attribute set is broader than this verified model's capabilities. Its HUAWEI
NBLK quirk does not establish HONOR BIOS 3.07 hwmon compatibility. See the
[selector mapping](research/hwmon-comparison.md#selector-mapping) when comparing
channels: temperature numbering differs between implementations.

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

The DKMS builds/installations, installed-module reload, WMI comparison and
896 concurrent sensor reads were repeated successfully for version 0.1.1.
This version fixes permanent channel hiding after a failed initial reading;
see the [upstream review record](upstream.md). Actual firmware fault injection,
suspend/resume and reboot remain untested.

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
The observed EC policy inputs were `FAPO=30`, `SLPO=25`, `SUPO=25`, `SSWS=22`,
and `TCTL=0`. `_Q60` submits Fast PPT, Slow PPT and STAPM from the first three
fields; `_Q52` and `_Q53` submit STAPM from `SSWS`; `_Q62` submits the temperature
parameter from `TCTL`. These inputs alone do not establish the active limits.

### Active SMU limits on AC

Two PM-table snapshots and two RyzenAdj `--info` readings on 2026-11-14 agreed
on these SMU-reported settings. The AC adapter was connected and the battery
was charging throughout the measurement session.

| Parameter | Reported setting |
| --- | --- |
| STAPM limit | 25 W |
| PPT Fast limit | 30 W |
| PPT Slow limit | 25 W |
| Core temperature limit | 92 °C |
| TDC VDD / SoC | 35 / 10 A |
| EDC VDD / SoC | 52 / 13 A |
| STAPM time constant | 100 s |
| Slow PPT time constant | 5 s |

Thus active STAPM was 25 W despite `SSWS=22`, and the reported temperature
limit was 92 °C despite `TCTL=0`. The last policy-setting event was not observed.
The time constants are not guaranteed boost durations. These are snapshots of
configured limits, not a measurement of sustained power or enforcement under
load. DC operation, raising limits, firmware ceilings and cooling headroom
were not tested.

The initial `/dev/mem` backend failed with `EPERM` for both `mmap` and `pread`.
A temporary `ryzen_smu` module provided the supported alternative PM-table
backend without changing kernel memory-access protections. The audited
initialization/monitoring path requests versions, addresses and telemetry
transfers; no limit-setting commands were requested. The driver itself also
provides writable command interfaces and is not a read-only driver. It was
unloaded after the measurements, with no DKMS installation or autoload entry.

Verified versions:

- RyzenAdj v0.19.0, commit `5775fc3e6dbb25c7030ee2d100a1bdd6e8bf2d0a`.
- `amkillam/ryzen_smu`, interface 0.1.7, commit
  `d2983668300dd2a598e5a7dc40e71ce0678cc270`.
- SMU firmware 4.30.68.51, BIOS interface 19, PM-table version `0x1e0004`.
- Temporary module built with LLVM and tested on `7.3.3-1-cachyos`.

Both saved 1,708-byte tables were independently decoded as little-endian
float32 values and matched the CLI output. Limit offsets were `0x000`, `0x008`,
`0x010` (STAPM/Fast/Slow), `0x018`, `0x020` (TDC VDD/SoC), `0x028`, `0x034`
(EDC VDD/SoC), and `0x058` (core temperature); time constants were at `0x5e0`
and `0x5e4`. These offsets apply to the verified table version only. CLI entries
reported as `nan` were not interpreted as zero or as supported measurements.
Detailed local power evidence lives in `analysis/power/`; raw logs and binary
snapshots are excluded from publication.

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
- [Verified RyzenAdj source](https://github.com/FlyGoat/RyzenAdj/tree/5775fc3e6dbb25c7030ee2d100a1bdd6e8bf2d0a)
- [Verified ryzen_smu source](https://github.com/amkillam/ryzen_smu/tree/d2983668300dd2a598e5a7dc40e71ce0678cc270)
- [coreboot AMD Family 17h boot flow](https://doc.coreboot.org/soc/amd/family17h.html)
