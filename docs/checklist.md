# Platform support checklist

Updated: 2026-11-14. Target: HONOR NBLK-WAX9X, NBLK-WAX9X-PCB/M1010,
Ryzen 5 3500U, BIOS 3.07. Installed supplemental module: **0.1.1**.
Current platform driver: external Huawei-WMI **master.1ae8fb55**, deployed
separately at the owner's request; see [installation and rollback](install.md#external-huawei-wmi-deployment).

Checked items have supporting code, observed results or explicitly attributed
owner confirmation. Unchecked items are unfinished work or candidates to
investigate, not promises of firmware support.
See the [research index](research/README.md) for evidence and sources.

## Completed

### Hardware and firmware understanding

- [x] Identify the exact model, board, CPU and BIOS; restrict the driver to the
  verified DMI combination.
- [x] Match the original DSDT and ten SSDTs against the running firmware and
  disassemble all eleven tables. Record unresolved optional references.
- [x] Audit `GTMP`/`GFNS`, temperature selectors, response formats and the WMAA
  two-invocation counter.
- [x] Confirm existing battery current/voltage telemetry on battery and while
  charging from AC; reuse standard power_supply/BAT1 support.
- [x] Read active SMU limits twice on AC: STAPM/Fast/Slow 25/30/25 W, core
  temperature limit 92 °C. Independently decode the saved PM tables.
- [x] Distinguish active SMU settings from event-dependent EC policy inputs.
- [x] Extract and validate the local VFCT VBIOS image.
- [x] Establish the full SPI-read blocker and remaining coreboot prerequisites.

### Implemented and deployed

- [x] Expose six verified EC temperatures and one confirmed fan through hwmon.
- [x] Preserve firmware labels for sensors whose physical positions are unknown.
- [x] Keep all supplemental monitoring attributes read-only, initially alongside
  stock `huawei-wmi`; the later external platform-driver deployment is separate.
- [x] Serialize reads, validate responses and cache values/errors for one second.
- [x] Implement PM gating and cache invalidation on resume.
- [x] Fix permanent channel hiding after a failed initial reading in 0.1.1.
- [x] Build/install DKMS 0.1.1 for 7.3.3-1-cachyos, 7.3.4-arch1-1 and
  6.18.63-2-lts; configure boot-time module loading.
- [x] Correct installed source-directory permissions and document upgrades/removal.

### Existing controls and owner-confirmed functionality

- [x] Recheck `/sys/devices/platform/huawei-wmi`: `charge_control_thresholds`
  and `fn_lock_state` remain readable after switching to external Huawei-WMI.
  `kbdlight_timeout` is now present; `kbdlight` brightness remains absent.
- [x] Confirm that BAT1 already exposes standard
  `charge_control_start_threshold` and `charge_control_end_threshold` attributes;
  adding these interfaces is not remaining work.
- [x] Record the owner's confirmation that the keyboard-backlight timeout works
  on this laptop, historically through
  `/sys/devices/platform/huawei-wmi/kbdlight_timeout`. A read command for this
  exact attribute remains in shell history; the package log also records
  installation and removal of `huawei-wmi-dkms-git` on 2026-06-23. These findings
  corroborate the owner's recollection, but do not retain the old read result
  or constitute an independent physical timing test here. A later same-value
  sysfs set/readback test is recorded below.
- [x] Distinguish that historical external-driver interface from the in-tree
  driver used before this deployment. The previously loaded module matched
  the installed CachyOS module;
  none of the three installed in-tree Huawei modules contains the
  `kbdlight_timeout` attribute name. A filename search across the accessible
  pre-deployment sysfs tree found no keyboard-backlight timeout attribute.
- [x] Install unmodified external Huawei-WMI root-source commit `1ae8fb55` as
  DKMS package `master.1ae8fb55`, not an old `dkms/` snapshot; verify the source
  hash, loaded module identity and archived stock-module hashes.
- [x] Build/install for all three installed kernel packages, exercise the Arch
  kernel/header-update hook and verify a clean automatic LTS rebuild/reinstall.
  Match target compiler families and enable DKMS autoinstall.
- [x] Restore `kbdlight_timeout`; read 60 seconds, write the same value and
  verify readback. Preserve charge thresholds/Fn-lock, pass 64 serial control
  reads and repeat the supplemental module's 896-read integration test.

### Verification and publication

