# План второй части: модули ядра и ADC

Эта часть курса начинается после того, как BeagleBone Black стабильно
загружается в Linux, UART-консоль работает, а boot chain понятен.

Цель второй части - пройти путь от простого out-of-tree модуля до практического
драйвера внешнего ADC. Финальный проект не должен быть игрушкой: он должен
напоминать реальную embedded-задачу с Device Tree, шиной, подсистемой ядра и
проверкой на железе.

Выбранная линия практики:

```text
Hello, World module
  -> встроенный ADC AM335x через существующий драйвер и IIO
  -> внешний ADS1115 как бюджетный промышленный аналоговый вход
```

## 1. Этап Hello, World

Первый этап нужен не ради самого `printk`, а ради освоения рабочего цикла
kernel-разработки.

Что изучаем:

- out-of-tree сборка модуля;
- `Makefile` для Kbuild;
- `module_init()` и `module_exit()`;
- `printk()`, `pr_info()`, `dmesg`;
- `insmod`, `rmmod`, `modprobe`, `lsmod`, `modinfo`;
- параметры модуля;
- лицензия модуля и `MODULE_*` metadata;
- соответствие версии модуля версии запущенного ядра.

Минимальный результат:

```text
исходники модуля редактируются на Linux-хосте в VS Code
исходники передаются на BBB через SSH/SCP/rsync
модуль собирается на BBB под текущую версию ядра
модуль загружается в ядро BBB
dmesg показывает init/exit сообщения
модуль принимает параметр
```

Учебный вывод: модуль ядра - это код, который загружается в адресное
пространство ядра, а не обычная userspace-программа.

Рабочий цикл первого модуля:

```text
Linux host / VS Code
  -> modules/hello/hello.c
  -> modules/hello/Makefile
  -> rsync/scp
  -> BeagleBone Black /home/andrey/bbb-course/modules/hello
  -> make -C /lib/modules/$(uname -r)/build M=$PWD modules
  -> sudo insmod / sudo rmmod / dmesg
```

То есть исходники создаются и редактируются на хосте. BeagleBone Black на этом
этапе используется как target для сборки и проверки модуля против своего
запущенного ядра `6.12.76-bone50`.

### Проверка build tree текущего ядра

Перед сборкой первого out-of-tree модуля нужно проверить стандартный путь Kbuild
для текущего запущенного ядра:

```sh
ls -ld /lib/modules/$(uname -r)/build
readlink -f /lib/modules/$(uname -r)/build
```

Команда `uname -r` возвращает версию запущенного ядра. Для текущей платы:

```text
6.12.76-bone50
```

Поэтому путь:

```text
/lib/modules/$(uname -r)/build
```

становится:

```text
/lib/modules/6.12.76-bone50/build
```

Полученный результат:

```text
/lib/modules/6.12.76-bone50/build -> /usr/src/linux-headers-6.12.76-bone50
```

Символ `->` означает symbolic link: `build` не является самостоятельным
каталогом, а указывает на реальный каталог kernel headers:

```text
/usr/src/linux-headers-6.12.76-bone50
```

Первая буква `l` в длинном выводе `ls -l` тоже указывает на symbolic link:

```text
lrwxrwxrwx
^
```

Зачем это нужно:

- модули текущего ядра лежат в `/lib/modules/<kernel-version>/`;
- headers/build tree обычно лежит в `/usr/src/linux-headers-<kernel-version>/`;
- ссылка `build` дает Kbuild стандартную точку входа для сборки внешних
  модулей.

Типовая команда сборки внешнего модуля использует именно этот путь:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

### Проверка инструментов сборки

После проверки build tree нужно убедиться, что на BeagleBone Black есть базовые
инструменты сборки:

```sh
which make gcc ld
make --version | head -n 1
gcc --version | head -n 1
ld --version | head -n 1
```

Проверенный результат:

```text
/usr/bin/make
/usr/bin/gcc
/usr/bin/ld
GNU Make 4.3
gcc (Debian 12.2.0-14+deb12u1) 12.2.0
GNU ld (GNU Binutils for Debian) 2.40
```

Вывод: target имеет минимальную среду для сборки первого модуля через Kbuild.

### Структура исходников на хосте

Исходники первого модуля создаются в репозитории на Linux-хосте:

```sh
mkdir -p modules/hello
```

Ключ `-p` у `mkdir` означает `parents`: команда создает недостающие
родительские каталоги и не завершится ошибкой, если нужный каталог уже
существует.

Одного аргумента с вложенным путем достаточно:

```sh
mkdir -p a/b/c/d
```

Эта команда создает всю цепочку каталогов:

```text
a/
a/b/
a/b/c/
a/b/c/d/
```

