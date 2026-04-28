# Host-target network: Ethernet, SSH и обход VPN

Этот раздел фиксирует рабочую сетевую схему между Linux-хостом и BeagleBone
Black. Она нужна для следующей части курса: писать код в VS Code на хосте,
передавать файлы на плату по SSH/SCP/rsync и проверять модули ядра на target.

Главная цель:

```text
Linux host
  -> Ethernet / SSH
  -> BeagleBone Black
```

При этом VPN на хосте может оставаться включенным для остальной работы.

## 1. Физическая схема

Использован неуправляемый Ethernet switch:

```text
Linux host eno0
  -> unmanaged switch
  -> BeagleBone Black eth0
```

USB gadget interfaces тоже присутствуют, но основной рабочий канал выбран через
Ethernet:

```text
host eno0       -> BBB eth0
host enx...     -> BBB usb0/usb1, запасной путь
```

## 2. Проверка адресов на BeagleBone

Команда на BeagleBone:

```sh
ip -br addr
```

Результат:

```text
lo               UNKNOWN        127.0.0.1/8 ::1/128
dummy0           DOWN
eth0             UP             10.129.1.152/23 metric 1024 fe80::32e2:83ff:fe53:2bc8/64
usb0             UP             192.168.7.2/24 fe80::32e2:83ff:fe53:2bcb/64
usb1             UP             192.168.6.2/24 fe80::32e2:83ff:fe53:2bcd/64
```

Вывод:

- `eth0` поднят;
- Ethernet-адрес платы: `10.129.1.152/23`;
- USB gadget network тоже поднят:
  - `usb0`: `192.168.7.2/24`;
  - `usb1`: `192.168.6.2/24`;
- для рабочего SSH-канала выбран `eth0`.

Команда:

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

- BeagleBone получила адрес в сети `10.129.0.0/23`;
- gateway для платы: `10.129.0.1`;
- сеть и DHCP/маршрутизация на стороне Ethernet работают.

## 3. Проверка адресов на Linux-хосте

Команда на хосте:

```sh
ip -br addr
```

Ключевой результат:

```text
eno0             UP             10.129.1.110/23 fe80::aceb:ae5e:5183:805e/64
outline-tun1     UNKNOWN        10.0.85.5/32 fe80::204f:380a:a511:1ff4/64
enx30e283532bca  UNKNOWN        192.168.7.1/24 fe80::a41f:a2f2:8d12:7357/64
enx30e283532bcc  UP             192.168.6.1/24 fe80::edc8:e807:a2f:9793/64
```

Вывод:

- Ethernet-интерфейс хоста: `eno0`;
- адрес хоста в той же сети: `10.129.1.110/23`;
- плата и хост находятся в одной подсети:

```text
host eno0:        10.129.1.110/23
BeagleBone eth0:  10.129.1.152/23
network:          10.129.0.0/23
```

## 4. Первичная проверка связности

Команда на хосте:

```sh
ping -c 3 10.129.1.152
```

Результат:

```text
3 packets transmitted, 3 received, 0% packet loss
rtt min/avg/max/mdev = 0.308/0.344/0.384/0.031 ms
```

Вывод: ICMP-связность есть. Но одного `ping` недостаточно, если на хосте
работает VPN с policy routing. Нужно проверить реальный маршрут.

## 5. Проблема: VPN перехватывал маршрут к плате

Команда на хосте:

```sh
ip route get 10.129.1.152
```

Проблемный результат:

```text
10.129.1.152 via 10.0.85.5 dev outline-tun1 table 7113 src 10.0.85.5 uid 1000
    cache
```

Вывод:

- хост пытался идти к `10.129.1.152` через `outline-tun1`;
- VPN использовал policy routing table `7113`;
- SSH-попытка уходила не напрямую в локальный Ethernet-сегмент;
- поэтому обычный `ssh andrey@10.129.1.152` закрывался до нормального SSH
  handshake.

Важный диагностический вывод:

```text
ping может проходить, но это еще не доказывает правильный путь.
Для SSH/SCP/rsync всегда проверяем ip route get <target-ip>.
```

## 6. Диагностика SSH на BeagleBone

На BeagleBone проверили сервис:

```sh
sudo systemctl status ssh --no-pager
```

Ключевой результат:

```text
Active: active (running)
Server listening on 0.0.0.0 port 22.
Server listening on :: port 22.
```

Проверили socket:

```sh
sudo ss -ltnp | grep ':22'
```

Результат:

```text
LISTEN 0 128 0.0.0.0:22 0.0.0.0:* users:(("sshd",pid=1891,fd=3))
LISTEN 0 128    [::]:22    [::]:* users:(("sshd",pid=1891,fd=4))
```

Вывод:

- `sshd` на плате запущен;
- порт `22` слушается на IPv4 и IPv6;
- проблема была не в SSH-сервисе платы.

Дополнительная диагностика:

```sh
sudo /usr/sbin/sshd -ddd -p 2222 -D -e
```

И с хоста:

```sh
ssh -vvv -p 2222 andrey@10.129.1.152
```

Debug `sshd` на плате не показывал `Connection from ...`, пока маршрут на
хосте шел через VPN. Это подтвердило, что попытка не доходила до debug-сервера
на BeagleBone.

## 7. Неполное исправление: маршрут только в main table