- [x] Reload the installed module on 7.3.3-1-cachyos and compare all seven
  channels with the stock WMI getter transport.
- [x] Pass 896 concurrent sensor reads per integration run; preserve serial
  reads of existing Huawei controls.
- [x] Reproduce the separate stock WMI concurrency failure both with and without
  the supplemental driver.
- [x] Pass strict kernel checkpatch and W=1 builds, recording the compiler
  patch-version mismatch on the running CachyOS kernel.
- [x] Run independent review, fix confirmed defects, and repeat review with fresh
  reviewers: no outstanding confirmed code/spec findings at the reviewed revision.
- [x] Publish source and sanitized findings; exclude firmware, keys, raw hardware
  logs and generated local evidence. Check the staged public tree before pushing.
- [x] Enable GitHub secret scanning/push protection and use the project's public
  noreply Git identity.
- [x] Search existing GitHub implementations and record exact-model versus
  adjacent-model evidence.

## Remaining validation

- [ ] Run real suspend/resume tests, including reads during/after the transition.
- [ ] Test hibernation/restore and actual reboot/autoload behavior.
- [ ] Measure physical keyboard-backlight expiry and timeout persistence across
  sleep/reboot with the installed external revision. Input-device registration
  and same-value sysfs readback are not physical hotkey/timing tests.
- [ ] Boot and runtime-test the other two installed kernels; they are build-tested.
- [ ] Exercise a controlled transient getter failure and verify channel retention,
  cached errno and recovery after expiry; currently established by source review.
- [ ] Observe sensor behavior across longer idle/load periods and stopped-fan
  states; avoid interpreting one snapshot as a thermal or performance guarantee.
- [ ] Repeat SMU snapshots on battery and across AC/DC transitions. Determine
  which EC events actually change policy and how long settings persist.
- [ ] Characterize sustained power/temperature under a bounded workload before
  drawing conclusions about cooling headroom or increased power limits.
- [ ] Fix and independently test the stock WMI logical-command concurrency issue
  as a separate change, including its debugfs interaction.

## Available controls and possible additions

The following inventory reflects the external-driver deployment. A same-value
timeout write/readback was tested; other control-write behavior was not exercised
here. Physical timeout functionality is separately owner-confirmed. Its current
restoration must not be confused with its absence under the earlier in-tree
driver.

| Area | Observed interface or evidence | Useful next addition |
| --- | --- | --- |
| Battery charging | `huawei-wmi/charge_control_thresholds` and standard BAT1 `charge_control_start_threshold` / `charge_control_end_threshold` exist and are readable/writable by permission | Reuse the existing interfaces; verify setting readback and persistence across sleep/reboot |
| Fn-lock | `huawei-wmi/fn_lock_state` exists and is readable/writable by permission | Expose the existing control in the same small management tool |
| Microphone mute LED | External driver registers `huawei::micmute`, replacing stock `platform::micmute` | Account for the name change; verify behavior with actual mute events before adding custom policy |
| CPU governor/boost | `acpi-cpufreq`, selectable standard governors, boost enabled | Optional user-space presets using existing CPU interfaces; measure effects |
| Unified telemetry | Standard battery, CPU and new EC hwmon data are available | A read-only dashboard or machine-readable status command with units, labels and source identification |
| Keyboard backlight brightness | `GLIV` / `SLIV` implement a getter and three accepted setting values; no keyboard-backlight LED or `kbdlight` attribute was observed | Verify physical brightness/readback and expose supported levels through the LED class |
| Keyboard backlight timeout | Restored `/sys/devices/platform/huawei-wmi/kbdlight_timeout` reads 60 seconds; same-value write/readback passed | Independently measure physical timing and persistence across sleep/reboot |
| Platform performance profiles | Neither ACPI `platform_profile` attributes nor class devices were present | Identify real firmware modes before proposing a kernel profile interface |
| SMU power policy | Active limits can be read via a temporary SMU backend | First add reproducible AC/DC diagnostics; investigate optional profiles only after measuring accepted settings and restoration behavior |
| Fan speed control | Supplemental driver exposes RPM only; no PWM interface or validated NBLK setter | Audit this BIOS for a usable setter and automatic-mode restoration before any control implementation |

### Prioritized candidates

- [x] Compare our implementation with external Huawei-WMI and retain useful
  existing knowledge. [Decision](research/hwmon-comparison.md): keep verified
  0.1.1; no beneficial C-code transplant was identified for this BIOS.
