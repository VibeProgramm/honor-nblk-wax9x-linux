# Linux upstream readiness

This project is a tested external-module prototype. The following review is a
preparation record for a future kernel series, not a claim of maintainer approval.
The kernel reference is commit
[`2800868f63227c8237d70c41e70d441c919620d6`](https://github.com/torvalds/linux/tree/2800868f63227c8237d70c41e70d441c919620d6)
(2026-11-13).

## Existing upstream implementation

Linux already contains
[`drivers/hwmon/honor-fmi.c`](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/drivers/hwmon/honor-fmi.c),
which calls `\GFNS` directly and exports two fan inputs for HONOR **FMI-XX**.
Its DMI table does not include NBLK-WAX9X. It is a relevant implementation
precedent, not verified support for this laptop.

Before preparing a series, discuss the appropriate home for this support:

- Extend the existing HONOR hwmon driver with a verified model profile.
- Add monitoring to the platform driver's `huawei-wmi` implementation.
- Justify a separate driver if the existing drivers are unsuitable.

Preserve existing supported hardware and do not broaden DMI matching based only
on a similar model name. The audited NBLK getters accept the current eight-byte
request; another model's buffer layout, fan count and EC locking must be checked
independently. The stock WMI two-call concurrency finding is a separate bug and
should have a separate reproducer and patch.

## Review findings addressed in 0.1.1

The initial implementation inferred channel presence from a single probe-time
reading. A temporary read failure therefore hid a confirmed sensor until module
reload. Version 0.1.1 registers the fixed model's seven channels independently of
initial measurements. Reads still validate responses, cache errors for one
second, and retry after cache expiry.

The local DKMS source directory was also corrected from mode 0700 to 0755;
installation instructions now specify that mode explicitly and use privileged
DKMS diagnostics. The source review established the channel-recovery behavior
from control flow; no live firmware fault was injected.

## Before submission

The [hwmon submission guide](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/Documentation/hwmon/submitting-patches.rst)
and [kernel checklist](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/Documentation/process/submit-checklist.rst)
require work on the final in-tree patch, beyond validating this DKMS package:

- Choose the subsystem development tree and integration approach.
- Add or update Kconfig, Makefile and all dependencies. Add or update the driver
  RST documentation and its index entry; consider a MAINTAINERS entry.
- Describe verified models/BIOS versions, sensor labels and units, read-only
  behavior, cache/error handling and limitations. Existing standard hwmon
  attributes already have ABI documentation.
- Resolve the `update_interval` description: this module reports a 1000 ms
  software cache/retry interval, not a measured EC sampling period. Discuss
  retaining this optional attribute with explicit documentation or omitting it.
- Build the final patch on the target tree with relevant built-in/module and
  PM/SMP/SYSFS configurations; run W=1, sparse where supported, documentation
  checks and `checkpatch.pl --strict`.
- Repeat hardware load/unload and concurrent-read checks. Perform real
  suspend/resume and boot tests before describing those paths as tested.
- Separate cleanup, functional additions and independent bug fixes. Use
  `scripts/get_maintainer.pl` on the final diff and provide a self-contained
  explanation, evidence and explicit untested cases.

## Authorship and tool assistance

The current kernel's
[coding-assistant guidance](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/Documentation/process/coding-assistants.rst)
explicitly forbids AI agents from adding `Signed-off-by` tags. The human
submitter must understand and review the code, certify the DCO, and add their own
sign-off. Automated reviews here are not human `Reviewed-by` endorsements.

The [tool-generated content guidelines](https://github.com/torvalds/linux/blob/2800868f63227c8237d70c41e70d441c919620d6/Documentation/process/generated-content.rst)
also call for disclosing substantial assistance in the cover letter/changelog:
tools used, relevant prompt summary, affected code and verification performed.
This project's module, analysis, documentation and fixes were developed with
OpenCode AI assistance, including separate review/fixer agents. A future series
must describe that provenance accurately and provide a working discussion
address chosen by its human submitter.
