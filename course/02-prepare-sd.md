# Подготовка Linux-хоста и microSD

Этот раздел - учебный сценарий подготовки bootable microSD для BeagleBone
Black. Мы сознательно используем командную строку, чтобы видеть границы между
файлом образа, разделами, файловыми системами и блочным устройством.

Главная идея: образ Linux записывается не "в папку" и не "на раздел", а на
устройство microSD целиком. Для карты `/dev/mmcblk0` целью записи будет именно
`/dev/mmcblk0`, а не `/dev/mmcblk0p1` или `/dev/mmcblk0p2`.

## Блочное устройство, раздел и точка монтирования

Перед работой с microSD нужно разделять несколько понятий.

```text
/dev/mmcblk0     вся microSD целиком
/dev/mmcblk0p1   первый раздел на microSD
/dev/mmcblk0p2   второй раздел на microSD
/dev/mmcblk0p3   третий раздел на microSD
```

`/dev/mmcblk0p1` - это не папка. Это специальный файл устройства, через который
Linux получает доступ к сырым блокам первого раздела.

Команда `lsblk` показывает структуру блочного устройства:

```text
карта -> разделы -> размеры -> типы файловых систем -> mount points
```

Если `lsblk` показывает разделы и их размеры, это значит, что Linux прочитал
таблицу разделов. Но это еще не значит, что файлы внутри разделов доступны как
обычные файлы в дереве каталогов.

Чтобы увидеть файлы внутри раздела, файловую систему нужно смонтировать:

```sh
sudo mount /dev/mmcblk0p1 /tmp/bbb-boot
```

Эта команда означает:

```text
взять файловую систему с раздела /dev/mmcblk0p1
и сделать ее содержимое видимым через папку /tmp/bbb-boot
```

После этого:

```sh
ls /tmp/bbb-boot
```

покажет файлы с `BOOT`-раздела.

Колонка `MOUNTPOINTS` в `lsblk` показывает, через какую папку сейчас доступна
файловая система раздела. Если `MOUNTPOINTS` пустой, раздел существует и его
тип может быть распознан, но он не подключен ни к какой папке.

Модель:

```text
блочное устройство -> драйвер файловой системы -> точка монтирования -> обычные файлы
```

Еще одна полезная аналогия: драйвер файловой системы похож на переводчик между
сырыми блоками и человечески понятной структурой файлов.

```text
сырые блоки на /dev/mmcblk0p1
        |
        v
драйвер файловой системы vfat
        |
        v
каталоги, имена файлов, размеры, содержимое
        |
        v
/tmp/bbb-boot
```

Важно: это делает не сам раздел. Раздел - это область на носителе. Файловая
система `vfat` - это формат организации данных внутри этой области. Драйвер
`vfat` в Linux умеет читать и записывать этот формат.

Для нашего `BOOT`-раздела:

```text
раздел:                 /dev/mmcblk0p1
формат файловой системы: vfat
драйвер Linux:           vfat
точка монтирования:      /tmp/bbb-boot
```

Для записи образа через `dd` разделы должны быть не смонтированы. Для
редактирования `sysconf.txt` наоборот нужно смонтировать `BOOT`-раздел.

## 1. Инструменты на Linux-хосте

Debian/Ubuntu-хост:

```sh
sudo apt update
sudo apt install xz-utils util-linux coreutils picocom wget
```

В этом курсе используются:

- `lsblk` - найти microSD и проверить mount points.
- `xz` и `xzcat` - проверить и распаковать `.img.xz`.
- `sha256sum` - проверить целостность скачанного образа.
- `dd` - записать raw image на блочное устройство.
- `sync` - дождаться сброса буферов записи.
- `picocom` или `minicom` - открыть UART-консоль платы.

GUI-варианты вроде BeagleBoard Imaging Utility или balenaEtcher допустимы, но
для курса полезнее ручной путь: он показывает, что именно происходит.

## 2. Скачать официальный образ

Для текущего этапа выбран образ:

```text
BeagleBone Black Debian 12.13 2026-03-17 IoT (v6.12.x)
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Рабочий каталог проекта:

```sh
cd ~/projects/BeagleBone_Black/images
```

Скачать образ:

```sh
wget -O am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz \
  https://files.beagle.cc/file/beagleboard-public-2021/images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Скачать checksum:

```sh
wget -O am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.sha256sum \
  https://files.beagle.cc/file/beagleboard-public-2021/images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.sha256sum
```

Ожидаемая контрольная сумма для выбранного образа:

```text
4d6e46f33af68ab0b4ea1a9f7667934a5097ae1795af6515b17de1cdf5a85109
```

## 3. Проверить образ до записи

Проверить размер и тип файла:

```sh
ls -lh am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
file am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Нормальный файл имеет размер сотни мегабайт и определяется как XZ-compressed
data. Если файл имеет размер `0` или `file` пишет `empty`, запись запрещена:
это не образ.

Проверить SHA256:

```sh
sha256sum -c am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.sha256sum
```

Успешный результат:

```text
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz: ЦЕЛ
```

Проверить сам XZ-контейнер:

```sh
xz -t am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Успешный результат: команда завершается без вывода.

Типичная ошибка при плохой загрузке:

```text
xz: ...img.xz: Формат файла не опознан
```

В таком случае нужно проверить `ls -lh` и `file`, удалить или переименовать
плохой файл, скачать образ заново и снова пройти checksum.

## 4. Найти устройство microSD

Сначала выполнить команду без карты:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MODEL,MOUNTPOINTS
```

Затем вставить microSD и выполнить ее снова:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MODEL,MOUNTPOINTS
```