### Минимальный исходник hello module

Первый модуль:

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

Важное уточнение: `bbb_hello_init` и `bbb_hello_exit` не являются стандартными
именами функций ядра. Эти имена выбирает разработчик. Стандартный механизм -
это регистрация функций через макросы:

```c
module_init(bbb_hello_init);
module_exit(bbb_hello_exit);
```

Эти макросы сообщают ядру:

```text
при загрузке модуля вызвать bbb_hello_init()
при выгрузке модуля вызвать bbb_hello_exit()
```

Почему init-функция возвращает `int`:

```c
static int __init bbb_hello_init(void)
```

Загрузка реального драйвера может завершиться ошибкой: не хватило памяти,
устройство не найдено, неверный параметр, не удалось зарегистрировать подсистему
или запросить ресурс. Поэтому init-функция возвращает код результата:

```c
return 0;
```

`0` означает успешную загрузку. При ошибке обычно возвращается отрицательный
errno, например:

```c
return -ENOMEM;
return -ENODEV;
return -EINVAL;
```

Почему exit-функция возвращает `void`:

```c
static void __exit bbb_hello_exit(void)
```

Выгрузочная функция не решает, загрузился модуль или нет. Ее задача - освободить
ресурсы: отменить регистрацию, освободить память, отпустить GPIO/IRQ/I2C,
остановить timers/workqueues и убрать созданные интерфейсы. Возвращать результат
уже некуда, поэтому тип `void`.

Аннотации `__init` и `__exit` - это kernel-аннотации для linker sections. Они
помогают ядру понимать, какой код нужен только при инициализации, а какой - при
выгрузке. Ключевое слово `static` ограничивает видимость функций текущим
исходным файлом.

### VS Code и kernel headers

При редактировании `hello.c` на хосте VS Code может подсвечивать ошибки для:

```c
#include <linux/init.h>
#include <linux/module.h>
```

Это ожидаемо, потому что это headers target-ядра BeagleBone Black, а не обычные
userspace C headers хоста. Реальная проверка выполняется не IntelliSense, а
Kbuild на target:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Настройку IntelliSense для kernel headers можно сделать позже отдельным шагом.

### Kernel logging: pr_info и printk

В модуле ядра нельзя использовать обычный userspace `printf()` как основной
способ вывода. У kernel module нет стандартного `stdout`, связанного с
терминалом пользователя. Сообщения пишутся в kernel log ring buffer.

Для первого модуля используется:

```c
pr_info("bbb_hello: init\n");
```

`pr_info()` - стандартный kernel logging macro для информационных сообщений.
Он пишет сообщение в kernel log, который можно читать через:

```sh
dmesg
journalctl -k
```

Важно: это не гарантированный прямой вывод на текущий терминал. Сообщение может
быть видно в `dmesg`, в systemd journal, на serial console или на другой
консоли, если текущий console loglevel это позволяет.

`pr_info()` является удобной формой поверх `printk()`:

```c
pr_info("bbb_hello: init\n");
```

по смыслу близко к:

```c
printk(KERN_INFO "bbb_hello: init\n");
```

`printk()` - более базовый интерфейс. При его использовании уровень важности
обычно указывается явно:

```c
printk(KERN_INFO "normal information\n");
printk(KERN_WARNING "warning condition\n");
printk(KERN_ERR "error condition\n");
```

Современный код часто использует семейство `pr_*()`:

```c
pr_err("failed\n");
pr_warn("out of range\n");
pr_info("initialized\n");
pr_debug("debug value\n");
```

Для настоящих драйверов устройств, когда доступен `struct device *dev`, обычно
предпочтительнее `dev_*()`:

```c
dev_info(dev, "initialized\n");
dev_err(dev, "probe failed\n");
```

Преимущество `dev_*()` в том, что сообщение получает контекст конкретного
устройства. Для первого `hello` module устройства еще нет, поэтому `pr_info()`
является нормальным выбором.

### Начальный debugging workflow для kernel module

На первых этапах отладка kernel module обычно строится не вокруг IDE
breakpoints, а вокруг нескольких последовательных проверок.

1. Kbuild:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Эта проверка показывает, что код совместим с headers и API текущего ядра,
правильно подключены include-файлы и модуль вообще может быть собран как `.ko`.

2. Загрузка и выгрузка:

```sh
sudo insmod bbb_hello.ko
lsmod | grep bbb_hello
sudo rmmod bbb_hello
```

Эти команды проверяют, что модуль реально принимается ядром, появляется в
списке загруженных модулей и может быть выгружен.

3. Kernel log:

```sh
dmesg | tail -n 40
journalctl -k -n 40
```

