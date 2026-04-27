# BeagleBone Black: учебный курс по Linux и драйверам ядра

Этот репозиторий - рабочая тетрадь для установки Linux на BeagleBone Black,
изучения цепочки загрузки и дальнейшей разработки учебных модулей ядра.

Первая большая часть курса - не просто получить рабочую microSD, а разобрать
загрузочные модули платы: AM335x Boot ROM, U-Boot SPL / MLO, полный U-Boot,
Linux kernel, Device Tree, root filesystem и userspace.

Вторая большая часть курса - перейти к разработке модулей ядра Linux на
BeagleBone Black: от `Hello, World!` до практического драйвера внешнего ADC.

Рекомендуемый стартовый образ:

- Плата: BeagleBone Black
- Дистрибутив: Debian 12.13 IoT
- Семейство ядра: v6.12.x
- Имя образа BeagleBoard: `am335x-debian-12.13-base-v6.12-armhf-2026-03-17-4gb.img.xz`

Почему этот образ:

- Это официальный образ BeagleBoard для семейства AM335x.
- Debian 12 достаточно консервативен для учебного курса, но пакеты еще
  современные.
- Ядро v6.12.x - хороший баланс между актуальностью и стабильностью.
- Ubuntu на BeagleBone Black есть в основном как community/as-is вариант;
  основной поддерживаемый путь для BBB - Debian.

## Карта курса

Единая HTML-точка входа для просмотра учебной базы:

- [Course index](index.html)

0. [Общий отчет по работам](REPORT.md)
0.1. [Playbook следующей сессии](NEXT_SESSION.md)

### Часть 1. Установка Linux и boot chain

1. [Обзор цепочки загрузки](course/01-boot-chain.md)
2. [Подготовка Linux-хоста и microSD](course/02-prepare-sd.md)
3. [Serial console и первая загрузка](course/03-serial-console.md)
4. [U-Boot: остановка autoboot и изучение окружения](course/04-u-boot-environment.md)
5. [Linux-side: Device Tree, overlays и kernel log](course/05-linux-device-tree.md)
6. [Host-target network: Ethernet, SSH и обход VPN](course/06-host-target-network.md)
7. Прошивка eMMC

### Часть 2. Модули ядра и драйверы

20. [План второй части: модули ядра и ADC](course/20-kernel-modules-roadmap.md)

Первый целевой результат - загрузиться с microSD и увидеть все этапы через
UART. Только после этого будем прошивать внутреннюю eMMC.

Финальный практический результат второй части - учебный внешний ADC-вход на
ADS1115: `0-10 V` и `4-20 mA` сигналы, Device Tree, I2C, IIO и пересчет в
инженерные величины.

## Что нужно из железа

- BeagleBone Black
- microSD-карта, лучше 8 GB или больше
- Linux-хост с кардридером
- Надежное питание 5 V, желательно через barrel jack
- USB-UART адаптер 3.3 V для настоящих boot-логов
- Ethernet-кабель или USB gadget network для входа после загрузки

Не подключайте 5 V от USB-UART адаптера к debug-разъему BeagleBone. Нужны
только GND, TX и RX.

## Основные источники

- BeagleBoard latest images: https://www.beagleboard.org/distros
- BeagleBoard getting started: https://docs.beagleboard.org/intro/support/getting-started.html
- BeagleBone Black product page: https://www.beagleboard.org/p/products/beaglebone-black
- TI ADS1115 product page: https://www.ti.com/product/ADS1115
- TI ADS1015/ADS1115 Linux driver: https://www.ti.com/tool/ADS1015SW-LINUX