В нашей лабораторной сессии новое устройство было:

```text
/dev/mmcblk0      58,1G disk
```

Признаки, что это microSD:

- устройство появилось только после подключения карты;
- размер соответствует карте;
- у него есть разделы вида `/dev/mmcblk0p1`, `/dev/mmcblk0p2`;
- системный NVMe-диск `/dev/nvme0n1` уже был до подключения карты.

Правильная цель для записи:

```text
/dev/mmcblk0
```

Неправильные цели:

```text
/dev/mmcblk0p1
/dev/mmcblk0p2
/dev/nvme0n1
```

## 5. Размонтировать разделы карты

Если у разделов microSD есть mount points, их нужно размонтировать:

```sh
sudo umount /dev/mmcblk0p1 /dev/mmcblk0p2
```

Если система отвечает `not mounted`, это не ошибка для нашего сценария: значит
разделы уже не смонтированы.

Проверить:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MODEL,MOUNTPOINTS
```

Перед записью у разделов microSD поле `MOUNTPOINTS` должно быть пустым.

## 6. Записать образ на microSD

Находясь в каталоге `~/projects/BeagleBone_Black/images`, выполнить:

```sh
xzcat am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz | \
  sudo dd of=/dev/mmcblk0 bs=4M status=progress conv=fsync
```

Разбор команды:

- `xzcat` распаковывает `.img.xz` в поток.
- `sudo dd of=/dev/mmcblk0` пишет поток на устройство microSD целиком.
- `bs=4M` задает крупный блок записи.
- `status=progress` показывает прогресс.
- `conv=fsync` заставляет дождаться фактической записи на устройство.

После завершения:

```sh
sync
```

Учебный смысл шага: `dd` перезаписывает таблицу разделов карты и все начальные
секторы. После этого старая структура `BOOT`/`ROOTFS`, если она была на карте,
заменяется структурой из образа.

## 7. Проверить разметку после записи

Команда:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MODEL,MOUNTPOINTS
```

Ожидаемый результат для выбранного образа:

```text
/dev/mmcblk0      58,1G disk
├─/dev/mmcblk0p1    36M part vfat   BOOT
├─/dev/mmcblk0p2   512M part swap
└─/dev/mmcblk0p3     3G part ext4   rootfs
```

Что означает эта структура:

- `BOOT` - маленький FAT-раздел для начальной настройки и boot-файлов.
- `swap` - раздел под swap.
- `rootfs` - корневая файловая система Debian.

Образ имеет размер около 4 GB, поэтому на карте 58 GB после записи часть
пространства остается нераспределенной. Расширение `rootfs` можно выполнить
позже, после первой успешной загрузки и анализа штатных скриптов образа.

## 8. Задать пользователя для первого запуска

Перед первой загрузкой нужно настроить `sysconf.txt` на FAT-разделе `BOOT`.

Файл `sysconf.txt` уже находится в официальном образе BeagleBoard. Мы его не
создаем, а редактируем штатный файл первичной настройки.

Смонтировать раздел:

```sh
cd ~/projects/BeagleBone_Black
mkdir -p /tmp/bbb-boot
sudo mount /dev/mmcblk0p1 /tmp/bbb-boot
ls -la /tmp/bbb-boot
```

Проверить, что раздел действительно смонтирован:

```sh
mountpoint /tmp/bbb-boot
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINTS
```

Ожидаем увидеть связь:

```text
/dev/mmcblk0p1 ... vfat BOOT /tmp/bbb-boot
```

Сначала прочитать файл без редактирования:

```sh
sed -n '1,220p' /tmp/bbb-boot/sysconf.txt
```

Важные правила из файла:

- строки после `#` считаются комментариями и не применяются;
- рабочие настройки имеют вид `key=value`;
- файл будет прочитан при следующей загрузке и затем регенерирован, чтобы не
  хранить пароль в открытом виде.

Открыть файл на редактирование:

```sh
sudo nano /tmp/bbb-boot/sysconf.txt
```

Найти строки вида:

```text
#user_name=beagle
#user_password=FooBar
```

Убрать `#` и задать свои значения:

```text
user_name=andrey
user_password=<пароль>
timezone=Europe/Moscow
```

Пароль не записывается в отчет и не отправляется в чат.

Проверить результат без вывода пароля:

```sh
grep -nE '^(user_name|timezone)=' /tmp/bbb-boot/sysconf.txt
sudo grep -n '^user_password=' /tmp/bbb-boot/sysconf.txt >/dev/null && echo 'user_password is set'
```

Ожидаемый результат:

```text
user_name=andrey
timezone=Europe/Moscow
user_password is set
```

После сохранения:

```sh
sync
sudo umount /tmp/bbb-boot
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINTS
```

`sync` можно выполнять из любой директории: команда просит Linux сбросить
ожидающие записи из памяти на устройства хранения. `umount` отключает
`BOOT`-раздел от `/tmp/bbb-boot`. После успешного `umount` колонка
`MOUNTPOINTS` у `/dev/mmcblk0p1` снова должна быть пустой.

## 9. Правило первой загрузки

Для курса сначала загружаемся только с microSD:

1. Выключить BeagleBone Black.
2. Вставить microSD.
3. Подключить USB-UART 3.3 V к debug-разъему.
4. Зажать `S2` / `BOOT`.
5. Подать питание.
6. Отпустить кнопку после начала активности USR LED.

Пока не прошиваем eMMC. Сначала нам нужны UART-логи SPL, U-Boot, kernel и
userspace.
