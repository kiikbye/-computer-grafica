cmake_minimum_required(VERSION 3.20)
project(computer_g LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/utf-8)   # русские комментарии в исходниках
endif()

# GLFW и GLM скачиваются архивами при первой конфигурации.
include(FetchContent)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(glfw glm)

# Общий код всех недель: окно, шейдеры, клавиши + GLAD.
add_library(common STATIC
    common/gl_app.cpp
    external/glad/src/gl.c)
target_include_directories(common PUBLIC common external/glad/include)
target_link_libraries(common PUBLIC glfw glm::glm)

# Каждая неделя — своя папка со своим CMakeLists.txt.
# Подключаем только те, что уже есть, чтобы проект собирался на любом этапе.
foreach(week week1 week2 week3 week4 week5)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${week}/CMakeLists.txt)
        add_subdirectory(${week})
    endif()
endforeach()
