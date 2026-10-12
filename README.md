# Компьютерная графика — лабораторные 1–5

C++17 · OpenGL 3.3 Core · GLFW 3.4 · GLAD · GLM · CMake

```
computer_g/
├── CMakeLists.txt     сборка всего проекта
├── common/            общий код: окно, сборка шейдеров, клавиатура
├── external/glad/     загрузчик функций OpenGL 3.3 Core
├── week1/ … week5/    лабораторные (в каждой папке свой README)
└── screenshots/       результаты запуска
```

| Неделя | Тема | Программы |
|---|---|---|
| [1](week1/README.md) | окно, анимация фона, FPS | `week1` |
| [2](week2/README.md) | VAO/VBO, треугольники, пирамида | `week2_triangles`, `week2_pyramid` |
| [3](week3/README.md) | цвет вершин, интерполяция, ошибки шейдеров | `week3` |
| [4](week4/README.md) | движение и delta time | `week4` |
| [5](week5/README.md) | EBO, glDrawElements | `week5` |

## Сборка

GLFW и GLM скачиваются при первой конфигурации, нужен интернет.

**Visual Studio 2022** (компонент «Desktop development with C++»):
File → Open → Folder → `computer_g`, выбрать цель (`week1.exe` …) и F5.

**Командная строка** (x64 Native Tools Command Prompt for VS 2022):

```
cmake -B build
cmake --build build --config Debug
build\bin\Debug\week1.exe
build\bin\Debug\week2_pyramid.exe 6
```

Linux / macOS: `cmake -B build && cmake --build build && ./build/bin/week1`.

## common/

- `gl_app::start` — GLFW + окно 1280×720 с OpenGL 3.3 Core + GLAD, печать GPU и версии;
- `gl_app::buildProgram` — компиляция и линковка шейдеров, при ошибке печатает лог и возвращает 0;
- `gl_app::Keyboard` — `held(key)` (зажата) и `pressed(key)` (нажата в этом кадре).
