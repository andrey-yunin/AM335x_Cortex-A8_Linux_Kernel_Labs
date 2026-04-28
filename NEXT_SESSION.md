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
- eMMC обнаружена, но не изменялась;
- Ethernet-связь host <-> BeagleBone проверена;
- SSH с хоста на плату работает через точечный обход VPN.
- первый out-of-tree `hello` kernel module собран на BeagleBone Black,
  проверен через `modinfo`, загружен через `insmod`, подтвержден в `dmesg` и
  `lsmod`, затем выгружен через `rmmod`.

Используемый образ:

```text
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Ядро на плате:

```text
Linux BeagleBone 6.12.76-bone50 armv7l
```

Рабочая сеть для host-target workflow:

```text
host eno0:        10.129.1.110/23
BeagleBone eth0:  10.129.1.152/23
VPN interface:    outline-tun1
VPN table:        7113
SSH target:       andrey@10.129.1.152
```

VPN перехватывал маршрут к плате:

```text
10.129.1.152 via 10.0.85.5 dev outline-tun1 table 7113
```

Рабочее временное исключение:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

Решение по маршруту: исключение намеренно оставляем временным и
восстанавливаем вручную после перезагрузки хоста, перезапуска сети или VPN. Для
лабораторной работы это лучше, чем постоянная системная настройка, которая
позже может остаться ненужным мусором после завершения работы с BeagleBone
Black.

Для удобства добавлен непостоянный helper-скрипт:

```sh
scripts/bbb-route.sh check
scripts/bbb-route.sh apply
scripts/bbb-route.sh delete
```

Он не прописывает маршрут в systemd, NetworkManager или конфигурацию VPN.
Команда `apply` просто выполняет тот же временный `ip route replace`.

Проверка:

```sh
ip route get 10.129.1.152
ssh andrey@10.129.1.152
```

Ожидаемый маршрут:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110
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

Linux-side часть загрузки, Device Tree и runtime-проверки уже разобраны.
Host-target workflow закрыт, первый `Hello, World` kernel module собран и
проверен. Рекомендуемый следующий этап - расширить первый модуль параметром.

Цель этапа:

- добавить параметр в `modules/hello/hello.c` через `module_param`;
- обновить `MODULE_PARM_DESC`;
- передать обновленный модуль на BeagleBone через `rsync`;
- пересобрать модуль на BeagleBone под `6.12.76-bone50`;
- проверить `modinfo`, загрузку с параметром, `dmesg` и
  `/sys/module/hello/parameters`;
- не обновлять kernel-пакеты без отдельного решения.

Первые команды для следующей практики на хосте:

```sh
ip route get 10.129.1.152
ssh andrey@10.129.1.152 'hostname; uname -r; whoami'
printf 'bbb host to target transfer test\n' > /tmp/bbb_ssh_transfer_test.txt
scp /tmp/bbb_ssh_transfer_test.txt andrey@10.129.1.152:/home/andrey/
ssh andrey@10.129.1.152 \
  'ls -l /home/andrey/bbb_ssh_transfer_test.txt; cat /home/andrey/bbb_ssh_transfer_test.txt'
```

Если `ip route get` снова показывает `outline-tun1`, восстановить исключение:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

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

- HTML-точка входа: `index.html`
- Общий отчет: `REPORT.md`
- Boot chain: `course/01-boot-chain.md`
- Подготовка microSD: `course/02-prepare-sd.md`
- UART и первая загрузка: `course/03-serial-console.md`
- Host-target network: `course/06-host-target-network.md`
- U-Boot environment: `course/04-u-boot-environment.md`
- Linux Device Tree и kernel log: `course/05-linux-device-tree.md`
- План модулей ядра и ADC: `course/20-kernel-modules-roadmap.md`
