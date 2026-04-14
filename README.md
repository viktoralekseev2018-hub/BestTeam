Web-Agent
Автономный агент для распределённого выполнения задач
 
Обзор
Это кроссплатформенный клиент-агент, предназначенный для получения, обработки и отправки задач с удалённого сервера управления.
Агент работает независимо, автоматически регистрируется в системе и выполняет команды без участия пользователя.
 Подходит для:
•	распределённых систем
•	CI/CD воркеров
•	фоновых вычислений
•	удалённого управления узлами
 
Ключевые особенности
•	Кроссплатформенность
Поддержка Windows, Linux и macOS без изменений кода
•	Безопасное соединение
Работа через HTTPS (OpenSSL)
•	араллельное выполнение задач
Отдельные потоки для получения и выполнения задач
•	Саморегистрация агента
Генерация UID + получение access_code
•	Потокобезопасный логгер
Без конфликтов при записи из разных потоков
 
Технологический стек
Компонент	Используется
Язык	C++17
Сборка	CMake ≥ 3.10
HTTP клиент	cpp-httplib
Шифрование	OpenSSL
Конфигурация	INI
 
 
Компоненты
•	Agent — центральный управляющий класс
•	Poll Thread — запрашивает задачи с сервера
•	Task Queue — потокобезопасная очередь
•	Worker Thread — исполняет задачи
•	ServerClient — HTTP взаимодействие
•	TaskExecutor — логика выполнения задач
•	Logger — централизованное логирование
•	Config — управление настройками
 
 


Сборка проекта
git clone https://github.com/viktoralekseev2018-hub/DeepSeekers](https://github.com/viktoralekseev2018-hub/BestTeam

mkdir build && cd build
cmake ..
cmake --build .
Бинарник появится в:
/build/cardioagent
 
Быстрый старт
1. Создать конфиг
cp config/agent.ini.example config/agent.ini
 
2. Заполнить обязательные поля
server_url = https://your-server.com:9999
 
3. Опциональные настройки
descr = worker-node-1
poll_interval = 10
max_poll_interval = 300
 
4. Автоматические поля (не трогать)
UID=
access_code=
 
5. Запуск
./build/cardioagent
 
API взаимодействие
Регистрация агента
POST /api/wa_reg/
Тело запроса:
{
  "UID": "auto-generated",
  "descr": "worker-node"
}
Ответ:
{
  "access_code": "...",
  "code_response": "0"
}
 
Получение задачи
POST /api/wa_task/
{
  "UID": "...",
  "access_code": "..."
}
Ответ (если есть задача):
{
  "task_code": "...",
  "session_id": "...",
  "status": "RUN"
}
 
Отправка результата
POST /api/wa_result/
Формат: multipart/form-data
Поле	Тип	Описание
result_code	int	0 — OK
result	JSON	метаданные
files	file[]	файлы
 
Структура проекта
CardioAgent/
│
├── include/        # Заголовки
├── src/            # Реализация
├── config/         # Конфигурация
├── lib/            # Зависимости
├── scripts/        # Утилиты
├── logs/           # Логи
├── temp/           # Временные файлы
└── CMakeLists.txt
 
Роли в проекте
Роль	Ответственность
Тимлид	Алексеев Виктор
Архитектор	Алексеев Виктор
Разработчик	Стрижов Тимофей
Тестировщик	Стрижов Тимофей
Техписатель	Стрижов Тимофей



<img width="1024" height="1536" alt="structure" src="https://github.com/user-attachments/assets/ed8009b5-8e13-43f6-9347-762f284a0c7a" />


