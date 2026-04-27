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

Важно: команды из этого занятия выполняются на самой BeagleBone Black, а не на
Linux-хосте.

```text
правильно:   andrey@BeagleBone:~$
не то место: andrey@andrey-yunin:~/projects/BeagleBone_Black$
```

`/proc` и `/sys` - это pseudo-filesystems текущего запущенного ядра. Если
выполнить `find /proc/device-tree ...` на x86/Linux-хосте, мы будем смотреть
не Device Tree BeagleBone, а `/proc` хоста. На обычном PC пути
`/proc/device-tree` может вообще не быть.

## 2. Теория: зачем нужен Device Tree

Linux kernel один и тот же по исходному коду может запускаться на множестве
плат. Но у каждой платы разный набор устройств, адреса регистров, линии
прерываний, pinmux, clocks, regulators и внешние микросхемы.

Device Tree решает задачу описания конкретного железа вне C-кода драйверов.
Хорошая короткая формулировка: Device Tree - это паспорт железа для Linux
kernel.

В этом паспорте указано:

```text
какое устройство или контроллер есть
по какому адресу доступны его регистры
какие IRQ он использует
какие clocks и resets нужны
какие GPIO связаны с устройством
какой pinmux надо включить
какой драйвер может с ним работать
включено устройство или disabled
```

Модель:

```text
kernel driver
  -> знает, как управлять классом устройств

Device Tree
  -> описывает, где конкретное устройство находится на этой плате
```

Например, драйвер MMC-контроллера знает, как работать с OMAP/AM335x SDHCI. А
Device Tree говорит ядру:

```text
контроллер mmc0 находится по адресу 0x48060000
использует такие IRQ
имеет такие clocks
на этой плате он подключен к microSD
```

Без Device Tree ядру пришлось бы иметь отдельный board file с hardcoded
описанием каждой платы. На современных ARM/Linux-системах вместо этого обычно
используется Device Tree.

## 3. Форматы: DTS, DTSI, DTB и DTBO

В исходниках Device Tree встречаются несколько форматов:

```text
.dts   полный исходный файл описания платы
.dtsi  include-файл с общим описанием SoC или семейства плат
.dtb   скомпилированный бинарный Device Tree Blob
.dtbo  overlay, который изменяет или дополняет базовый DTB
```

Обычно логика такая:

```text
am33xx.dtsi
  -> общее описание SoC AM335x

am335x-bone-common.dtsi
  -> общее описание BeagleBone-плат

am335x-boneblack-uboot.dtb
  -> скомпилированное итоговое описание BeagleBone Black

BB-ADC-00A0.dtbo
BB-BONE-eMMC1-01-00A0.dtbo
BB-HDMI-TDA998x-00A0.dtbo
AM335X-PRU-UIO-00A0.dtbo
  -> overlays, которые U-Boot применяет перед стартом Linux
```

U-Boot загружает базовый `.dtb`, затем применяет `.dtbo`, и только после этого
передает ядру уже итоговый flattened Device Tree.

Важно: во время обычной загрузки U-Boot не собирает Device Tree с нуля из
исходных `.dts/.dtsi`. Эти исходники используются раньше, на этапе сборки ядра
или образа. Во время загрузки U-Boot работает уже с бинарными файлами:

```text
готовый DTB + выбранные DTBO overlays -> final Device Tree
```

В нашем случае:

```text
/boot/dtbs/6.12.76-bone50/am335x-boneblack-uboot.dtb
  + /boot/dtbs/6.12.76-bone50/BB-ADC-00A0.dtbo
  + /boot/dtbs/6.12.76-bone50/BB-BONE-eMMC1-01-00A0.dtbo
  + /boot/dtbs/6.12.76-bone50/BB-HDMI-TDA998x-00A0.dtbo
  + /boot/dtbs/6.12.76-bone50/AM335X-PRU-UIO-00A0.dtbo
        |
        v
final Device Tree, переданный Linux kernel
```