Если `insmod` или `rmmod` завершается ошибкой, причина часто видна именно в
kernel log.

4. Логи в коде:

```c
pr_info("bbb_hello: init enter\n");
pr_info("bbb_hello: init done\n");
pr_info("bbb_hello: exit enter\n");
```

Для учебного модуля нормально ставить логи в ключевые точки выполнения: вход в
функцию, успешное завершение этапа, ошибка, значение параметра. Это помогает
увидеть порядок выполнения и понять, дошел ли код до нужного места.

В более аккуратном драйверном коде уровни логирования разделяют:

```c
pr_info("initialized\n");
pr_err("failed: %d\n", ret);
pr_debug("value=%d\n", value);
```

- `pr_info()` - важные информационные этапы;
- `pr_err()` - ошибки;
- `pr_debug()` - подробная диагностика, которую позже можно включать и
  выключать через dynamic debug.

Для настоящих device drivers вместо `pr_*()` часто используют `dev_*()`, чтобы
сообщение было привязано к конкретному устройству.

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

Позже к этому добавляются dynamic debug, debugfs, ftrace, tracepoints, perf,
kgdb или JTAG. Но для первого out-of-tree модуля правильная база - сборка,
корректные return codes и понятные kernel log messages.

### Практический цикл hello module

Синхронизация исходников с хоста на BeagleBone:

```sh
rsync -av modules/hello/ andrey@10.129.1.152:/home/andrey/bbb-course/modules/hello/
```

На BeagleBone:

```sh
cd ~/bbb-course/modules/hello
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Ключевые строки успешной сборки:

```text
CC [M]  /home/andrey/bbb-course/modules/hello/hello.o
MODPOST /home/andrey/bbb-course/modules/hello/Module.symvers
LD [M]  /home/andrey/bbb-course/modules/hello/hello.ko
```

Проверка metadata:

```sh
/usr/sbin/modinfo hello.ko
```

Проверенный результат:

```text
description:    Minimal BeagleBone Black hello kernel module
author:         Andrey
license:        GPL
name:           hello
vermagic:       6.12.76-bone50 preempt mod_unload modversions ARMv7 thumb2 p2v8
```

Если `modinfo` не находится через `PATH`, использовать полный путь
`/usr/sbin/modinfo`. На Debian административные утилиты могут лежать в `sbin`,
который не всегда входит в `PATH` обычного пользователя.

Загрузка:

```sh
sudo /usr/sbin/insmod hello.ko
dmesg | tail -n 20
lsmod | grep hello
```

Ожидаемые признаки:

```text
hello: loading out-of-tree module taints kernel.
bbb_hello: init
hello                  12288  0
```

Сообщение `taints kernel` ожидаемо для out-of-tree модуля. Оно означает, что в
ядро был загружен модуль не из основного дерева/пакета ядра. Для учебной
лаборатории это нормально.

Выгрузка:

```sh
sudo /usr/sbin/rmmod hello
dmesg | tail -n 20
lsmod | grep hello
```

Ожидаемый лог:

```text
bbb_hello: exit
```

После выгрузки `lsmod | grep hello` не должен выводить строку с модулем.

### Следующий шаг: параметры модуля

После минимального `hello` module следующий учебный шаг - добавить параметр
модуля через `module_param`.

Главная идея: параметр позволяет менять заранее предусмотренное поведение
модуля без изменения исходника и без пересборки `.ko`.

Один и тот же файл:

```text
hello.ko
```

можно загрузить с разными значениями:

```sh
sudo /usr/sbin/insmod hello.ko username=Andrey
sudo /usr/sbin/insmod hello.ko username=Test
sudo /usr/sbin/insmod hello.ko username=BBB
```

Но передать можно только те параметры, которые заранее описаны в коде. Если в
модуле нет `module_param(username, ...)`, параметр `username=...` ядро не
примет.

Минимальная схема в коде:

```c
static char *username = "world";
module_param(username, charp, 0444);
MODULE_PARM_DESC(username, "Name to print in the hello message");
```

Разбор:

- `username` - переменная и имя параметра;
- `charp` - строковый параметр;
- `0444` - права sysfs: read-only для owner/group/others;
- `MODULE_PARM_DESC` добавляет описание параметра в `modinfo`.

Проверка после сборки:

```sh
/usr/sbin/modinfo hello.ko
```

Ожидаемый признак:

```text
parm: username:Name to print in the hello message (charp)
```

Временная ручная загрузка:

```sh
sudo /usr/sbin/insmod hello.ko username=Andrey
```

Проверка:

```sh
dmesg | tail -n 20
cat /sys/module/hello/parameters/username
```

#### Реальные применения module parameters

Практические примеры:

- `debug=1` - включить подробные диагностические логи;
- `poll_ms=1000` - задать интервал опроса;
- `timeout_ms=500` - задать timeout операции;
- `buffer_size=4096` - выбрать размер буфера, если драйвер это безопасно
  поддерживает.

Например:

```c
static bool debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Enable verbose debug logs");
```

Загрузка:

```sh
sudo /usr/sbin/insmod industrial_adc.ko debug=1
```

В коде:

```c
if (debug)
    pr_info("adc: ch=%u raw=%d scale=%d\n", channel, raw, scale);
