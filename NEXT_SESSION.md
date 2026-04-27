# Playbook следующей сессии

Дата подготовки: 2026-04-27.

Цель playbook - быстро вернуться к работе после паузы и не восстанавливать
контекст по памяти.

## Текущее состояние

Первый целевой результат курса достигнут:

- microSD записана официальным образом BeagleBoard Debian 12.13 IoT;
- `sysconf.txt` настроен;
- BeagleBone Black загружается с microSD;
- UART debug console работает через USB-UART адаптер;
- вход в Debian выполнен пользователем `andrey`;
- U-Boot environment изучен без изменения настроек;
- eMMC обнаружена, но не изменялась.

Используемый образ:

```text
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Ядро на плате:

```text
Linux BeagleBone 6.12.76-bone50 armv7l
```

## Важные выводы

Цепочка загрузки, которую уже увидели через UART:

```text
AM335x Boot ROM
  -> U-Boot SPL / MLO
  -> full U-Boot
  -> Linux kernel + Device Tree + initrd
  -> rootfs
  -> systemd
  -> login shell
```

Разметка внутри загруженной BeagleBone:

```text
mmcblk0      microSD
mmcblk0p1    BOOT, mounted at /boot/firmware
mmcblk0p2    swap
mmcblk0p3    rootfs, mounted at /
mmcblk1      встроенная eMMC, пока не трогаем
```

Ключевая командная строка ядра:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait
```

Что подтверждено в U-Boot:

```text
mmc 0      microSD
mmc 0:1    BOOT/FAT, sysconf.txt
mmc 0:2    swap
mmc 0:3    rootfs/ext4, /boot/uEnv.txt, kernel, DTB, initrd
bootdelay  0
```

Что U-Boot загружает в RAM перед стартом Linux:

```text
kernel image
final Device Tree Blob
initrd / initramfs
kernel command line
```

Root filesystem целиком в RAM не загружается. Linux монтирует его с
`/dev/mmcblk0p3`.

## Быстрый старт следующей сессии

1. Подключить USB-UART к BeagleBone Black:

```text
USB-UART GND -> BBB J1 pin 1 GND
USB-UART TX  -> BBB J1 pin 4 RX
USB-UART RX  -> BBB J1 pin 5 TX
```

Не подключать `VCC` / `5V` от USB-UART адаптера.

2. Вставить microSD в BeagleBone Black.

3. На Linux-хосте найти UART:

```sh
dmesg | tail -n 40
```

Ожидаемый адаптер:

```text
/dev/ttyUSB0
```

4. Открыть консоль:

```sh
sudo minicom -D /dev/ttyUSB0 -b 115200
```

5. Подать питание на BeagleBone Black. Если нужно гарантированно загрузиться с
microSD, зажать `S2` / `BOOT` при подаче питания и отпустить через пару секунд.

6. Войти:

```text
login: andrey
Password: <пароль, заданный в sysconf.txt при последней записи>
```

Пароль в документации не фиксируется.

## Команды проверки после входа

Выполнить на BeagleBone Black:

```sh
whoami
uname -a
cat /proc/cmdline
lsblk
```

Ожидаемые признаки нормальной загрузки:

```text
whoami -> andrey
root=/dev/mmcblk0p3
mmcblk0p1 -> /boot/firmware
mmcblk0p2 -> [SWAP]
mmcblk0p3 -> /
mmcblk1   -> eMMC
```

## Следующий учебный шаг

Рекомендуемый следующий этап - разобрать Linux-side часть загрузки: kernel log,
Device Tree, overlays и состояние встроенной eMMC без прошивки.

Цель этапа:

- понять, как Linux использует Device Tree, полученный от U-Boot;
- увидеть примененные overlays: ADC, eMMC, HDMI, PRU UIO;
- сопоставить `/proc/device-tree`, `dmesg`, `/boot/uEnv.txt` и реальные
  устройства в `/sys`;
- проверить состояние eMMC только в режиме чтения;
- подготовить решение о прошивке eMMC как отдельный осознанный этап.

Первые команды для следующей практики на BeagleBone:

```sh
cat /proc/cmdline
tr -d '\0' < /proc/device-tree/model; echo
find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
dmesg | grep -Ei 'OF:|fdt|overlay|mmc|adc|pru|hdmi'
ls -la /boot
sed -n '1,220p' /boot/uEnv.txt
lsblk -o NAME,SIZE,FSTYPE,LABEL,PARTUUID,MOUNTPOINTS
```

Перед чтением `/boot/uEnv.txt` помнить: пока только смотрим. Изменять файл
будем только после отдельного решения и бэкапа.

## Дальний план курса

После завершения первой большой части по загрузке Linux переходим ко второй
большой части: разработке модулей ядра Linux на BeagleBone Black.

Зафиксированный маршрут:

```text
Hello, World module
  -> встроенный ADC AM335x через существующий драйвер и IIO
  -> внешний ADS1115 как учебный промышленный аналоговый вход
```

Подробный план второй части: `course/20-kernel-modules-roadmap.md`.

Выбранный финальный макет:

```text
ADS1115 breakout
I2C к BeagleBone Black
вход 0-10 V через делитель
вход 4-20 mA через шунт 100 Ohm
IIO-драйвер и пересчет в инженерные величины
```

HART, мультиплексор HART-каналов, гальваническая изоляция и промышленное питание
`24 V` пока оставлены как необязательное расширение после базового внешнего ADC.

## Что не делать без отдельного решения

- Не выполнять `saveenv` в U-Boot.
- Не запускать прошивку eMMC.
- Не писать `dd` на `/dev/mmcblk1` внутри BeagleBone.
- Не изменять `/boot/uEnv.txt` без предварительного бэкапа.
- Не подключать `VCC` USB-UART адаптера к debug-разъему.

## Где смотреть подробности

- Общий отчет: `REPORT.md`
- Boot chain: `course/01-boot-chain.md`
- Подготовка microSD: `course/02-prepare-sd.md`
- UART и первая загрузка: `course/03-serial-console.md`
- U-Boot environment: `course/04-u-boot-environment.md`
- Linux Device Tree и kernel log: `course/05-linux-device-tree.md`
- План модулей ядра и ADC: `course/20-kernel-modules-roadmap.md`
