# Playbook следующей сессии

Дата подготовки: 2026-04-30.

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
- SSH с хоста на плату работает через точечный обход VPN;
- первый out-of-tree `hello` kernel module собран на BeagleBone Black,
  проверен через `modinfo`, загружен через `insmod`, подтвержден в `dmesg` и
  `lsmod`, затем выгружен через `rmmod`;
- в `hello` добавлен read-only параметр `username: charp, 0444`;
- в `hello` добавлен writable параметр `verbose: bool, 0644`;
- проверено: `modinfo` показывает оба параметра;
- проверено: `insmod hello.ko username=Yunin verbose=1` передает оба значения в
  модуль;
- проверено: `/sys/module/hello/parameters/username` доступен только на чтение;
- проверено: `/sys/module/hello/parameters/verbose` доступен root на запись;
- проверено: после `rmmod hello` sysfs-точки модуля исчезают;
- в `hello` добавлен callback-параметр `log_level: int, 0644` через
  `module_param_cb`;
- проверено: `modinfo` показывает `log_level` в metadata `.ko`;
- проверено: `insmod hello.ko username=Yunin verbose=1 log_level=2` вызывает
  `log_level_set()` до `bbb_hello_init()`;
- проверено: `/sys/module/hello/parameters/log_level` показывает значение и
  права `-rw-r--r--`;
- проверено: runtime-запись `echo 3 | sudo tee .../log_level` вызывает callback
  и обновляет значение;
- проверено: невалидное значение `9` возвращает `Invalid argument`, пишет
  предупреждение в `dmesg` и не меняет значение;
- проверено: после `rmmod hello` sysfs-точка `log_level` исчезает.

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

Модель первого kernel module:

```text
Linux host / VS Code
  -> modules/hello/hello.c
  -> modules/hello/Makefile
  -> rsync
  -> BeagleBone Black /home/andrey/bbb-course/modules/hello
  -> Kbuild
  -> modinfo
  -> insmod/rmmod
  -> dmesg/sysfs
```

Текущий `hello.c` содержит:

```text
username: charp, 0444
verbose:  bool,  0644
log_level: int,   0644 через module_param_cb
```

Ключевая модель sysfs:

```text
/sys/module/hello/parameters/username
  -> точка доступа к параметру username загруженного модуля hello
```

Значение параметра хранится в RAM ядра как часть состояния загруженного модуля.
Sysfs path не является адресом RAM и не является обычным файлом на microSD.
Это контролируемая точка доступа, которую ядро предоставляет userspace.

Ключевая модель виртуальной памяти:

```text
software работает с virtual addresses
MMU переводит virtual -> physical
hardware видит physical addresses
```

Для модуля:

```text
hello.ko загружен
  -> код и данные размещены в kernel virtual address space
  -> MMU связывает virtual addresses с physical RAM
  -> после rmmod память и sysfs-представление модуля удаляются
```

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

## Быстрый старт разработки модулей

Работаем в двух терминалах:

```text
HOST -> Linux host, репозиторий /home/andrey/projects/BeagleBone_Black
BBB  -> отдельный терминал BeagleBone Black, каталог ~/bbb-course/modules/hello
```

### HOST

```sh
cd ~/projects/BeagleBone_Black
pwd
sed -n '1,220p' modules/hello/hello.c
```

Ожидаемый host path:

```text
/home/andrey/projects/BeagleBone_Black
```

Если перед практикой нужно восстановить временный маршрут к плате:

```sh
sh scripts/bbb-route.sh check
sh scripts/bbb-route.sh apply
```

`apply` нужен только если `check` снова показывает `outline-tun1`.

### BBB

```sh
cd ~/bbb-course/modules/hello
hostname
uname -r
pwd
lsmod | grep hello
```

Ожидаемо:

```text
BeagleBone
6.12.76-bone50
/home/andrey/bbb-course/modules/hello
lsmod | grep hello -> no output
```

Если `hello` все еще загружен, выгрузить перед новой практикой:

```sh
sudo /usr/sbin/rmmod hello
```