Была добавлена команда:

```sh
sudo ip route add 10.129.1.152/32 dev eno0
```

Но проверка осталась прежней:

```sh
ip route get 10.129.1.152
```

Результат:

```text
10.129.1.152 via 10.0.85.5 dev outline-tun1 table 7113 src 10.0.85.5 uid 1000
```

Вывод:

- маршрут попал в обычную `main` routing table;
- но VPN использовал policy table `7113`;
- route из `main` не победил policy route VPN.

## 8. Рабочее исправление: исключение в VPN table 7113

Так как `ip route get` показал `table 7113`, маршрут до платы добавили именно
в эту таблицу:

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

- трафик к плате теперь идет напрямую через `eno0`;
- VPN остается включенным для остального трафика;
- правило затрагивает только один target IP: `10.129.1.152/32`.

Это работает не только для SSH, а для любого трафика к плате:

```text
ssh
scp
rsync
ping
nc
```

## 9. Проверка SSH после исправления маршрута

Команда на хосте:

```sh
ssh andrey@10.129.1.152
```

При первом подключении SSH показал fingerprint:

```text
ED25519 key fingerprint is SHA256:KscTrYzK7rNQxXp3yDxh1EfEuRmWj4hY1qqoC78/hoU.
```

Он совпадает с ED25519 host key, который был виден в debug `sshd`:

```text
ssh-ed25519 SHA256:KscTrYzK7rNQxXp3yDxh1EfEuRmWj4hY1qqoC78/hoU
```

После подтверждения:

```text
Warning: Permanently added '10.129.1.152' (ED25519) to the list of known hosts.
Debian GNU/Linux 12
BeagleBoard.org Debian Bookworm Base Image 2026-03-17
Last login: Tue Mar 17 23:52:09 2026
andrey@BeagleBone:~$
```

Вывод: SSH с хоста на BeagleBone работает.

## 10. Команды для восстановления после перезагрузки/VPN restart

Эти настройки временные. После перезагрузки хоста, перезапуска сети или VPN
маршрут может пропасть.

Принятое решение для курса: не делать это исключение постоянным по умолчанию.
Маршрут восстанавливается вручную перед работой с BeagleBone Black, чтобы после
завершения лабораторных работ в системе не оставалась лишняя настройка.

В репозитории есть helper-скрипт для этой ручной операции:

```sh
scripts/bbb-route.sh check
scripts/bbb-route.sh apply
scripts/bbb-route.sh delete
```

Скрипт не делает настройку постоянной. Он только проверяет текущий маршрут,
добавляет временное исключение через `ip route replace` или удаляет его из
policy table `7113`.

Проверить текущий путь:

```sh
ip route get 10.129.1.152
```

Если снова видно:

```text
dev outline-tun1 table 7113
```

добавить или заменить маршрут:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

Проверить:

```sh
ip route get 10.129.1.152
ssh andrey@10.129.1.152
```

Ожидаемый маршрут:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110
```

## 11. Откат временной настройки

Удалить маршрут из VPN policy table:

```sh
sudo ip route del 10.129.1.152/32 table 7113
```

Если ранее был добавлен маршрут в `main` table, можно удалить и его:

```sh
sudo ip route del 10.129.1.152/32 dev eno0
```

После отката:

```sh
ip route get 10.129.1.152
```

снова покажет маршрут, который выбирает система/VPN.

## 12. Использование для разработки

Проверка SSH без интерактивного входа:

```sh
ssh andrey@10.129.1.152 'hostname; uname -r; whoami'
```

Тест копирования файла:

```sh
scp README.md andrey@10.129.1.152:/home/andrey/
```

Проверка на плате:

```sh
ls -l /home/andrey/README.md
```

Синхронизация каталога для будущих модулей:

```sh
rsync -av modules/ andrey@10.129.1.152:/home/andrey/bbb-course/modules/
```

Финальная проверка host-to-target передачи была выполнена с Linux-хоста.

Проверка маршрута:

```sh
ip route get 10.129.1.152
```

Результат:

```text
10.129.1.152 dev eno0 table 7113 src 10.129.1.110 uid 1000
    cache
```

Проверка SSH без интерактивной сессии:

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

Передача на BeagleBone:

```sh
scp /tmp/bbb_ssh_transfer_test.txt andrey@10.129.1.152:/home/andrey/
```

Результат:

```text
bbb_ssh_transfer_test.txt 100% 33
```

Проверка файла на BeagleBone через SSH:

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

проверен end-to-end.

Рабочий цикл второй части курса:

```text
VS Code на хосте
  -> редактирование исходников
  -> scp/rsync over SSH
  -> BeagleBone
  -> make / insmod / dmesg / rmmod
```

## 13. Итог

Зафиксирован рабочий сетевой канал:

```text
host eno0:        10.129.1.110/23
BeagleBone eth0:  10.129.1.152/23
VPN interface:    outline-tun1, table 7113
VPN bypass:       10.129.1.152/32 -> eno0 in table 7113
SSH:              works
```

Главная команда восстановления маршрута:

```sh
sudo ip route replace 10.129.1.152/32 dev eno0 src 10.129.1.110 table 7113
```

Главная проверка:

```sh
ip route get 10.129.1.152
ssh andrey@10.129.1.152
```
