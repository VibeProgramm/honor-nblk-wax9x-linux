# Platform support checklist

Updated: 2026-11-14. Target: HONOR NBLK-WAX9X, NBLK-WAX9X-PCB/M1010,
Ryzen 5 3500U, BIOS 3.07. Installed supplemental module: **0.1.1**.

Checked items have supporting code or observed results. Unchecked items are
unfinished work or candidates to investigate, not promises of firmware support.
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
- [x] Keep all monitoring attributes read-only and retain stock `huawei-wmi`.
- [x] Serialize reads, validate responses and cache values/errors for one second.
- [x] Implement PM gating and cache invalidation on resume.
- [x] Fix permanent channel hiding after a failed initial reading in 0.1.1.
- [x] Build/install DKMS 0.1.1 for 7.3.3-1-cachyos, 7.3.4-arch1-1 and
  6.18.63-2-lts; configure boot-time module loading.
- [x] Correct installed source-directory permissions and document upgrades/removal.

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

The following inventory was read on the tested system; control writes were not
exercised by this investigation.

| Area | Observed interface or evidence | Useful next addition |
| --- | --- | --- |
| Battery charging | Stock `huawei-wmi/charge_control_thresholds` exists and is readable/writable by permission | CLI/UI using the existing interface; verify setting readback and persistence across sleep/reboot |
| Fn-lock | Stock `huawei-wmi/fn_lock_state` exists and is readable/writable by permission | Expose the existing control in the same small management tool |
| Microphone mute LED | LED-class `platform::micmute` exists | Verify behavior with actual mute events before adding any custom policy |
| CPU governor/boost | `acpi-cpufreq`, selectable standard governors, boost enabled | Optional user-space presets using existing CPU interfaces; measure effects |
| Unified telemetry | Standard battery, CPU and new EC hwmon data are available | A read-only dashboard or machine-readable status command with units, labels and source identification |
| Keyboard backlight/timeout | External Huawei-WMI has an NBLK-specific quirk; no keyboard-backlight LED/timeout attribute was observed here | Establish physical/firmware support on HONOR BIOS 3.07 and test the exact matching implementation |
| Platform performance profiles | Neither ACPI `platform_profile` attributes nor class devices were present | Identify real firmware modes before proposing a kernel profile interface |
| SMU power policy | Active limits can be read via a temporary SMU backend | First add reproducible AC/DC diagnostics; investigate optional profiles only after measuring accepted settings and restoration behavior |
| Fan speed control | Supplemental driver exposes RPM only; no PWM interface or validated NBLK setter | Audit this BIOS for a usable setter and automatic-mode restoration before any control implementation |

### Prioritized candidates

- [ ] Compare our model-specific implementation with external Huawei-WMI and
  retain demonstrably useful ideas from both; keep compatibility claims scoped
  to tested hardware. **Comparison in progress.**
- [ ] Evaluate the existing matebook-applet for charge/Fn-lock controls; add a
  focused CLI or telemetry view only for useful functionality it does not cover.
- [ ] Add reproducible SMU AC/DC measurements and evaluate power presets from data.
- [ ] Investigate the keyboard-backlight quirk on this exact HONOR firmware.
- [ ] Evaluate fan-control feasibility only after identifying its actual firmware
  contract; other models' EC offsets and setters are not compatibility evidence.

Fingerprint patches and desktop-specific fixes found during research are listed
in the [prior-art report](research/github-model-prior-art.md). Their presence
does not mean the currently installed authentication or desktop stack needs
replacement.

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