### Последняя проверенная точка

`hello.ko` с параметрами `username`, `verbose` и `log_level` уже был собран и
проверен на BeagleBone Black. Этап `module_param_cb` закрыт.

Ключевая формула:

```text
modinfo подтверждает наличие параметра в metadata .ko,
а insmod/sysfs/dmesg подтверждают runtime-логику параметра.
```

Проверенный результат для `log_level`:

```text
insmod log_level=2
  -> log_level_set("2", kp)
  -> bbb_hello_init() видит log_level == 2

echo 3 > /sys/module/hello/parameters/log_level
  -> log_level_set("3", kp)
  -> значение обновляется до 3

echo 9 > /sys/module/hello/parameters/log_level
  -> log_level_set("9", kp)
  -> callback возвращает -EINVAL
  -> значение остается 3

rmmod hello
  -> /sys/module/hello/parameters/log_level исчезает
```

## Следующий учебный шаг

Linux-side часть загрузки, Device Tree и runtime-проверки уже разобраны.
Host-target workflow закрыт. Первый `Hello, World` kernel module собран и
проверен. Read-only, writable и callback-параметры модуля разобраны и
проверены.

Рекомендуемый следующий этап - аккуратная диагностика в kernel module:
`pr_info()`, `pr_warn()`, `pr_err()`, `pr_debug()` и базовая идея dynamic debug.

Цель этапа:

- понять уровни сообщений ядра и когда использовать каждый уровень;
- заменить часть учебных сообщений на более подходящие уровни;
- добавить `pr_debug()` для подробной диагностики;
- проверить, почему `pr_debug()` обычно не виден без включения debug;
- передать обновленный модуль на BeagleBone через `rsync`;
- пересобрать модуль на BeagleBone под `6.12.76-bone50`;
- проверить `insmod`, `dmesg`, runtime-поведение и `rmmod`;
- не обновлять kernel-пакеты без отдельного решения.

Первые команды для следующей практики:

HOST:

```sh
ip route get 10.129.1.152
sed -n '1,180p' modules/hello/hello.c
```

Если `ip route get` снова показывает `outline-tun1`, восстановить исключение:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

BBB:

```sh
cd ~/bbb-course/modules/hello
hostname
uname -r
whoami
lsmod | grep hello
```

## Дальний план курса

После завершения первой большой части по загрузке Linux переходим ко второй
большой части: разработке модулей ядра Linux на BeagleBone Black.

Зафиксированный маршрут:

```text
Hello, World module
  -> встроенный ADC AM335x через существующий драйвер и IIO
  -> HC-06 Bluetooth UART lab через Linux tty
  -> внешний ADS1115 как учебный промышленный аналоговый вход
```

Подробный план второй части: `course/20-kernel-modules-roadmap.md`.

HC-06 добавлен именно как будущая UART/tty-лаба, а не как разработка нового
Bluetooth-драйвера. Ожидаемая модель:

```text
смартфон
  -> Bluetooth SPP
  -> HC-06
  -> UART TX/RX
  -> AM335x UART controller
  -> Linux tty
  -> /dev/ttyS*
  -> userspace read/write
```

К этой теме возвращаемся после этапа встроенного ADC AM335x.

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
- Не включать автозагрузку учебного модуля без отдельного решения.
- Не обновлять kernel-пакеты на BeagleBone без отдельного решения.

## Где смотреть подробности

- HTML-точка входа: `docs/index.html`
- Общий отчет: `REPORT.md`
- Boot chain: `course/01-boot-chain.md`
- Подготовка microSD: `course/02-prepare-sd.md`
- UART и первая загрузка: `course/03-serial-console.md`
- Host-target network: `course/06-host-target-network.md`
- U-Boot environment: `course/04-u-boot-environment.md`
- Linux Device Tree и kernel log: `course/05-linux-device-tree.md`
- План модулей ядра и ADC: `course/20-kernel-modules-roadmap.md`
- Правила работы ассистента: `ASSISTANT_RULES.md`
