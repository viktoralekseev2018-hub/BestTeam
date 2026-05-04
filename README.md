# Web-Agent

**Автономный кроссплатформенный веб-агент для получения, выполнения и отправки результатов задач на удалённый сервер.**

## Обзор

Web-Agent — это фоновый клиент-агент, который работает на Windows, Linux и macOS.  
Он подключается к серверу управления, получает задания, выполняет их на клиентской машине и отправляет результат обратно на сервер.

Агент может:

- регистрироваться на сервере по `uid` и `access_code`;
- периодически опрашивать сервер на наличие новых заданий;
- получать задания через HTTP/HTTPS;
- выполнять системные команды;
- копировать и отправлять файлы;
- изменять параметры конфигурации;
- отправлять результаты выполнения через `POST /wa_result/`;
- вести логирование в файл `agent.log`.

## Подходит для

- распределённых систем;
- CI/CD воркеров;
- фоновых вычислений;
- удалённого управления узлами;
- автоматизированного запуска задач на разных ОС.

## Ключевые особенности

- **Кроссплатформенность**  
  Поддержка Windows, Linux и macOS.

- **Работа через HTTP/HTTPS**  
  Агент обращается к серверу управления и отправляет результаты через API.

- **JSON-конфигурация**  
  Основные параметры задаются в `config.json`.

- **Поддержка заданий разных типов**  
  `FILE`, `TASK`, `CONF`, `TIMEOUT`.

- **Логирование**  
  Все основные действия записываются в лог.

- **Автоматический цикл опроса**  
  Агент сам периодически обращается к серверу за новыми задачами.

## Технологический стек

| Компонент | Используется |
|---|---|
| Язык | C++17 |
| Сборка | CMake |
| HTTP-клиент | cpr / libcurl |
| JSON | nlohmann/json |
| Логирование | spdlog |
| Конфигурация | JSON-файл `config.json` |
| Потоки | `std::thread` |

## Структура проекта

```text
Webagent/
├── CMakeLists.txt
├── config.json
├── include/
│   ├── agent.h
│   ├── config.h
│   ├── http_client.h
│   └── logger.h
├── src/
│   ├── agent.cpp
│   ├── config.cpp
│   ├── http_client.cpp
│   ├── logger.cpp
│   └── main.cpp
├── tests/
└── README.md
```

## Конфигурация

Перед запуском проверьте файл `config.json`.

Пример:

```json
{
    "access_code": "8b4567-99bb-257a-d32d-072aa651",
    "log_file": "./agent.log",
    "max_poll_interval": 300,
    "max_retries": 3,
    "poll_interval": 30,
    "results_folder": "./results",
    "retry_delay": 5,
    "server_url": "https://xdev.arkcom.ru:9999/app/webagent1/api",
    "tasks_folder": "./tasks",
    "timeout": 30,
    "uid": "00767"
}
```

### Основные параметры

| Поле | Описание |
|---|---|
| `uid` | Уникальный идентификатор агента |
| `access_code` | Код доступа, полученный при регистрации агента |
| `server_url` | URL API сервера управления |
| `poll_interval` | Интервал опроса сервера в секундах |
| `timeout` | Таймаут HTTP-запросов |
| `max_poll_interval` | Максимальный интервал опроса при ошибках |
| `max_retries` | Количество повторных попыток при ошибке |
| `retry_delay` | Задержка между повторными попытками |
| `tasks_folder` | Папка для файлов заданий |
| `results_folder` | Папка для результатов |
| `log_file` | Файл логов |

> В заданиях сервера может использоваться ключ `poll_interval_sec`, но при ручном редактировании текущего `config.json` используется поле `poll_interval`.

## Установка и сборка

### Linux

Пример для Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y git cmake build-essential libcurl4-openssl-dev
```

Сборка проекта:

```bash
git clone https://github.com/viktoralekseev2018-hub/BestTeam
cd Webagent
mkdir build
cd build
cmake ..
cmake --build .
```

Запуск агента:

```bash
./web-agent -c ../config.json
```

### macOS

Установите инструменты разработчика:

```bash
xcode-select --install
```

Если используется Homebrew, установите зависимости:

```bash
brew install git cmake curl
```

Сборка проекта:

```bash
git clone https://github.com/viktoralekseev2018-hub/BestTeam
cd Webagent
mkdir build
cd build
cmake ..
cmake --build .
```

Запуск агента:

```bash
./web-agent -c ../config.json
```

### Windows

Установите:

- Git for Windows;
- CMake;
- Visual Studio Build Tools с компонентом **Desktop development with C++**;
- libcurl, например через vcpkg.

Пример установки libcurl через vcpkg:

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install curl:x64-windows
```

