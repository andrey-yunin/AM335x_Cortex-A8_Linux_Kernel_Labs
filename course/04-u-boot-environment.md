# U-Boot: остановка autoboot и изучение окружения

Этот раздел - безопасная лабораторная работа с U-Boot. Цель не менять загрузку,
а увидеть, как загрузчик находит microSD, читает конфигурацию и передает ядру
Linux параметры старта.

Главное правило занятия: читаем и наблюдаем, но не сохраняем изменения.

```text
можно:   version, bdinfo, mmc list, mmc dev, mmc part, printenv, ls, boot
нельзя:  saveenv, env save, mmc write, fatwrite, ext4write, erase
```

## 1. Где U-Boot находится в цепочке загрузки

К этому моменту мы уже видели полную цепочку:

```text
AM335x Boot ROM
  -> U-Boot SPL / MLO
  -> full U-Boot
  -> Linux kernel + Device Tree + initrd
  -> rootfs
  -> systemd
  -> login shell
```

В этой лабораторной мы останавливаемся на третьем этапе:

```text
U-Boot SPL / MLO
        |
        v
full U-Boot  <- остановка autoboot и интерактивные команды
        |
        v
Linux kernel
```

SPL уже сделал раннюю работу: инициализировал DDR3 RAM и загрузил полный
U-Boot. Полный U-Boot умеет работать с MMC-устройствами, разделами, файлами,
переменными окружения и boot-скриптами.

## 2. Что такое environment

Environment в U-Boot - это набор переменных `name=value`, из которых собирается
сценарий загрузки.

Примеры типичных переменных:

```text
bootcmd=...
bootdelay=...
bootargs=...
console=ttyS0,115200n8
mmcdev=...
bootpart=...
fdtfile=...
```

Важно понимать три уровня:

```text
команды U-Boot       то, что мы вводим руками в консоли
environment          переменные и скрипты загрузчика
boot files           kernel, initrd, Device Tree, uEnv.txt на носителе
```

Когда U-Boot загружает Linux, он обычно делает такую работу:

```text
1. выбирает устройство MMC;
2. читает таблицу разделов;
3. ищет конфигурацию загрузки;
4. загружает kernel в RAM;
5. загружает Device Tree Blob в RAM;
6. загружает initrd, если он используется;
7. формирует kernel command line;
8. передает управление ядру.
```

На нашей первой успешной загрузке в UART-логе уже были видны эти признаки:

```text
SD/MMC found on device 0
Loaded environment from /boot/uEnv.txt
loading /boot/vmlinuz-6.12.76-bone50 ...
loading /boot/dtbs/6.12.76-bone50/am335x-boneblack-uboot.dtb ...
loading /boot/initrd.img-6.12.76-bone50 ...
```

Задача текущей практики - остановиться до автоматической загрузки и посмотреть
эти связи руками.

## 3. Подготовка перед включением

Подключение остается тем же, что в занятии про UART:

```text
USB-UART GND -> BBB J1 pin 1 GND
USB-UART TX  -> BBB J1 pin 4 RX
USB-UART RX  -> BBB J1 pin 5 TX
```

Не подключать `VCC` / `5V` от USB-UART адаптера.

На Linux-хосте открыть debug UART:

```sh
sudo minicom -D /dev/ttyUSB0 -b 115200
```

Если имя адаптера неизвестно, сначала проверить:

```sh
dmesg | tail -n 40
```

Для гарантированной загрузки с microSD:

1. Выключить питание BeagleBone Black.
2. Вставить подготовленную microSD.
3. Зажать `S2` / `BOOT`.
4. Подать питание.
5. Отпустить кнопку через пару секунд после начала загрузки.

## 4. Остановить autoboot

В UART-консоли дождаться строки U-Boot с обратным отсчетом. В разных сборках
она может выглядеть немного по-разному:

```text
Hit any key to stop autoboot
```

или:

```text
Press SPACE to abort autoboot
```

Нажать пробел или любую клавишу. После остановки должно появиться приглашение
U-Boot:

```text
=>
```

Контрольная точка:

```text
если видно приглашение "=>", мы находимся в интерактивной консоли U-Boot
```

## 5. Безопасный маршрут команд

Вводить команды по одной. Не выполнять `saveenv`.

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

Команда `boot` в конце продолжит обычную загрузку Linux.

## 6. Команда version

Команда:

```text
version
```

Что показывает:

