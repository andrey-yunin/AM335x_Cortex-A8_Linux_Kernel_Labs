# Общий отчет по BeagleBone Black

Работы ведутся по датам. В отчет заносятся выполненные команды, полученные
результаты и краткое пояснение, зачем команда выполнялась и что из результата
следует.

## 2026-04-23

### Цель этапа

Начать подготовку Linux-хоста к установке Linux на BeagleBone Black и зафиксировать
исходное состояние перед подключением microSD.

### Принятое техническое решение

Для учебного курса выбран официальный Debian-образ BeagleBoard для BeagleBone
Black:

```text
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Причина выбора: Debian является основным поддерживаемым вариантом для BeagleBone
Black, а версия с ядром v6.12.x подходит для учебной работы с boot chain,
U-Boot, kernel, Device Tree и eMMC.

### Проверка утилит на Linux-хосте

Команда:

```sh
which lsblk xzcat sha256sum dd wget minicom
```

Результат:

```text
/usr/bin/lsblk
/usr/bin/xzcat
/usr/bin/sha256sum
/usr/bin/dd
/usr/bin/wget
/usr/bin/minicom
```

Пояснение:

- `lsblk` нужен для безопасного определения microSD-устройства.
- `xzcat` нужен для распаковки `.img.xz` образа при записи.
- `sha256sum` нужен для проверки целостности скачанного образа.
- `dd` нужен для низкоуровневой записи образа на карту.
- `wget` нужен для скачивания образа.
- `minicom` нужен для UART-консоли и просмотра этапов SPL, U-Boot и kernel.

Вывод: необходимые утилиты на хосте установлены.

### Снимок блочных устройств до подключения microSD

Команда:

```sh
lsblk -p -o NAME,SIZE,TYPE,MODEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE MODEL                          MOUNTPOINTS
/dev/nvme0n1     476,9G disk Micron MTFDKCD512TGE-1BK1AABLA 
├─/dev/nvme0n1p1   512M part                                /boot/efi
└─/dev/nvme0n1p2 476,4G part                                /
```

Пояснение:

До подключения microSD виден только системный NVMe-диск:

```text
/dev/nvme0n1
```

Этот диск нельзя использовать как цель для записи образа. После подключения
microSD нужно повторить `lsblk` и найти новое устройство.

### Следующий шаг

Подключить microSD к Linux-хосту и повторить команду:

```sh
lsblk -p -o NAME,SIZE,TYPE,MODEL,MOUNTPOINTS
```

По разнице между снимками будет определено имя microSD-устройства. Для записи
образа понадобится устройство целиком, например `/dev/sdb` или `/dev/mmcblk0`,
а не раздел вроде `/dev/sdb1` или `/dev/mmcblk0p1`.

## 2026-04-24

### Цель этапа

Подключить microSD, безопасно определить ее имя в Linux, проверить скачанный
образ BeagleBoard и записать его на карту. В учебном материале этот этап
показывает границу между файловой системой и блочным устройством: мы не копируем
файл на раздел, а перезаписываем образ на устройство microSD целиком.

### Определение microSD после подключения

Команда:

```sh
lsblk -p -o NAME,SIZE,TYPE,MODEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE MODEL                          MOUNTPOINTS
/dev/mmcblk0      58,1G disk                                
├─/dev/mmcblk0p1   512M part                                /media/andrey/BOOT
└─/dev/mmcblk0p2  57,6G part                                /media/andrey/ROOTFS
/dev/nvme0n1     476,9G disk Micron MTFDKCD512TGE-1BK1AABLA 
├─/dev/nvme0n1p1   512M part                                /boot/efi
└─/dev/nvme0n1p2 476,4G part                                /
```

Вывод:

- Новое устройство - `/dev/mmcblk0`, размер 58,1G.
- `/dev/mmcblk0p1` и `/dev/mmcblk0p2` - разделы на microSD.
- Для записи образа нужна цель `/dev/mmcblk0`, а не `p1` или `p2`.
- Системный диск `/dev/nvme0n1` не трогаем.

### Размонтирование разделов microSD

Команда:

```sh
sudo umount /dev/mmcblk0p1 /dev/mmcblk0p2
```

Результат:

```text
umount: /dev/mmcblk0p1: not mounted.
umount: /dev/mmcblk0p2: not mounted.
```

Контрольная проверка:

```sh
lsblk -p -o NAME,SIZE,TYPE,MODEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE MODEL                          MOUNTPOINTS
/dev/mmcblk0      58,1G disk                                
├─/dev/mmcblk0p1   512M part                                
└─/dev/mmcblk0p2  57,6G part                                
/dev/nvme0n1     476,9G disk Micron MTFDKCD512TGE-1BK1AABLA 
├─/dev/nvme0n1p1   512M part                                /boot/efi
└─/dev/nvme0n1p2 476,4G part                                /
```

Вывод: разделы microSD не смонтированы, карту можно перезаписывать.

### Обнаружение некорректно скачанного образа

Проверка `.xz`-архива:

```sh
xz -t images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Результат:

```text
xz: images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz: Формат файла не опознан
```

Проверка размера и типа файла:

```sh
ls -lh images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
file images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Результат:

```text
-rw-rw-r-- 1 andrey andrey 0 апр 23 14:49 images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz: empty
```

Вывод: файл образа был пустым. Записывать такой файл на microSD нельзя; сначала
нужно заново скачать образ и проверить его целостность.

### Повторная загрузка образа и checksum

Переход в каталог образов:

```sh
cd ~/projects/BeagleBone_Black/images
```

Пустой файл сохранен для диагностики:

```sh
mv am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz \
  am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.empty
```

Повторное скачивание:

```sh
wget -O am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz \
  https://files.beagle.cc/file/beagleboard-public-2021/images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Ключевой результат `wget`:

```text
Длина: 363061572 (346M) [application/x-xz]
сохранён [363061572/363061572]
```

Скачивание checksum:

```sh
wget -O am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.sha256sum \
  https://files.beagle.cc/file/beagleboard-public-2021/images/am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.sha256sum
```

Контрольная сумма:

```text
4d6e46f33af68ab0b4ea1a9f7667934a5097ae1795af6515b17de1cdf5a85109  am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Проверка checksum:

```sh
sha256sum -c am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz.sha256sum
```

Результат:

```text
am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz: ЦЕЛ
```

Проверка контейнера XZ:

```sh
xz -t am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Результат: команда завершилась без вывода, значит XZ-архив читается нормально.

Размер файла после повторной загрузки:

```sh
ls -lh am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Результат:

```text
-rw-rw-r-- 1 andrey andrey 347M апр 24 10:14 am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz
```

Вывод: образ скачан корректно и готов к записи.

### Запись образа на microSD

Контроль перед записью:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MODEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE FSTYPE LABEL  MODEL                          MOUNTPOINTS
/dev/mmcblk0      58,1G disk                                              
├─/dev/mmcblk0p1   512M part vfat   BOOT                                  
└─/dev/mmcblk0p2  57,6G part ext4   ROOTFS                                
/dev/nvme0n1     476,9G disk               Micron MTFDKCD512TGE-1BK1AABLA 
├─/dev/nvme0n1p1   512M part vfat                                         /boot/efi
└─/dev/nvme0n1p2 476,4G part ext4                                         /
```

Команда записи:

```sh
xzcat am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz | \
  sudo dd of=/dev/mmcblk0 bs=4M status=progress conv=fsync
```

Итоговый результат `dd`:

```text
0+419299 записей получено
0+419299 записей отправлено
3774873600 байтов (3,8 GB, 3,5 GiB) скопировано, 106,197 s, 35,5 MB/s
```

Пояснение:

- `xzcat` распаковывает образ в поток.
- `dd of=/dev/mmcblk0` пишет поток на microSD целиком.
- `conv=fsync` заставляет дождаться сброса данных на устройство.
- Старая таблица разделов на карте заменяется таблицей из образа.

### Проверка результата записи

Команды:

```sh
sync
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MODEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE FSTYPE LABEL  MODEL                          MOUNTPOINTS
/dev/mmcblk0      58,1G disk                                              
├─/dev/mmcblk0p1    36M part vfat   BOOT                                  
├─/dev/mmcblk0p2   512M part swap                                         
└─/dev/mmcblk0p3     3G part ext4   rootfs                                
/dev/nvme0n1     476,9G disk               Micron MTFDKCD512TGE-1BK1AABLA 
├─/dev/nvme0n1p1   512M part vfat                                         /boot/efi
└─/dev/nvme0n1p2 476,4G part ext4                                         /
```

Вывод: microSD успешно записана. Ожидаемая структура после записи:

- `BOOT` - FAT-раздел с начальными настройками и файлом `sysconf.txt`.
- `swap` - swap-раздел.
- `rootfs` - Linux root filesystem.

Образ занимает первые несколько гигабайт карты. Остальное пространство 58G
microSD пока не используется образом напрямую; расширение root filesystem можно
рассмотреть отдельно после первой загрузки.

### План перед первой загрузкой

После записи карты следующим шагом была настройка пользователя на `BOOT`-разделе
через `sysconf.txt`:

```sh
mkdir -p /tmp/bbb-boot
sudo mount /dev/mmcblk0p1 /tmp/bbb-boot
sudo nano /tmp/bbb-boot/sysconf.txt
sync
sudo umount /tmp/bbb-boot
```

В `sysconf.txt` нужно раскомментировать и заполнить строки `user_name=` и
`user_password=`. Пароль не фиксируется в отчете.

Фактическое выполнение этого шага зафиксировано ниже в разделе
`Настройка sysconf.txt на BOOT-разделе`.

### Теоретическое уточнение boot chain

Зафиксировано разделение этапов загрузки:

```text
Boot ROM  - внутри AM335x SoC, изменить нельзя
SPL / MLO - на microSD или eMMC, можно заменить
U-Boot    - на microSD или eMMC, можно заменить и настроить
Linux     - на разделах образа, можно заменить и настроить
rootfs    - корневая файловая система Debian
userspace - systemd, сервисы, login, SSH и пользовательские программы
```

Учебный вывод:

- Boot ROM - заводской код внутри процессора AM335x.
- Boot ROM читает boot-пины `SYSBOOT`, выбирает источник загрузки и запускает
  SPL / MLO.
- SPL / MLO - первый загрузчик на microSD/eMMC; он инициализирует DDR3 RAM и
  запускает полный U-Boot.
- U-Boot загружает Linux kernel, Device Tree и передает управление ядру.
- `sysconf.txt` не является загрузчиком; он применяется позже, уже после запуска
  Linux userspace.

### Настройка sysconf.txt на BOOT-разделе

Перед первой загрузкой microSD был смонтирован `BOOT`-раздел:

```text
/dev/mmcblk0p1 -> /tmp/bbb-boot
```

Учебное пояснение:

- `/dev/mmcblk0p1` - первый раздел microSD с файловой системой `vfat` и меткой
  `BOOT`.
- `/tmp/bbb-boot` - точка монтирования на Linux-хосте.
- После монтирования файлы с `BOOT`-раздела видны как обычные файлы в
  `/tmp/bbb-boot`.
- Команда `lsblk` показывает mount point в колонке `MOUNTPOINTS`.

Проверка точки монтирования:

```sh
mountpoint /tmp/bbb-boot
```

Результат:

```text
/tmp/bbb-boot является точкой монтирования
```

Проверка через `lsblk`:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE FSTYPE LABEL  MOUNTPOINTS
/dev/mmcblk0      58,1G disk               
├─/dev/mmcblk0p1    36M part vfat   BOOT   /tmp/bbb-boot
├─/dev/mmcblk0p2   512M part swap          
└─/dev/mmcblk0p3     3G part ext4   rootfs 
/dev/nvme0n1     476,9G disk               
├─/dev/nvme0n1p1   512M part vfat          /boot/efi
└─/dev/nvme0n1p2 476,4G part ext4          /
```

Содержимое `BOOT`-раздела:

```sh
ls -la /tmp/bbb-boot
```

Результат:

```text
итого 45
drwxr-xr-x  3 root root   512 янв  1  1970 .
drwxrwxrwt 40 root root 36864 апр 24 11:39 ..
-rwxr-xr-x  1 root root    54 мар 17 21:29 ID.txt
drwxr-xr-x  2 root root   512 мар 17 21:29 services
-rwxr-xr-x  1 root root   204 мар 17 21:29 START.HTM
-rwxr-xr-x  1 root root  2600 мар 17 21:29 sysconf.txt
```

Вывод: `sysconf.txt` уже входит в официальный образ. Мы его не создавали, а
открыли штатный файл первичной настройки, который лежит на `BOOT`-разделе.

Файл был просмотрен без редактирования:

```sh
sed -n '1,220p' /tmp/bbb-boot/sysconf.txt
```

Ключевые строки файла:

```text
# This file will be automatically evaluated and installed at next boot
# time, and regenerated (to avoid leaking passwords and such information).
...
# user_name - Set a user name for the user (1000)
#user_name=beagle

# user_password - Set a password for user (1000)
#user_password=FooBar

# timezone - Set the system timezone.
#timezone=America/Chicago
```

Учебный вывод:

- `#` в начале строки означает комментарий; такая строка не применяется.
- Строки формата `key=value` применяются сервисом `bbbio-set-sysconf` при
  следующей загрузке.
- `user_name` задает обычного пользователя с UID 1000.
- `user_password` задает пароль этого пользователя.
- `timezone` задает часовой пояс системы.
- После применения файл регенерируется, чтобы не оставлять пароль в открытом
  виде.

В `sysconf.txt` были настроены:

```text
user_name=andrey
user_password=<задан, не фиксируется в отчете>
timezone=Europe/Moscow
```

Проверка без вывода пароля:

```sh
grep -nE '^(user_name|timezone)=' /tmp/bbb-boot/sysconf.txt
sudo grep -n '^user_password=' /tmp/bbb-boot/sysconf.txt >/dev/null && echo 'user_password is set'
```

Результат:

```text
30:user_name=andrey
54:timezone=Europe/Moscow
user_password is set
```

После настройки был выполнен сброс буферов записи и размонтирование:

```sh
sync
sudo umount /tmp/bbb-boot
```

Пояснение:

- `sync` просит Linux записать ожидающие изменения из памяти на устройства
  хранения.
- `umount` отключает файловую систему `BOOT` от точки монтирования
  `/tmp/bbb-boot` и завершает работу с разделом.

Контроль после размонтирования:

```sh
lsblk -p -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINTS
```

Результат:

```text
NAME               SIZE TYPE FSTYPE LABEL  MOUNTPOINTS
/dev/mmcblk0      58,1G disk               
├─/dev/mmcblk0p1    36M part vfat   BOOT   
├─/dev/mmcblk0p2   512M part swap          
└─/dev/mmcblk0p3     3G part ext4   rootfs 
/dev/nvme0n1     476,9G disk               
├─/dev/nvme0n1p1   512M part vfat          /boot/efi
└─/dev/nvme0n1p2 476,4G part ext4          /
```

Вывод: у `/dev/mmcblk0p1` поле `MOUNTPOINTS` пустое, значит `BOOT`-раздел
размонтирован. microSD готова к первой загрузке на BeagleBone Black.

### Повторная настройка sysconf.txt и успешная первая загрузка

При первой попытке входа в Debian пользователь `andrey` был создан, но пароль не
подошел. Для учебного процесса выбран простой и безопасный путь: не править
`/etc/shadow` вручную, а заново перезаписать microSD официальным образом,
повторно настроить `sysconf.txt` и задать простой временный пароль латиницей и
цифрами.

Учебный вывод: если на плате еще нет ценных данных, перезапись microSD часто
проще и безопаснее ручного восстановления пароля в root filesystem.

После повторной записи образа карта снова имела исходную структуру:

```text
NAME               SIZE TYPE FSTYPE LABEL  MOUNTPOINTS
/dev/mmcblk0      58,1G disk               
├─/dev/mmcblk0p1    36M part vfat   BOOT   
├─/dev/mmcblk0p2   512M part swap          
└─/dev/mmcblk0p3     3G part ext4   rootfs 
/dev/nvme0n1     476,9G disk               
├─/dev/nvme0n1p1   512M part vfat          /boot/efi
└─/dev/nvme0n1p2 476,4G part ext4          /
```

В `sysconf.txt` повторно настроены:

```text
user_name=andrey
user_password=<задан простой временный пароль, не фиксируется в отчете>
timezone=Europe/Moscow
```

Контроль:

```text
30:user_name=andrey
54:timezone=Europe/Moscow
user_password is set
```