Сборка проекта:

```powershell
git clone https://github.com/viktoralekseev2018-hub/BestTeam
cd Webagent
mkdir build
cd build
cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

Запуск агента:

```powershell
.\Release\web-agent.exe -c ..\config.json
```

Если проект собирается без конфигурации `Release`, исполняемый файл может находиться прямо в папке `build`:

```powershell
.\web-agent.exe -c ..\config.json
```

## Запуск

Основная команда запуска из папки `build`:

```bash
./web-agent -c ../config.json
```

Для Windows:

```powershell
.\Release\web-agent.exe -c ..\config.json
```

Параметр `-c` указывает путь к файлу конфигурации.

Показать справку:

```bash
./web-agent -h
```

или:

```bash
./web-agent --help
```

## Как работает агент

1. Пользователь запускает агент.
2. Агент читает `config.json`.
3. Если `access_code` уже есть, агент использует его.
4. Агент начинает опрашивать сервер.
5. Сервер возвращает задание.
6. Агент выполняет задание.
7. Результат и файлы отправляются на сервер через `POST /wa_result/`.
8. Агент продолжает опрашивать сервер с заданным интервалом.

## Типы заданий

Агент поддерживает следующие типы заданий:

| Код задания | Назначение |
|---|---|
| `FILE` | Получение/отправка файла |
| `TASK` | Выполнение системной команды |
| `CONF` | Изменение конфигурации |
| `TIMEOUT` | Изменение интервала опроса |

---

# 1. FILE — получение и отправка файла

## Что отправляет сервер

```json
{
    "task_code": "FILE",
    "options": "{\"filename\":\"task.json\"}"
}
```

## Что делает агент

1. Получает задание с кодом `FILE`.
2. Читает поле `filename` из `options`.
3. Ищет файл `task.json` в своей рабочей директории.
4. Если агент запущен из папки `build`, рабочей директорией будет `build/`.
5. Копирует найденный файл в папку `results/`.
6. Отправляет файл на сервер через `POST /wa_result/`.

## Ручная имитация

Linux/macOS:

```bash
# Перейти в папку сборки
cd build

# Создать файл, который должен найти агент
echo "test content" > task.json

# Запустить агента
./web-agent -c ../config.json
```

Windows PowerShell:

```powershell
cd build

# Создать файл, который должен найти агент
"test content" > task.json

# Запустить агента
.\Release\web-agent.exe -c ..\config.json
```

## Результат

- файл копируется в `results/task.json`;
- файл отправляется на сервер;
- в логах появляется сообщение:

```text
[info] Задание FILE выполнено успешно
```

---

# 2. TASK — выполнение команды

## Что отправляет сервер

```json
{
    "task_code": "TASK",
    "options": "{\"command\":\"uname -a\"}"
}
```

## Что делает агент

1. Получает задание с кодом `TASK`.
2. Извлекает команду из поля `options`.
3. Выполняет команду через `std::system()`.
4. Сохраняет код возврата:
   - `0` — команда выполнена успешно;
   - не `0` — команда завершилась с ошибкой.
5. Проверяет новые файлы в папке `results/`.
6. Отправляет код возврата и найденные файлы на сервер.

## Ручная имитация

Linux/macOS:

```bash
# Команда, которую может выполнить агент
uname -a

# Проверить код возврата
echo $?
```

Windows PowerShell:

```powershell
# Пример команды для Windows
whoami

# Проверить код возврата
echo $LASTEXITCODE
```

## Примеры команд для Linux/macOS

```json
{"command":"ls -la"}
{"command":"date"}
{"command":"python3 script.py"}
{"command":"df -h"}
{"command":"ps aux"}
{"command":"uname -a"}
```

## Примеры команд для Windows

```json
{"command":"dir"}
{"command":"whoami"}
{"command":"echo Hello"}
{"command":"python script.py"}
{"command":"ipconfig"}
{"command":"tasklist"}
```

## Результат

- код возврата команды отправляется на сервер;
- новые файлы из `results/` прикрепляются к результату;
- в логах отображается статус выполнения задания.

---

# 3. CONF — изменение конфигурации

## Что отправляет сервер

```json
{
    "task_code": "CONF",
    "options": "{\"key\":\"poll_interval_sec\",\"value\":\"30\"}"
}
```

## Что делает агент

1. Получает задание с кодом `CONF`.
2. Парсит JSON из поля `options`.
3. Извлекает `key` и `value`.
4. Обновляет параметр конфигурации в памяти.
5. Сохраняет изменения в `config.json`.
6. Продолжает работу с обновлёнными настройками.

## Поддерживаемые ключи

| Ключ | Что меняет |
|---|---|
| `poll_interval_sec` | Интервал опроса сервера |
| `timeout` | Таймаут HTTP-запросов |
| `max_poll_interval` | Максимальный интервал при ошибках |
| `max_retries` | Количество попыток при ошибке |
| `retry_delay` | Задержка между попытками |

Дополнительно агент может принимать ключ `poll_interval`, так как именно это поле используется в текущем `config.json`.

## Ручная имитация

Linux/macOS:

```bash
# Открыть config.json
nano ../config.json