- версию U-Boot;
- дату сборки;
- иногда информацию о компиляторе и конфигурации.

Ожидаемая связь с уже виденным boot log:

```text
U-Boot 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
```

Учебный вывод: версия полного U-Boot должна совпадать с той, что печаталась при
автоматической загрузке.

Место для результата:

```text
U-Boot version:
Build date:
```

## 7. Команда bdinfo

Команда:

```text
bdinfo
```

`bdinfo` показывает board information: адреса RAM, параметры загрузки и часть
низкоуровневого состояния платы.

На этом этапе особенно полезно найти:

```text
DRAM bank
boot_params
baudrate
```

Связь с boot chain:

```text
SPL инициализировал DDR3 RAM
        |
        v
full U-Boot теперь видит рабочую RAM и может размещать там kernel, initrd, DTB
```

Место для результата:

```text
RAM size:
baudrate:
boot params address:
```

## 8. MMC в U-Boot и Linux: не путать номера

U-Boot и Linux могут называть одни и те же физические устройства по-разному.

В нашем Linux после загрузки было:

```text
mmcblk0  microSD
mmcblk1  встроенная eMMC
```

В U-Boot это обычно выглядит как `mmc 0`, `mmc 1`, но всегда нужно проверять
фактический вывод, а не угадывать.

Команда:

```text
mmc list
```

Что показывает:

- какие MMC-контроллеры видит U-Boot;
- какие устройства соответствуют microSD и eMMC;
- иногда пометки вроде `SD` или `eMMC`.

Место для результата:

```text
mmc 0:
mmc 1:
```

Команда:

```text
mmc dev 0
```

Что делает:

- выбирает MMC-устройство номер 0 для следующих команд;
- не записывает данные на карту;
- только меняет текущий выбранный контекст в U-Boot.

Ожидаемый смысл для нашей загрузки: `mmc 0` должен быть microSD. Но конкретный
раздел, с которого U-Boot читает `/boot/uEnv.txt` и файлы ядра, нужно проверить
отдельно. В нашем образе `mmc 0:1` - маленький FAT-раздел `BOOT` с
`sysconf.txt`, а Linux rootfs находится на `mmc 0:3`.

## 9. Таблица разделов через U-Boot

Команда:

```text
mmc part
```

Что показывает:

- таблицу разделов выбранного MMC-устройства;
- номера разделов в формате U-Boot;
- типы и размеры разделов.

Ожидаемая структура для microSD с нашим образом:

```text
mmc 0
  partition 1 -> BOOT, FAT
  partition 2 -> swap
  partition 3 -> rootfs, ext4
```

Связь с Linux:

```text
U-Boot:  mmc 0:1       Linux: /dev/mmcblk0p1   /boot/firmware
U-Boot:  mmc 0:2       Linux: /dev/mmcblk0p2   swap
U-Boot:  mmc 0:3       Linux: /dev/mmcblk0p3   /
```

Модель:

```text
mmc 0:1
 |   |
 |   +-- номер раздела на выбранном MMC-устройстве
 +------ номер MMC-устройства в U-Boot
```

Место для результата:

```text
partition 1:
partition 2:
partition 3:
```

## 10. Просмотр файлов на BOOT и rootfs-разделах

Команда:

```text
ls mmc 0:1 /
```

Что делает:

- читает корень первого раздела на `mmc 0`;
- не монтирует раздел как Linux;
- показывает файлы через драйвер файловой системы U-Boot.

Ожидаемые знакомые файлы:

```text
ID.txt
START.HTM
sysconf.txt
services/
```

Это тот же физический раздел, который Linux после загрузки показывает как
`/dev/mmcblk0p1` и монтирует в `/boot/firmware`. В нашем образе он нужен для
первичной настройки через `sysconf.txt`, но kernel и `/boot/uEnv.txt` нужно
искать на rootfs-разделе.

Команда:

```text
ls mmc 0:3 /
```

Что проверяем:

```text
etc/
home/
boot/
usr/
var/
```

Команда:

```text
ls mmc 0:3 /boot
```

Что ищем на rootfs-разделе:

```text
uEnv.txt
vmlinuz-6.12.76-bone50
initrd.img-6.12.76-bone50
dtbs/
```