```

Так один и тот же `.ko` может работать в обычном тихом режиме или в
диагностическом режиме.

#### Что происходит после reboot

Если модуль загружен вручную:

```sh
sudo /usr/sbin/insmod hello.ko username=Andrey
```

после перезагрузки Linux этот модуль и его параметр не сохранятся. `insmod`
загружает модуль только в текущее работающее ядро.

Для автозагрузки используется другой механизм:

```text
/lib/modules/$(uname -r)/extra/hello.ko
/etc/modules-load.d/hello.conf
/etc/modprobe.d/hello.conf
```

Типовая схема:

```sh
sudo mkdir -p /lib/modules/$(uname -r)/extra
sudo cp hello.ko /lib/modules/$(uname -r)/extra/
sudo depmod
```

Файл автозагрузки:

```text
/etc/modules-load.d/hello.conf
```

Содержимое:

```text
hello
```

Файл параметров:

```text
/etc/modprobe.d/hello.conf
```

Содержимое:

```text
options hello username=Andrey
```

После reboot система может загрузить модуль через `modprobe hello`, применив
параметры из `modprobe.d`.

Разница:

```text
insmod ./hello.ko username=Andrey
  -> грузит конкретный файл
  -> не ищет зависимости
  -> не читает /etc/modprobe.d
  -> не сохраняется после reboot

