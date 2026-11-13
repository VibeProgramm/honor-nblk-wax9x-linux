# Existing GitHub work for NBLK-WAX9X

Search date: **2026-11-14**. Target: **HONOR NBLK-WAX9X**, board
**NBLK-WAX9X-PCB/M1010**, Ryzen 5 3500U, BIOS **3.07**. This repository and copies
of its work were excluded as prior art.

## Main result

Existing Linux implementations and reverse-engineering materials for **NBLK-WAX9X**
were found. Most importantly, the external **Huawei-WMI development driver has
offered generic fan/temperature monitoring since 2024**, and a public NBLK ACPI/EC
map dates to 2022. Reading these getters is established prior art.

Exact-model references must still be qualified: several describe a **HUAWEI**
variant or another BIOS. They do not establish interchangeable support for the
complete HONOR/M1010/BIOS 3.07 configuration tested in this project.

## Most relevant sources

| Source | Verified evidence | Applicability |
| --- | --- | --- |
| [Huawei-WMI hwmon implementation](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/huawei-wmi.c#L1374-L1551) | Generic fan and temperature getters; added in [commit 85f09851](https://github.com/aymanbagabas/Huawei-WMI/commit/85f09851d0dce76f063ad7fe359be6780510b7f1), 2024-01-10 | Useful existing Linux code, distinct from the stock in-tree driver |
| [Huawei-WMI PR #110](https://github.com/aymanbagabas/Huawei-WMI/pull/110) | NBLK keyboard-backlight quirk, merged 2026-04-13; author reports timeout control working | Exact model name, HUAWEI DMI quirk; not a test of all monitoring functions on HONOR BIOS 3.07 |
| [NBLK DSDT](https://github.com/sermart1234/huawei-wmi-tool/blob/a67e8f4d6b7cbd1f35d0b36593b7a83daa870cf2/dsdt/NBLK-WAX9X_1.7.dsl#L175-L222) and [EC map](https://github.com/sermart1234/huawei-wmi-tool/blob/a67e8f4d6b7cbd1f35d0b36593b7a83daa870cf2/ec_map/NBLK-WAX9X_1.7.txt) | Public model-specific materials added 2022-02-12; six known temperature selectors | Strong protocol reference; another firmware version, not a ready driver |
| [4rmcyt NixOS matebook configuration](https://github.com/4rmcyt/nix-config/tree/17e629ce7c5e3fd85b9535fc734001b01bd171d1/hosts/nixos/matebook) | NBLK-WAX9X-PCB/M1010, Ryzen 5 3500U, BIOS 1.19 | Close HUAWEI variant; user configuration, not proof that each tuning choice helps this HONOR |
| [libfprint experimental PR #5](https://github.com/buxel/libfprint/pull/5) | Author reports enroll/verify and resume on NBLK-WAX9X with Goodix 27c6:5110 | Open/unmerged at search time; requires matching USB hardware and software conditions |
| [Deepin issue #3722](https://github.com/linuxdeepin/developer-center/issues/3722), [fix PR #159](https://github.com/linuxdeepin/deepin-kwin/pull/159) | HONOR NBLK-WAX9X screen-blanking report and linked desktop fix, merged 2023-04-17 | A desktop-specific fix, not firmware/hwmon support |
| [HONOR FMI-XX upstream addition](https://github.com/torvalds/linux/commit/9d5443e2cdaaff3df7c4c383b8656bc7fc9f4144) | Direct read-only `GFNS` fan monitoring; merged into Linus's tree for 7.4 | Different model, FMI-XX; useful upstream precedent, not NBLK support |

## Huawei-WMI details

The development repository and the standard kernel driver have different feature
sets. At the pinned development revision:

- Commands are `FAN_SPEED_GET = 0x0802` and `TEMP_GET = 0x0202`, corresponding
  to `GFNS` and `GTMP`.
- The generic hwmon code exposes **two fan attributes and eleven temperature
  attributes**. That does not prove two installed fans or eleven valid sensors
  on NBLK; this project's verified contract remains six temperatures and one fan.
- The [NBLK entry](https://github.com/aymanbagabas/Huawei-WMI/blob/1ae8fb55b54c1ad6a7220276a9322468cd8225de/huawei-wmi.c#L290-L298)
  matches vendor **HUAWEI**. It belongs to a **quirk table**, not a complete driver
  allowlist: the entry alone neither proves nor rules out HONOR compatibility.
- The [2026 quirk change](https://github.com/aymanbagabas/Huawei-WMI/commit/8272148e18ae8b7f639252d5503ecb185c04071d)
  has an author's backlight-timeout test. It is not independent validation of
  our seven monitoring channels on BIOS 3.07.

An existing user interface is
[matebook-applet](https://github.com/nekr0z/matebook-applet/blob/d5396f7f2934a50227c43cabd38c98372990c207/README.md#L70-L92),
which uses huawei-wmi controls for features such as Fn-lock and battery protection.
Existing interfaces should be evaluated before creating another management UI.

## Public ACPI material

The older public NBLK DSDT recognizes selectors **00, 05, 07, 08, 0b, 0e** for
**CTMP, TSLO, TNTC, CNTC, DNTC, BTMP**. These are the same selector names used
by our validated model-specific module. Its `GFNS` response is status followed
by little-endian fan RPM. Multiple AML fan branches do not establish how many
physical fans are present.

This is source-level corroboration of the getter contract, not proof that the
older public firmware image is identical to the local BIOS 3.07. Original
third-party dumps are linked as evidence rather than copied into this repository.

## Other findings and compatibility limits

- [Historical NBLK kernel configuration](https://gist.github.com/argraur/0f72bcb0716b24a8a4f79b91e33d66c1):
  Linux 5.6.2, 2020-04-03; configuration rather than a new model driver.
- [Huawei-WMI discussion #38](https://github.com/aymanbagabas/Huawei-WMI/issues/38):
  early NBLK-WAX9X-PCB/M1010/BIOS 1.07 reverse-engineering participation.
- [Deepin #3772](https://github.com/linuxdeepin/developer-center/issues/3772) and
  [#5611](https://github.com/linuxdeepin/developer-center/issues/5611): exact-model
  suspend/display/power-button reports on older desktop stacks; not proven fixes
  for the current system.
- [Brunch #855](https://github.com/sebanc/brunch/issues/855): older NBLK/RTL8822CE
  Wi-Fi report; closed as stale/not planned, not evidence of a verified fix.
- [NBLK OpenCore project](https://github.com/herrnst/HuaweiMatebookD14AMD-OpenCore/blob/e3f7904cebd6f97a008d678de04c0c4c00ccd30b/README.md):
  exact model/CPU, but macOS/OpenCore material rather than Linux driver support.
- [ZQC-P EC sensors](https://github.com/rs0x29a/Linux-on-HONOR-MagicBook-14-Pro-2026-AI_ZQC-P_M1010/tree/95631852eeda83b8a32a4913f5431475daad3ca1/patch/fan),
  [KLVG-XX fan daemon](https://github.com/duny31030/matebook-fanctld/blob/149543dc5ece575e4360202526e19a256a2d0752/README.md),
  [HonorControl](https://github.com/ZachAR3/HonorControl/blob/2f61ef76b1ced234cccb44ee2a2feb8d920ddd6c/README.md),
  and [FRI-H76 fan controller](https://github.com/sealeelike/Honor-FRI-H76-fan-controller-for-linux/blob/73c6fe9f096c1a9b5760c3833e182d8c60c9ea18/README.md)
  target other boards/CPUs. Their EC writes, offsets and fan policies have not
  been established as compatible with NBLK. A shared `M1010` string is insufficient.

No tested NBLK-specific fan-control implementation, BIOS-unlock package or
coreboot port was identified in the surveyed sources. The searched coreboot
mainboard/documentation tree at
[`86da80af`](https://github.com/coreboot/coreboot/tree/86da80afb95d88beac8b4bb6fd04582e0a551a1e/src/mainboard)
had no NBLK/HONOR/HUAWEI board match. These are bounded search results, not proof
that no such project exists anywhere.

## Search coverage and reproducibility

Primary GitHub source, commits, issues and PRs were inspected for NBLK-WAX9X,
NBLK-WAX9X-PCB, WAX9X/NBLK spelling variants, MagicBook/MateBook with Ryzen 3500U,
and combinations with hwmon, fan, GTMP, battery, BIOS and coreboot.

The exact-model issue/PR searches covered 14 issues and 5 PRs, with relevant
threads inspected separately. All 84 comments in Huawei-WMI #38 were searched
for model-specific evidence. Wider code searches contained hundreds of hardware
inventories and were narrowed rather than exhausted. Twenty-three pinned source
links in the research evidence were checked.

Some GitHub MCP issue queries incorrectly returned zero and were cross-checked
against public REST/web results. Rate limiting and incomplete broad code-search
results also occurred. Negative findings therefore remain qualified. No newly
discovered driver, firmware or fan daemon was installed during this research.