Если `ls mmc 0:3 /boot` не находит файлы, нужно внимательно сверить вывод
`mmc part` и переменные `bootpart`, `bootdir`, `bootfile`, `fdtfile` в
`printenv`. В разных образах структура может отличаться, но в нашей успешной
загрузке U-Boot явно читал `/boot/uEnv.txt` и файлы ядра.

## 11. printenv: читать, но не менять

Команда:

```text
printenv
```

Она выводит все переменные окружения. Вывод может быть длинным, поэтому
полезно искать смысловые группы.

Что искать глазами:

```text
bootcmd
bootdelay
bootargs
console
mmcdev
bootpart
loadaddr
fdtaddr
rdaddr
fdtfile
```

Мини-модель адресов:

```text
microSD/eMMC files
        |
        v
U-Boot читает файлы в RAM
        |
        v
loadaddr   -> kernel image
fdtaddr    -> Device Tree Blob
rdaddr     -> initrd
```

Почему это важно: Linux kernel не читает сам себя с карты на этом этапе.
Сначала U-Boot кладет kernel, Device Tree и initrd в RAM, затем командой
загрузки передает управление ядру.

Место для результата:

```text
bootcmd:
bootdelay:
console:
mmcdev:
bootpart:
fdtfile:
loadaddr:
fdtaddr:
rdaddr:
```

## 12. bootargs и /proc/cmdline

`bootargs` - это строка параметров, которую U-Boot передает Linux kernel.

После загрузки Linux мы уже видели:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
```

В U-Boot нужно найти, где формируется эта строка. Это может быть прямая
переменная `bootargs` или набор переменных, которые склеиваются boot-скриптом.

Практический вывод после сравнения:

```text
U-Boot environment / boot script
        |
        v
kernel command line
        |
        v
/proc/cmdline внутри Linux
```

Особенно важные параметры:

- `console=ttyS0,115200n8` - консоль Linux на debug UART.
- `root=/dev/mmcblk0p3` - корневая файловая система на третьем разделе microSD.
- `rootfstype=ext4` - тип root filesystem.
- `rootwait` - ждать появления MMC-устройства.

## 13. Вернуться к обычной загрузке

Когда команды просмотрены, продолжить загрузку:

```text
boot
```

Ожидаемый результат:

```text
Starting kernel ...
Booting Linux on physical CPU 0x0
Linux version 6.12.76-bone50
Debian GNU/Linux 12 BeagleBone ttyS0
BeagleBone login:
```

После входа в Linux выполнить контроль:

```sh
cat /proc/cmdline
lsblk
```

Смысл проверки: сопоставить то, что мы увидели в U-Boot, с тем, как Linux
фактически загрузился.

## 14. Таблица наблюдений для занятия

Заполнить после выполнения практики.

```text
Параметр                         Наблюдение
---------------------------------------------------------------
Версия U-Boot
Какой MMC является microSD
Какой MMC является eMMC
Где находится BOOT-раздел
Где находится /boot/uEnv.txt
Какой kernel загружает U-Boot
Какой DTB загружает U-Boot
Какой initrd загружает U-Boot
Что передано в root=
Что передано в console=
```

## 15. Контрольные вопросы

1. Почему Boot ROM не загружает Linux kernel напрямую?
2. Зачем нужен SPL / MLO перед полным U-Boot?
3. Чем `mmc 0:1` в U-Boot отличается от `/dev/mmcblk0p1` в Linux?
4. Почему `sysconf.txt` не влияет на выбор kernel и Device Tree?
5. Какой параметр говорит Linux, где искать root filesystem?
6. Почему во время этого занятия не выполняем `saveenv`?

## 16. Мини-практика

Выполнить без изменения настроек:

```text
1. Остановить autoboot.
2. Выполнить version и записать версию U-Boot.
3. Выполнить mmc list и определить microSD/eMMC.
4. Выполнить mmc dev 0 и mmc part.
5. Найти BOOT-раздел через ls mmc 0:1 /.
6. Проверить rootfs через ls mmc 0:3 /.
7. Найти /boot/uEnv.txt и файлы kernel/initrd/DTB через ls mmc 0:3 /boot.
8. Выполнить printenv и выписать bootcmd, bootargs, fdtfile.
9. Выполнить boot и убедиться, что Linux загрузился.
10. Сравнить /proc/cmdline с найденными переменными U-Boot.
```

Готовый результат занятия - не измененная плата, а понимание связки:

```text
U-Boot environment
  -> boot files on microSD
  -> kernel command line
  -> Linux root filesystem
```