## 4. Структура Device Tree

Device Tree - это дерево узлов. Узел обычно описывает устройство, контроллер,
шину, область памяти или логический блок.

Упрощенный пример:

```dts
mmc0: mmc@48060000 {
    compatible = "ti,am335-sdhci";
    reg = <0x48060000 0x1000>;
    interrupts = <64>;
    status = "okay";
};
```

Ключевые элементы:

- `mmc0:` - label. Удобное имя для ссылок из других мест Device Tree.
- `mmc@48060000` - имя узла. После `@` обычно идет базовый адрес регистров.
- `compatible` - строка для выбора подходящего драйвера.
- `reg` - адрес и размер MMIO-региона или адрес устройства на шине.
- `interrupts` - линии прерываний.
- `status` - включено устройство или нет.

Типичные значения `status`:

```text
okay      устройство включено
disabled  устройство описано, но не используется
```

## 5. Как Linux находит драйвер

Ядро можно рассматривать как диспетчер между Device Tree и драйвером.

```text
Device Tree
  -> описывает железо

Kernel device model / OF subsystem
  -> читает Device Tree
  -> создает kernel devices
  -> сопоставляет devices с drivers

Driver
  -> получает устройство в probe()
  -> забирает ресурсы
  -> управляет железом
```

Драйвер в ядре обычно содержит таблицу совместимости. Упрощенно:

```c
static const struct of_device_id my_driver_of_match[] = {
    { .compatible = "vendor,device-name" },
    { }
};
```

Device Tree содержит:

```dts
some_device@12340000 {
    compatible = "vendor,device-name";
    reg = <0x12340000 0x1000>;
};
```

Kernel сравнивает `compatible` из Device Tree с таблицами драйверов. Если есть
совпадение, вызывается `probe()` драйвера, и драйвер получает ресурсы:

```text
MMIO address from reg
IRQ from interrupts
GPIO from gpios
clock from clocks
DMA channels from dmas
child devices from subnodes
```

Фраза "для устройства нужен драйвер, который понимает `ti,am335-sdhci`" значит:

```text
в Device Tree есть:
  compatible = "ti,am335-sdhci"

в драйвере есть of_match_table с:
  .compatible = "ti,am335-sdhci"

kernel видит совпадение:
  связывает этот DT node с этим драйвером
  вызывает probe()
```

`compatible` - это не имя файла драйвера и не команда. Это ключ для matching.

Это напрямую пригодится во второй части курса, когда мы будем писать драйверы:
для внешнего ADS1115 Device Tree node будет связывать I2C-устройство с
драйвером через `compatible`.

## 6. Что такое overlays

Overlay - это частичное изменение базового Device Tree.

Зачем overlays нужны на BeagleBone:

- включить или отключить встроенные блоки SoC;
- настроить pinmux для header-пинов;
- добавить cape или внешнюю плату;
- выбрать PRU mode;
- включить ADC, HDMI, eMMC и другие виртуальные capes.

Упрощенная модель:

```text
base DTB
  + BB-ADC-00A0.dtbo
  + BB-BONE-eMMC1-01-00A0.dtbo
  + BB-HDMI-TDA998x-00A0.dtbo
  + AM335X-PRU-UIO-00A0.dtbo
        |
        v
final Device Tree для Linux
```

В нашем boot log U-Boot явно загружал overlays:

```text
BB-ADC-00A0.dtbo
BB-BONE-eMMC1-01-00A0.dtbo
BB-HDMI-TDA998x-00A0.dtbo
AM335X-PRU-UIO-00A0.dtbo
```

В `/boot/uEnv.txt` это управляется строками:

```text
enable_uboot_overlays=1
uboot_overlay_pru=AM335X-PRU-UIO-00A0.dtbo
```

И комментариями:

```text
#disable_uboot_overlay_emmc=1
#disable_uboot_overlay_video=1
#disable_uboot_overlay_adc=1
```