После `sync` и `umount` у разделов microSD поле `MOUNTPOINTS` снова было пустым,
и карта была перенесена в BeagleBone Black.

### UART и этапы загрузки

На Linux-хосте USB-UART адаптер определился как `/dev/ttyUSB0`:

```text
pl2303 converter now attached to ttyUSB0
```

Также был виден `ttyACM0`, но это USB gadget самой BeagleBone Black, который
появляется уже после частичной загрузки платы. Для ранних boot-логов использован
debug UART через `/dev/ttyUSB0`.

Консоль была открыта командой:

```sh
sudo minicom -D /dev/ttyUSB0 -b 115200
```

В UART-логе подтверждены этапы загрузки:

```text
U-Boot SPL 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
U-Boot 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
CPU  : AM335X-GP rev 2.1
Model: TI AM335x BeagleBone Black
DRAM:  512 MiB
Loaded environment from /boot/uEnv.txt
loading /boot/vmlinuz-6.12.76-bone50 ...
loading /boot/dtbs/6.12.76-bone50/am335x-boneblack-uboot.dtb ...
loading /boot/initrd.img-6.12.76-bone50 ...
Starting kernel ...
Linux version 6.12.76-bone50
```

Командная строка ядра:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
```

Учебные выводы:

- Boot ROM успешно запустил SPL/MLO с microSD.
- SPL инициализировал RAM и запустил полный U-Boot.
- U-Boot прочитал `/boot/uEnv.txt`, kernel, Device Tree и initrd.
- Ядро стартовало с console на `ttyS0` и root filesystem на
  `/dev/mmcblk0p3`.
- `bbbio-set-sysconf` прочитал `/boot/firmware/sysconf.txt` и применил
  настройки пользователя.

После загрузки появился login prompt:

```text
Debian GNU/Linux 12 BeagleBone ttyS0
BeagleBoard.org Debian Bookworm Base Image 2026-03-17
default username is [andrey]
```

Вход пользователем `andrey` выполнен успешно.

### Проверка системы после входа

Команда:

```sh
whoami
```

Результат:

```text
andrey
```

Команда:

```sh
uname -a
```

Результат:

```text
Linux BeagleBone 6.12.76-bone50 #1 PREEMPT Fri Mar  6 07:09:07 UTC 2026 armv7l GNU/Linux
```

Команда:

```sh
cat /proc/cmdline
```

Результат:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
```

Команда:

```sh
lsblk
```

Результат:

```text
NAME         MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
mmcblk0      179:0    0  58.1G  0 disk 
├─mmcblk0p1  179:1    0    36M  0 part /boot/firmware
├─mmcblk0p2  179:2    0   512M  0 part [SWAP]
└─mmcblk0p3  179:3    0  57.6G  0 part /
zram0        252:0    0 241.3M  0 disk [SWAP]
mmcblk1      179:256  0   3.6G  0 disk 
└─mmcblk1p1  179:257  0   3.6G  0 part 
mmcblk1boot0 179:512  0     2M  1 disk 
mmcblk1boot1 179:768  0     2M  1 disk 
```

Вывод:

- `mmcblk0` - microSD, с которой загружена система.
- `mmcblk0p1` - `BOOT`, смонтирован как `/boot/firmware`.
- `mmcblk0p2` - swap.
- `mmcblk0p3` - root filesystem, смонтирован как `/` и расширен до 57.6G.
- `zram0` - дополнительный compressed swap в RAM.
- `mmcblk1` - встроенная eMMC BeagleBone Black; на этом этапе она только
  обнаружена, но не изменялась.

Первый целевой результат курса достигнут: BeagleBone Black загружена с microSD,
ранние этапы загрузки видны через UART, вход в Debian выполнен пользователем,
заданным через `sysconf.txt`.

## 2026-04-27

### Цель этапа

Подготовить следующий учебный блок после первой успешной загрузки: безопасную
работу в интерактивной консоли U-Boot без изменения настроек платы.

### Документация

Добавлено занятие:

```text
course/04-u-boot-environment.md
```

Содержание занятия:

- место полного U-Boot в цепочке загрузки;
- понятие U-Boot environment;
- безопасная остановка autoboot через UART;
- команды `version`, `bdinfo`, `mmc list`, `mmc dev`, `mmc part`, `printenv`,
  `ls mmc 0:1 /`, `ls mmc 0:3 /`, `ls mmc 0:3 /boot`, `boot`;
- связь между `mmc 0:1` в U-Boot и `/dev/mmcblk0p1` в Linux;
- проверка, что `/boot/uEnv.txt`, kernel, DTB и initrd находятся на том разделе,
  который реально использует U-Boot;
- связь между `bootargs` и `/proc/cmdline`;
- таблица наблюдений и контрольные вопросы для практики.

### Важное ограничение

Фактические результаты U-Boot пока не заносились, потому что команды должны
быть выполнены на физической BeagleBone Black через UART. В следующей
практической сессии нужно остановить autoboot, снять вывод безопасных команд и
добавить его в этот отчет отдельным подразделом.

### Практическое уточнение: Linux shell не является U-Boot

При попытке выполнить команды U-Boot после полной загрузки Debian был получен
ожидаемый результат:

```text
andrey@BeagleBone:~$ version
version: command not found

andrey@BeagleBone:~$ bdinfo
bdinfo: command not found
```

Учебный вывод: приглашение `andrey@BeagleBone:~$` означает, что управление уже
передано Linux userspace. Команды `version` и `bdinfo` из этой лабораторной
нужно вводить только в интерактивной консоли U-Boot, где приглашение выглядит
так:

```text
=>
```

Чтобы попасть туда из уже загруженного Linux, нужно выполнить `sudo reboot` или
перезапустить питание платы и остановить autoboot в первые секунды загрузки.

### Попытка остановки autoboot и bootdelay=0

Через правильный debug UART `/dev/ttyUSB0` был получен ранний boot log:

```text
U-Boot SPL 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
Trying to boot from MMC1

U-Boot 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
CPU  : AM335X-GP rev 2.1
Model: TI AM335x BeagleBone Black
DRAM:  512 MiB
MMC:   OMAP SD/MMC: 0, OMAP SD/MMC: 1
Press SPACE to abort autoboot in 0 seconds
```

Вывод: порт выбран правильно, потому что видны SPL и полный U-Boot. Однако
окно остановки autoboot практически отсутствует: `bootdelay` равен `0`.
Обычное нажатие `Space` после появления строки может не успеть остановить
загрузку.

Практический прием для следующей попытки: открыть `minicom` на `/dev/ttyUSB0`,
полностью снять питание с платы, зажать `Space` в окне `minicom`, затем подать
питание и держать `Space` до появления приглашения U-Boot `=>`.

В этом же логе подтверждено, что U-Boot читает загрузочные файлы с microSD:

```text
Checking for: /boot/uEnv.txt ...
Loaded environment from /boot/uEnv.txt
loading /boot/vmlinuz-6.12.76-bone50 ...
loading /boot/dtbs/6.12.76-bone50/am335x-boneblack-uboot.dtb ...
uboot_overlays: loading /boot/dtbs/6.12.76-bone50/BB-ADC-00A0.dtbo ...
loading /boot/initrd.img-6.12.76-bone50 ...
```

### U-Boot console: version, bdinfo и MMC

Autoboot был успешно остановлен. Приглашение U-Boot:

```text
=>
```

Команда:

```text
version
```

Результат:

```text
U-Boot 2022.04-g5509547b (Jan 22 2026 - 19:56:08 +0000)
arm-linux-gnueabihf-gcc (Debian 12.2.0-14+deb12u1) 12.2.0
GNU ld (GNU Binutils for Debian) 2.40
```

Вывод: версия полного U-Boot совпадает с версией, которую видели в boot log.
U-Boot собран Debian toolchain для `arm-linux-gnueabihf`.

Команда:

```text
bdinfo
```

Ключевые строки:

```text
boot_params = 0x80000100
DRAM bank   = 0x00000000
-> start    = 0x80000000
-> size     = 0x20000000
baudrate    = 115200 bps
relocaddr   = 0x9ff67000
Build       = 32-bit
fdt_blob    = 0x9df32150
devicetree  = separate
```

Вывод:

- DDR3 RAM начинается с адреса `0x80000000`;
- размер RAM `0x20000000`, то есть 512 MiB;
- UART работает на скорости `115200`;
- U-Boot 32-битный;
- Device Tree используется как отдельный blob.

Команда:

```text
mmc list
```

Результат:

```text
OMAP SD/MMC: 0 (SD)
OMAP SD/MMC: 1
```

Вывод: в U-Boot устройство `mmc 0` - это microSD. Устройство `mmc 1` - второй
MMC-контроллер, соответствующий встроенной eMMC.

Команды:

```text
mmc dev 0
mmc part
```

Результат:

```text
switch to partitions #0, OK
mmc0 is current device

Partition Map for MMC device 0  --   Partition Type: DOS

Part    Start Sector    Num Sectors     UUID            Type
  1     8192            73728           37b37d29-01     0c Boot
  2     81920           1048576         37b37d29-02     82
  3     1130496         120775647       37b37d29-03     83
```

Вывод:

- `mmc 0:1` - первый раздел microSD, FAT/BOOT;
- `mmc 0:2` - swap-раздел, тип `82`;
- `mmc 0:3` - Linux rootfs, тип `83`;
- разметка, видимая U-Boot, соответствует тому, что Linux позже показывает как
  `/dev/mmcblk0p1`, `/dev/mmcblk0p2`, `/dev/mmcblk0p3`.

### U-Boot console: файлы на разделах microSD

Команда:

```text
ls mmc 0:3 /
```

Ключевой результат:

```text
<DIR>       4096 boot
<DIR>       4096 etc
<DIR>       4096 home
<DIR>       4096 usr
<DIR>       4096 var
```

Вывод: `mmc 0:3` - это Linux root filesystem. U-Boot умеет читать его ext4 и
видит обычную структуру корневой файловой системы Debian.

Команда:

```text
ls mmc 0:3 /boot
```

Результат:

```text
<DIR>       4096 firmware
<DIR>       4096 dtbs
         8722654 initrd.img-6.12.76-bone50
          213488 config-6.12.76-bone50
         8778240 vmlinuz-6.12.76-bone50
         5498010 System.map-6.12.76-bone50
            2063 uEnv.txt
             549 SOC.sh
```

Вывод:

- `/boot/uEnv.txt` находится на `mmc 0:3`, то есть на rootfs-разделе;
- kernel image - `/boot/vmlinuz-6.12.76-bone50`;
- initrd - `/boot/initrd.img-6.12.76-bone50`;
- Device Tree files находятся в `/boot/dtbs`;
- это совпадает с boot log, где U-Boot загрузил kernel, DTB и initrd из `/boot`.

Команда:

```text
ls mmc 0:1 /
```

Результат:

```text
       54   ID.txt
      204   START.HTM
     2599   sysconf.txt
            services/
```

Вывод: `mmc 0:1` - маленький FAT-раздел `BOOT`, который Linux позже монтирует
как `/boot/firmware`. В этом образе он содержит `sysconf.txt` и сервисные
файлы первичной настройки, но не содержит kernel и `/boot/uEnv.txt`.

### U-Boot console: что именно загружается перед Linux

На уровне U-Boot загрузка Linux состоит не в монтировании всего Debian, а в
чтении нескольких boot-файлов с microSD в RAM и передаче управления ядру.

Фактическая карта нашей загрузки:

```text
Источник на microSD                                      Адрес/переменная RAM          Назначение
--------------------------------------------------------------------------------------------------------
mmc 0:3 /boot/uEnv.txt                                  loadaddr=0x82000000           импорт boot-настроек
mmc 0:3 /boot/vmlinuz-6.12.76-bone50                    loadaddr=0x82000000           Linux kernel image
mmc 0:3 /boot/dtbs/6.12.76-bone50/am335x-boneblack-uboot.dtb
                                                         fdtaddr=0x88000000            базовый Device Tree
mmc 0:3 /boot/dtbs/6.12.76-bone50/BB-ADC-00A0.dtbo      rdaddr=0x88080000             overlay ADC
mmc 0:3 /boot/dtbs/6.12.76-bone50/BB-BONE-eMMC1-01-00A0.dtbo
                                                         rdaddr=0x88080000             overlay eMMC
mmc 0:3 /boot/dtbs/6.12.76-bone50/BB-HDMI-TDA998x-00A0.dtbo
                                                         rdaddr=0x88080000             overlay HDMI
mmc 0:3 /boot/dtbs/6.12.76-bone50/AM335X-PRU-UIO-00A0.dtbo
                                                         rdaddr=0x88080000             overlay PRU UIO
mmc 0:3 /boot/initrd.img-6.12.76-bone50                 rdaddr=0x88080000             initrd
bootargs                                                kernel command line           параметры ядра
```

Пояснения:

- `uEnv.txt` сначала читается во временный буфер и импортируется как набор
  переменных U-Boot;
- kernel затем загружается по тому же адресу `0x82000000`, потому что текст
  `uEnv.txt` после импорта больше не нужен как файл в RAM;
- base DTB загружается в `0x88000000`;
- `.dtbo` overlays временно загружаются в `0x88080000` и применяются к DTB;
- после применения overlays по адресу `0x88080000` загружается initrd;
- rootfs целиком U-Boot не загружает.

Итоговая передача управления ядру из boot log:

```text
bootz 0x82000000 0x88080000:8518de 88000000
```

Расшифровка:

```text
0x82000000            Linux kernel image
0x88080000:8518de     initrd address and size
0x88000000            final Device Tree Blob
```

Отдельно важно: `mmc 0:1` с `sysconf.txt` не является источником kernel,
Device Tree или initrd. `sysconf.txt` будет прочитан уже после старта Linux
userspace сервисом первичной настройки.

### Концептуальное уточнение: RAM, rootfs и Device Tree

На момент строки:

```text
Starting kernel ...
```

в RAM находятся не все части Debian, а только то, что нужно ядру для старта:

```text
Linux kernel image
final Device Tree Blob
initrd / initramfs
kernel command line
```

Настоящая root filesystem остается на microSD. В нашем случае путь к ней
передается ядру через командную строку:

```text
root=/dev/mmcblk0p3
```

То есть U-Boot не загружает rootfs целиком в RAM. Он передает ядру информацию:

```text
kernel уже в RAM
Device Tree уже в RAM
initrd уже в RAM
rootfs нужно искать на /dev/mmcblk0p3
```

Device Tree также не является набором устройств, загруженных в RAM. Это
описание железа для Linux kernel:

```text
какие контроллеры есть в SoC
по каким адресам доступны их регистры
какие IRQ, clocks и resets используются
какие блоки включены
какой pinmux нужен
какие overlays применены
```

BeagleBone Black построен на SoC TI AM335x, а не на микроконтроллере. В SoC
есть CPU ARM Cortex-A8 и встроенная периферия: UART, I2C, SPI, GPIO, MMC, ADC,
PWM, USB, Ethernet и другие блоки. U-Boot передает ядру описание этой
периферии через Device Tree, а Linux уже подбирает и запускает соответствующие
драйверы.

Полная картина:

```text
U-Boot загружает в RAM:
  kernel
  final Device Tree
  initrd
  bootargs

Linux kernel стартует:
  читает Device Tree
  инициализирует драйверы SoC и платы
  использует initrd для ранней инициализации
  монтирует rootfs с /dev/mmcblk0p3
  запускает systemd/userspace
```

### U-Boot console: printenv

Сначала была сделана опечатка:

```text
printev
Unknown command 'printev' - try 'help'
```

Учебный вывод: U-Boot не выполнил неизвестную команду и подсказал использовать
`help`. Это безопасная ошибка, состояние носителей не изменилось.

Команда:

```text
printenv
```

Назначение команды: вывести U-Boot environment - набор переменных и скриптов,
из которых загрузчик собирает сценарий запуска Linux.

Ключевые переменные платы и консоли:

```text
arch=arm
board=am335x
board_name=A335BNLT
board_rev=00C0
board_serial=2125SBB11598
baudrate=115200
console=ttyS0,115200n8
stdin=serial@0
stdout=serial@0
stderr=serial@0
```

Вывод:

- плата определена как BeagleBone Black (`A335BNLT`, revision `00C0`);
- U-Boot работает через UART0;
- скорость консоли `115200`;
- эта же консоль затем передается Linux через `console=ttyS0,115200n8`.

Переменная, объясняющая сложность остановки autoboot:

```text
bootdelay=0
```

