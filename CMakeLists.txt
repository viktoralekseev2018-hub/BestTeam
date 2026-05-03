cmake_minimum_required(VERSION 3.14)
project(WebAgent VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Сборка тестов" OFF)

# Поиск системных библиотек ДО FetchContent
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Подключение зависимостей
include(FetchContent)

# CPR - HTTP клиент (с указанием использовать системную CURL)
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system curl" FORCE)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

# nlohmann/json - JSON парсинг
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# spdlog - Логирование
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Заголовочные файлы
set(INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
)

# Исходные файлы
set(SOURCES
    src/agent.cpp
    src/config.cpp
    src/http_client.cpp
    src/logger.cpp
)

# Статическая библиотека
add_library(web-agent-lib STATIC ${SOURCES})
target_include_directories(web-agent-lib PUBLIC ${INCLUDE_DIRS})
target_link_libraries(web-agent-lib
    cpr::cpr
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Threads::Threads
    ${CURL_LIBRARIES}
)

# Исполняемый файл
add_executable(web-agent src/main.cpp)
target_link_libraries(web-agent web-agent-lib)

# Тесты
if(BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
    )
    FetchContent_MakeAvailable(doctest)

    set(TEST_SOURCES
        tests/test_main.cpp
        tests/test_http.cpp
        tests/test_parser.cpp
        tests/test_task.cpp
        tests/test_config.cpp
        tests/test_logger.cpp
        tests/test_agent.cpp
    )
    add_executable(tests ${TEST_SOURCES})
    target_include_directories(tests PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(tests
        web-agent-lib
        doctest::doctest
    )
    add_test(NAME web-agent-tests COMMAND tests)
endif()