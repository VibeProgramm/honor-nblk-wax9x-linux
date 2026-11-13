# Fan-control feasibility on BIOS 3.07

Status on 2026-11-14: **RPM monitoring works; a usable fan-control command for
this firmware has not been identified.** No fan-control setting was changed.

## What the available firmware establishes

The local DSDT and ten SSDTs were searched, and the relevant method bodies,
field references and WMI dispatch branches were inspected:

- `GFNS` reads the first tachometer from FRHI/FRLO and the second firmware channel
  from F2HI/F2LO. These fields are only declared/read in the available AML; no
  RPM-target or duty-setting operation was identified from their references.
- No `SFND` method was found in these tables. The WMAA sensor branch exposes
  `GFNS` as command 0x0802 but has no identified fan-set dispatch path.
- No standard ACPI fan-control path using `_FSL` or a PNP0C0B fan device was
  identified in the available tables.
- `GFNM`/`SFNM` exist, but merely pass input to `HSMI` and return `DAOU`. Their
  names do not establish a supported fan-mode protocol; the SMI implementation
  is outside the disassembled AML.
- `STMT` writes upper/lower temperature fields and clears associated flags. This
  does not establish a configurable fan curve, RPM target or duty control.
- `PWMN` appears as a GNVS field declaration without other AML references. Its
  name alone is not evidence of an available PWM control interface.

These are bounded source-analysis findings. An undocumented EC/SMI interface may
exist; its behavior and automatic-mode restoration have not been established.
The full SPI firmware read is currently blocked as described in the
[firmware findings](../findings.md#firmware-and-coreboot).

## Why another model's controller is not immediately reusable

The [KLVG-XX fan-control research](https://github.com/duny31030/matebook-fanctld/blob/149543dc5ece575e4360202526e19a256a2d0752/docs/ec-reverse-engineering.md)
describes a real `SFND` manual/automatic interface on that model. Our BIOS does
not expose that method in the available tables. Other projects target different
EC layouts and CPU generations. Neither their register addresses nor their
command bytes establish a writable NBLK interface.

The external Huawei-WMI **hwmon** implementation compared with ours also only
reads fan RPM; it does not supply the missing fan-speed setter for this task.

## Required implementation path

1. **Identify the actual command contract.** Investigate matching vendor firmware
   or the official platform software/driver for this exact model. If it changes
   fan behavior, trace that supported operation and determine whether it selects
   a profile, requests RPM, or sets duty. Identify the EC/controller and SMI
   command semantics if a direct ACPI method is unavailable. Do not infer them
   from similarly named methods or another board's offsets.
2. **Establish control and recovery semantics.** Determine valid ranges/units,
   mode readback, automatic-mode restoration, firmware thermal overrides, and
   behavior across suspend/reboot. A successful command return alone is not
   proof that the requested setting was applied.
3. **Use the matching Linux interface.** Expose only capabilities that the
   firmware actually provides:
   - Duty control: `pwm1` (0..255) and supported `pwm1_enable` modes.
   - Genuine closed-loop RPM control: `fan1_target`, measured in RPM.
   - Firmware performance/thermal profiles only: investigate a suitable
     platform-profile implementation instead of inventing arbitrary PWM control.
4. **Keep firmware automatic control as the normal starting state.** Coordinate
   any manual operation with the driver's locking/PM lifecycle and define a
   tested recovery strategy for errors, service failure and module removal.
5. **Add user-space curves only after the kernel/control contract is proven.**
   Use measured temperature/RPM, bounded requests and hysteresis. A GUI or daemon
   cannot provide the missing hardware interface by itself.
6. **Verify and review the resulting change.** Check accepted values against
   actual RPM, automatic restoration, concurrent monitoring, relevant thermal
   behavior and sleep/boot transitions. Then repeat the review/fix/review cycle.

The [standard hwmon ABI](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/Documentation/ABI/testing/sysfs-class-hwmon)
defines `pwm1_enable=0` as no speed control/full speed, `1` as manual PWM and
`2+` as automatic control with driver-specific details. `fan1_target` requires
actual closed-loop hardware support. `fan1_enable` controls sensor availability;
it is not a generic command to turn the physical fan off.

Until the first two steps are resolved, adding writable sysfs attributes would
advertise a capability that this project cannot correctly implement.
