# Installation and removal

## Supported hardware

The current module accepts only these exact DMI values:

- Vendor: `HONOR`
- Product: `NBLK-WAX9X`
- Board: `NBLK-WAX9X-PCB`
- BIOS: `3.07`

Other hardware/firmware returns `ENODEV`. Validate its AML and actual readings
before extending the table. Six EC temperature channels and one confirmed fan
are exposed. The module has no fan-speed, voltage, charging or power-limit setters.

## DKMS

Install DKMS, make, the compiler used by your kernel, and matching kernel
headers. On Arch/CachyOS, use the appropriate `linux-headers`,
`linux-cachyos-headers` or `linux-lts-headers` package.

From the repository root:

```sh
sudo install -d -m 755 /usr/src/honor-acpi-hwmon-0.1.1
sudo install -m 644 honor-acpi-hwmon/honor_acpi_hwmon.c \
    honor-acpi-hwmon/Makefile honor-acpi-hwmon/dkms.conf \
    /usr/src/honor-acpi-hwmon-0.1.1/
sudo dkms add -m honor-acpi-hwmon -v 0.1.1
sudo dkms install -m honor-acpi-hwmon -v 0.1.1 -k "$(uname -r)"
sudo modprobe honor_acpi_hwmon
sensors honor_acpi_hwmon-isa-0000
```

When upgrading an already loaded version, unload it after the new DKMS build
has installed successfully, then load the installed build:

```sh
sudo modprobe -r honor_acpi_hwmon
sudo modprobe honor_acpi_hwmon
cat /sys/module/honor_acpi_hwmon/version
```

The loaded version should be `0.1.1`. Keep the previous DKMS version available
until verification succeeds, then remove its registration and source directory
using that previous version number.

For another installed kernel, repeat `dkms install` with its release.
`AUTOINSTALL=yes` enables rebuilding through DKMS's normal kernel-update hooks.
For boot-time loading, create `/etc/modules-load.d/honor-acpi-hwmon.conf` with:

```text
honor_acpi_hwmon
```

The supplemental-module instructions above do not replace or blacklist
`huawei-wmi`. A separate, explicitly requested external-driver deployment is
recorded below. DKMS may sign the supplemental module; systems enforcing module
signatures require a trusted signing key. The tested system did not enforce
signatures.

## Verification

```sh
python3 honor-acpi-hwmon/verify_hwmon.py
sudo python3 honor-acpi-hwmon/verify_hwmon.py --compare-wmi
sudo dkms status -m honor-acpi-hwmon
modinfo -F filename honor_acpi_hwmon
```

The second test needs the Huawei-WMI debugfs interface, provided by either the
stock driver or the deployed external revision. It selects only temperature/fan
getter commands and restores the previous debugfs argument.
Avoid concurrent use of that raw debugfs interface during the comparison.

The test covers attributes/units/permissions, refreshes across cache expiry,
896 concurrent sensor reads, and serial reads of existing Huawei controls.
Actual suspend/resume and reboot are separate hardware tests and were not
performed during installation. Resume handlers are implemented but must not
be described as runtime-verified until those tests are done.

## Removal

```sh
sudo rm /etc/modules-load.d/honor-acpi-hwmon.conf
sudo modprobe -r honor_acpi_hwmon
sudo dkms remove -m honor-acpi-hwmon -v 0.1.1 --all
sudo rm -r /usr/src/honor-acpi-hwmon-0.1.1
```

These paths belong to this supplemental module. Removing it does not remove
the selected Huawei platform driver or its battery/hotkey support.

## External Huawei-WMI deployment

On 2026-11-14, the owner requested installation of the development driver's
current `master`, specifically the repository-root `huawei-wmi.c`, not an older
version under its `dkms/` directory.

- Repository: <https://github.com/aymanbagabas/Huawei-WMI>.
- Commit: `1ae8fb55b54c1ad6a7220276a9322468cd8225de`.
- DKMS registration: `huawei-wmi/master.1ae8fb55`.
- Source directory: `/usr/src/huawei-wmi-master.1ae8fb55`.
- Upstream C SHA256:
  `fd6ab5e4d8c172230a6cf3f99794d7b673b81c4d9252470c7c12474b5521def6`.

