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

### План второй большой части курса

После обсуждения финального направления для разработки модулей ядра принято
решение строить вторую часть курса в три этапа:

```text
1. Hello, World module
2. Встроенный ADC AM335x через существующий драйвер и IIO
3. Внешний ADS1115 как бюджетный промышленный аналоговый вход
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