modprobe hello
  -> ищет модуль в /lib/modules/$(uname -r)/
  -> учитывает зависимости
  -> читает /etc/modprobe.d/*.conf
  -> подходит для автозагрузки
```

Для текущего курса автозагрузку пока не включаем, потому что это постоянное
изменение target-системы. На лабораторном этапе безопаснее вручную загружать и
выгружать модуль.

#### Read-only и writable параметры

Права в `module_param` определяют, будет ли параметр доступен через sysfs:

```c
module_param(username, charp, 0444);
```

`0444` означает read-only:

```sh
cat /sys/module/hello/parameters/username
```

но запись после загрузки запрещена.

Writable пример:

```c
module_param(debug, bool, 0644);
```

Такой параметр можно изменить во время работы:

```sh
cat /sys/module/hello/parameters/debug
echo 1 | sudo tee /sys/module/hello/parameters/debug
```

Модель:

```text
modprobe.d задает значение при загрузке
sysfs может менять значение после загрузки, если права позволяют
```

#### Риски runtime-изменения параметров

`0644` сам по себе не делает параметр безопасным. Он только отвечает на вопрос:

```text
может ли root записать новое значение в sysfs-файл параметра?
```

Он не проверяет:

```text
валидное ли значение;
безопасно ли менять его сейчас;
не идет ли параллельное чтение;
не нужен ли lock;
не сломает ли значение внутреннюю структуру данных;
можно ли менять hardware mode на лету.
```

Writable parameters могут привести к ошибкам ядра, если драйвер не защищает
доступ и не валидирует значения. Параметр становится общей переменной между
кодом ядра и записью из userspace через sysfs.

Возможные проблемы:

- race condition;
- неконсистентное состояние;
- выход за пределы массива;
- use-after-free;
- деление на ноль;
- невалидный hardware режим;
- kernel oops или panic.

Пример опасного параметра:

```c
static int channel = 0;
module_param(channel, int, 0644);
```

Если код делает:

```c
value = adc_channels[channel].last_value;
```

а userspace запишет:

```sh
echo 999 | sudo tee /sys/module/driver/parameters/channel
```

без проверки диапазона возможен выход за пределы массива.

Поэтому в реальном коде:

- опасные параметры делают read-only: `0444`;
- writable параметры валидируют;
- доступ защищают lock-ами;
- для сложных случаев используют `module_param_cb`;
- для runtime-настроек часто создают отдельный sysfs/configfs/ioctl/netlink
  интерфейс с проверками;
- hardware-конфигурацию часто задают через Device Tree и не меняют на лету.

Для нашего следующего шага безопасный выбор:

```text
username: charp, 0444
```

То есть параметр задается при `insmod`, читается через sysfs, но не меняется во
время работы модуля. Позже можно отдельно показать безопасный writable параметр
`debug: bool, 0644`.

Более точная формулировка для будущего writable-параметра:

```text
verbose можно сделать writable не потому, что 0644 сам по себе безопасен,
а потому что код использует verbose только как переключатель дополнительных
логов
```

Например:

```c
if (verbose)
    pr_info("extra log\n");
```

В таком варианте изменение `verbose` влияет только на печать дополнительного
сообщения. Если writable-параметр используется как индекс массива, размер
буфера, режим железа или адрес устройства, безопасность должна обеспечиваться
дополнительной логикой драйвера.

#### Writable verbose parameter

После read-only параметра `username` добавлен второй параметр:

```c
static bool verbose;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable verbose hello module messages");
```

Теперь модуль имеет два параметра:

```text
username: charp, 0444
verbose:  bool,  0644
```

`username` задается при загрузке и доступен только для чтения через sysfs.
`verbose` можно задать при загрузке и изменить во время работы модуля через
sysfs.

Пример загрузки:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin verbose=1
```

Проверка `modinfo`:

```text
parm: username:Name to print in the hello message (charp)
parm: verbose:Enable verbose hello module messages (bool)
```

Проверка `dmesg`:

```text
bbb_hello: init, username=Yunin
bbb_hello: verbose mode is enabled
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
-r--r--r-- ... /sys/module/hello/parameters/username
-rw-r--r-- ... /sys/module/hello/parameters/verbose
```

Для bool-параметра ядро показывает `Y` / `N`, а не обязательно `1` / `0`.

Изменение runtime-параметра:

```sh
echo 0 | sudo tee /sys/module/hello/parameters/verbose
cat /sys/module/hello/parameters/verbose
```

Результат:

```text
0
N
```

`tee` печатает `0` в stdout, а `cat` показывает, что ядро приняло новое значение
как `N`.

Важный вывод: обычный `module_param` меняет связанную переменную, но не вызывает
пользовательскую функцию в момент записи. Поэтому после:

```sh
echo 0 | sudo tee /sys/module/hello/parameters/verbose
```

новая строка в `dmesg` не появляется автоматически. Наш код читает `verbose`
только в `init` и `exit`:

```c
if (verbose)
    pr_info("bbb_hello: verbose mode is enabled\n");
```

и:

```c
if (verbose)
    pr_info("bbb_hello: verbose mode was enabled\n");
```

После изменения `verbose` с `Y` на `N` при выгрузке появилось:

```text
bbb_hello: exit, username=Yunin
```

и не появилось:

```text
bbb_hello: verbose mode was enabled
```

Это подтверждает, что переменная изменилась, но эффект виден только там, где код
модуля затем прочитал эту переменную.

Если нужна реакция именно в момент записи нового значения в sysfs, используется
другой механизм:

```text
module_param_cb
```

Callback-параметр позволяет задать custom set/get callbacks, которые будут
вызваны при записи или чтении параметра. Это следующий уровень после обычного
`module_param`.

#### Sysfs-представление параметра

После загрузки модуля:

```sh
sudo /usr/sbin/insmod hello.ko username=Yunin
```

ядро создает sysfs-представление модуля:

```text
/sys/module/hello/
```

Если в модуле есть параметр:

```c
module_param(username, charp, 0444);
```

то появляется точка доступа:

```text
/sys/module/hello/parameters/username
```

Рабочая модель для курса:

```text
/sys/module/hello/parameters/username
  -> точка доступа к параметру username загруженного модуля hello
```

При чтении:

```sh
cat /sys/module/hello/parameters/username
```

пользователь получает текущее значение параметра:

```text
Yunin
```

Технически значение хранится не как обычный файл на microSD/eMMC. Значение
находится в RAM ядра, в переменной модуля:

```c
static char *username;
```

`sysfs` дает контролируемый файловый интерфейс к этому значению. Команда `cat`
работает как userspace-программа: она вызывает обычное чтение файла, а ядро
через sysfs возвращает текущее значение параметра как текст.

Важно:

```text
/sys/module/hello/parameters/username
```

не является адресом RAM и не дает userspace прямой доступ к памяти ядра. Это
именованная точка доступа, которую ядро связывает с параметром модуля.

Права `0444` видны как:

```text
-r--r--r--
```

Поэтому чтение разрешено:

```sh
cat /sys/module/hello/parameters/username
```

а запись запрещена:

```sh
echo Test | sudo tee /sys/module/hello/parameters/username
```

ожидаемо возвращает `Permission denied`.

После выгрузки:

```sh
sudo /usr/sbin/rmmod hello
```

модуль исчезает из ядра, и его sysfs-представление:

```text
/sys/module/hello/
```

тоже исчезает.

#### Минимальная модель памяти для kernel module

Загружаемый модуль `.ko` размещается в RAM ядра, но код ядра обычно работает не
с физическими адресами напрямую, а с kernel virtual addresses.

При загрузке:

```sh
sudo /usr/sbin/insmod hello.ko
```

ядро разбирает ELF-файл модуля и размещает его секции в памяти:

```text
.text      -> машинный код функций
.rodata    -> read-only данные и строковые литералы
.data      -> инициализированные global/static переменные
.bss       -> неинициализированные global/static переменные
.init.text -> код инициализации
.exit.text -> код выгрузки
```

Это не один простой массив байтов. Модуль - набор секций, которые loader ядра
размещает в kernel virtual address space, выравнивает, настраивает права и
исправляет relocations.

Полезная аналогия: внутри каждой размещенной секции адресация похожа на:

```text
base address + offset
```

Но весь модуль точнее понимать как набор ELF-секций, а не как одну C-структуру.

Примеры:

```c
static char *username = "world";
```

- сама переменная `username` имеет адрес в данных модуля;
- строковый литерал `"world"` обычно лежит в read-only данных;
- функция `bbb_hello_init()` имеет адрес в кодовой секции;
- функция `bbb_hello_exit()` имеет адрес в exit-секции.

Все эти адреса для кода ядра - kernel virtual addresses.

Минимальная модель:

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

Основная идея виртуальной памяти:

```text
software работает с virtual addresses,
MMU переводит их в physical addresses,
hardware видит physical addresses.
```

Зачем это нужно:

- изоляция userspace-процессов друг от друга;
- защита kernel memory от обычных программ;
- возможность давать коду непрерывный virtual range поверх разных физических
  страниц RAM;
- права доступа на уровне страниц: read-only, executable, no-execute,
  kernel-only;
- отображение регистров устройств в адресное пространство ядра;
- управляемая загрузка и выгрузка модулей.

Важно различать physical address ranges:

```text
physical RAM addresses
  -> реальные ячейки оперативной памяти

physical MMIO addresses
  -> регистры периферии: GPIO, UART, I2C, ADC и т.п.
```

Устройства обычно не "подключаются к RAM-адресам". У SoC есть physical address
map, где одни диапазоны ведут к RAM, а другие - к регистрам устройств.

Для RAM:

```text
kernel virtual address -> physical RAM
```

Для устройства:

```text
kernel virtual address -> physical MMIO registers
```

Драйверы обычно получают physical MMIO address из Device Tree property `reg`, а
затем отображают его в kernel virtual address через `ioremap` /
`devm_ioremap_resource`. После этого код драйвера читает и пишет регистры через
kernel virtual address, например с помощью `readl()` / `writel()`.

Для нашего `hello` module:

```text
hello.ko загружен
  -> код и данные размещены в kernel virtual address space
  -> MMU связывает эти virtual addresses с physical RAM
  -> username существует как переменная загруженного модуля
  -> /sys/module/hello/parameters/username дает точку доступа к значению
```

После:

```sh
sudo /usr/sbin/rmmod hello
```

ядро:

- вызывает exit-функцию;
- удаляет sysfs-представление `/sys/module/hello/`;
- освобождает память, занятую кодом, данными и служебными структурами модуля;
- удаляет active kernel objects, связанные с этим модулем.

Практический вывод:

```text
нет модуля в ядре
  -> нет активной переменной username
  -> нет /sys/module/hello/parameters/username
```

#### Точка доступа к ресурсу ядра

Userspace не обращается к произвольной памяти ядра напрямую. Даже если у
переменной или структуры ядра есть адрес, обычная программа не может безопасно
читать или писать этот адрес.

Доступ возможен, если ядро явно открыло ресурс через интерфейс и права доступа
разрешают нужное действие.

Формула:

```text
ресурс ядра доступен userspace,
если ядро создало для него точку доступа
и права разрешают чтение или запись
```

Для нашего примера:

```text
ресурс ядра: параметр username модуля hello
точка доступа: /sys/module/hello/parameters/username
права: -r--r--r--
действие: чтение через cat
```

Команда:

```sh
cat /sys/module/hello/parameters/username
```

работает потому, что:

- модуль `hello` загружен;
- параметр `username` зарегистрирован через `module_param`;
- ядро создало sysfs-точку доступа;
- права `0444` разрешают чтение.

Запись:

```sh
echo Test | sudo tee /sys/module/hello/parameters/username
```

не работает, потому что права `0444` не дают write-доступ.

Типовые интерфейсы, через которые ядро открывает ресурсы userspace:

```text
/sys      sysfs: устройства, драйверы, модули, параметры
/proc     procfs: процессы и часть runtime-информации ядра
/dev      device files: character/block devices
debugfs   отладочные интерфейсы ядра
ioctl     команды управления через file descriptor
netlink   обмен сообщениями kernel <-> userspace
syscall   системные вызовы
```

Главный вывод:

```text
userspace работает не с адресами объектов ядра,
а с точками доступа, которые ядро специально предоставляет.
```

Краткий справочник по интерфейсам:

```text
/sys
  Sysfs. Представляет kernel objects как дерево файлов и каталогов:
  устройства, шины, драйверы, классы устройств, модули, параметры.
  Примеры из курса:
    /sys/module/hello/parameters/username
    /sys/bus/iio/devices/iio:device0/in_voltage0_raw

/proc
  Procfs. Исторически содержит информацию о процессах и runtime-состоянии
  ядра. Часть файлов относится к процессам, часть - к системе в целом.
  Примеры из курса:
    /proc/cmdline
    /proc/device-tree

/dev
  Device files. Точки доступа к character/block devices. Через них userspace
  обычно делает read/write/mmap/ioctl к драйверу устройства.
  Примеры:
    /dev/ttyUSB0
    /dev/mmcblk0

debugfs
  Отладочная файловая система ядра. Удобна для диагностики, tracing и
  dynamic debug, но не считается стабильным userspace ABI.
  Обычно монтируется в:
    /sys/kernel/debug

ioctl
  Управляющие команды к драйверу через уже открытый file descriptor.
  Используется, когда простых read/write недостаточно.
  Типовая форма:
    ioctl(fd, COMMAND, arg)

netlink
  Канал обмена сообщениями между kernel и userspace через socket API.
  Часто используется для сетевой подсистемы, routing, udev-событий и
  конфигурационных сообщений.

syscall
  Системный вызов - базовый контролируемый вход userspace в ядро.
  Любые open/read/write/ioctl/socket в итоге проходят через syscalls.
```

Эти интерфейсы отличаются формой, но общая идея одна:

```text
ядро не отдает userspace прямой адрес своего объекта,
а предоставляет контролируемую операцию через выбранный интерфейс.
```

## 2. Этап встроенного ADC AM335x

У BeagleBone Black есть встроенный ADC в SoC AM335x. Для него уже существует
драйвер в ядре Linux, поэтому на этом этапе мы не пишем ADC-драйвер с нуля.

Задача этапа другая: понять, как готовый драйвер связывается с железом.

Что изучаем:

- Device Tree описание встроенной периферии;
- pinmux и ограничения analog input pins;
- IIO subsystem;
- sysfs-интерфейс IIO;
- чтение `raw` и `scale`;
- связь между напряжением на пине и значением ADC;
- безопасные пределы входного напряжения.

Практический входной сигнал:

```text
потенциометр 10 kOhm от 1.8 V ADC reference к analog ground
средний вывод -> AIN0
```

Важно: analog inputs BeagleBone Black не являются 3.3 V tolerant. Для
встроенного ADC работаем только в безопасном диапазоне `0..1.8 V`.

Минимальный результат:

```text
найдено IIO-устройство встроенного ADC
прочитан raw-код канала
прочитан scale
изменение положения потенциометра меняет показания
напряжение пересчитано в вольты
```

Учебный вывод: для встроенной периферии чаще нужно не писать драйвер заново, а
понять существующий драйвер, Device Tree, pinmux и ABI подсистемы ядра.

## 3. Этап внешнего ADC

Финальный выбранный вариант - внешний ADC **ADS1115** и простая входная
обвязка, собранная на макетной плате.

Это бюджетная версия промышленного аналогового входа. Она не заменяет
сертифицированный PLC-модуль, но повторяет важную инженерную идею:

```text
стандартный промышленный сигнал
        |
        v
входная обвязка: делитель, шунт, фильтр, защита
        |
        v
ADC
        |
        v
Linux kernel driver
        |
        v
IIO values in userspace
```

Почему выбран ADS1115:

- доступен как дешевый breakout-модуль;
- работает от `2.0..5.5 V`;
- совместим с `3.3 V` I2C BeagleBone Black при питании от `3.3 V`;
- имеет 4 single-ended или 2 differential входа;
- имеет 16-bit delta-sigma ADC и programmable gain amplifier;
- поддерживается mainline Linux-драйвером `ti-ads1015`;
- достаточно прост для учебного драйвера, но не сводится к `Hello, World`.

## 4. Архитектура финального макета

Общая схема:

```text
BeagleBone Black
  P9_19 I2C2_SCL ---- ADS1115 SCL
  P9_20 I2C2_SDA ---- ADS1115 SDA
  3.3 V ------------ ADS1115 VDD
  GND -------------- ADS1115 GND

ADS1115 A0 <- вход 0-10 V через делитель
ADS1115 A1 <- вход 4-20 mA через шунт
```

Вход `0-10 V`:

```text
0-10 V signal
    |
   100 kOhm
    |
    +---- ADS1115 A0
    |
   33 kOhm
    |
   GND
```

При `10 V` на входе ADC увидит примерно:

```text
10 V * 33 / (100 + 33) = 2.48 V
```

Вход `4-20 mA`:

```text
current input +
    |
    +---- ADS1115 A1
    |
   100 Ohm
    |
current input - / GND
```

Ожидаемые уровни:

```text
4 mA  -> 0.4 V
12 mA -> 1.2 V
20 mA -> 2.0 V
24 mA -> 2.4 V
```

Такой диапазон остается ниже питания ADS1115 `3.3 V`.

## 5. Компоненты для макета

Базовый набор:

```text
ADS1115 breakout module
макетная плата
провода Dupont
потенциометр 10 kOhm
резисторы 100 kOhm
резисторы 33 kOhm или 33.2 kOhm
резисторы 100 Ohm, лучше 0.1%
резисторы 1 kOhm
конденсаторы 10 nF
конденсаторы 100 nF
мультиметр
```

Для проверки `4-20 mA` позже понадобится один из вариантов:

```text
готовый 4-20 mA simulator
или простой учебный генератор тока
или лабораторный БП с контролем тока и мультиметром
```

## 6. Драйверная цель

Путь разработки:

```text
1. Подключить ADS1115 к I2C и увидеть адрес через i2cdetect.
2. Описать устройство в Device Tree overlay.
3. Проверить работу штатного драйвера ti-ads1015 как эталон.
4. Написать учебный out-of-tree IIO-драйвер.
5. Реализовать probe/remove.
6. Реализовать чтение регистров ADS1115.
7. Реализовать IIO channels.
8. Реализовать raw/scale.
9. Сравнить показания с мультиметром.
10. Добавить пересчет 0-10 V и 4-20 mA в engineering values в userspace.
```

Важно: сначала используем штатный драйвер как эталон поведения. Учебный драйвер
пишем не потому, что штатный плохой, а чтобы понять архитектуру Linux IIO и
работу с реальным устройством.

## 7. Пересчет в инженерные величины

ADC измеряет напряжение. Дальше userspace может пересчитать электрический сигнал
в физическую величину.

Для `4-20 mA -> 0-10 bar`:

```text
pressure_bar = (current_mA - 4) * 10 / 16
```

Для `0-10 V -> 0-100 %`:

```text
percent = voltage_V * 100 / 10
```

Общая формула:

```text
value = physical_min +
        (signal - signal_min) *
        (physical_max - physical_min) /
        (signal_max - signal_min)
```

Учебный вывод: драйвер ядра должен корректно отдать измеренный сигнал, а
прикладная логика уже может интерпретировать его как давление, температуру,
уровень, расход или положение.

## 8. Что пока не включаем в обязательный проект

HART, гальваническую изоляцию и промышленное питание `24 V` пока не делаем
обязательными.

Они остаются как возможное расширение после основного ADC-проекта:

```text
HART modem:       AD5700-1
HART mux:         ADG704 или более дешевый учебный аналоговый mux
digital isolation: ADuM3151
isolated power:    ADuM5411
24 V DC/DC:        ADP2441 или похожий industrial buck
```

Причина: эти компоненты добавляют уже другой слой задачи - industrial
communication, изоляцию, питание, EMC и безопасность. Для первой версии
учебного драйвера важнее довести до конца связку:

```text
Device Tree -> I2C -> kernel driver -> IIO -> измерение -> пересчет
```

## 9. Контрольные вопросы

1. Почему для встроенного ADC мы сначала изучаем существующий драйвер, а не
   пишем свой?
2. Чем IIO отличается от простого character device?
3. Почему ADS1115 при питании `3.3 V` нельзя подавать на вход больше `3.3 V`?
4. Зачем делитель на входе `0-10 V`?
5. Почему для `4-20 mA` нужен шунт?
6. Где должен жить пересчет в bar, percent или degrees: в драйвере или в
   userspace?

## 10. Источники

- TI ADS1115 product page: https://www.ti.com/product/ADS1115
- TI ADS1015/ADS1115 Linux driver: https://www.ti.com/tool/ADS1015SW-LINUX
- Linux IIO subsystem documentation: https://docs.kernel.org/driver-api/iio/
