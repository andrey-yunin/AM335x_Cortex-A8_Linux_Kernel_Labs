# AM335x Cortex-A8 Linux Kernel Labs

Practical embedded Linux course for the **TI AM335x SoC** with **ARM
Cortex-A8**, using **BeagleBone Black** as the reference board.

The repository is a hands-on learning workbook for boot chain inspection,
U-Boot, Linux runtime analysis, Device Tree, host-target workflow, kernel
modules, and ADC/IIO driver practice.

The course is intentionally practical: each step records the command, the
observed result, and the engineering conclusion.

## Target Platform

- SoC: TI AM335x
- CPU: ARM Cortex-A8
- Reference board: BeagleBone Black
- OS: Debian 12.13
- Kernel family: Linux v6.12.x / bone
- Current tested kernel: `6.12.76-bone50`
- Main workflow: Linux host + BeagleBone Black target over SSH/SCP/rsync

Recommended BeagleBoard image:

```text
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Why this image:

- It is an official BeagleBoard image for the AM335x family.
- Debian 12 is conservative enough for reproducible labs.
- Linux v6.12.x is modern enough for current kernel and Device Tree practice.
- BeagleBone Black is used as a stable reference board with accessible headers,
  UART console, Ethernet, eMMC, microSD, ADC, I2C, SPI, GPIO, and PRU.

## Course Map

Single HTML entry point for browsing the learning base:

- [Course index](index.html)

Core documents:

- [Work report](REPORT.md)
- [Next session playbook](NEXT_SESSION.md)

### Part 1. Linux Installation and Boot Chain

1. [Boot chain overview](course/01-boot-chain.md)
2. [Linux host and microSD preparation](course/02-prepare-sd.md)
3. [Serial console and first boot](course/03-serial-console.md)
4. [U-Boot environment lab](course/04-u-boot-environment.md)
5. [Linux Device Tree, overlays, and kernel log](course/05-linux-device-tree.md)
6. [Host-target network, SSH, and VPN bypass](course/06-host-target-network.md)
7. eMMC flashing, planned as a separate controlled step

### Part 2. Kernel Modules and Drivers

20. [Kernel modules and ADC roadmap](course/20-kernel-modules-roadmap.md)

Planned route:

```text
Hello, World kernel module
  -> AM335x built-in ADC through the existing Linux IIO driver
  -> external ADS1115 ADC as a budget industrial analog input
```

The final practical target is an educational analog input module:

```text
0-10 V signal  -> resistor divider -> ADS1115 -> I2C -> Linux IIO
4-20 mA signal -> shunt resistor    -> ADS1115 -> I2C -> Linux IIO
```

## Current Lab State

- Debian boots from microSD.
- U-Boot SPL, full U-Boot, kernel, DTB, overlays, initrd, and rootfs path have
  been inspected.
- Device Tree and U-Boot overlays have been inspected from Linux.
- AM335x built-in ADC is visible through IIO.
- Host-target Ethernet and SSH workflow is working.
- VPN routing issue on the host has been documented and bypassed with a
  per-target route.
- First out-of-tree `hello` kernel module has been built on the BeagleBone
  Black, inspected with `modinfo`, loaded with `insmod`, verified through
  `dmesg`/`lsmod`, and unloaded with `rmmod`.

Current host-target route:

```text
host eno0:        10.129.1.110/23
BeagleBone eth0:  10.129.1.152/23
VPN table:        7113
SSH target:       andrey@10.129.1.152
```

Route restore command when the VPN captures target traffic:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

The VPN bypass route is intentionally temporary. It is restored manually only
when working with the BeagleBone Black, so the host system does not keep stale
lab-specific routing configuration later.

Helper script:

```sh
sh scripts/bbb-route.sh check
sh scripts/bbb-route.sh apply
sh scripts/bbb-route.sh delete
```

## Hardware

Required:

- BeagleBone Black
- microSD card, preferably 8 GB or larger
- Linux host with card reader
- Reliable 5 V power supply, preferably barrel jack
- 3.3 V USB-UART adapter for boot logs
- Ethernet cable or USB gadget network for post-boot access

Do **not** connect the USB-UART adapter VCC/5V pin to the BeagleBone debug
header. Use only GND, TX, and RX.

Planned for ADC labs:

- ADS1115 breakout module
- Breadboard
- Dupont wires
- 10 kOhm potentiometer
- Resistors for voltage divider and current shunt
- Multimeter

## Development Workflow

Expected workflow for kernel module labs:

```text
VS Code on Linux host
  -> edit source files
  -> scp/rsync over SSH
  -> BeagleBone Black target
  -> make / insmod / dmesg / rmmod
```

The first module source files are edited on the Linux host and transferred to
the BeagleBone Black over SSH/SCP/rsync. The initial build is done on the target
to validate the installed kernel headers and Kbuild environment. Later labs can
move toward a host-side cross-build workflow.

## Safety Notes

- Do not run `saveenv` in U-Boot unless the change is intentional.
- Do not flash eMMC until a separate eMMC procedure is documented.
- Do not write to `/dev/mmcblk1` from Linux unless the target is confirmed.
- Do not edit `/boot/uEnv.txt` without a backup.
- BeagleBone Black analog inputs are **not 3.3 V tolerant**.
- Built-in ADC inputs must stay within `0..1.8 V`.

## Main References

- BeagleBoard latest images: https://www.beagleboard.org/distros
- BeagleBoard getting started: https://docs.beagleboard.org/intro/support/getting-started.html
- BeagleBone Black product page: https://www.beagleboard.org/p/products/beaglebone-black
- TI AM335x product page: https://www.ti.com/product/AM3358
- TI ADS1115 product page: https://www.ti.com/product/ADS1115
- TI ADS1015/ADS1115 Linux driver: https://www.ti.com/tool/ADS1015SW-LINUX