The C file is unmodified. Local Makefile/DKMS packaging builds with `W=1`, uses
LLVM when the target kernel has `CONFIG_CC_IS_CLANG=y`, and installs to
`/lib/modules/KERNEL/updates/dkms/`. `AUTOINSTALL="yes"` is enabled. The package
version deliberately contains no hyphen: Arch's installed DKMS hook splits
`/usr/src/NAME-VERSION` at the last hyphen when discovering source packages.

The external driver replaces the selected `huawei_wmi` module, not its module
name or WMI alias. It is not loaded alongside the in-tree module. DKMS archived
the three original modules with their original paths under
`/var/lib/dkms/huawei-wmi/original_module/`; their SHA256 checksums were verified
against the pre-installation copies. No blacklist was added. The supplemental
`honor_acpi_hwmon` module and its boot-time configuration were left intact.

### Installation and kernel-update checks

- Builds, signatures and installation passed for `7.3.3-1-cachyos`,
  `7.3.4-arch1-1` and `6.18.63-2-lts`.
- GCC builds had no reported warnings. CachyOS reported the existing compiler
  patch-version mismatch: kernel Clang 23.1.4 versus installed Clang 23.1.5.
- Installation was exercised through `/usr/share/libalpm/scripts/dkms install`
  with the installed kernels' header-update targets, rather than merely assuming
  the package hook would discover the module.
- A clean automatic rebuild was then tested for the non-running LTS kernel:
  its installed/build state was removed using `dkms unbuild`, and the same
  package hook discovered, rebuilt and reinstalled the registered source.
- The selected module path and vermagic matched each target kernel. The WMI
  alias resolves to `huawei_wmi`. None of the three current initramfs images
  contains Huawei/HONOR modules, so no image regeneration was needed.

Keep the matching kernel headers and compiler installed. DKMS rebuilds this
**pinned revision** for new kernels; it does not automatically download a newer
Huawei-WMI revision. A future incompatible kernel API can still require an
upstream update. Runtime checks below used the running CachyOS kernel only;
actual reboot, sleep and booting the other kernels were not performed.

### Runtime result and limitations

The external module was loaded and identified by srcversion
`9C93EED253B09D332EE3F9A` on `7.3.3-1-cachyos`.

- `/sys/devices/platform/huawei-wmi/kbdlight_timeout` now exists with mode 0644
  and reads `60` seconds. Writing the same value and reading it back passed;
  the configured timeout was not changed. Physical expiry timing remains
  owner-confirmed, not independently measured in this installation.
- Charge thresholds stayed `0 100`, Fn-lock stayed `0`, and the battery threshold
  attributes remained available. A 64-read serial control check passed.
- The hotkey input device registered. Physical key presses were not tested.
- The mic-mute LED is now named `huawei::micmute`, not `platform::micmute`;
  brightness stayed zero. Applications using the old hardcoded name need to
  account for this upstream naming difference.
- Six external temperature channels and fan selector zero were readable and
  agreed with the supplemental module in the inventory. The latter also passed
  its 896 concurrent-read integration test after the switch.
- Generic external attributes `temp2_input`, `temp4_input`, `temp9_input`,
  `temp10_input`, `temp11_input` return `ENODEV` on this firmware. `fan2_input`
  returned zero; a second physical fan is still unconfirmed. These files were
  not hidden by modifying upstream code or changing global sensor configuration.
- Both hwmon devices remain exposed, so monitoring applications can display
  duplicate readings. Use the validated `honor_acpi_hwmon` channels when that
  distinction matters.
- Neither `kbdlight` brightness nor a keyboard-backlight LED appeared. Smart
  Charge, power unlock and manual fan control were not gained by installing
  this driver. The previously documented WMI transaction-locking limitation is
  not fixed by this installation.

### Roll back to the in-tree platform driver

This leaves the supplemental hwmon module installed:

```sh
sudo modprobe -r huawei_wmi
sudo dkms remove -m huawei-wmi -v master.1ae8fb55 --all
sudo install -d -m 755 /var/lib/dkms-disabled
sudo mv /usr/src/huawei-wmi-master.1ae8fb55 /var/lib/dkms-disabled/
sudo modprobe huawei_wmi
modinfo -F filename huawei_wmi
```

DKMS restores the archived originals to their recorded paths. Moving this
deployment's source outside `/usr/src` is important: leaving its `dkms.conf`
there would let Arch's next kernel-update hook discover and install it again.
The restored selected path should be under `kernel/drivers/platform/x86/`, not
`updates/dkms/`. This stock-restore/reinstall path was exercised on the LTS
kernel during the automatic-rebuild test; the full live rollback was not needed.