Вывод: U-Boot не ждет несколько секунд перед автозагрузкой. Поэтому остановить
autoboot получилось только удержанием `Space` еще до подачи питания.

Основная цепочка boot-команд:

```text
bootcmd=run findfdt; run init_console; run envboot; run distro_bootcmd
boot_targets=mmc0 legacy_mmc0 mmc1 legacy_mmc1 usb0 pxe dhcp
distro_bootcmd=for target in ${boot_targets}; do run bootcmd_${target}; done
```

Пояснение:

- `findfdt` выбирает Device Tree по модели платы;
- `init_console` задает правильный UART для консоли;
- `envboot` пытается найти и импортировать `uEnv.txt`;
- `distro_bootcmd` перебирает стандартные варианты загрузки: microSD, eMMC,
  USB, PXE, DHCP.

Переменные, связанные с MMC и boot files:

```text
mmcdev=0
bootdir=/boot
bootenv=uEnv.txt
bootenvfile=uEnv.txt
bootfile=zImage
loadbootenv=load ${devtype} ${bootpart} ${loadaddr} ${bootenvfile}
loadimage=load ${devtype} ${bootpart} ${loadaddr} ${bootdir}/${bootfile}
loadfdt=echo loading ${fdtdir}/${fdtfile} ...; load ${devtype} ${bootpart} ${fdtaddr} ${fdtdir}/${fdtfile}
loadrd=load ${devtype} ${bootpart} ${rdaddr} ${bootdir}/${rdfile}; setenv rdsize ${filesize}
```

Вывод:

- `mmcdev=0` соответствует microSD;
- `bootdir=/boot` совпадает с найденным каталогом `/boot` на `mmc 0:3`;
- `bootenvfile=uEnv.txt` объясняет поиск `/boot/uEnv.txt`;
- `bootfile=zImage` - значение по умолчанию, но в нашей загрузке оно позже
  переопределяется через `uEnv.txt`/`uname_boot` на
  `vmlinuz-6.12.76-bone50`.

Адреса загрузки в RAM:

```text
loadaddr=0x82000000
kernel_addr_r=0x82000000
fdtaddr=0x88000000
fdt_addr_r=0x88000000
rdaddr=0x88080000
ramdisk_addr_r=0x88080000
dtboaddr=0x89000000
fdtoverlay_addr_r=0x89000000
```

Вывод: U-Boot читает kernel, Device Tree, initrd и overlays с microSD в RAM, а
затем передает управление ядру командой `bootz`.

Переменные формирования kernel command line:

```text
args_mmc=run finduuid;setenv bootargs console=${console} ${cape_uboot} root=PARTUUID=${uuid} ro rootfstype=${mmcrootfstype} ${uboot_detected_capes} ${cmdline}
args_uenv_root=setenv bootargs console=${console} ${optargs} ${cape_uboot} root=${uenv_root} ro rootfstype=${mmcrootfstype} ${uboot_detected_capes} ${cmdline}
mmcrootfstype=ext4 rootwait
```

Пояснение: здесь собирается строка `bootargs`, которую потом увидит Linux в
`/proc/cmdline`. В нашем уже зафиксированном Linux boot результат был:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait ...
```

Важное наблюдение: в выводе `printenv` до запуска `envboot` нет отдельной строки
`uname_r=6.12.76-bone50`. Это ожидаемо: переменные из `/boot/uEnv.txt`
импортируются только во время выполнения boot-сценария. Поэтому boot log уже
показывал:

```text
Checking if uname_r is set in /boot/uEnv.txt...
Running uname_boot ...
loading /boot/vmlinuz-6.12.76-bone50 ...
```

Итоговый вывод по `printenv`: environment объясняет, как U-Boot выбирает
консоль, microSD, boot directory, адреса RAM и параметры ядра. Фактические
значения из `uEnv.txt` подключаются позже, во время автозагрузки.

### U-Boot console: продолжение загрузки командой boot

После просмотра environment была выполнена команда:

```text
boot
```

Назначение команды: вручную продолжить обычный сценарий загрузки после
остановки autoboot. Это контрольный шаг: мы убеждаемся, что чтение команд
U-Boot не изменило состояние платы и штатная загрузка Linux по-прежнему
работает.

Ключевые признаки успешного результата:

```text
Starting kernel ...
Welcome to Debian GNU/Linux 12 (bookworm)!
```

Вывод:

- U-Boot успешно передал управление Linux kernel;
- ядро стартовало после ручной остановки U-Boot;
- userspace Debian 12 начал загрузку;
- в ходе лабораторной работы настройки U-Boot не сохранялись и загрузочная
  цепочка не была изменена.

### Linux-side проверка после U-Boot

После загрузки Debian проверили, что Linux фактически получил параметры,
подготовленные U-Boot.

Команда:

```sh
cat /proc/cmdline
```

Результат:

```text
console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
```

Вывод:

- `console=ttyS0,115200n8` подтверждает, что Linux console осталась на debug
  UART;
- `root=/dev/mmcblk0p3` подтверждает, что rootfs монтируется с третьего
  раздела microSD;
- `rootfstype=ext4 rootwait` совпадает с логикой U-Boot environment
  `mmcrootfstype=ext4 rootwait`;
- U-Boot не загрузил rootfs в RAM, а передал ядру параметр, где его искать.

Команда:

```sh
lsblk -o NAME,SIZE,FSTYPE,LABEL,PARTUUID,MOUNTPOINTS
```

Результат:

```text
NAME         SIZE FSTYPE LABEL PARTUUID     MOUNTPOINTS
mmcblk0     58.1G
├─mmcblk0p1   36M vfat   BOOT  37b37d29-01  /boot/firmware
├─mmcblk0p2  512M swap         37b37d29-02  [SWAP]
└─mmcblk0p3 57.6G ext4   rootfs
                                    37b37d29-03  /
zram0      241.3M                                 [SWAP]
mmcblk1      3.6G
└─mmcblk1p1  3.6G ext4   rootfs cae33b65-01
mmcblk1boot0  2M
mmcblk1boot1  2M
```

Вывод:

- `mmcblk0` в Linux соответствует `mmc 0` в U-Boot, то есть microSD;
- `mmcblk0p1` соответствует `mmc 0:1` и смонтирован как `/boot/firmware`;
- `mmcblk0p2` соответствует `mmc 0:2` и используется как swap;
- `mmcblk0p3` соответствует `mmc 0:3` и смонтирован как `/`;
- `mmcblk1` - встроенная eMMC, она видна Linux, но в этой лабораторной не
  изменялась.

Команда:

```sh
tr -d '\0' < /proc/device-tree/model; echo
```

Результат:

```text
TI AM335x BeagleBone Black
```

Вывод: Linux получил Device Tree, в котором модель платы определена как
`TI AM335x BeagleBone Black`.

Команда:

```sh
dmesg | grep -E 'Kernel command line|Machine model|mmcblk0|mmcblk1'
```

Ключевые строки:

```text
[    0.000000] OF: fdt: Machine model: TI AM335x BeagleBone Black
[    0.000000] Kernel command line: console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
[    5.658365] mmcblk0: mmc0:b371 CBADS 58.1 GiB
[    5.676483] mmcblk1: mmc1:0001 M62704 3.56 GiB
[    5.681287]  mmcblk0: p1 p2 p3
[    5.697943]  mmcblk1: p1
[   10.745447] EXT4-fs (mmcblk0p3): mounted filesystem ... ro with ordered data mode.
[   20.281989] EXT4-fs (mmcblk0p3): re-mounted ... r/w.
[   26.477534] Adding 524284k swap on /dev/mmcblk0p2.
```

Итоговый вывод:

```text
U-Boot
  -> передал kernel command line
  -> передал final Device Tree
  -> передал управление kernel

Linux kernel
  -> увидел модель платы из Device Tree
  -> обнаружил microSD как mmcblk0
  -> обнаружил eMMC как mmcblk1
  -> смонтировал rootfs с mmcblk0p3
  -> подключил swap с mmcblk0p2
  -> запустил userspace Debian 12
```

### Linux-side: /boot/uEnv.txt, overlays и eMMC

Теоретическое уточнение по Device Tree:

```text
Device Tree = паспорт железа для Linux kernel
```

Он описывает:

```text
какие устройства и контроллеры есть на плате
по каким адресам доступны их регистры
какие IRQ, clocks, resets, GPIO и pinmux используются
какие устройства включены или disabled
какой драйвер может быть сопоставлен через compatible
```

Ядро выступает диспетчером между Device Tree и драйверами:

```text
Device Tree
  -> описывает железо

Kernel device model / OF subsystem
  -> читает Device Tree
  -> создает kernel devices
  -> сопоставляет compatible с таблицами драйверов

Driver
  -> получает устройство в probe()
  -> забирает ресурсы
  -> управляет железом
```

Формулировка "драйвер понимает `ti,am335-sdhci`" означает:

```text
Device Tree node:
  compatible = "ti,am335-sdhci"

Driver of_match_table:
  .compatible = "ti,am335-sdhci"

Kernel:
  видит совпадение
  связывает node с driver
  вызывает probe()
```

Важно: `compatible` - это не имя файла и не команда, а строковый ключ для
matching между описанием устройства и драйвером.

Также уточнено место U-Boot: он не собирает Device Tree с нуля из `.dts/.dtsi`
во время загрузки. U-Boot загружает уже готовый бинарный `.dtb`, применяет к
нему выбранные `.dtbo` overlays и передает Linux итоговый flattened Device Tree:

```text
готовый DTB + выбранные DTBO overlays -> final Device Tree
```

### Практическое уточнение: host /proc и target /proc

При продолжении практики команды были случайно выполнены на Linux-хосте:

```text
andrey@andrey-yunin:~/projects/BeagleBone_Black$ readlink -f /proc/device-tree
/proc/device-tree

andrey@andrey-yunin:~/projects/BeagleBone_Black$ find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
find: ‘/proc/device-tree’: Нет такого файла или каталога
```

Учебный вывод: `/proc` и `/sys` относятся к текущему запущенному ядру. На
Linux-хосте мы видим `/proc` хоста, а не BeagleBone Black. Device Tree платы
нужно смотреть внутри shell BeagleBone:

```text
andrey@BeagleBone:~$
```

Именно там должны выполняться команды:

```sh
readlink -f /proc/device-tree
find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
```

Повторная проверка уже на BeagleBone:

```sh
readlink -f /proc/device-tree
```

Результат:

```text
/sys/firmware/devicetree/base
```

Вывод: `/proc/device-tree` действительно указывает на итоговый Device Tree,
доступный также через `/sys/firmware/devicetree/base`.

Команда:

```sh
find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
```

Ключевые элементы вывода:

```text
/proc/device-tree
/proc/device-tree/opp-table
/proc/device-tree/__symbols__
/proc/device-tree/soc
/proc/device-tree/ocp
/proc/device-tree/ocp/P8_46_pinmux
/proc/device-tree/ocp/P9_29_pinmux
/proc/device-tree/ocp/interrupt-controller@48200000
/proc/device-tree/leds
/proc/device-tree/aliases
/proc/device-tree/chosen
/proc/device-tree/chosen/overlays
/proc/device-tree/memory@80000000
/proc/device-tree/sound
```

Вывод:

- Device Tree доступен как дерево каталогов;
- `ocp` содержит On-Chip Peripheral узлы AM335x и pinmux-узлы header-пинов;
- `P8_*_pinmux` и `P9_*_pinmux` показывают, что pinmux header-пинов тоже
  представлен в Device Tree;
- `chosen/overlays` содержит информацию о примененных overlays;
- `memory@80000000` соответствует RAM, начинающейся с адреса `0x80000000`, что
  совпадает с `bdinfo` U-Boot.

Команды:

```sh
tr -d '\0' < /proc/device-tree/model; echo
tr -d '\0' < /proc/device-tree/compatible; echo
```

Результат:

```text
TI AM335x BeagleBone Black
ti,am335x-bone-blackti,am335x-boneti,am33xx
```

Вывод:

- `model` - человекочитаемое имя платы;
- `compatible` содержит цепочку совместимости от конкретной платы к более
  общему семейству;
- из-за `NUL`-разделителей строки склеились при простом `tr -d '\0'`; для
  более читаемого вывода лучше использовать `strings`.

Логическая цепочка compatible:

```text
ti,am335x-bone-black
ti,am335x-bone
ti,am33xx
```

Это означает: плата совместима сначала с конкретным BeagleBone Black, затем с
семейством BeagleBone, затем с SoC AM33xx.

Команда:

```sh
find -L /proc/device-tree -iname '*mmc*'
```

Ключевые элементы вывода:

```text
/proc/device-tree/__symbols__/mmc1
/proc/device-tree/__symbols__/mmc2
/proc/device-tree/__symbols__/mmc3
/proc/device-tree/ocp/interconnect@48000000/segment@100000/target-module@d8000/mmc@0
/proc/device-tree/ocp/interconnect@48000000/segment@0/target-module@60000/mmc@0
/proc/device-tree/ocp/target-module@47810000/mmc@0
/proc/device-tree/ocp/interconnect@44c00000/.../pinmux@800/mmc1-pins
/proc/device-tree/ocp/interconnect@44c00000/.../pinmux@800/emmc-pins
/proc/device-tree/aliases/mmc0
/proc/device-tree/aliases/mmc1
/proc/device-tree/aliases/mmc2
/proc/device-tree/chosen/overlays/BB-BONE-eMMC1-01-00A0.kernel
```

Вывод:

- в итоговом Device Tree есть несколько MMC-related узлов и aliases;
- `mmc1-pins` и `emmc-pins` показывают pinmux для microSD/eMMC;
- `chosen/overlays/BB-BONE-eMMC1-01-00A0.kernel` подтверждает, что eMMC overlay
  присутствует в итоговом дереве;
- путь к MMC-контроллерам в runtime DT глубже, чем простое
  `/proc/device-tree/ocp/mmc@...`, поэтому для чтения свойств нужно брать
  фактические найденные пути.

Команда чтения свойств двух MMC child-узлов:

```sh
for node in \
  /proc/device-tree/ocp/interconnect@48000000/segment@0/target-module@60000/mmc@0 \
  /proc/device-tree/ocp/interconnect@48000000/segment@100000/target-module@d8000/mmc@0
do
  echo "=== $node ==="
  echo -n "compatible: "; tr '\0' '\n' < "$node/compatible"
  echo -n "status: "; tr -d '\0' < "$node/status"; echo
  echo "reg:"
  hexdump -C "$node/reg"
done
```

Результат:

```text
=== /proc/device-tree/ocp/interconnect@48000000/segment@0/target-module@60000/mmc@0 ===
compatible: ti,am335-sdhci
status: okay
reg:
00000000  00 00 00 00 00 00 10 00

=== /proc/device-tree/ocp/interconnect@48000000/segment@100000/target-module@d8000/mmc@0 ===
compatible: ti,am335-sdhci
status: okay
reg:
00000000  00 00 00 00 00 00 10 00
```

Вывод:

- оба MMC child-узла совместимы с драйвером `ti,am335-sdhci`;
- оба узла имеют `status=okay`, значит они включены в итоговом Device Tree;
- оба узла могут быть сопоставлены с одним и тем же kernel-драйвером;
- `reg` внутри child-узла `mmc@0` показывает offset `0` и размер `0x1000`;
- полный адрес контроллера получается из родительского `target-module` и bus
  mapping, поэтому следующий шаг - посмотреть свойства parent-узлов, чтобы
  связать DT path с адресами из `dmesg`: `48060000.mmc` и `481d8000.mmc`.

Команда чтения свойств parent `target-module`:

```sh
for node in \
  /proc/device-tree/ocp/interconnect@48000000/segment@0/target-module@60000 \
  /proc/device-tree/ocp/interconnect@48000000/segment@100000/target-module@d8000
do
  echo "=== $node ==="
  echo -n "compatible: "; tr '\0' '\n' < "$node/compatible"
  echo "reg:"
  hexdump -C "$node/reg"
  if [ -f "$node/ranges" ]; then
    echo "ranges:"
    hexdump -C "$node/ranges"
  fi
done
```

Результат:

```text
=== .../segment@0/target-module@60000 ===
compatible: ti,sysc-omap2
ti,sysc
reg:
00000000  00 06 02 fc 00 00 00 04  00 06 01 10 00 00 00 04
00000010  00 06 01 14 00 00 00 04
ranges:
00000000  00 00 00 00 00 06 00 00  00 00 10 00

