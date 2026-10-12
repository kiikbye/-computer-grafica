// Общие помощники для всех лабораторных: окно, шейдеры, клавиатура.
#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace gl_app {

// Запускает GLFW, открывает окно с OpenGL 3.3 Core, подключает GLAD
// и печатает видеокарту и версию OpenGL. При ошибке возвращает nullptr.
GLFWwindow* start(const char* title, int width, int height, bool vsync);

// Закрывает окно и выключает GLFW.
void finish(GLFWwindow* window);

// Собирает шейдерную программу из исходников GLSL.
// Если вершинный/фрагментный шейдер не компилируется или программа
// не линкуется — печатает лог OpenGL и возвращает 0.
GLuint buildProgram(const char* vsText, const char* fsText);

// Опрос клавиатуры каждый кадр.
//   held(key)    — клавиша сейчас зажата;
//   pressed(key) — клавишу нажали именно в этом кадре (для переключателей).
// Для pressed() запоминаем, была ли клавиша зажата в прошлом кадре.
class Keyboard {
public:
    // GLFW_STICKY_KEYS: если клавишу нажали и отпустили между двумя кадрами,
    // glfwGetKey всё равно один раз вернёт PRESS — быстрое нажатие не потеряется.
    explicit Keyboard(GLFWwindow* window) : window_(window) {
        glfwSetInputMode(window_, GLFW_STICKY_KEYS, GLFW_TRUE);
    }

    bool held(int key) const { return glfwGetKey(window_, key) == GLFW_PRESS; }

    bool pressed(int key) {
        const bool now = held(key);
        const bool wasDown = previous_[key];
        previous_[key] = now;
        return now && !wasDown;
    }

private:
    GLFWwindow* window_;
    bool previous_[GLFW_KEY_LAST + 1] = {};
};

} // namespace gl_app
