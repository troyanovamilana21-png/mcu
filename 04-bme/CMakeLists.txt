# Требуем минимальную версию cmake
cmake_minimum_required(VERSION 3.15)

# Задаем параметр с используемой платой и микроконтроллером
set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

# Добавляем файл, автоматически подтягивающий Pico SDK
include(pico_sdk_import.cmake)

# Создаем проект
project(04-bme)

# Вызываем функцию инициализации Pico SDK
pico_sdk_init()

# Добавляем библиотеки из папки libs
add_subdirectory("../libs/bme280-driver" "${PROJECT_BINARY_DIR}/bme280_driver")
add_subdirectory("../libs/protocol" "${PROJECT_BINARY_DIR}/protocol")

# Добавляем в проект файлы с исходным кодом
add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-task/stdio-task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task/led-task.c
)

# Указываем на какие библиотеки ссылается наш код
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_gpio
    hardware_i2c      # Добавляем I2C
    protocol
    bme280_driver     # Добавляем драйвер BME280
)

# Явно указываем пути для поиска заголовочных файлов
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-task
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task
)

# Указываем файл, в котором находится линкер скрипт
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/memmap_rp2040.ld)

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Добавляем задачи генерации бинарника в разных форматах
pico_add_extra_outputs(${PROJECT_NAME})