=== .../segment@100000/target-module@d8000 ===
compatible: ti,sysc-omap2
ti,sysc
reg:
00000000  00 0d 82 fc 00 00 00 04  00 0d 81 10 00 00 00 04
00000010  00 0d 81 14 00 00 00 04
ranges:
00000000  00 00 00 00 00 0d 80 00  00 00 10 00
```

Вывод:

- `target-module` описывает не сам MMC-драйвер, а системный wrapper/sysconfig
  блок TI (`compatible = ti,sysc-omap2`, `ti,sysc`);
- child `mmc@0` внутри него уже описывает сам MMC-контроллер
  (`compatible = ti,am335-sdhci`);
- `ranges` показывает отображение локального адреса child-узла в адресное
  пространство родительской шины;
- для первого контроллера виден offset `0x60000` внутри interconnect
  `0x48000000`, что дает полный адрес `0x48060000`;
- для второго контроллера нужно учитывать не только `target-module@d8000`, но и
  промежуточный `segment@100000`. Поэтому следующий шаг - прочитать свойства
  `segment@0` и `segment@100000`, чтобы собрать полный путь адресации без
  предположений.

Команда чтения промежуточных `segment`-узлов:

```sh
for node in \
  /proc/device-tree/ocp/interconnect@48000000/segment@0 \
  /proc/device-tree/ocp/interconnect@48000000/segment@100000
do
  echo "=== $node ==="
  echo "reg:"
  hexdump -C "$node/reg"
  if [ -f "$node/ranges" ]; then
    echo "ranges:"
    hexdump -C "$node/ranges"
  fi
done
```

Результат: у обоих `segment`-узлов файла `reg` нет:

```text
hexdump: .../segment@0/reg: No such file or directory
hexdump: .../segment@100000/reg: No such file or directory
```

Это не ошибка для нашей задачи. Эти узлы работают как промежуточные bus/segment
узлы и задают адресную трансляцию через `ranges`.

Для `segment@0` в длинном `ranges` есть нужная запись:

```text
00 06 00 00  00 06 00 00  00 00 10 00
```

Расшифровка:

```text
child address   0x00060000
parent address  0x00060000
size            0x00001000
```

Вместе с родительским `interconnect@48000000` это дает:

```text
0x48000000 + 0x00060000 = 0x48060000
```

Это совпадает с kernel log:

```text
48060000.mmc -> mmc0 -> microSD
```

Для `segment@100000` в `ranges` есть нужная запись:

```text
00 0d 80 00  00 1d 80 00  00 00 10 00
```

Расшифровка:

```text
child address   0x000d8000
parent address  0x001d8000
size            0x00001000
```

Вместе с родительским `interconnect@48000000` это дает:

```text
0x48000000 + 0x001d8000 = 0x481d8000
```

Это совпадает с kernel log:

```text
481d8000.mmc -> mmc1 -> eMMC
```

Итоговая адресная цепочка:

```text
interconnect@48000000
  -> segment@0
    -> target-module@60000
      -> mmc@0
        -> 0x48060000
        -> Linux mmc0
        -> microSD

interconnect@48000000
  -> segment@100000
    -> target-module@d8000
      -> mmc@0
        -> 0x481d8000
        -> Linux mmc1
        -> eMMC
```

Учебный вывод: полный physical address устройства в Device Tree может
получаться не из одного `reg`, а через цепочку родительских `ranges`.

Команда чтения MMC aliases:

```sh
for alias in mmc0 mmc1 mmc2; do
  echo -n "$alias -> "
  tr -d '\0' < /proc/device-tree/aliases/$alias
  echo
