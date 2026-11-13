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

The standard `huawei-wmi` module remains installed and loaded. There is no
blacklist or replacement of that driver. DKMS may sign the supplemental module;
systems enforcing module signatures require a trusted signing key. The tested
system did not enforce signatures.

## Verification

```sh
python3 honor-acpi-hwmon/verify_hwmon.py
sudo python3 honor-acpi-hwmon/verify_hwmon.py --compare-wmi
sudo dkms status -m honor-acpi-hwmon
modinfo -F filename honor_acpi_hwmon
```

The second test needs the stock `huawei-wmi` debugfs interface. It selects only
temperature/fan getter commands and restores the previous debugfs argument.
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

These paths belong to this supplemental module. Stock battery/hotkey support
continues to be provided by the kernel's existing drivers.
