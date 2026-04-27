# Serial console и первая загрузка

Serial console - главный инструмент для изучения загрузки. SSH появляется
поздно; UART показывает SPL, U-Boot и сообщения ядра.

## 1. Подключение

Использовать USB-UART адаптер 3.3 V.

Debug-разъем J1 на BeagleBone Black:

```text
J1 pin 1: GND
J1 pin 4: UART0 RX
J1 pin 5: UART0 TX
```

Подключить:

```text
USB-UART GND -> J1 pin 1 GND
USB-UART TX  -> J1 pin 4 BBB RX
USB-UART RX  -> J1 pin 5 BBB TX
```

Не подключать 5 V от адаптера.

## 2. Открыть консоль

Найти адаптер:

```sh
dmesg -w
```

Типичные имена устройств:

```text
/dev/ttyUSB0
/dev/ttyACM0
```

В нашей лабораторной сессии одновременно были видны два serial-устройства:

```text
pl2303 converter now attached to ttyUSB0
cdc_acm ... ttyACM0: USB ACM device
```

`/dev/ttyUSB0` - это внешний USB-UART адаптер на debug-разъеме J1. Он нужен для
ранних boot-логов: SPL, U-Boot и kernel.

`/dev/ttyACM0` - это USB gadget самой BeagleBone Black через mini-USB. Он
появляется уже после того, как плата частично загрузилась, поэтому для изучения
ранней загрузки он не заменяет debug UART.

Открыть консоль:

```sh
picocom -b 115200 /dev/ttyUSB0
```

Или через `minicom`:

```sh
sudo minicom -D /dev/ttyUSB0 -b 115200
```

Выход из picocom:

```text
Ctrl-A, then Ctrl-X
```

Выход из `minicom`:

```text
Ctrl-A, then X
```

## 3. Сохранить boot log

Использовать `script`, чтобы сохранить сессию:

```sh
mkdir -p ~/bbb-course/logs
script -f ~/bbb-course/logs/first-boot-uart.log
picocom -b 115200 /dev/ttyUSB0
```

После выхода из `picocom` выйти из `script`:

```sh
exit
```

## 4. Что отметить при первой загрузке

Во время первой загрузки отметить эти точки:

```text
U-Boot SPL starts
Full U-Boot starts
Autoboot countdown appears
Kernel starts
Root filesystem is mounted
systemd starts
Login prompt appears
```

## 5. Что было видно в первом успешном boot log

Первая важная строка:

```text
U-Boot SPL 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
```

Учебный вывод: AM335x Boot ROM выбрал источник загрузки и смог запустить
SPL/MLO. Сам Boot ROM привычных подробных логов не печатает, поэтому его работу
мы видим по факту запуска SPL.

Дальше появился полный U-Boot:

```text
U-Boot 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
CPU  : AM335X-GP rev 2.1
Model: TI AM335x BeagleBone Black
DRAM:  512 MiB
```

Учебный вывод: SPL инициализировал DDR3 RAM и передал управление полному
U-Boot. U-Boot определил SoC, модель платы и объем RAM.

U-Boot нашел карту и загрузочные файлы:

```text
SD/MMC found on device 0
Loaded environment from /boot/uEnv.txt
loading /boot/vmlinuz-6.12.76-bone50 ...
loading /boot/dtbs/6.12.76-bone50/am335x-boneblack-uboot.dtb ...
loading /boot/initrd.img-6.12.76-bone50 ...
```

Учебный вывод: U-Boot прочитал конфигурацию, Linux kernel, Device Tree Blob и
initrd с microSD.

Командная строка ядра:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait
```

Учебный вывод:

- `console=ttyS0,115200n8` оставляет системную консоль на debug UART.
- `root=/dev/mmcblk0p3` указывает ядру, где находится корневая файловая система.
- `rootwait` говорит ядру ждать появления SD/eMMC устройства.

Старт ядра:

```text
Starting kernel ...
Booting Linux on physical CPU 0x0
Linux version 6.12.76-bone50
OF: fdt: Machine model: TI AM335x BeagleBone Black
```

Учебный вывод: управление перешло от U-Boot к Linux kernel, а описание платы
пришло через Device Tree.

Обнаружение носителей Linux-ядром:

```text
mmcblk0: mmc0:b370 CBADS 58.1 GiB
mmcblk0: p1 p2 p3
mmcblk1: mmc1:0001 M62704 3.56 GiB
mmcblk1: p1
```

Учебный вывод:

- внутри запущенной BeagleBone `mmcblk0` - microSD;
- `mmcblk1` - встроенная eMMC;
- eMMC пока не трогаем.

Применение `sysconf.txt`:

```text
bbbio-set-sysconf: Reading the system configuration settings from /boot/firmware/sysconf.txt
default username is [andrey]
```

Учебный вывод: first-boot сервис прочитал `sysconf.txt` с `BOOT`-раздела и
создал пользователя `andrey`.

## 6. Проверка после входа в Debian

После появления приглашения:

```text
Debian GNU/Linux 12 BeagleBone ttyS0
BeagleBone login:
```

войти пользователем, заданным в `sysconf.txt`.

Первые контрольные команды:

```sh
whoami
uname -a
cat /proc/cmdline
lsblk
```

Пример успешного результата:

```text
whoami
andrey

uname -a
Linux BeagleBone 6.12.76-bone50 ... armv7l GNU/Linux

cat /proc/cmdline
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait ...

lsblk
mmcblk0p1  /boot/firmware
mmcblk0p2  [SWAP]
mmcblk0p3  /
mmcblk1    встроенная eMMC
```

Учебный вывод:

- login работает, пользователь создан;
- система загружена с microSD;
- `BOOT`-раздел смонтирован внутри Debian как `/boot/firmware`;
- `rootfs` находится на `/`;
- swap-раздел активирован;
- eMMC видна отдельным устройством `mmcblk1`, но пока не изменялась.

## 7. Остановиться в U-Boot

Когда U-Boot пишет "Hit any key to stop autoboot", нажать любую клавишу.

Первые полезные команды:

```text
version
bdinfo
mmc list
mmc dev 0
mmc part
printenv
ls mmc 0:1 /
ls mmc 0:3 /
ls mmc 0:3 /boot
boot
```

Цель пока не менять настройки. Цель - прочитать окружение и понять, какие файлы
загружает U-Boot.