done
```

Результат:

```text
mmc0 -> /ocp/interconnect@48000000/segment@0/target-module@60000/mmc@0
mmc1 -> /ocp/interconnect@48000000/segment@100000/target-module@d8000/mmc@0
mmc2 -> /ocp/target-module@47810000/mmc@0
```

Вывод:

- aliases дают короткие стабильные имена для длинных Device Tree paths;
- `mmc0` указывает на контроллер microSD;
- `mmc1` указывает на контроллер eMMC;
- `mmc2` описан в Device Tree как дополнительный MMC-контроллер/интерфейс, но
  в текущей системе основная практическая связка уже подтверждена для
  `mmc0 -> mmcblk0` и `mmc1 -> mmcblk1`.

Команда просмотра overlays в итоговом Device Tree:

```sh
find -L /proc/device-tree/chosen/overlays -maxdepth 1 -type f -print
```

Результат:

```text
/proc/device-tree/chosen/overlays/BB-HDMI-TDA998x-00A0.kernel
/proc/device-tree/chosen/overlays/AM335X-PRU-UIO-00A0.kernel
/proc/device-tree/chosen/overlays/BB-BONE-eMMC1-01-00A0.kernel
/proc/device-tree/chosen/overlays/BB-ADC-00A0.kernel
/proc/device-tree/chosen/overlays/name
```

Вывод:

- в итоговом Device Tree есть отметки о примененных overlays;
- список совпадает с тем, что U-Boot загружал в boot log;
- `BB-ADC-00A0` включает ADC overlay;
- `BB-BONE-eMMC1-01-00A0` включает eMMC overlay;
- `BB-HDMI-TDA998x-00A0` включает HDMI overlay;
- `AM335X-PRU-UIO-00A0` включает PRU UIO overlay.

Команда чтения содержимого marker-файлов overlays:

```sh
for f in /proc/device-tree/chosen/overlays/*.kernel; do
  echo "=== $f ==="
  hexdump -C "$f"
done
```

Результат для каждого overlay-файла содержит строку:

```text
Fri Mar  6 05:23:58 2026
```

Вывод:

- файлы в `/proc/device-tree/chosen/overlays/*.kernel` не являются исходными
  `.dtbo`;
- это небольшие marker-файлы с информацией о примененном overlay;
- сами изменения overlay уже внесены в итоговый Device Tree;
- чтобы увидеть эффект overlay, нужно искать измененные/добавленные узлы и
  свойства в дереве, а не ожидать увидеть полный `.dtbo` внутри
  `/chosen/overlays`.

Команда поиска ADC/TSC_ADC узлов:

```sh
find -L /proc/device-tree -iname '*adc*' -o -iname '*tscadc*'
```

Результат:

```text
/proc/device-tree/__symbols__/bone_adc
/proc/device-tree/__symbols__/adc_tsc_fck
/proc/device-tree/__symbols__/am335x_adc
/proc/device-tree/__symbols__/tscadc
/proc/device-tree/ocp/interconnect@44c00000/segment@200000/target-module@d000/tscadc@0
/proc/device-tree/ocp/interconnect@44c00000/segment@200000/target-module@d000/tscadc@0/adc
/proc/device-tree/ocp/interconnect@44c00000/segment@200000/target-module@d000/tscadc@0/adc/ti,adc-channels
/proc/device-tree/ocp/interconnect@44c00000/segment@200000/target-module@10000/scm@0/scm_conf@0/clocks/clock-adc-tsc-fck
/proc/device-tree/chosen/overlays/BB-ADC-00A0.kernel
```

Вывод:

- в итоговом Device Tree присутствует TSC_ADC блок AM335x;
- внутри него есть child-узел `adc`;
- свойство `ti,adc-channels` должно описывать включенные ADC-каналы;
- marker `BB-ADC-00A0.kernel` подтверждает, что ADC overlay применен;
- это будет точкой входа для следующей большой темы: встроенный ADC через
  существующий Linux IIO-драйвер.

Команды чтения свойств TSC_ADC и ADC child:

```sh
tsc=/proc/device-tree/ocp/interconnect@44c00000/segment@200000/target-module@d000/tscadc@0
adc=$tsc/adc

echo -n "compatible: "; tr '\0' '\n' < "$tsc/compatible"
echo -n "status: "; tr -d '\0' < "$tsc/status"; echo
hexdump -C "$tsc/reg"

echo -n "compatible: "; tr '\0' '\n' < "$adc/compatible"
hexdump -C "$adc/ti,adc-channels"
```

Результат:

```text
=== TSCADC ===
compatible: ti,am3359-tscadc
status: okay
reg:
00000000  00 00 00 00 00 00 10 00

=== ADC child ===
compatible: ti,am3359-adc
ti,adc-channels:
00000000  00 00 00 00 00 00 00 01  00 00 00 02 00 00 00 03
00000010  00 00 00 04 00 00 00 05  00 00 00 06 00 00 00 07
```

Вывод:

- parent TSC_ADC node совместим с `ti,am3359-tscadc`;
- parent node имеет `status=okay`, значит блок включен;
- child ADC node совместим с `ti,am3359-adc`;
- свойство `ti,adc-channels` содержит big-endian 32-bit значения каналов
  `0, 1, 2, 3, 4, 5, 6, 7`;
- ADC overlay включил все восемь ADC-каналов;
- следующий практический мост - найти соответствующее IIO-устройство в Linux и
  сопоставить Device Tree channels с файлами `in_voltage*_raw`.

Команда просмотра IIO-устройств:

```sh
ls -la /sys/bus/iio/devices
```

Результат:

```text
iio:device0 -> ../../../devices/platform/ocp/44c00000.interconnect/44c00000.interconnect:segment@200000/44e0d000.target-module/44e0d000.tscadc/TI-am335x-adc.0.auto/iio:device0
```

Команда:

```sh
for dev in /sys/bus/iio/devices/iio:device*; do
  echo "=== $dev ==="
  cat "$dev/name" 2>/dev/null
  ls "$dev"/in_voltage*_raw 2>/dev/null
done
```

Результат:

```text
=== /sys/bus/iio/devices/iio:device0 ===
TI-am335x-adc.0.auto
/sys/bus/iio/devices/iio:device0/in_voltage0_raw
/sys/bus/iio/devices/iio:device0/in_voltage1_raw
/sys/bus/iio/devices/iio:device0/in_voltage2_raw
/sys/bus/iio/devices/iio:device0/in_voltage3_raw
/sys/bus/iio/devices/iio:device0/in_voltage4_raw
/sys/bus/iio/devices/iio:device0/in_voltage5_raw
/sys/bus/iio/devices/iio:device0/in_voltage6_raw
/sys/bus/iio/devices/iio:device0/in_voltage7_raw
```

Вывод:

- Linux создал IIO device для встроенного ADC AM335x;
- имя устройства `TI-am335x-adc.0.auto`;
- IIO ABI экспортирует восемь raw-каналов `in_voltage0_raw` ...
  `in_voltage7_raw`;
- это соответствует Device Tree свойству `ti,adc-channels = <0 1 2 3 4 5 6 7>`;
- связка `Device Tree -> driver probe -> IIO device -> sysfs raw channels`
  подтверждена.

Команда первого чтения ADC raw-кодов:

```sh
for f in /sys/bus/iio/devices/iio:device0/in_voltage*_raw; do
  echo -n "$(basename "$f"): "
  cat "$f"
done
```

Результат:

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

Что делает команда:

- проходит по всем файлам `in_voltage*_raw`;
- печатает имя канала через `basename`;
- читает raw-код ADC из каждого файла;
- обращается уже не к Device Tree, а к Linux IIO ABI, который создал драйвер.

Интерпретация:

- `raw` - это сырой цифровой код ADC, а не вольты;
- ADC AM335x 12-битный, нормальный диапазон raw-кода: `0..4095`;
- диапазон аналогового входа BeagleBone Black: `0..1.8 V`;
- примерная оценка: `voltage ~= raw * 1.8 / 4096`.

Примерная оценка полученных значений:

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

Эти числа подтверждают, что чтение ADC через драйвер и IIO работает. Но если
аналоговые входы физически не подключены к стабильному источнику сигнала, это
не измерение датчика. Неподключенный вход находится в floating-состоянии, на
него влияют наводки, утечки, входная емкость и предыдущие переключения
мультиплексора ADC.

Для точного пересчета raw-кода нужно проверить scale, который экспортирует
драйвер:

```sh
cat /sys/bus/iio/devices/iio:device0/in_voltage_scale
```

Модель пересчета:

```text
voltage = raw * scale
```

Для `in_voltage` в IIO это обычно дает значение в millivolts.

Учебный вывод:

```text
Device Tree включил ADC channels.
Драйвер создал IIO device.
Userspace смог прочитать raw-коды.
Полезным измерением это станет после подключения корректной аналоговой схемы.
```

Концептуальное уточнение по userspace-приложению:

```text
Device Tree
  -> описывает железо и конфигурацию платы

Kernel driver
  -> инициализирует контроллер
  -> включает ресурсы: clocks, IRQ, MMIO, pinctrl
  -> регистрирует kernel ABI

Userspace application
  -> использует готовый ABI
  -> читает данные
  -> выполняет прикладную обработку
```

В случае встроенного ADC BeagleBone Black приложение пользователя не должно
настраивать MMIO-регистры ADC напрямую. Если kernel driver успешно выполнил
`probe()` и создал файлы:

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

Userspace может выполнять только ту прикладную настройку, которую драйвер
экспортирует через ABI, например:

```text
in_voltage_scale
sampling_frequency
buffer/trigger
channel enable
GPIO управления внешней схемой
```

Итоговая формулировка:

```text
Если драйвер поднял ADC и экспортировал IIO-файлы,
userspace-приложение читает данные через IIO.
Отдельная инициализация ADC из userspace не нужна.
```

Практический блок проверки пути данных для userspace-приложения:

Задача: приложение пользователя должно получить данные ADC из userspace. Для
этого нужно пройти путь от IIO device до конкретного файла
`in_voltageN_raw`, который приложение сможет открыть и прочитать.

Шаг 1. Убедиться, что IIO bus есть в системе:

```sh
ls -la /sys/bus/iio/devices
```

Зачем:

- проверить, что подсистема IIO доступна;
- увидеть все IIO-устройства, созданные драйверами;
- найти candidates вида `iio:device0`, `iio:device1` и т.д.

В нашей системе найден:

```text
/sys/bus/iio/devices/iio:device0
```

Шаг 2. Понять, какое `iio:deviceX` является ADC:

```sh
for dev in /sys/bus/iio/devices/iio:device*; do
  echo "=== $dev ==="
  cat "$dev/name" 2>/dev/null
done
```

Зачем:

- `iio:device0` - runtime index, а не постоянное имя конкретного железа;
- если в системе появятся другие IIO-устройства, номер может измениться;
- файл `name` связывает `iio:deviceX` с конкретным драйвером.

Ожидаемое имя для встроенного ADC:

```text
TI-am335x-adc.0.auto
```

Шаг 3. Зафиксировать путь найденного ADC:

```sh
adc_dev=/sys/bus/iio/devices/iio:device0
cat "$adc_dev/name"
```

Зачем:

- получить короткую переменную для дальнейших команд;
- убедиться, что выбран именно ADC.

Для будущего приложения лучше искать устройство по `name`, а не жестко
зашивать номер `iio:device0`.

Шаг 4. Посмотреть физический путь устройства в sysfs:

```sh
readlink -f "$adc_dev"
```

Зачем:

- увидеть, к какому platform device привязан IIO device;
- связать userspace path с Device Tree и kernel driver;
- подтвердить, что это ABI поверх реального драйвера.

Ожидаемая логика пути:

```text
/sys/devices/platform/.../44e0d000.tscadc/.../iio:device0
```

Шаг 5. Увидеть файлы, которые может читать приложение:

```sh
ls "$adc_dev"/in_voltage*_raw
```

Зачем:

- получить список каналов, экспортированных драйвером;
- увидеть конкретные файлы для чтения из userspace.

Для нашей системы:

```text
/sys/bus/iio/devices/iio:device0/in_voltage0_raw
...
/sys/bus/iio/devices/iio:device0/in_voltage7_raw
```

Это основные пути чтения данных.

Шаг 6. Проверить scale для пересчета raw-кода:

```sh
cat "$adc_dev/in_voltage_scale"
```

Зачем:

- получить коэффициент пересчета raw-кода;
- не зашивать приблизительное `1.8 / 4096`, если драйвер экспортирует scale;
- использовать стандартную IIO-модель:

```text
voltage = raw * scale
```

Результат практики:

```text
0.439453125
```

Для `in_voltage` это значение обычно интерпретируется как millivolts per ADC
count. Поэтому для userspace-приложения:

```text
voltage_mV = raw * 0.439453125
voltage_V  = voltage_mV / 1000.0
```

Проверка полного диапазона:

```text
4096 * 0.439453125 = 1800 mV = 1.8 V
```

Это совпадает с допустимым диапазоном встроенного ADC BeagleBone Black
`0..1.8 V`.

Пример для ранее прочитанного значения `in_voltage0_raw = 2421`:

```text
2421 * 0.439453125 = 1063.92 mV = 1.064 V
```

Шаг 7. Прочитать один канал так, как это сделает приложение:

```sh
cat "$adc_dev/in_voltage0_raw"
```

Зачем:

- проверить минимальный путь данных;
- убедиться, что файл читается без ошибок;
- увидеть путь, который приложение будет открывать для канала 0.

Модель для приложения:

```text
open("/sys/bus/iio/devices/iio:device0/in_voltage0_raw")
read()
parse integer
```

Шаг 8. Прочитать все каналы:

```sh
for f in "$adc_dev"/in_voltage*_raw; do
  echo -n "$(basename "$f"): "
  cat "$f"
done
```

Зачем:

- проверить, что все каналы, созданные драйвером, читаются;
- увидеть текущие raw-коды;
- подготовиться к проверке будущей схемы подключения датчика.

Итоговый путь данных:

```text
AM335x ADC hardware
  -> kernel driver
  -> IIO sysfs ABI
  -> /sys/bus/iio/devices/iio:device0/in_voltageN_raw
  -> userspace application
```

Важное ограничение для следующей практики: analog inputs BeagleBone Black не
являются 3.3 V tolerant. Для встроенного ADC работать только в диапазоне
`0..1.8 V`.

### Концептуальное уточнение: Device Tree и активность устройств

U-Boot не устанавливает и не активирует все устройства из образа. В этой части
загрузки U-Boot:

```text
загружает kernel
загружает базовый Device Tree
применяет overlays
передает bootargs
стартует kernel
```

Дальше Linux kernel читает итоговый Device Tree и пытается создать устройства,
сопоставить их с драйверами и вызвать `probe()`.

Возможные состояния устройства:

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

Правильная проверочная цепочка:

```text
Device Tree node
  -> status
  -> compatible
  -> driver match
  -> probe()
  -> runtime evidence in dmesg / sysfs / devfs
```

В этой лабораторной активность подтверждена для MMC:

```text
DT node есть
compatible = ti,am335-sdhci
status = okay
dmesg показывает SDHCI controller и mmc0/mmc1
Linux видит mmcblk0/mmcblk1
```

И для ADC:

```text
DT node есть
compatible = ti,am3359-adc
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

### Концептуальное уточнение: Device Tree properties и поведение драйвера

Device Tree property влияет на поведение устройства только тогда, когда драйвер
это свойство реально поддерживает и читает.

Правило:

```text
DT property существует
+ driver читает эту property
= поведение меняется

DT property существует
+ driver игнорирует эту property
= поведение не меняется
```

Пример со встроенным ADC:

```text
ti,adc-channels = <0 1 2 3 4 5 6 7>;
```

Если драйвер `ti,am3359-adc` поддерживает это свойство, то в `probe()` он
читает список каналов и регистрирует соответствующие IIO channels.

Для конфигурации только одного канала правильная модель такая:

```text
Device Tree overlay:
  ti,adc-channels = <0>;

driver probe():
  читает property "ti,adc-channels"
  создает список каналов
  регистрирует только channel 0

userspace:
  видит in_voltage0_raw
```

Если драйвер не читает `ti,adc-channels` и всегда жестко создает все 8 каналов,
то изменение Device Tree не изменит поведение. Поэтому перед изменением DT
нужно смотреть binding/documentation или код драйвера.

Важный вывод из обсуждения: если в Device Tree убрать каналы, но драйвер не
использует это свойство, драйвер продолжит работать по своей внутренней логике.
Device Tree сам по себе не запрещает драйверу обращаться к каналам. Он только
передает конфигурационные данные, которые драйвер может прочитать и применить.

Инженерный вывод:

```text
Device Tree задает конфигурацию платы.
Драйвер определяет, какие DT properties он учитывает.
Поведение устройства меняется только в пределах того, что поддерживает драйвер.
```

Переписывать драйвер имеет смысл, когда штатный драйвер не поддерживает нужный
режим, содержит ошибку или когда нужно добавить поддержку нового внешнего
устройства. Если нужно изменить только конфигурацию платы, сначала проверяют
вариант через Device Tree overlay.

Команда:

```sh
find /proc/device-tree -maxdepth 2 -type d | head -n 80
```

Результат: команда не вывела каталогов.

Пояснение: это не означает, что Device Tree отсутствует. На этой системе
`/proc/device-tree` ведет себя как ссылка на дерево в `/sys/firmware`, а
`find` без `-L` не переходит по symlink. Для следующей проверки нужно
использовать:

```sh
find -L /proc/device-tree -maxdepth 2 -type d | head -n 80
```

или:

```sh
find /sys/firmware/devicetree/base -maxdepth 2 -type d | head -n 80
```

Команда:

```sh
dmesg | grep -Ei 'OF:|fdt|overlay|mmc|adc|pru|hdmi'
```

Ключевые строки:

```text
[    0.000000] OF: fdt: Machine model: TI AM335x BeagleBone Black
[    0.000000] OF: reserved mem: Reserved memory: No reserved-memory node in the DT
[    0.000000] Kernel command line: console=ttyS0,115200n8 root=/dev/mmcblk0p3 ro rootfstype=ext4 rootwait fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
[    5.553165] mmc0: SDHCI controller on 48060000.mmc [48060000.mmc] using External DMA
[    5.592715] mmc1: SDHCI controller on 481d8000.mmc [481d8000.mmc] using External DMA
[    5.651115] mmc0: new high speed SDXC card at address b371
[    5.667078] mmc1: new high speed MMC card at address 0001
```

Вывод:

- Linux получил final Device Tree с моделью `TI AM335x BeagleBone Black`;
- `48060000.mmc` стал `mmc0`, то есть microSD;
- `481d8000.mmc` стал `mmc1`, то есть встроенная eMMC;
- отдельные строки про применение overlays в `dmesg` не обязательны, потому что
  U-Boot применяет `.dtbo` до старта Linux, а ядро видит уже итоговый FDT.

Команда:

```sh
ls -la /boot
```

Результат:

```text
drwxr-xr-x  4 root   root      4096 Mar 17 21:29 .
drwxr-xr-x 18 root   root      4096 Mar 17 21:29 ..
-rw-r--r--  1 root   root       549 Mar 17 21:29 SOC.sh
-rw-r--r--  1 root   root   5498010 Mar  6 08:24 System.map-6.12.76-bone50
-rw-r--r--  1 root   root    213488 Mar  6 08:24 config-6.12.76-bone50
drwxr-xr-x  3 root   root      4096 Mar 17 21:20 dtbs
drwxr-xr-x  3 andrey andrey     512 Jan  1  1970 firmware
-rw-r--r--  1 root   root   8722654 Mar 17 21:28 initrd.img-6.12.76-bone50
-rw-r--r--  1 andrey andrey    2063 Mar 17 21:29 uEnv.txt
-rwxr-xr-x  1 root   root   8778240 Mar  6 08:24 vmlinuz-6.12.76-bone50
```

Вывод: Linux видит те же boot-файлы, которые U-Boot читал с `mmc 0:3 /boot`:
kernel, initrd, `uEnv.txt`, каталог `dtbs` и служебные файлы ядра.

Команда:

```sh
sed -n '1,220p' /boot/uEnv.txt
```

Ключевые строки:

```text
uname_r=6.12.76-bone50
enable_uboot_overlays=1
uboot_overlay_pru=AM335X-PRU-UIO-00A0.dtbo
console=ttyS0,115200n8
cmdline=fsck.repair=yes earlycon coherent_pool=1M net.ifnames=0 lpj=1990656 rng_core.default_quality=100
#cmdline=init=/usr/sbin/init-beagle-flasher
```

Вывод:

- `uname_r=6.12.76-bone50` объясняет, почему U-Boot загрузил
  `vmlinuz-6.12.76-bone50`, `initrd.img-6.12.76-bone50` и DTB из
  `/boot/dtbs/6.12.76-bone50`;
- `enable_uboot_overlays=1` включает механизм U-Boot overlays;
- `uboot_overlay_pru=AM335X-PRU-UIO-00A0.dtbo` выбирает PRU UIO overlay;
- `console=` и `cmdline=` входят в итоговую `/proc/cmdline`;
- строка `init=/usr/sbin/init-beagle-flasher` закомментирована, значит eMMC
  flasher не включен.

Повторная проверка `lsblk`:

```text
mmcblk0      58.1G
├─mmcblk0p1    36M vfat BOOT   37b37d29-01  /boot/firmware
├─mmcblk0p2   512M swap        37b37d29-02  [SWAP]
└─mmcblk0p3  57.6G ext4 rootfs 37b37d29-03  /
mmcblk1       3.6G
└─mmcblk1p1   3.6G ext4 rootfs cae33b65-01
mmcblk1boot0    2M
mmcblk1boot1    2M
```

Вывод:

- система сейчас загружена с microSD, потому что `/` смонтирован с
  `mmcblk0p3`;
- eMMC видна как `mmcblk1` и содержит раздел `mmcblk1p1` с ext4/rootfs, но этот
  раздел не смонтирован;
- в этой лабораторной eMMC не изменялась;
- наличие rootfs на eMMC не означает, что текущая система загружена с eMMC.

### План второй большой части курса

После обсуждения финального направления для разработки модулей ядра принято
решение строить вторую часть курса в несколько этапов:

```text
1. Hello, World module
2. Встроенный ADC AM335x через существующий драйвер и IIO
3. HC-06 Bluetooth UART lab через Linux tty
4. Внешний ADS1115 как бюджетный промышленный аналоговый вход
```

От дорогого промышленного модуля CN0414/AD4111 на первом проходе отказались,
потому что он усложняет и удорожает проект. Вместо него выбран макетный
вариант на ADS1115:

```text
0-10 V input  -> делитель -> ADS1115
4-20 mA input -> шунт     -> ADS1115
ADS1115       -> I2C      -> BeagleBone Black
Linux driver  -> IIO      -> raw/scale
userspace     -> пересчет в инженерные величины
```

Компоненты HART и промышленной обвязки пока не входят в обязательный проект:

```text
AD5700-1  HART modem
ADG704    HART multiplexer
ADuM3151  digital isolation
ADuM5411  isolated power
ADP2441   24 V industrial DC/DC
```

Они остаются как возможное расширение после того, как будет доведена до конца
базовая связка Device Tree, I2C, kernel driver, IIO и измерение `0-10 V` /
`4-20 mA`.

Добавлен учебный план:

```text
course/20-kernel-modules-roadmap.md
```

### Host-target network, SSH и обход VPN

Перед переходом к разработке модулей ядра подготовлен сетевой канал между
Linux-хостом и BeagleBone Black. Цель - писать код в VS Code на хосте,
передавать файлы на плату по SSH/SCP/rsync и проверять результат на target.

Физическая схема:

```text
Linux host eno0
  -> unmanaged switch
  -> BeagleBone Black eth0
```

Проверка адресов на BeagleBone:

```sh
ip -br addr
```

Ключевой результат:

```text
eth0  UP  10.129.1.152/23 metric 1024
usb0  UP  192.168.7.2/24
usb1  UP  192.168.6.2/24
```

Проверка маршрутов на BeagleBone:

```sh
ip route
```

Ключевой результат:

```text
default via 10.129.0.1 dev eth0 proto dhcp src 10.129.1.152 metric 1024
10.129.0.0/23 dev eth0 proto kernel scope link src 10.129.1.152 metric 1024
192.168.6.0/24 dev usb1 proto kernel scope link src 192.168.6.2
192.168.7.0/24 dev usb0 proto kernel scope link src 192.168.7.2
```

Вывод:

- плата получила Ethernet-адрес `10.129.1.152/23`;
- gateway платы: `10.129.0.1`;
- USB gadget network тоже поднят, но основной рабочий канал выбран через
  `eth0`.

Проверка адресов на хосте:

```sh
ip -br addr
```

Ключевой результат:

```text
eno0             UP       10.129.1.110/23
outline-tun1     UNKNOWN  10.0.85.5/32
enx30e283532bca  UNKNOWN  192.168.7.1/24
enx30e283532bcc  UP       192.168.6.1/24
```

Вывод:

- Ethernet-интерфейс хоста: `eno0`;
- адрес хоста в той же сети: `10.129.1.110/23`;
- VPN-интерфейс: `outline-tun1`;
- USB gadget interfaces на хосте тоже видны как `192.168.7.1/24` и
  `192.168.6.1/24`.

Проверка `ping` с хоста:

```sh
ping -c 3 10.129.1.152
```

Результат:

```text
3 packets transmitted, 3 received, 0% packet loss
rtt min/avg/max/mdev = 0.308/0.344/0.384/0.031 ms
```

Но `ping` оказался недостаточным доказательством правильного пути, потому что
на хосте работал VPN с policy routing.

Проверка реального маршрута:

```sh
ip route get 10.129.1.152
```

Проблемный результат:

```text
10.129.1.152 via 10.0.85.5 dev outline-tun1 table 7113 src 10.0.85.5 uid 1000
    cache
```

Вывод:

- хост отправлял трафик к плате через VPN `outline-tun1`;
- VPN использовал routing table `7113`;
- поэтому SSH-подключение к `10.129.1.152` закрывалось до нормального
  handshake.

Симптом SSH на хосте:

```sh
ssh -vvv andrey@10.129.1.152
```

Ключевой фрагмент:

```text
Connection established.
Local version string SSH-2.0-OpenSSH_9.6p1 Ubuntu-3ubuntu13.4
kex_exchange_identification: Connection closed by remote host
Connection closed by 10.129.1.152 port 22
```

Проверка SSH-сервиса на BeagleBone:

```sh
sudo systemctl status ssh --no-pager
sudo ss -ltnp | grep ':22'
```

Ключевые результаты:

```text
Active: active (running)
Server listening on 0.0.0.0 port 22.
Server listening on :: port 22.
LISTEN ... 0.0.0.0:22 ... users:(("sshd",pid=1891,fd=3))
LISTEN ... [::]:22       ... users:(("sshd",pid=1891,fd=4))
```

Вывод: `sshd` на плате работал корректно; проблема была в маршрутизации на
хосте.

Дополнительная диагностика:

```sh
sudo /usr/sbin/sshd -ddd -p 2222 -D -e
```

И с хоста:

```sh
ssh -vvv -p 2222 andrey@10.129.1.152
```

Debug `sshd` на плате не показывал строку `Connection from ...`, пока маршрут
шел через VPN. Это подтвердило, что попытка не доходила до debug-сервера на
BeagleBone.

Первая попытка исправления:

```sh
sudo ip route add 10.129.1.152/32 dev eno0
```

Она добавила route в `main` table, но не изменила результат `ip route get`,
потому что VPN использовал policy table `7113`.

Рабочее исправление:

```sh
sudo ip route add 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

Проверка:

```sh
ip route get 10.129.1.152
```

Рабочий результат:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110 uid 1000
    cache
```

Вывод:

- только трафик к `10.129.1.152/32` идет напрямую через `eno0`;
- VPN остается включенным для остального трафика;
- это исключение работает для SSH, SCP, rsync, ping и других соединений к
  плате.

Проверка SSH после исправления:

```sh
ssh andrey@10.129.1.152
```

При первом подключении подтвержден host key:

```text
ED25519 key fingerprint is SHA256:KscTrYzK7rNQxXp3yDxh1EfEuRmWj4hY1qqoC78/hoU.
```

Этот fingerprint совпал с ED25519 host key, который был виден в debug `sshd`:

```text
ssh-ed25519 SHA256:KscTrYzK7rNQxXp3yDxh1EfEuRmWj4hY1qqoC78/hoU
```

После подтверждения выполнен успешный вход:

```text
Debian GNU/Linux 12
BeagleBoard.org Debian Bookworm Base Image 2026-03-17
andrey@BeagleBone:~$
```

Итоговая рабочая схема:

```text
host eno0:         10.129.1.110/23
BeagleBone eth0:   10.129.1.152/23
VPN interface:     outline-tun1
VPN routing table: 7113
VPN bypass:        10.129.1.152/32 -> eno0 in table 7113
SSH:               works
```

Команда восстановления после перезагрузки хоста, перезапуска сети или VPN:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

Главная проверка:

```sh
ip route get 10.129.1.152
ssh andrey@10.129.1.152
```

Ожидаемый маршрут:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110
```

Откат временной настройки:

```sh
sudo ip route del 10.129.1.152/32 table 7113
```

Если остался route в `main` table:

```sh
sudo ip route del 10.129.1.152/32 dev eno0
```

Тесты для будущей передачи файлов:

```sh
ssh andrey@10.129.1.152 'hostname; uname -r; whoami'
scp README.md andrey@10.129.1.152:/home/andrey/
rsync -av modules/ andrey@10.129.1.152:/home/andrey/bbb-course/modules/
```

Финальная проверка host-to-target передачи перед коммитом:

```sh
ip route get 10.129.1.152
```

Результат:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110 uid 1000
    cache
```

Команда:

```sh
ssh andrey@10.129.1.152 'hostname; uname -r; whoami'
```

Результат:

```text
BeagleBone
6.12.76-bone50
andrey
```

Создание тестового файла на хосте:

```sh
printf 'bbb host to target transfer test\n' > /tmp/bbb_ssh_transfer_test.txt
```

Передача на плату:

```sh
scp /tmp/bbb_ssh_transfer_test.txt andrey@10.129.1.152:/home/andrey/
```

Результат:

```text
bbb_ssh_transfer_test.txt 100% 33
```

Проверка содержимого на плате через SSH:

```sh
ssh andrey@10.129.1.152 \
  'ls -l /home/andrey/bbb_ssh_transfer_test.txt; cat /home/andrey/bbb_ssh_transfer_test.txt'
```

Результат:

```text
-rw-r--r-- 1 andrey andrey 33 Apr 27 16:38 /home/andrey/bbb_ssh_transfer_test.txt
bbb host to target transfer test
```

Вывод:

```text
host -> eno0 -> switch -> BeagleBone eth0 -> SSH/SCP -> /home/andrey
```

проверен end-to-end. Канал готов для передачи исходников и модулей ядра.

Добавлен отдельный учебный раздел:

```text
course/06-host-target-network.md
```

### Единая HTML-точка входа

Для удобной навигации по учебной базе добавлен статический файл:

```text
docs/index.html
```

Назначение:

- дать одну точку входа в документацию;
- не искать информацию вручную по всем `.md` файлам;
- разделить учебную теорию, карту документов и базу команд;
- быстро находить команды по словам `ssh`, `vpn`, `device tree`, `adc`, `iio`,
  `u-boot`, `kernel`;
- открыть документацию в браузере без локального сервера.

Структура `docs/index.html`:

```text
Текущее состояние
Документы курса
Учебная теория
База команд
Рабочий цикл разработки
Следующие шаги
```

В базу команд вынесены проверенные команды:

```text
host-target network / SSH / SCP
Linux runtime checks
Device Tree inspection
ADC через IIO
U-Boot inspection
```

Также добавлены:

- поиск по странице;
- кнопки копирования команд;
- ссылки на все основные документы курса.

## 2026-04-28

### Уточнение режима работы ассистента

Создан файл:

```text
ASSISTANT_RULES.md
```

Назначение файла - фиксировать правила работы для следующих сессий:

- ассистент работает как учитель-консультант;
- практические команды на хосте и BeagleBone Black обычно выполняет
  пользователь;
- ассистент дает команду, объясняет смысл действия, ожидаемый результат и
  интерпретацию;
- документацию проекта ассистент может править сам;
- команды, сборку, SSH-действия и изменения кода ассистент выполняет только по
  явному запросу пользователя.

### Решение по маршруту к BeagleBone Black

После перезагрузки или перезапуска VPN маршрут к плате снова может уходить через
VPN policy table:

```text
10.129.1.152 via 10.0.85.5 dev outline-tun1 table 7113 src 10.0.85.5
```

Проверка интерфейса хоста показала, что рабочий адрес Ethernet не изменился:

```text
eno0 UP 10.129.1.110/23
```

Принятое решение: не делать исключение постоянной системной настройкой.
Временный route восстанавливается вручную только на время работы с BeagleBone
Black. Это уменьшает риск оставить в системе ненужную лабораторную настройку
после завершения работы с платой.

Рабочие параметры:

```text
BeagleBone IP: 10.129.1.152
host interface: eno0
host source IP: 10.129.1.110
VPN policy table: 7113
```

Ручная команда восстановления:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

Ожидаемый результат проверки:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110
```

### Helper-скрипт для временного маршрута

Добавлен скрипт:

```text
scripts/bbb-route.sh
```

Скрипт не делает настройку постоянной и не изменяет конфигурацию systemd,
NetworkManager или VPN. Он только упрощает ручные операции:

```sh
sh scripts/bbb-route.sh check
sh scripts/bbb-route.sh apply
sh scripts/bbb-route.sh delete
```

Режимы:

- `check` - показать текущий маршрут к BeagleBone Black;
- `apply` - добавить или заменить временный route в table `7113`;
- `delete` - удалить временный route из table `7113`.

### Проверка build tree для первого kernel module

На BeagleBone Black выполнена проверка текущего ядра:

```sh
hostname
uname -r
whoami
pwd
```

Результат:

```text
BeagleBone
6.12.76-bone50
andrey
/home/andrey
```

Проверен стандартный путь Kbuild для сборки внешних модулей:

```sh
ls -ld /lib/modules/$(uname -r)/build
readlink -f /lib/modules/$(uname -r)/build
```

Результат:

```text
/lib/modules/6.12.76-bone50/build -> /usr/src/linux-headers-6.12.76-bone50
/usr/src/linux-headers-6.12.76-bone50
```

Вывод:

- `/lib/modules/$(uname -r)/build` существует;
- `build` является symbolic link на каталог headers текущего ядра;
- kernel headers для `6.12.76-bone50` установлены;
- первый out-of-tree модуль можно пробовать собирать прямо на BeagleBone
  Black.

### Проверка инструментов сборки на BeagleBone Black

На target проверено наличие минимальных инструментов для сборки модуля:

```sh
which make gcc ld
make --version | head -n 1
gcc --version | head -n 1
ld --version | head -n 1
```

Результат:

```text
/usr/bin/make
/usr/bin/gcc
/usr/bin/ld
GNU Make 4.3
gcc (Debian 12.2.0-14+deb12u1) 12.2.0
GNU ld (GNU Binutils for Debian) 2.40
```

Вывод:

- `make`, `gcc` и `ld` установлены на BeagleBone Black;
- target готов к первой сборке out-of-tree модуля через Kbuild;
- сборку первого модуля можно выполнить на плате, а исходники при этом держать
  в репозитории на хосте.

### Уточнение host/target workflow

Уточнен рабочий процесс для первого kernel module.

Правильная схема:

```text
Linux host / VS Code
  -> редактирование modules/hello/hello.c и Makefile
  -> передача на BeagleBone по SSH/SCP/rsync
  -> сборка и проверка на BeagleBone
```

Причина: исходники курса должны жить в git-репозитории на хосте, а BeagleBone
Black используется как target с реальным ядром `6.12.76-bone50`, headers и
Kbuild environment.

При интерактивном входе по SSH prompt меняется на target:

```text
andrey@BeagleBone:~$
```

При разовой SSH-команде:

```sh
ssh andrey@10.129.1.152 'hostname; uname -r; whoami'
```

команда внутри кавычек выполняется на BeagleBone, но после завершения SSH
пользователь возвращается в shell хоста:

```text
andrey@andrey-yunin:~/projects/BeagleBone_Black$
```

Для снижения путаницы принято использовать два окна терминала:

```text
Окно 1: BeagleBone Black через интерактивный SSH
Окно 2: Linux host в репозитории ~/projects/BeagleBone_Black
```

### Каталог первого модуля на хосте

На Linux-хосте в репозитории создан каталог:

```sh
mkdir -p modules/hello
ls -ld modules modules/hello
```

Результат:

```text
drwxrwxr-x 3 andrey andrey 4096 апр 28 09:38 modules
drwxrwxr-x 2 andrey andrey 4096 апр 28 09:38 modules/hello
```

Пояснение по `mkdir -p`:

- `-p` означает `parents`;
- команда создает всю цепочку недостающих каталогов;
- если каталог уже существует, это не считается ошибкой;
- одного аргумента с вложенным путем достаточно, например
  `mkdir -p modules/hello`.

Также разобрана команда просмотра части файла:

```sh
sed -n '1,120p' modules/hello/hello.c
```

Пояснение:

- `sed` - stream editor, здесь используется как просмотрщик выбранного диапазона
  строк;
- `-n` отключает автоматическую печать всех строк;
- `'1,120p'` печатает строки с 1 по 120;
- последний аргумент - путь к файлу.

Команды `mkdir -p` и `sed -n '1,120p'` добавлены в HTML-справочник
`docs/index.html` в раздел базовых shell-команд.

### Начало разбора исходника hello kernel module

Для первого модуля выбран минимальный исходник:

```c
#include <linux/init.h>
#include <linux/module.h>

static int __init bbb_hello_init(void)
{
    pr_info("bbb_hello: init\n");
    return 0;
}

static void __exit bbb_hello_exit(void)
{
    pr_info("bbb_hello: exit\n");
}

module_init(bbb_hello_init);
module_exit(bbb_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey");
MODULE_DESCRIPTION("Minimal BeagleBone Black hello kernel module");
```

На хосте VS Code может подсвечивать ошибки для:

```c
#include <linux/init.h>
#include <linux/module.h>
```

Причина: это kernel headers target-ядра, а не обычные userspace C headers.
Headers текущего target-ядра находятся на BeagleBone Black:

```text
/usr/src/linux-headers-6.12.76-bone50
```

Поэтому подсветка VS Code на хосте является проблемой IntelliSense, а не
доказательством ошибки сборки. Реальная проверка будет выполняться Kbuild на
BeagleBone Black.

Разобрана модель init/exit функций:

- имена `bbb_hello_init` и `bbb_hello_exit` выбраны разработчиком;
- стандартным является механизм регистрации через `module_init()` и
  `module_exit()`;
- init-функция имеет тип `int`, потому что загрузка модуля может завершиться
  успешно или с ошибкой;
- `return 0` означает успешную загрузку;
- при ошибке init-функция вернула бы отрицательный errno, например `-ENOMEM`,
  `-ENODEV` или `-EINVAL`;
- exit-функция имеет тип `void`, потому что при выгрузке она не принимает
  решение о результате, а освобождает ресурсы;
- `__init` и `__exit` являются kernel-аннотациями для размещения кода в
  специальных секциях;
- `static` ограничивает видимость функций текущим `.c` файлом.

### Kernel logging: pr_info и printk

Разобран вывод сообщений из kernel module.

В первом модуле используется:

```c
pr_info("bbb_hello: init\n");
```

Вывод:

- `pr_info()` - стандартный kernel logging macro для информационных сообщений;
- сообщение пишется в kernel log, а не обязательно прямо в текущий терминал;
- читать сообщения можно через `dmesg` или `journalctl -k`;
- в ядре не используется обычный userspace `printf()` как основной интерфейс
  вывода, потому что у модуля ядра нет стандартного `stdout`;
- `pr_info()` по смыслу близок к `printk(KERN_INFO "...")`;
- `printk()` является более базовым интерфейсом, где уровень лога обычно
  указывается явно через `KERN_INFO`, `KERN_WARNING`, `KERN_ERR` и т.п.;
- для настоящих драйверов устройств при наличии `struct device *dev` часто
  предпочтительнее `dev_info()`, `dev_err()` и другие `dev_*()` helpers,
  потому что они добавляют контекст конкретного устройства.

### Начальный debugging workflow для kernel module

Зафиксирована базовая модель отладки первого kernel module.

Короткая схема:

```text
1. Kbuild проверяет совместимость с headers/API текущего ядра.
2. insmod/rmmod проверяют загрузку и выгрузку модуля.
3. dmesg или journalctl -k показывают kernel log.
4. pr_info/pr_err/pr_debug помогают отслеживать выполнение кода.
```

Для учебного модуля допустимо ставить информационные сообщения в ключевые
точки:

```c
pr_info("bbb_hello: init enter\n");
pr_info("bbb_hello: init done\n");
pr_info("bbb_hello: exit enter\n");
```

В более аккуратном драйверном коде уровни разделяются:

```c
pr_info("initialized\n");
pr_err("failed: %d\n", ret);
pr_debug("value=%d\n", value);
```

Базовый цикл первого этапа:

```text
редактировать на хосте
  -> передать на BBB
  -> собрать через Kbuild
  -> загрузить insmod
  -> проверить dmesg
  -> выгрузить rmmod
  -> проверить dmesg
  -> исправить код
```

Более тяжелые инструменты (`dynamic debug`, `debugfs`, `ftrace`, `tracepoints`,
`perf`, `kgdb`, `JTAG`) оставлены для следующих этапов после освоения базового
цикла.

### Подготовка исходников первого модуля на хосте

Исходники первого kernel module создавались на Linux-хосте в репозитории, а не
на BeagleBone Black напрямую.

Рабочий каталог на хосте:

```text
/home/andrey/projects/BeagleBone_Black
```

Создан файл:

```text
modules/hello/hello.c
```

Проверка содержимого на хосте:

```sh
sed -n '1,120p' modules/hello/hello.c
```

Итоговое содержимое:

```c
#include <linux/init.h>
#include <linux/module.h>

static int __init bbb_hello_init(void)
{
    pr_info("bbb_hello: init\n");
    return 0;
}

static void __exit bbb_hello_exit(void)
{
    pr_info("bbb_hello: exit\n");
}

module_init(bbb_hello_init);
module_exit(bbb_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey");
MODULE_DESCRIPTION("Minimal BeagleBone Black hello kernel module");
```

Сначала в файле были лишние отступы перед top-level объявлениями:

```c
  static int __init ...
  module_init(...)
  MODULE_LICENSE(...)
```

Для C-компилятора это допустимо, но для учебного kernel-style принято решение
выровнять top-level объявления без начальных пробелов.

### Makefile для Kbuild

На хосте создан файл:

```text
modules/hello/Makefile
```

Содержимое:

```make
obj-m += hello.o
```

Проверка:

```sh
sed -n '1,40p' modules/hello/Makefile
```

Результат:

```text
obj-m += hello.o
```

Пояснение:

- `obj-m` - специальная переменная Kbuild;
- `m` означает module, то есть сборку загружаемого `.ko`;
- `+=` добавляет объект в список модулей;
- `hello.o` собирается из `hello.c`;
- итоговая цепочка: `hello.c -> hello.o -> hello.ko`.

Изначально перед `obj-m` был лишний отступ:

```make
  obj-m += hello.o
```

Для этой строки Makefile это не должно ломать сборку, но отступы в Makefile
имеют особое значение, поэтому строка была приведена к чистому виду без
начальных пробелов.

### Синхронизация исходников на BeagleBone Black

Передача исходников с хоста на target выполнена через `rsync`:

```sh
rsync -av modules/hello/ andrey@10.129.1.152:/home/andrey/bbb-course/modules/hello/
```

Пояснение команды:

- `rsync` синхронизирует файлы и каталоги;
- `-a` включает archive mode: рекурсивная передача, сохранение структуры, прав
  и временных меток;
- `-v` включает verbose output;
- `modules/hello/` - источник на хосте;
- финальный `/` после `hello/` означает передачу содержимого каталога, а не
  создание вложенного каталога `hello/hello`;
- `andrey@10.129.1.152:` - SSH-доступ к BeagleBone Black;
- `/home/andrey/bbb-course/modules/hello/` - каталог назначения на target.

Результат синхронизации:

```text
sending incremental file list
./
Makefile
hello.c

sent 641 bytes  received 57 bytes  55,84 bytes/sec
total size is 442  speedup is 0,63
```

Вывод:

- SSH/SCP/rsync workflow работает;
- на target переданы `Makefile` и `hello.c`;
- исходники продолжают жить в git-репозитории на хосте;
- BeagleBone используется для сборки и проверки под своим running kernel.

### Проверка исходников на BeagleBone Black перед сборкой

На BeagleBone Black выполнен переход в каталог модуля:

```sh
cd ~/bbb-course/modules/hello
```

Проверка файлов:

```sh
ls -l
```

Результат:

```text
total 8
-rw-rw-r-- 1 andrey andrey  17 Apr 28 10:50 Makefile
-rw-rw-r-- 1 andrey andrey 425 Apr 28 10:15 hello.c
```

После исправления форматирования и повторной синхронизации проверено итоговое
содержимое:

```sh
sed -n '1,120p' hello.c
sed -n '1,120p' Makefile
```

Ключевой результат:

```text
obj-m += hello.o
```

и `hello.c` без лишних top-level отступов. После этой проверки каталог target
готов к сборке через Kbuild.

### Сборка первого Hello kernel module

На BeagleBone Black в каталоге:

```text
/home/andrey/bbb-course/modules/hello
```

выполнена сборка внешнего модуля:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Пояснение команды:

- `make` запускает систему сборки;
- `-C /lib/modules/$(uname -r)/build` переводит `make` в build tree текущего
  ядра;
- `$(uname -r)` подставляет `6.12.76-bone50`;
- `M=$PWD` сообщает Kbuild, что внешний модуль находится в текущем каталоге;
- `modules` - цель Kbuild для сборки модулей.

Результат сборки:

```text
make: Entering directory '/usr/src/linux-headers-6.12.76-bone50'
  CC [M]  /home/andrey/bbb-course/modules/hello/hello.o
  MODPOST /home/andrey/bbb-course/modules/hello/Module.symvers
  CC [M]  /home/andrey/bbb-course/modules/hello/hello.mod.o
  CC [M]  /home/andrey/bbb-course/modules/hello/.module-common.o
  LD [M]  /home/andrey/bbb-course/modules/hello/hello.ko
make: Leaving directory '/usr/src/linux-headers-6.12.76-bone50'
```

Итоговый файл:

```text
hello.ko
```

Ключевые файлы после сборки:

- `hello.c` - исходник;
- `Makefile` - инструкция для Kbuild;
- `hello.o` - объектный файл из `hello.c`;
- `hello.mod.c` - сгенерированный Kbuild файл с metadata;
- `hello.mod.o` - объект metadata;
- `hello.ko` - итоговый загружаемый kernel module;
- `Module.symvers` - таблица символов;
- `modules.order` - порядок модулей;
- `.hello.*.cmd` - служебные файлы Kbuild с командами сборки и зависимостями.

### Проверка metadata через modinfo

Попытка выполнить:

```sh
modinfo hello.ko
```

дала результат:

```text
modinfo: команда не найдена
```

Дополнительно была выполнена команда:

```sh
sudo apt update
```

Результат: списки пакетов обновились, но `modinfo` не появился как команда в
обычном `PATH`. Это ожидаемо: `apt update` только обновляет индексы пакетов и
не устанавливает новые пакеты.

Важно: сообщение `4 packages can be upgraded` не является указанием обновлять
систему в рамках этого этапа. Kernel-пакеты и связанные компоненты не обновляем
без отдельного решения.

Команда `modinfo` не находилась через обычный `PATH`, хотя пакет `kmod` уже был
установлен:

```sh
dpkg -l kmod
```

Результат:

```text
ii  kmod  30+20221128-1 armhf  tools for managing Linux kernel modules
```

Проверка показала, что `modinfo` находится в `sbin`:

```sh
which insmod rmmod lsmod modinfo
ls -l /usr/sbin/modinfo /sbin/modinfo
```

Результат:

```text
/usr/bin/lsmod
```

и:

```text
/sbin/modinfo -> /bin/kmod
/usr/sbin/modinfo -> /bin/kmod
```

Вывод: у пользователя `andrey` в `PATH` есть `lsmod`, но нет пути к `modinfo`
в `sbin`. Поэтому для административных утилит модулей на этой системе удобно
использовать полный путь.

Поэтому использован полный путь:

```sh
/usr/sbin/modinfo hello.ko
```

Результат:

```text
filename:       /home/andrey/bbb-course/modules/hello/hello.ko
description:    Minimal BeagleBone Black hello kernel module
author:         Andrey
license:        GPL
depends:
name:           hello
vermagic:       6.12.76-bone50 preempt mod_unload modversions ARMv7 thumb2 p2v8
```

Вывод:

- metadata из `MODULE_DESCRIPTION`, `MODULE_AUTHOR`, `MODULE_LICENSE` попала в
  модуль;
- `vermagic` соответствует ядру `6.12.76-bone50`;
- модуль собран под ARMv7 target, а не под x86_64 host.

### Загрузка и выгрузка первого модуля

Перед загрузкой проверен хвост kernel log:

```sh
dmesg | tail -n 20
```

Затем модуль загружен:

```sh
sudo /usr/sbin/insmod hello.ko
```

`insmod` ничего не вывел, что нормально для успешного выполнения Unix-команды.

Проверка `dmesg` показала:

```text
[ 7189.137718] hello: loading out-of-tree module taints kernel.
[ 7189.147553] bbb_hello: init
```

Вывод:

- модуль загружен в ядро;
- init-функция `bbb_hello_init()` выполнена;
- сообщение `pr_info("bbb_hello: init\n")` попало в kernel log;
- ядро помечено как tainted из-за загрузки out-of-tree модуля.

Проверка `lsmod`:

```sh
lsmod | grep hello
```

Результат:

```text
hello                  12288  0
```

Расшифровка:

- `hello` - имя модуля;
- `12288` - размер в памяти;
- `0` - счетчик использования.

Модуль выгружен:

```sh
sudo /usr/sbin/rmmod hello
```

Проверка `dmesg`:

```text
[ 7328.004583] bbb_hello: exit
```

Проверка:

```sh
lsmod | grep hello
```

не вывела строк, значит модуль больше не загружен.

Итог: первый out-of-tree kernel module успешно прошел полный цикл:

```text
редактирование на хосте
  -> rsync на BeagleBone
  -> Kbuild сборка на target
  -> modinfo
  -> insmod
  -> dmesg init
  -> lsmod
  -> rmmod
  -> dmesg exit
```

### Систематизация HTML-справочника команд

Обновлен справочник команд в:

```text
docs/index.html
```

Команды сначала были разнесены по темам текущей работы, затем порядок изменен
по мере развития курса: от подготовки Linux-хоста и загрузочной цепочки к
Linux runtime, Device Tree/IIO, host-target workflow и только потом к kernel
modules.

Актуальные разделы:

```text
0. Базовые shell-команды
1. Linux host и подготовка microSD
2. UART и первая загрузка
3. U-Boot
4. Linux runtime на BBB
5. Device Tree
6. ADC через IIO
7. Host-target сеть, SSH и rsync
8. Kernel modules: подготовка исходников на хосте
9. Kernel modules: target-среда и Kbuild
10. Kernel modules: metadata, загрузка и выгрузка
```

В справочник добавлены команды полного цикла первого модуля:

- создание и проверка `modules/hello`;
- проверка `hello.c` и `Makefile`;
- синхронизация через `rsync`;
- проверка target identity;
- проверка `/lib/modules/$(uname -r)/build`;
- проверка `make`, `gcc`, `ld`;
- сборка через Kbuild;
- просмотр артефактов сборки;
- очистка Kbuild-артефактов через `make ... clean`;
- диагностика `kmod`, `PATH` и `modinfo`;
- `insmod`, `dmesg`, `lsmod`, `rmmod`;
- альтернативный просмотр kernel log через `journalctl -k`.

### Теория следующего шага: параметры модуля

После успешного `hello` module разобрана теория `module_param`.

Ключевая идея:

```text
module parameter позволяет менять заранее предусмотренное поведение модуля без
изменения исходника и без пересборки .ko
```

Пример:

```sh
sudo /usr/sbin/insmod hello.ko username=Andrey
sudo /usr/sbin/insmod hello.ko username=Test
```

Один и тот же `hello.ko` загружается с разными значениями параметра.

Минимальная схема в коде:

```c
static char *username = "world";
module_param(username, charp, 0444);
MODULE_PARM_DESC(username, "Name to print in the hello message");
```

Разобрано:

- параметр должен быть заранее описан в коде;
- `charp` означает строковый параметр;
- `0444` делает параметр read-only через sysfs;
- `MODULE_PARM_DESC` добавляет описание в `modinfo`;
- без `module_param(username, ...)` значение `username=...` при загрузке не
  будет принято ядром.

Реальные применения:

- `debug=1` - включить подробные диагностические логи;
- `poll_ms=1000` - задать интервал опроса;
- `timeout_ms=500` - задать timeout;
- `buffer_size=4096` - задать размер буфера, если драйвер поддерживает это
  безопасно.

Разобрана временность ручной загрузки:

```sh
sudo /usr/sbin/insmod hello.ko username=Andrey
```

После reboot вручную загруженный модуль и его параметр не сохраняются.

Для автозагрузки теоретически используется схема:

```text
/lib/modules/$(uname -r)/extra/hello.ko
/etc/modules-load.d/hello.conf
/etc/modprobe.d/hello.conf
```

и параметры:

```text
options hello username=Andrey
```

Разница:

```text
insmod
  -> грузит конкретный .ko файл
  -> не читает /etc/modprobe.d
  -> не сохраняется после reboot

modprobe
  -> ищет модуль в /lib/modules/$(uname -r)/
  -> учитывает зависимости
  -> читает /etc/modprobe.d/*.conf
  -> подходит для автозагрузки
```

Для текущего курса принято не включать автозагрузку, потому что это постоянное
изменение target-системы.

Отдельно разобраны права параметров:

```c
module_param(username, charp, 0444);
module_param(debug, bool, 0644);
```

- `0444` - параметр можно читать через `/sys/module/.../parameters`, но нельзя
  менять после загрузки;
- `0644` - root может менять параметр через sysfs во время работы модуля.

Разобран риск writable parameters:

```text
если параметр меняется из userspace в момент, когда драйвер активно использует
его в ядре, возможны race condition, неконсистентное состояние, выход за пределы
массива, use-after-free, деление на ноль, kernel oops или panic
```

Уточнена формулировка про `0644`.

`0644` не делает параметр безопасным. Он только разрешает root запись в sysfs:

```text
0644 = политика доступа к sysfs-файлу
валидация и безопасность = ответственность кода драйвера
```

Он не проверяет:

```text
валидное ли значение;
безопасно ли менять его сейчас;
не нужен ли lock;
не идет ли параллельное чтение;
не сломается ли структура данных;
можно ли менять hardware mode на лету.
```

Корректная формулировка для будущего writable-параметра:

```text
verbose можно сделать writable не потому, что 0644 сам по себе безопасен,
а потому что код использует verbose только как переключатель дополнительных
логов
```

Если writable-параметр используется как индекс массива, размер буфера, режим
железа или адрес устройства, безопасность должна обеспечиваться валидацией,
синхронизацией и правильной логикой применения значения.

### Практика writable-параметра verbose

В `hello.c` добавлен второй параметр:

```c
static bool verbose;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable verbose hello module messages");
```

Итоговая модель параметров:

```text
username: charp, 0444
verbose:  bool,  0644
```

После синхронизации на BeagleBone Black и пересборки:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

`modinfo` показал:

```text
parm:           username:Name to print in the hello message (charp)
parm:           verbose:Enable verbose hello module messages (bool)
```

Модуль загружен с двумя параметрами:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin verbose=1
```

Проверка `dmesg`:

```text
[27365.184170] bbb_hello: init, username=Yunin
[27365.189237] bbb_hello: verbose mode is enabled
```

Проверка sysfs:

```sh
cat /sys/module/hello/parameters/username
cat /sys/module/hello/parameters/verbose
ls -l /sys/module/hello/parameters/username /sys/module/hello/parameters/verbose
```

Результат:

```text
Yunin
Y
-r--r--r-- 1 root root 4096 Apr 28 16:52 /sys/module/hello/parameters/username
-rw-r--r-- 1 root root 4096 Apr 28 16:52 /sys/module/hello/parameters/verbose
```

Вывод:

- `username` остался read-only параметром;
- `verbose` стал writable параметром для root;
- bool-параметры в sysfs отображаются как `Y` / `N`.

Runtime-изменение:

```sh
echo 0 | sudo tee /sys/module/hello/parameters/verbose
cat /sys/module/hello/parameters/verbose
```

Результат:

```text
0
N
```

При проверке `dmesg` после записи нового значения сразу не появилось новой
строки. Это ожидаемо: обычный `module_param` меняет переменную, но не вызывает
пользовательский callback в момент записи.

Код читает `verbose` только в `init` и `exit`. После изменения `verbose` с `Y`
на `N` при выгрузке:

```sh
sudo /usr/sbin/rmmod hello
```

в `dmesg` появилась строка:

```text
[27861.472041] bbb_hello: exit, username=Yunin
```

и не появилась строка:

```text
bbb_hello: verbose mode was enabled
```

Вывод: runtime-запись в sysfs изменила переменную `verbose`, но эффект виден
только в том месте кода, где модуль потом прочитал эту переменную.

Отдельно зафиксирован следующий механизм для будущего изучения:

```text
module_param_cb
```

Он нужен, если драйвер должен выполнить custom callback прямо в момент записи
нового значения параметра через sysfs.

Вывод для следующей практики:

```text
username: charp, 0444
```

Это безопасный read-only параметр: задается при `insmod`, читается через sysfs,
но не меняется во время работы. Writable параметр `debug: bool, 0644` лучше
разобрать позже отдельным шагом.

### Sysfs как точка доступа к параметру модуля

После добавления:

```c
static char *username = "world";
module_param(username, charp, 0444);
MODULE_PARM_DESC(username, "Name to print in the hello message");
```

собранный модуль показывает параметр в `modinfo`:

```text
parm: username:Name to print in the hello message (charp)
```

Разобрано, что после загрузки:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin
```

ядро создает sysfs-представление параметра:

```text
/sys/module/hello/parameters/username
```

Практическая модель:

```text
/sys/module/hello/parameters/username
  -> точка доступа к параметру username загруженного модуля hello
```

Значение `Yunin` хранится не в обычном файле на microSD/eMMC, а в RAM ядра, в
переменной загруженного модуля. `sysfs` дает контролируемый файловый интерфейс
к этому значению.

При чтении:

```sh
cat /sys/module/hello/parameters/username
```

userspace-программа `cat` обращается к sysfs, а ядро возвращает текущее значение
параметра как текст:

```text
Yunin
```

Важно: этот путь не является адресом RAM и не дает прямой доступ к памяти ядра.
Это именованная точка доступа, которую ядро связывает с параметром модуля.

Проверка прав:

```sh
ls -l /sys/module/hello/parameters/username
```

Результат:

```text
-r--r--r-- 1 root root 4096 Apr 28 14:21 /sys/module/hello/parameters/username
```

Это соответствует `0444`: читать можно, писать нельзя.

Попытка записи:

```sh
echo Test | sudo tee /sys/module/hello/parameters/username
```

Результат:

```text
tee: /sys/module/hello/parameters/username: Permission denied
Test
```

`Permission denied` подтверждает read-only режим. Строка `Test` дополнительно
появилась в терминале потому, что `tee` дублирует входные данные в stdout даже
при ошибке записи в файл.

Повторное чтение:

```sh
cat /sys/module/hello/parameters/username
```

Результат:

```text
Yunin
```

Вывод: параметр был задан при `insmod`, доступен для чтения через sysfs, но не
изменяется во время работы из-за прав `0444`.

### Минимальная модель памяти для kernel module

В ходе обсуждения параметра `username` разобрана базовая модель памяти
загружаемого модуля.

Главная формулировка:

```text
модуль .ko размещается в RAM ядра, но код ядра работает с kernel virtual
addresses, а не напрямую с физическими адресами RAM
```

При загрузке:

```sh
sudo /usr/sbin/insmod hello.ko
```

ядро разбирает ELF-файл модуля и размещает его секции:

```text
.text      -> машинный код функций
.rodata    -> read-only данные и строковые литералы
.data      -> инициализированные global/static переменные
.bss       -> неинициализированные global/static переменные
.init.text -> код инициализации
.exit.text -> код выгрузки
```

Модуль не является одной простой непрерывной C-структурой. Он ближе к набору
ELF-секций, которые kernel module loader размещает в kernel virtual address
space, выравнивает, настраивает права доступа и исправляет relocations.

Аналогия с массивом/структурой частично полезна:

```text
внутри секции есть base address + offset
конкретная функция или переменная имеет адрес
```

Но точнее:

```text
kernel module = ELF-секции, размещенные в kernel virtual address space
```

Для нашего кода:

```c
static char *username = "world";
```

- переменная `username` имеет адрес в данных модуля;
- строковый литерал `"world"` обычно лежит в read-only данных;
- `bbb_hello_init()` имеет адрес в кодовой/init-секции;
- `bbb_hello_exit()` имеет адрес в exit-секции.

Все эти адреса являются kernel virtual addresses.

Разобрана роль MMU:

```text
software / CPU instruction
        |
        v
virtual address
        |
        v
MMU + page tables
        |
        v
physical address
        |
        v
RAM or device registers
```

Ключевой вывод:

```text
software обычно работает с virtual addresses
MMU переводит virtual -> physical
hardware видит physical addresses
```

При этом physical address может указывать не только на RAM:

```text
physical RAM addresses
  -> реальные ячейки оперативной памяти

physical MMIO addresses
  -> регистры периферийных устройств
```

Устройства SoC обычно имеют свои physical address ranges в общей physical
address map, а не "подключаются к RAM-адресам".

Для RAM:

```text
kernel virtual address -> physical RAM
```

Для устройства:

```text
kernel virtual address -> physical MMIO registers
```

Связь с будущими драйверами:

```text
Device Tree reg
  -> physical MMIO address
  -> ioremap / devm_ioremap_resource
  -> kernel virtual address
  -> readl / writel
```

Для текущего модуля:

```text
hello.ko загружен
  -> код и данные размещены в kernel virtual address space
  -> MMU связывает virtual addresses с physical RAM
  -> username существует как переменная загруженного модуля
  -> /sys/module/hello/parameters/username дает точку доступа к значению
```

После выгрузки:

```sh
sudo /usr/sbin/rmmod hello
```

ядро удаляет sysfs-представление и освобождает память, занятую кодом, данными и
служебными структурами модуля.

Практический вывод:

```text
нет модуля в ядре
  -> нет активной переменной username
  -> нет /sys/module/hello/parameters/username
```

### Точка доступа к ресурсу ядра

Сформулировано общее правило доступа userspace к ресурсам ядра.

Короткая версия:

```text
если ядро специально открыло ресурс через интерфейс,
то со стороны userspace достаточно знать точку доступа и иметь права
```

Для нашего примера:

```text
ресурс ядра: параметр username модуля hello
точка доступа: /sys/module/hello/parameters/username
права: -r--r--r--
действие: cat /sys/module/hello/parameters/username
```

Команда чтения работает потому, что:

- модуль `hello` загружен;
- параметр `username` зарегистрирован через `module_param`;
- ядро создало sysfs-точку доступа;
- права `0444` разрешают чтение.

Запись через:

```sh
echo Test | sudo tee /sys/module/hello/parameters/username
```

не работает, потому что права `0444` не разрешают write-доступ.

Разобрано важное ограничение:

```text
userspace не может обращаться к любому ресурсу ядра просто по адресу
```

Нужен официальный интерфейс, например:

```text
/sys
/proc
/dev
debugfs
ioctl
netlink
syscall
```

Вывод:

```text
userspace работает не с адресами объектов ядра,
а с точками доступа, которые ядро специально предоставляет и защищает правами
```

Добавлен краткий справочник по этим интерфейсам:

```text
/sys
  Sysfs: устройства, шины, драйверы, классы устройств, модули и параметры.
  Примеры:
    /sys/module/hello/parameters/username
    /sys/bus/iio/devices/iio:device0/in_voltage0_raw

/proc
  Procfs: процессы и runtime-информация ядра.
  Примеры:
    /proc/cmdline
    /proc/device-tree

/dev
  Device files: character/block devices для read/write/mmap/ioctl.
  Примеры:
    /dev/ttyUSB0
    /dev/mmcblk0

debugfs
  Отладочная файловая система ядра, обычно /sys/kernel/debug.
  Не считается стабильным userspace ABI.

ioctl
  Управляющие команды к драйверу через открытый file descriptor.

netlink
  Обмен сообщениями между kernel и userspace через socket API.

syscall
  Базовый контролируемый вход userspace в ядро: open/read/write/ioctl/socket.
```

Общий смысл:

```text
ядро не отдает userspace прямой адрес объекта,
а предоставляет контролируемую операцию через выбранный интерфейс
```

### Завершение сессии 2026-04-28

Сессия завершена после проверки второго параметра модуля:

```text
username: charp, 0444
verbose:  bool,  0644
```

Практически проверено:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin verbose=1
dmesg | tail -n 20
cat /sys/module/hello/parameters/username
cat /sys/module/hello/parameters/verbose
ls -l /sys/module/hello/parameters/username /sys/module/hello/parameters/verbose
echo 0 | sudo tee /sys/module/hello/parameters/verbose
cat /sys/module/hello/parameters/verbose
sudo /usr/sbin/rmmod hello
dmesg | tail -n 20
cat /sys/module/hello/parameters/username
```

Ключевые результаты:

```text
bbb_hello: init, username=Yunin
bbb_hello: verbose mode is enabled
username -> Yunin
verbose  -> Y
username permissions -> -r--r--r--
verbose permissions  -> -rw-r--r--
echo 0 -> verbose becomes N
bbb_hello: exit, username=Yunin
after rmmod -> /sys/module/hello/parameters/username does not exist
```

Важный вывод:

```text
обычный module_param меняет связанную переменную,
но не вызывает пользовательский обработчик в момент записи через sysfs
```

Эффект изменения `verbose` стал виден только при следующем чтении переменной в
коде модуля, то есть в `bbb_hello_exit()`.

Следующий учебный шаг:

```text
module_param_cb
```

Цель следующей сессии:

- разобрать custom set/get callbacks;
- показать, как выполнить код именно в момент записи параметра через sysfs;
- добавить валидацию значения;
- сравнить `module_param` и `module_param_cb`;
- сохранить прежний host -> rsync -> BBB -> Kbuild -> insmod/rmmod workflow.

Для быстрого входа в следующую сессию обновлен:

```text
NEXT_SESSION.md
```

### Перенос HTML-справочника в docs

Статическая HTML-точка входа перенесена:

```text
index.html -> docs/index.html
```

Причина: файл `index.html` в корне репозитория мешал GitHub корректно
воспринимать проект как в первую очередь документацию/код для embedded Linux и
модулей ядра, а не как HTML-проект.

Обновлены ссылки:

```text
README.md
NEXT_SESSION.md
docs/index.html
REPORT.md
```

Внутренние ссылки из `docs/index.html` к `README.md`, `REPORT.md`,
`NEXT_SESSION.md` и файлам `course/` скорректированы на относительные пути
через `../`.

Дополнительно добавлен файл:

```text
.gitattributes
```

Содержимое:

```text
docs/index.html linguist-documentation
```

Это подсказывает GitHub Linguist считать HTML-справочник документацией, чтобы он
не перетягивал языковую статистику репозитория в сторону HTML.

## 2026-04-29

### Планирование HC-06 Bluetooth UART lab

В дальний план курса добавлена отдельная будущая лабораторная работа с модулем
**HC-06** после этапа встроенного ADC AM335x и перед внешним ADS1115.

Принятое техническое уточнение: HC-06 не требует написания нового
Bluetooth-драйвера Linux. Этот модуль сам реализует Bluetooth SPP, а со стороны
BeagleBone Black выглядит как обычное UART-устройство.

Учебная модель будущей лабораторной:

```text
смартфон app
  -> Bluetooth SPP
  -> HC-06
  -> UART TX/RX
  -> AM335x UART controller
  -> Linux tty driver
  -> /dev/ttyS*
  -> userspace read/write
```

Цель этапа:

- выбрать и включить свободный UART BeagleBone Black;
- разобрать pinmux и Device Tree/config-pin для UART;
- безопасно подключить `TX/RX/GND` с учетом уровней `3.3 V`;
- принять символы со смартфона через Linux tty;
- отправить ответ с BeagleBone Black обратно на смартфон;
- написать минимальную userspace-программу на C через `termios`.

К теме возвращаемся позже, когда будет завершен текущий блок по параметрам
модулей и этап встроенного ADC/IIO.

## 2026-04-30

### Восстановление состояния после некорректного выхода

При сверке после некорректного выхода обнаружено расхождение между документацией
и локальным исходником:

```text
REPORT/NEXT_SESSION/course:
  module_param_cb описан как следующий учебный шаг

modules/hello/hello.c:
  log_level через module_param_cb уже подготовлен локально
```

Принятое состояние:

- `username: charp, 0444` и `verbose: bool, 0644` уже практически проверены на
  BeagleBone Black;
- `log_level: int, 0644` через `module_param_cb` есть в локальном исходнике;
- `log_level` еще не считается практически проверенным, пока не выполнен цикл
  `rsync -> make -> modinfo -> insmod -> sysfs write -> dmesg -> rmmod` на
  BeagleBone Black.

Смысл следующей практики: сравнить обычный `module_param` и `module_param_cb`.
Обычный параметр меняет связанную переменную через стандартный обработчик, а
callback-параметр дает драйверу выполнить собственный код в момент записи через
sysfs: распарсить значение, проверить диапазон, вернуть `-EINVAL` при ошибке и
залогировать успешное изменение.

### Сборка hello module с module_param_cb

Обновленный `modules/hello/hello.c` с параметром `log_level` передан на
BeagleBone Black через `rsync`.

Сборка выполнена на target:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Результат:

```text
CC [M]  /home/andrey/bbb-course/modules/hello/hello.o
MODPOST /home/andrey/bbb-course/modules/hello/Module.symvers
CC [M]  /home/andrey/bbb-course/modules/hello/hello.mod.o
LD [M]  /home/andrey/bbb-course/modules/hello/hello.ko
```

Вывод: код с `module_param_cb` успешно собрался против headers текущего ядра
`6.12.76-bone50`. Следующий шаг - проверить metadata через `modinfo` и
убедиться, что параметр `log_level` появился в `.ko`.

Важное разделение проверок:

```text
modinfo hello.ko
  -> проверяет static/module metadata внутри .ko
  -> показывает, что параметр объявлен и виден в файле модуля

insmod/sysfs/dmesg
  -> проверяют runtime-поведение уже загруженного модуля в ядре
```

То есть `modinfo` отвечает на вопрос:

```text
Собрался ли .ko так, что ядро видит объявленный параметр модуля?
```

Runtime-проверки отвечают на другие вопросы:

```text
Принял ли insmod значение log_level=2?
Создался ли /sys/module/hello/parameters/log_level?
Вызывается ли log_level_set() при записи через sysfs?
Отклоняется ли неверное значение через -EINVAL?
Появляется ли ожидаемая строка в dmesg?
```

Формула для курса:

```text
modinfo подтверждает наличие параметра в metadata .ko,
а insmod/sysfs/dmesg подтверждают runtime-логику параметра.
```

Проверка `modinfo` на BeagleBone Black показала:

```text
filename:       /home/andrey/bbb-course/modules/hello/hello.ko
description:    Minimal BeagleBone Black hello kernel module
author:         Andrey
license:        GPL
name:           hello
vermagic:       6.12.76-bone50 preempt mod_unload modversions ARMv7 thumb2 p2v8
parm:           username:Name to print in the hello message (charp)
parm:           verbose:Enable verbose hello module messages (bool)
parm:           log_level:Logging level from 0 to 3
```

Вывод: `log_level` появился в metadata собранного `.ko`, значит параметр
объявлен и виден для kernel module loader. Отсутствие `(int)` у `log_level` не
считается ошибкой: параметр зарегистрирован через `module_param_cb` с custom
callbacks, поэтому `modinfo` показывает описание параметра, но не обязательно
показывает тип в той же форме, что у стандартного `module_param(..., int, ...)`.

После добавления учебных комментариев к `struct kernel_param_ops` и
`module_param_cb` модуль был повторно передан на BeagleBone Black и пересобран.
Повторный `modinfo hello.ko` показал те же параметры:

```text
parm:           username:Name to print in the hello message (charp)
parm:           verbose:Enable verbose hello module messages (bool)
parm:           log_level:Logging level from 0 to 3
```

Вывод: комментарии не изменили metadata и runtime-логику модуля; `log_level`
по-прежнему присутствует в собранном `.ko`.

### Runtime-проверка log_level при insmod

Модуль загружен с параметрами:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin verbose=1 log_level=2
```

Фактический `dmesg`:

```text
hello: loading out-of-tree module taints kernel.
bbb_hello: log_level changed to 2
bbb_hello: init, username=Yunin
bbb_hello: verbose mode is enabled
bbb_hello: log_level=2
```

Вывод: параметр `log_level=2`, переданный в `insmod`, обработан через
`log_level_set()` до вызова `bbb_hello_init()`. Поэтому init-функция уже видит
обновленное значение `log_level == 2`.

Строка:

```text
hello: loading out-of-tree module taints kernel.
```

ожидаема для внешнего модуля, собранного вне дерева исходников ядра. Это не
ошибка загрузки.

### Sysfs-представление log_level

После повторной загрузки модуля с:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin verbose=1 log_level=2
```

проверен sysfs-параметр:

```sh
cat /sys/module/hello/parameters/log_level
ls -l /sys/module/hello/parameters/log_level
```

Фактический результат:

```text
2
-rw-r--r-- 1 root root 4096 Apr 30 12:12 /sys/module/hello/parameters/log_level
```

Вывод:

```text
модуль загружен
  -> /sys/module/hello/parameters/log_level существует
  -> текущее значение log_level равно 2
  -> права 0644 отображаются как -rw-r--r--
  -> root может менять параметр через sysfs
```

### Runtime-запись log_level через sysfs

Проверена запись нового значения во время работы модуля:

```sh
echo 3 | sudo tee /sys/module/hello/parameters/log_level
dmesg | tail -n 10
cat /sys/module/hello/parameters/log_level
```

Фактический результат:

```text
3
bbb_hello: log_level changed to 3
3
```

Вывод:

```text
echo 3 > /sys/module/hello/parameters/log_level
  -> sysfs write
  -> kernel parameter core
  -> log_level_set("3", kp)
  -> значение принято
  -> переменная log_level обновлена до 3
  -> pr_info попал в dmesg
```

Это подтверждает главное отличие от обычного `module_param(verbose, bool,
0644)`: callback-параметр выполняет пользовательский код именно в момент записи
через sysfs.

### Проверка валидации log_level

Проверена попытка записать значение вне допустимого диапазона:

```sh
echo 9 | sudo tee /sys/module/hello/parameters/log_level
dmesg | tail -n 10
cat /sys/module/hello/parameters/log_level
```

Фактический результат:

```text
9
tee: /sys/module/hello/parameters/log_level: Invalid argument
bbb_hello: log_level must be between 0 and 3
3
```

Интерпретация:

- `tee` сначала печатает `9` в stdout, потому что он получил это значение на
  вход;
- запись в sysfs возвращает ошибку `Invalid argument`, потому что
  `log_level_set()` вернул `-EINVAL`;
- `pr_warn()` из callback попал в `dmesg`;
- значение параметра осталось прежним: `3`.

Вывод: `module_param_cb` позволяет не только реагировать на запись, но и
отклонять невалидные runtime-значения без изменения состояния модуля.

### Выгрузка hello после проверки module_param_cb

После проверки runtime-записи и валидации модуль выгружен:

```sh
sudo /usr/sbin/rmmod hello
dmesg | tail -n 10
lsmod | grep hello
cat /sys/module/hello/parameters/log_level
```

Фактический результат:

```text
bbb_hello: exit, username=Yunin
bbb_hello: verbose mode was enabled
lsmod | grep hello -> no output
cat: /sys/module/hello/parameters/log_level: No such file or directory
```

Вывод:

```text
rmmod hello
  -> ядро вызвало bbb_hello_exit()
  -> модуль исчез из lsmod
  -> /sys/module/hello/parameters/log_level удален
```

Этап `module_param_cb` закрыт:

- `modinfo` подтвердил наличие `log_level` в metadata `.ko`;
- `insmod log_level=2` вызвал `log_level_set()` до `bbb_hello_init()`;
- sysfs показал текущее значение и права `0644`;
- runtime-запись `3` вызвала callback и обновила значение;
- невалидное значение `9` вернуло `-EINVAL` и не изменило состояние;
- `rmmod` удалил модуль и его sysfs-представление.

### Точка остановки

На этом месте сессия остановлена.

Состояние на конец работы:

```text
hello module выгружен
lsmod | grep hello -> no output
/sys/module/hello/parameters/log_level отсутствует
```

Закрытые темы текущего блока:

- обычный read-only параметр `username`;
- обычный writable параметр `verbose`;
- callback-параметр `log_level`;
- различие между `modinfo` metadata и runtime-проверками;
- `module_param_cb`, `struct kernel_param_ops`, `.set/.get` callbacks;
- runtime-валидация с возвратом `-EINVAL`.

Следующая сессия начинается не с повторной проверки `log_level`, а с нового
содержательного шага: уровни kernel logging и диагностика в модуле
`hello` (`pr_info`, `pr_warn`, `pr_err`, `pr_debug`, базовая идея
dynamic debug).

Рабочий формат следующих команд:

```text
HOST -> окно Linux host с репозиторием
BBB  -> отдельное окно терминала BeagleBone Black
```

Удаленные команды вида `ssh andrey@10.129.1.152 'command'` не использовать по
умолчанию; команды для платы давать в окно `BBB`.
