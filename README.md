# ant
**Assistant Network Testing** - Утилита для проверки интернет соединения через icmp ping и tcp ping.

Для получения справки используйте параметр `--help` или `-h`
```bash
ant --help
```
```
command list:
-h [ --help ]         print help information
-H [ --host ] arg     target hostname for tcp or icmp check
-P [ --port ] arg     target port for tcp check
```

Пример использования:
```bash
ant --host ya.ru --port 443
```
```
hostname             : ya.ru       
remote address       : 77.88.44.242
reply time           : 443
tcp test succeeded   : true 
```

Если вызвать утилиту без параметров, то произойдет icmp ping проверка до адреса google.com
```bash
ant
```
```
hostname             : google.com    
remote address       : 142.250.27.113
reply time           : 152ms
icmp test succeeded  : true
```

## Сборка проекта
Для управления пакетами в проекте используется [vcpkg](https://learn.microsoft.com/ru-ru/vcpkg/), поэтому нужна установленная переменная среды **VCPKG_ROOT**.

Устанавливаем зависимости:
```bash
vcpkg install
```

Для сборки проекта используется CMake:
```bash
cmake --preset=vcpkg-release -S . -B build
cmake --build build
```