# Изменить поле:
# "poll_interval": 30

# Запустить агент снова
./web-agent -c ../config.json
```

Windows PowerShell:

```powershell
# Открыть config.json в блокноте
notepad ..\config.json

# Изменить поле:
# "poll_interval": 30

# Запустить агент снова
.\Release\web-agent.exe -c ..\config.json
```

## Результат

- параметр изменяется в `config.json`;
- агент использует новые настройки;
- в логах появляется сообщение об изменении параметра.

---

# 4. TIMEOUT — изменение интервала опроса

## Что отправляет сервер

```json
{
    "task_code": "TIMEOUT",
    "options": "{\"interval\":\"15\"}"
}
```

## Что делает агент

1. Получает задание с кодом `TIMEOUT`.
2. Читает значение `interval`.
3. Изменяет интервал опроса сервера на указанное количество секунд.
4. Сохраняет новое значение в `config.json`.

## Альтернативный формат

```json
{
    "task_code": "TIMEOUT",
    "options": "{\"key\":\"poll_interval_sec\",\"value\":\"15\"}"
}
```

## Ручная имитация

Linux/macOS:

```bash
nano ../config.json

# Изменить:
# "poll_interval": 15

./web-agent -c ../config.json
```

Windows PowerShell:

```powershell
notepad ..\config.json

# Изменить:
# "poll_interval": 15

.\Release\web-agent.exe -c ..\config.json
```

## Результат

- интервал опроса изменяется на `15` секунд;
- новое значение сохраняется в `config.json`;
- агент начинает опрашивать сервер с новым интервалом.

---

## Пример полного пользовательского сценария

### Linux/macOS

```bash
git clone https://github.com/viktoralekseev2018-hub/BestTeam
cd BestTeam
mkdir build
cd build
cmake ..
cmake --build .

# Создать тестовый файл для задания FILE
echo "test content" > task.json

# Запустить агента
./web-agent -c ../config.json
```

### Windows PowerShell

```powershell
git clone https://github.com/viktoralekseev2018-hub/BestTeam
cd Webagent
mkdir build
cd build
cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release

# Создать тестовый файл для задания FILE
"test content" > task.json

# Запустить агента
.\Release\web-agent.exe -c ..\config.json
```

## Проверка логов

После запуска агент пишет события в файл:

```text
agent.log
```

Пример просмотра логов на Linux/macOS:

```bash
tail -f agent.log
```

Пример просмотра логов на Windows PowerShell:

```powershell
Get-Content agent.log -Wait
```

В логах можно увидеть:

- запуск агента;
- загрузку конфигурации;
- регистрацию;
- получение задания;
- выполнение задания;
- отправку результата;
- ошибки сети или конфигурации.

## Частые проблемы

### Агент не находит файл для FILE-задания

Проверьте, из какой папки запущен агент.  
Если запуск выполняется из `build/`, файл `task.json` должен лежать в `build/`.

Правильно:

```bash
cd build
echo "test content" > task.json
./web-agent -c ../config.json
```

### Не найден `config.json`

Укажите путь к конфигу через `-c`:

```bash
./web-agent -c ../config.json
```

На Windows:

```powershell
.\Release\web-agent.exe -c ..\config.json
```

### Ошибка подключения к серверу

Проверьте поле `server_url` в `config.json`:

```json
"server_url": "https://xdev.arkcom.ru:9999/app/webagent1/api"
```

Также проверьте интернет-соединение и доступность сервера.

### Команда TASK завершилась с ошибкой

Проверьте команду вручную в терминале.

Linux/macOS:

```bash
uname -a
echo $?
```

Windows:

```powershell
whoami
echo $LASTEXITCODE
```

Код `0` означает успешное выполнение.

## Диаграмма проекта

<img width="1024" height="1536" alt="structure" src="https://github.com/user-attachments/assets/b794d103-d21a-4e17-b3a5-794aa2b0b25a" />
