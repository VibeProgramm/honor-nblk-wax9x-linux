# Research results

This index links the public, sanitized conclusions from the investigation.
Firmware binaries, original dumps, device-specific identifiers and detailed
local execution logs are excluded from Git.

| Investigation | Public result | Status and limits |
| --- | --- | --- |
| ACPI, EC temperatures and fan | [Monitoring findings](../findings.md#monitoring) | Getter contract and seven channels verified on BIOS 3.07; four physical sensor positions unknown |
| Existing Linux battery/control support | [Findings](../findings.md#monitoring), [control inventory](../checklist.md#available-controls-and-possible-additions) | Battery/control reads verified; external-driver timeout restored with same-value write/readback; physical timing/sleep/reboot untested |
| CPU power policy and SMU | [Active limits on AC](../findings.md#active-smu-limits-on-ac) | Two independently decoded snapshots; EC inputs distinguished from active limits; DC/load/raising limits untested |
| Firmware and coreboot | [Firmware findings](../findings.md#firmware-and-coreboot) | VBIOS validated; full SPI read blocked; board port and PSB feasibility unresolved |
| Existing GitHub implementations | [Model-specific prior art](github-model-prior-art.md) | Existing NBLK code/materials found; exact HONOR BIOS 3.07 compatibility not established by third-party reports |
| Two hwmon implementations | [Comparison](hwmon-comparison.md), [external deployment](../install.md#external-huawei-wmi-deployment) | Both now loaded on CachyOS; six matching temperatures/RPM, duplicate channels and five unsupported external temperatures documented |
| External driver and kernel updates | [Deployment and rollback](../install.md#external-huawei-wmi-deployment) | Unmodified root-source master pinned as DKMS `master.1ae8fb55`; three kernels built/installed, automatic LTS rebuild tested; runtime on one kernel |
| Fan-control feasibility | [Firmware findings and implementation path](fan-control.md) | Tachometer getter confirmed; no usable setter/automatic-restoration contract identified |
| Linux standards and submission | [Upstream review](../upstream.md) | Code review/fix/review completed; final in-tree series and maintainer decisions remain |

The [checklist](../checklist.md) separates completed work, remaining verification,
possible control features and prerequisites for upstream/coreboot work.