Если строка `disable_uboot_overlay_adc=1` закомментирована, auto-load ADC
overlay не отключен. Поэтому U-Boot может загрузить `BB-ADC-00A0.dtbo`.

## 7. Device Tree и root filesystem

Device Tree не описывает содержимое Debian rootfs. Он описывает железо.

Разделение:

```text
Device Tree:
  SoC blocks, buses, devices, interrupts, pinmux, clocks

Kernel command line:
  root=/dev/mmcblk0p3, console=ttyS0, rootwait

Root filesystem:
  /bin, /sbin, /etc, /usr, /lib, systemd, userspace
```

U-Boot передает ядру и Device Tree, и command line. Но монтирование rootfs
делает уже Linux kernel.

## 8. Device Tree не гарантирует активность устройства

U-Boot не устанавливает и не активирует все устройства из образа. Его роль в
этой части загрузки:

```text
загрузить kernel
загрузить базовый Device Tree
применить overlays
передать bootargs
стартовать kernel
```

Дальше Linux kernel читает итоговый Device Tree и пытается поднять описанные
устройства.

Возможные состояния:

```text
status = "disabled"
  -> устройство описано, но Linux его не поднимает

status = "okay", но драйвера нет
  -> устройство есть в DT, но работать не будет

status = "okay", драйвер есть, probe() успешен
  -> устройство инициализировано и активно

status = "okay", драйвер есть, probe() failed
  -> устройство найдено, но не поднялось из-за ошибки ресурсов
```

Поэтому правильная проверка всегда идет по цепочке:

```text
Device Tree node
  -> status
  -> compatible
  -> driver match
  -> probe()
  -> runtime evidence in dmesg / sysfs / devfs
```

Пример MMC в нашей практике:

```text
DT node есть
compatible = "ti,am335-sdhci"
status = "okay"
dmesg показывает sdhci-omap и mmc0/mmc1
Linux видит mmcblk0/mmcblk1
```

Пример ADC:

```text
DT node есть
compatible = "ti,am3359-adc"
BB-ADC overlay применен
IIO device появился
in_voltage0_raw ... in_voltage7_raw есть
```

Итоговая формулировка:

```text
U-Boot подготавливает описание железа.
Linux по этому описанию создает устройства и запускает драйверы.
Активность устройства подтверждается только runtime-признаками Linux.
```

## 9. Device Tree настраивает только то, что драйвер читает

Device Tree - это входные данные для драйвера, но не магический механизм
изменения любого поведения. Свойство в Device Tree влияет на работу устройства
только если драйвер это свойство поддерживает и читает.

Правило:

```text
DT property существует
+ driver читает эту property
= поведение меняется

DT property существует
+ driver игнорирует эту property
= поведение не меняется
```

Для встроенного ADC примером является:

```text
ti,adc-channels = <0 1 2 3 4 5 6 7>;
```

Если драйвер поддерживает это свойство, то в `probe()` он читает список каналов
и регистрирует соответствующие IIO channels.

Модель:

```text
DT node:
  ti,adc-channels = <0>;

driver probe():
  читает property "ti,adc-channels"
  строит список каналов
  регистрирует только channel 0

userspace:
  видит in_voltage0_raw
```

Если драйвер всегда жестко создает все 8 каналов и не читает
`ti,adc-channels`, изменение Device Tree не изменит поведение. Поэтому перед
изменением Device Tree нужно смотреть binding/documentation или код драйвера:
какие свойства он реально поддерживает.

То есть опасение правильное: если в Device Tree убрать каналы, но драйвер не
использует это свойство, драйвер продолжит работать по своей внутренней логике.
Device Tree не блокирует драйвер автоматически. Он только передает данные,
которые драйвер может учесть.

Правильное инженерное разделение:

```text
Device Tree:
  что есть на этой плате и как это сконфигурировано

Driver:
  как управлять этим типом железа и какие DT properties учитывать

Userspace:
  как использовать предоставленный kernel ABI
```

Переписывать драйвер имеет смысл, когда штатный драйвер не поддерживает нужный
режим, содержит ошибку или когда мы пишем поддержку нового внешнего устройства.
Если нужно только изменить конфигурацию платы, сначала проверяют возможность
сделать это через Device Tree overlay.

## 10. Device Tree в работающей Linux-системе

После старта Linux итоговый Device Tree доступен как pseudo-filesystem:

```text
/proc/device-tree
/sys/firmware/devicetree/base
```

Это не исходный `.dts` файл, а отображение бинарного DTB в виде каталогов и
файлов. Каждый каталог - узел, каждый файл - property.

Пример:

```text
/proc/device-tree/model
/proc/device-tree/compatible
/proc/device-tree/ocp/mmc@48060000/compatible
/proc/device-tree/ocp/mmc@48060000/reg
```

Некоторые properties бинарные, поэтому их нельзя всегда читать обычным `cat`.
Для строк удобно:

```sh
tr -d '\0' < /proc/device-tree/model; echo
```

Для бинарных чисел лучше использовать:

```sh
hexdump -C /proc/device-tree/.../reg
```

## 11. Kernel command line

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

## 12. Модель платы из Device Tree

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

## 13. Как правильно обходить /proc/device-tree

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

## 14. Kernel log

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

## 15. /boot и /boot/uEnv.txt

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

## 16. MMC/eMMC со стороны Linux

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

## 17. Первое чтение ADC через IIO

Команда:

```sh
for f in /sys/bus/iio/devices/iio:device0/in_voltage*_raw; do
  echo -n "$(basename "$f"): "
  cat "$f"
done
```

Зачем:

- проверить, что ADC не только описан в Device Tree, но и реально доступен
  через Linux IIO ABI;
- прочитать raw-коды, которые возвращает драйвер ADC;
- подготовить переход от Device Tree к практическому измерению напряжения.

Результат практики:

```text
in_voltage0_raw: 2421
in_voltage1_raw: 1721
in_voltage2_raw: 2389
in_voltage3_raw: 244
in_voltage4_raw: 299
in_voltage5_raw: 455
in_voltage6_raw: 444
in_voltage7_raw: 3850
```

Как это читать:

- `in_voltageN_raw` - это не напряжение в вольтах, а сырой ADC code;
- ADC AM335x 12-битный, поэтому нормальный диапазон raw-кода: `0..4095`;
- для BeagleBone Black диапазон аналогового входа ADC: `0..1.8 V`;
- примерная оценка напряжения: `voltage ~= raw * 1.8 / 4096`.

Примерная интерпретация результата:

```text
in_voltage0_raw: 2421 -> около 1.064 V
in_voltage1_raw: 1721 -> около 0.756 V
in_voltage2_raw: 2389 -> около 1.050 V
in_voltage3_raw: 244  -> около 0.107 V
in_voltage4_raw: 299  -> около 0.131 V
in_voltage5_raw: 455  -> около 0.200 V
in_voltage6_raw: 444  -> около 0.195 V
in_voltage7_raw: 3850 -> около 1.692 V
```

Но эти значения нельзя считать измерением датчика, если входы физически никуда
не подключены. Неподключенный ADC input находится в floating-состоянии: на него
влияют наводки, утечки, емкость входного мультиплексора и предыдущие
измерения. Поэтому числа могут быть ненулевыми и меняться от запуска к запуску.

Инженерный вывод:

```text
Device Tree включил ADC channels.
Драйвер создал IIO device.
Userspace смог прочитать raw-коды.
Но полезным измерением это станет только после подключения корректной схемы.
```

## 18. Разделение ответственности: driver и userspace

Для embedded Linux это базовая рабочая модель:

```text
Device Tree
  -> описывает железо и конфигурацию платы

Kernel driver
  -> инициализирует контроллер
  -> включает нужные ресурсы: clocks, IRQ, MMIO, pinctrl
  -> регистрирует kernel ABI

Userspace application
  -> использует готовый ABI
  -> читает данные
  -> выполняет прикладную обработку
```

В нашем случае приложение пользователя не инициализирует ADC на уровне
регистров. Если при загрузке kernel driver успешно выполнил `probe()` и создал:

```text
/sys/bus/iio/devices/iio:device0/in_voltage0_raw
...
/sys/bus/iio/devices/iio:device0/in_voltage7_raw
```

то минимальному userspace-приложению достаточно читать эти файлы и пересчитывать
raw-код в напряжение.

Приложение не делает:

```text
probe()
настройку MMIO-регистров ADC
включение clocks
регистрацию IIO channels
```

Это зона ответственности драйвера.

Но userspace может выполнять прикладную настройку, если драйвер ее экспортирует
через ABI:

```text
in_voltage_scale
sampling_frequency
buffer/trigger
channel enable
GPIO управления внешней схемой
```

Это не инициализация железа заново, а настройка уже поднятого драйвером
устройства.

Итог:

```text
Если драйвер поднял ADC и экспортировал IIO-файлы,
userspace-приложение читает данные через IIO.
Отдельная инициализация ADC из userspace не нужна.
```

### Чеклист: найти путь данных для userspace-приложения

Задача: мы пишем приложение пользователя и хотим понять, какой файл читать,
чтобы получить данные ADC.

#### Шаг 1. Убедиться, что IIO bus есть в системе

```sh
ls -la /sys/bus/iio/devices
```

Зачем:

- проверить, что подсистема IIO доступна;
- увидеть все IIO-устройства, созданные драйверами;
- найти candidates вида `iio:device0`, `iio:device1` и т.д.

В нашей практике был найден путь:

```text
/sys/bus/iio/devices/iio:device0
```

#### Шаг 2. Понять, какое `iio:deviceX` является ADC

```sh
for dev in /sys/bus/iio/devices/iio:device*; do
  echo "=== $dev ==="
  cat "$dev/name" 2>/dev/null
done
```

Зачем:

- `iio:device0` - это не стабильное имя конкретного железа, а runtime index;
- приложение не должно слепо полагаться на номер `0`, если в системе может
  появиться больше IIO-устройств;
- файл `name` помогает сопоставить `iio:deviceX` с драйвером.

Для нашего ADC ожидаем:

```text
TI-am335x-adc.0.auto
```

#### Шаг 3. Зафиксировать путь найденного ADC в переменную

Для текущей системы:

```sh
adc_dev=/sys/bus/iio/devices/iio:device0
cat "$adc_dev/name"
```

Зачем:

- получить короткое имя переменной для следующих проверок;
- убедиться, что выбран именно ADC, а не другое IIO-устройство.

Более надежная идея для будущего приложения: искать устройство по `name`, а
номер `iio:deviceX` считать результатом обнаружения.

#### Шаг 4. Посмотреть физический путь устройства в sysfs

```sh
readlink -f "$adc_dev"
```

Зачем:

- увидеть, к какому platform device привязан IIO device;
- связать userspace path с тем, что ранее видели в Device Tree и `dmesg`;
- подтвердить, что это не абстрактный файл, а ABI поверх реального драйвера.

Ожидаемая логика пути:

```text
/sys/devices/platform/.../44e0d000.tscadc/.../iio:device0
```

#### Шаг 5. Увидеть файлы, которые может читать приложение

```sh
ls "$adc_dev"/in_voltage*_raw
```

Зачем:

- получить список каналов, экспортированных драйвером;
- увидеть конкретные файлы, которые может открыть userspace-приложение.

Для нашей системы:

```text
/sys/bus/iio/devices/iio:device0/in_voltage0_raw
...
/sys/bus/iio/devices/iio:device0/in_voltage7_raw
```