- [ ] Add an automated kernel-build matrix using matching toolchains and prepared
  headers/Module.symvers, without suppressing unresolved-symbol failures.
- [ ] Evaluate the existing matebook-applet for charge/Fn-lock controls; add a
  focused CLI or telemetry view only for useful functionality it does not cover.
- [ ] Add reproducible SMU AC/DC measurements and evaluate power presets from data.
- [ ] Verify keyboard-backlight brightness and the exact matching quirk on this
  HONOR firmware; do not treat the owner's timeout confirmation as a brightness
  test.
- [x] Restore the historically used timeout sysfs interface through the
  explicitly requested external-driver deployment. Both hwmon modules were kept;
  duplicate readings and five unsupported generic external temperature channels
  are documented. The physical timing and sleep/reboot checks remain open.
- [ ] Evaluate fan-control feasibility only after identifying its actual firmware
  contract; other models' EC offsets and setters are not compatibility evidence.
- [x] Inspect the available AML for fan-control entry points and record the
  [specific blocker and implementation path](research/fan-control.md): no usable
  setter or automatic-mode restoration contract has been established.

Fingerprint patches and desktop-specific fixes found during research are listed
in the [prior-art report](research/github-model-prior-art.md). Their presence
does not mean the currently installed authentication or desktop stack needs
replacement.

### Remaining ACPI-backed feature inventory

These candidates come from the local DSDT and ten SSDTs, not feature lists for
other Huawei/HONOR models. No new sysfs ABI or hardware-write support is claimed
by listing them. The local sources are `analysis/asl/dsdt.dsl`,
`analysis/asl/ssdt1.dsl` and `analysis/asl/ssdt5.dsl`; generated firmware analysis
is intentionally excluded from Git.

#### Monitoring, thermal events and diagnostics

- [ ] Validate adapter-side power from `GPCI` selector 0, including units,
  scaling, zero/error behavior and AC/DC transitions, before exposing a power
  channel.
- [ ] Evaluate battery power from `GPCI` selector 2 against existing battery
  current/voltage; expose it only if it adds useful, validated information.
- [ ] Audit upper/lower temperature thresholds in `STMT` for selectors
  `00`, `05`, `07`, `08`, `0b`; establish safe ranges, readback and restoration
  before adding control. These are not a proven fan curve or thermal safety
  limit interface.
- [ ] Decode threshold flags and the associated EC/WMI events; evaluate standard
  hwmon/thermal notifications without interfering with firmware protection.
- [ ] Decode `GLOG` / `GSWL`: four shutdown and four wake records, their codes
  and time units; decide whether a diagnostic tool is more appropriate than a
  stable sysfs attribute.
- [ ] Validate EC version/date fields `VERM`, `VERS`, `VERT`, `YEAR`, `MONT`,
  `DAYS` before exporting useful firmware diagnostics.
- [ ] Evaluate `GVER`, `GTSI`, `GPSI` as protocol/capability metadata, accounting
  for constant responses; do not advertise their bitmaps as verified sensors.
- [ ] Investigate additional EC fields such as `STM0`..`STM2`, extra battery
  values, error, watchdog and power-state flags. Declarations alone establish
  neither physical sensors, units nor supported controls; keep raw exploration
  separate from a stable sysfs ABI.

#### AMD ALIB power-control candidates

`ALIB(0x0c)` dispatches through `A026` in SSDT1. The following paths are present
in AML and correspond to Picasso commands in the inspected RyzenAdj source;
their writable behavior and safe ranges have not been tested on this laptop.
Adding a driver must also address the underlying mailbox's unbounded wait and
coordination with firmware policy updates.

- [ ] Validate STAPM, PPT Fast and PPT Slow control (ALIB selectors `05`, `06`,
  `07`) with actual SMU readback and a tested restoration policy.
- [ ] Validate the CPU temperature-limit control (selector `03`) without
  confusing the EC `TCTL` input with the active SMU setting.
- [ ] Validate the STAPM and Slow PPT time constants (selectors `01`, `08`);
  these are not guaranteed boost durations.
- [ ] Investigate CPU/VRM TDC and EDC current limits (selectors `0b`, `0c`).
- [ ] Investigate CPU and SoC PSI0 current thresholds (selectors `0d`, `0f`).
- [ ] Investigate PROCHOT deassertion ramp (selector `09`); retain firmware
  thermal protections and establish units/accepted values first.
