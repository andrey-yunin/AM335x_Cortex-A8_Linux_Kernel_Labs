# Linux-side: Device Tree, overlays и kernel log

Этот раздел показывает, что происходит после строки:

```text
Starting kernel ...
```

U-Boot уже загрузил в RAM kernel, final Device Tree, initrd и kernel command
line. Дальше Linux kernel использует эти данные, чтобы определить модель платы,
найти встроенную периферию SoC, запустить драйверы и смонтировать rootfs.

## 1. Что проверяем со стороны Linux

Цель занятия - сопоставить три уровня:

```text
U-Boot boot log
  -> что загрузчик прочитал с microSD и передал ядру

/boot/uEnv.txt
  -> какие настройки и overlays использовал U-Boot

Linux runtime
  -> что ядро реально увидело после старта
```

Главная идея: Linux обычно не знает историю применения каждого overlay как
отдельное событие. Он получает уже итоговый flattened Device Tree и работает с
ним как с описанием железа.

## 2. Kernel command line

Команда:

```sh
cat /proc/cmdline
```

Зачем:

- увидеть фактические bootargs, полученные Linux kernel;
- проверить console, rootfs, filesystem type и дополнительные параметры;
- сопоставить `/proc/cmdline` с `cmdline=` и `console=` из `/boot/uEnv.txt`.

Что ищем:

```text
console=ttyS0,115200n8
root=/dev/mmcblk0p3
rootfstype=ext4
rootwait
```

Как используется:

- `console=ttyS0,115200n8` объясняет, почему login prompt виден в debug UART;
- `root=/dev/mmcblk0p3` объясняет, где Linux ищет root filesystem;
- `rootwait` говорит ядру ждать появления microSD/eMMC устройства.

## 3. Модель платы из Device Tree

Команда:

```sh
tr -d '\0' < /proc/device-tree/model; echo
```

Зачем:

- проверить, какую модель платы kernel получил из Device Tree;
- убедиться, что U-Boot передал правильный DTB для BeagleBone Black.

Ожидаемый результат:

```text
TI AM335x BeagleBone Black
```

Файлы в `/proc/device-tree` содержат бинарные свойства Device Tree. Строковые
свойства обычно завершаются `NUL`-байтом, поэтому используется `tr -d '\0'`.

## 4. Как правильно обходить /proc/device-tree

На многих системах `/proc/device-tree` является symlink на:

```text
/sys/firmware/devicetree/base
```

Поэтому команда:

```sh
find /proc/device-tree -maxdepth 2 -type d
```

может ничего не вывести: `find` без `-L` не переходит по symlink.

Правильные варианты:

```sh
find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
```

или:

```sh
find /sys/firmware/devicetree/base -maxdepth 2 -type d | head -n 80
```

Зачем:

- увидеть узлы итогового Device Tree;
- найти контроллеры SoC: MMC, I2C, SPI, UART, ADC, PRU, HDMI;
- сопоставить адреса узлов с kernel log и документацией AM335x.

## 5. Kernel log

Команда:

```sh
dmesg | grep -Ei 'OF:|fdt|overlay|mmc|adc|pru|hdmi'
```

Зачем:

- увидеть, какую модель платы kernel получил из FDT;
- найти строки инициализации MMC/eMMC;
- найти следы ADC, PRU, HDMI и других подсистем;
- сопоставить kernel log с тем, что U-Boot загрузил и передал.

Важное ограничение: если U-Boot применил overlays до старта Linux, kernel чаще
видит уже итоговый Device Tree. Поэтому в `dmesg` может не быть отдельных строк
`overlay applied`, хотя в U-Boot boot log overlay-файлы загружались.

## 6. /boot и /boot/uEnv.txt

Команда:

```sh
ls -la /boot
```

Зачем:

- увидеть реальные boot-файлы на rootfs-разделе;
- сопоставить их с тем, что U-Boot читал из `mmc 0:3 /boot`;
- проверить kernel, initrd, DTB directory и `uEnv.txt`.

Команда:

```sh
sed -n '1,220p' /boot/uEnv.txt
```

Зачем:

- увидеть настройки, которые U-Boot импортирует через `env import`;
- проверить `uname_r`, overlays и kernel command line;
- понять, почему загружается именно этот kernel и эти overlays.

Что важно искать:

```text
uname_r=...
enable_uboot_overlays=1
disable_uboot_overlay_...
uboot_overlay_pru=...
console=...
cmdline=...
init=/usr/sbin/init-beagle-flasher
```

Строка flasher должна оставаться закомментированной, пока мы не приняли
отдельное решение прошивать eMMC.

## 7. MMC/eMMC со стороны Linux

Команда:

```sh
lsblk -o NAME,SIZE,FSTYPE,LABEL,PARTUUID,MOUNTPOINTS
```

Зачем:

- сопоставить U-Boot `mmc 0:1`, `mmc 0:2`, `mmc 0:3` с Linux
  `/dev/mmcblk0p1`, `/dev/mmcblk0p2`, `/dev/mmcblk0p3`;
- убедиться, что rootfs смонтирован с microSD;
- увидеть встроенную eMMC, но не изменять ее.

Ожидаемая картина:

```text
mmcblk0      microSD
mmcblk0p1    BOOT, /boot/firmware
mmcblk0p2    swap
mmcblk0p3    rootfs, /
mmcblk1      встроенная eMMC
```

Если `mmcblk1p1` имеет label `rootfs`, это не значит, что система сейчас
загружена с eMMC. Текущий root определяется по mount point `/` и параметру
`root=` в `/proc/cmdline`.

## 8. Контрольные вопросы

1. Почему `/proc/cmdline` является главным доказательством того, какие bootargs
   получил Linux?
2. Почему `find /proc/device-tree ...` может ничего не вывести без `-L`?
3. Почему в `dmesg` может не быть отдельных строк о применении U-Boot overlays?
4. Чем `/boot/uEnv.txt` отличается от `/boot/firmware/sysconf.txt`?
5. Почему наличие `mmcblk1p1` с label `rootfs` не означает загрузку с eMMC?
6. Какая строка в `/boot/uEnv.txt` включает eMMC flasher и почему пока она
   должна оставаться закомментированной?

## 9. Мини-практика

Выполнить на BeagleBone Black:

```sh
cat /proc/cmdline
tr -d '\0' < /proc/device-tree/model; echo
find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
dmesg | grep -Ei 'OF:|fdt|overlay|mmc|adc|pru|hdmi'
ls -la /boot
sed -n '1,220p' /boot/uEnv.txt
lsblk -o NAME,SIZE,FSTYPE,LABEL,PARTUUID,MOUNTPOINTS
```

Результат занятия - связать:

```text
U-Boot boot log
  -> /boot/uEnv.txt
  -> final Device Tree
  -> Linux dmesg
  -> /proc/cmdline
  -> lsblk
```