Это и есть основные пути чтения данных.

#### Шаг 6. Проверить scale для пересчета raw-кода

```sh
cat "$adc_dev/in_voltage_scale"
```

Зачем:

- получить коэффициент пересчета raw-кода в физическую величину;
- не зашивать в приложение приблизительное `1.8 / 4096`, если драйвер уже
  экспортирует scale;
- использовать стандартную IIO-модель:

```text
voltage = raw * scale
```

Результат практики:

```text
0.439453125
```

Для `in_voltage` значение scale обычно интерпретируется в millivolts per ADC
count. Значит:

```text
voltage_mV = raw * 0.439453125
voltage_V  = voltage_mV / 1000.0
```

Проверка полного диапазона:

```text
4096 * 0.439453125 = 1800 mV = 1.8 V
```

Это совпадает с допустимым диапазоном ADC BeagleBone Black `0..1.8 V`.

Пример для ранее прочитанного значения `in_voltage0_raw = 2421`:

```text
2421 * 0.439453125 = 1063.92 mV = 1.064 V
```

#### Шаг 7. Прочитать один канал так, как это сделает приложение

```sh
cat "$adc_dev/in_voltage0_raw"
```

Зачем:

- проверить минимальный путь данных;
- убедиться, что файл читается без ошибок;
- увидеть, какой именно файл будет открывать приложение для канала 0.

Для приложения это превращается в простую модель:

```text
open("/sys/bus/iio/devices/iio:device0/in_voltage0_raw")
read()
parse integer
```

#### Шаг 8. Прочитать все каналы

```sh
for f in "$adc_dev"/in_voltage*_raw; do
  echo -n "$(basename "$f"): "
  cat "$f"
done
```

Зачем:

- проверить, что все каналы, созданные драйвером, реально читаются;
- увидеть текущие raw-коды;
- быстро проверить будущую схему подключения датчика.

Итоговый путь данных:

```text
AM335x ADC hardware
  -> kernel driver
  -> IIO sysfs ABI
  -> /sys/bus/iio/devices/iio:device0/in_voltageN_raw
  -> userspace application
```

Важно: на аналоговые входы BeagleBone Black нельзя подавать `3.3 V` или `5 V`.
Для практики со встроенным ADC сначала проектируем делитель/защиту и работаем
только в диапазоне `0..1.8 V`.

## 19. Контрольные вопросы

1. Зачем Device Tree нужен на ARM/Linux-платах?
2. Чем `.dts`, `.dtsi`, `.dtb` и `.dtbo` отличаются друг от друга?
3. Как свойство `compatible` связывает Device Tree node и kernel driver?
4. Что меняет overlay и почему BeagleBone активно использует overlays?
5. Почему `status = "okay"` еще не гарантирует, что устройство реально
   работает?
6. Какими runtime-признаками Linux можно подтвердить, что драйвер поднял
   устройство?
7. Почему Device Tree property меняет поведение только если драйвер ее читает?
8. Почему для изменения набора ADC-каналов лучше сначала смотреть
   `ti,adc-channels`, а не переписывать драйвер?
9. Почему userspace-приложение обычно не инициализирует ADC напрямую, если
   драйвер уже создал IIO device?
10. Почему `/proc/cmdline` является главным доказательством того, какие bootargs
   получил Linux?
11. Почему `find /proc/device-tree ...` может ничего не вывести без `-L`?
12. Почему в `dmesg` может не быть отдельных строк о применении U-Boot overlays?
13. Чем `/boot/uEnv.txt` отличается от `/boot/firmware/sysconf.txt`?
14. Почему наличие `mmcblk1p1` с label `rootfs` не означает загрузку с eMMC?
15. Какая строка в `/boot/uEnv.txt` включает eMMC flasher и почему пока она
   должна оставаться закомментированной?

## 20. Мини-практика

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