- [ ] Decode selector `0a`, which sends SMU command `0x27`; do not assign an
  unverified function to it.
- [ ] Audit selector `10`'s firmware capability gate before claiming a mode
  switch; the branch alone does not establish usable performance profiles.
- [ ] Add useful read-only EC policy diagnostics for `FAPO`, `SLPO`, `SUPO`,
  `SSWS`, `TCTL`, clearly distinguished from active SMU limits. Evaluate a
  platform-profile interface only after real, supported profile semantics are
  established.

#### BIOS flags and opaque firmware commands

`GSTP` / `SSTP` reference the flags below, but most meanings are inferred from
names. `SSTP` delegates application to SMI, and its WMAA write branch is empty
on this BIOS. None of the following is a confirmed writable BIOS interface.

- [ ] Establish meanings, supported values and application/persistence behavior
  for `UPEN`, `HDEN`, `PDEN`, `CAEN`, `AUEN`, `WFEN`, `BTEN`, `FPEN`, `VTEN`
  before proposing firmware controls for USB, storage, touchpad, camera, audio,
  radios, fingerprint or virtualization. These would be BIOS settings, not
  duplicates of existing runtime rfkill/device controls.
- [ ] Investigate any genuinely additional BIOS-setting operation for `SEBO`
  and `TPCM`; reuse existing EFI Secure Boot status and TPM/PPI interfaces rather
  than duplicating them or bypassing firmware authentication/protection.
- [ ] Investigate `OWUS`, whose connection to USB `_PRW` is visible in AML;
  distinguish it from existing device wakeup controls and test applicable sleep
  states before exposing another policy switch.
- [ ] Determine the meanings of `KSVA` and `OSAR` without guessing from names.
- [ ] Decode the `LPBF` switch behind `PAFS` / `PAFF` before exposing it.
- [ ] Decode the parameter behind `SPDT` / `RPDT` and its valid values/units.
- [ ] Decode `GPPT` behind `RPPT`, which clears the field after reading; do not
  expose it as a harmless repeatedly polled sensor.
- [ ] Research opaque SMI-backed entry points `GODP/SODP`, `GFNM/SFNM`,
  `GTUB/STUB`, `GLGO/SLGO`, `GLED/SLED`, `GNTB/SNTB`, `SSWL`, `GECL/SECL`,
  `GPDV/SPDV`, `GOSF/SOSF` only with a verified command contract. Several have
  no usable WMAA dispatch branch here; names alone do not establish fan, LED,
  power-unlock or other support.

Do not count existing charge thresholds, Fn-lock, mic-mute LED, seven EC hwmon
channels, battery current/voltage/cycle count, display backlight, CPU frequency/
boost controls, runtime rfkill, TPM/PPI or CPU energy accounting as missing
features. `GCPL` / `SPLV` are no-op success responses; neither a second physical
fan, extra physical temperature sensors, a usable fan setter nor CPPC support
has been established by these tables.

## Upstream preparation

- [ ] Decide with maintainers whether to extend `honor-fmi`, add hwmon to
  `huawei-wmi`, or justify another placement.
- [ ] Prepare the in-tree Kconfig/Makefile, driver documentation/index and
  appropriate dependencies; consider MAINTAINERS coverage.
- [ ] Resolve/document `update_interval` as a software cache/retry interval.
- [ ] Build/test the final series against its target kernel tree and relevant
  configuration variants, including sparse/documentation checks where applicable.
- [ ] Prepare logical patches with evidence, explicit untested cases, correct
  recipients and transparent AI-assistance disclosure.
- [ ] Obtain human review, DCO certification and the submitter's own sign-off.
- [ ] Submit and address actual maintainer feedback.

Details and pinned kernel references: [upstream readiness](upstream.md).

## Firmware/coreboot: prerequisites still missing

- [ ] Obtain a verified full SPI backup through a supported path.
- [ ] Identify the physical flash and EC chips and required board details.
- [ ] Establish PSP/APCB requirements and the actual hardware PSB/signature policy.
- [ ] Find a matching official firmware package, if available.
- [ ] Assess and implement a board port only after those prerequisites are known.

The validated VBIOS image and Picasso SoC support do not constitute a full BIOS
backup or an existing NBLK coreboot port.
