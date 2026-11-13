# Our hwmon versus external Huawei-WMI

Compared on 2026-11-14 for HONOR NBLK-WAX9X / BIOS 3.07:

- [Our 0.1.1](https://github.com/VibeProgramm/honor-nblk-wax9x-linux/blob/cabc9dce9e217514ac2c45234f44dfa95477ceb1/honor-acpi-hwmon/honor_acpi_hwmon.c), commit `cabc9dc`.
- [External Huawei-WMI](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/huawei-wmi.c), commit `1ae8fb55`.

## Decision

**Keep the tested 0.1.1 module alongside stock huawei-wmi.** No beneficial
C-code transplant was identified for this BIOS. The useful getter commands,
selectors, decoding and firmware labels from existing work are already
represented. Existing generic monitoring predates this project: see
[Huawei-WMI's January 2024 addition](https://github.com/aymanbagabas/Huawei-WMI/commit/85f09851d0dce76f063ad7fe359be6780510b7f1).

The external project is broader in platform functionality; this conclusion is
specific to verified monitoring on this laptop. The external driver was
inspected, not installed or runtime-tested here. Stock and external Huawei-WMI
must not be confused.

## Comparison

| Area | Our 0.1.1 | External Huawei-WMI | Choice for this target |
| --- | --- | --- | --- |
| Firmware access | Direct audited GTMP/GFNS, eight-byte request | WMI dispatcher with up to two attempts and multiple response-envelope formats | Keep direct getters, avoiding this BIOS's shared invocation counter |
| Locking | Mutex coordinates our reads, cache and PM | Mutex covers one WMI invocation, not the whole retry loop | Keep our synchronization; address WMI transaction locking separately |
| Cache | Per-channel value or errno for 1000 ms | Each hwmon read sends a command | Keep bounded polling/retry traffic; no latency or energy benchmark is claimed |
| Channels | Six confirmed temperatures and one fan | Eleven temperature attributes and two fan attributes | Keep the verified model profile |
| Initial read failure | Channels remain visible and expired errors are retried | Failed selector-zero setup read skips the corresponding group until reinitialization | Keep 0.1.1 recovery behavior |
| Validation | Response type, minimum length, status and invalid-value checks | WMI envelope/status validation; no three-byte minimum in the package-buffer path | Keep the narrower getter validation; malformed external responses were not injected |
| hwmon API | Managed hwmon_ops and channel information | Manual attribute creation/removal and macros | Keep the current recommended API |
| PM | Explicit read/PM coordination and cache invalidation | No platform PM callback or software sensor cache | Keep our coordination; no runtime sleep comparison was performed |
| Platform features | Monitoring alongside existing stock controls | Also model-dependent keyboard, LED, charging and other functions | Evaluate additional features separately on this firmware |
| Compatibility | Exact vendor/product/board/BIOS; runtime evidence here | Broad discovery and model quirks | Do not replace the whole stock platform driver without relevant tests |
| Build coverage | Three installed kernels built; runtime on one | Automated kernel builds and README compatibility claims | Keep local verification; consider CI as a maintenance improvement |

Sources: [external transport](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/huawei-wmi.c#L322-L428),
[external hwmon](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/huawei-wmi.c#L1374-L1551),
[hwmon submission guide](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/Documentation/hwmon/submitting-patches.rst),
and our [verified results](../findings.md).

## Selector mapping

Compare labels/selectors rather than assuming identical `tempN` numbering:

| GTMP selector | Firmware field | Our attribute | External attribute |
| --- | --- | --- | --- |
| `00` | CTMP | temp1 | temp1 |
| `05` | TSLO | temp2 | temp3 |
| `07` | TNTC | temp3 | temp5 |
| `08` | CNTC | temp4 | temp6 |
| `0b` | DNTC | temp5 | temp7 |
| `0e` | BTMP | temp6 | temp8 |
| `01`, `06`, `0f`, `15`, `16` | Not implemented by this BIOS's GTMP | Absent | temp2, temp4, temp9, temp10, temp11 |

Unknown selectors return nonzero status and `0xff`; the external helper reports
an error. It would be incorrect to claim its extra files necessarily display
255 °C. A generic attribute count is not a count of valid hardware sensors.

Both implementations read fan selector zero. The external driver also exports
selector one; its physical presence on this laptop remains unconfirmed. Zero
RPM is valid and does not prove a fan is absent. Our `0xffff` RPM rejection is a
defensive driver policy, not a separately proven firmware error sentinel.

The external HUAWEI NBLK entry is a quirk, not a complete allowlist or validation
of HONOR BIOS 3.07. See [prior-art evidence](github-model-prior-art.md).

## Concurrency evidence

In the pinned external source, `huawei_wmi_call()` locks one invocation and
unlocks before returning to `huawei_wmi_cmd()`'s retry loop. Its
[raw debugfs call](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/huawei-wmi.c#L1613-L1653)
uses that same one-invocation helper. With the audited shared WMAA counter, a
possible sequence is A's first attempt, B executing/resetting the counter, then
A's final attempt receiving another retry status.

This is a source-level finding about the external version, not a measured
failure of that module here. The recorded ENODEV reproduction used the stock
driver. Our direct getters avoid WMAA but do not globally serialize other
ACPI/WMI clients or fix the stock issue.

A separate WMI fix must cover whole logical commands and raw debugfs with one
consistent lock, preserving raw one-call semantics. That does not make separate
user-space writes to `arg` and reads of `call` an atomic multi-process transaction.

## What to retain, reuse and defer

- Retain the exact six-plus-one profile, direct getters, honest labels, cached
  errors/recovery, managed hwmon registration and PM synchronization.
- Credit existing protocol research and evaluate existing platform controls and
  matebook-applet before duplicating their user interfaces.
- Consider automated builds inspired by the external
  [workflow](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/.github/workflows/build.yml).
  Its modules_prepare path sets KBUILD_MODPOST_WARN=1; a future project CI should
  instead use prepared headers/Module.symvers and matching toolchains, with W=1
  and unresolved-symbol failures left visible.
- Potential contributions back include verified model capabilities, resilient
  channel exposure and modern hwmon callbacks. Removing discovery checks for all
  unknown models would not be an appropriate generic fix.
- Do not broaden BIOS/model matching, add extra channels, or import platform
  setters without independent evidence for the added hardware/function.

No external C code was copied in this comparison. Future literal GPLv2-compatible
reuse must preserve relevant notices and attribution and identify its source
revision; protocol prior art is distinct from literal code authorship.

For upstream, first discuss a verified NBLK profile in the existing
[honor-fmi driver](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/drivers/hwmon/honor-fmi.c).
Preserve FMI-XX's three-byte request/two-channel contract. If maintainers prefer
huawei-wmi, evaluate direct model-gated getters or separately verified WMI
transaction locking. Neither architecture has maintainer approval yet.
