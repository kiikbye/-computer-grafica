// Неделя 1. Окно 1280x720, фон меняет цвет по синусу,
// пока зажат SPACE — фон белый, FPS раз в секунду в терминал.

#include "gl_app.hpp"

#include <cmath>
#include <iostream>

namespace {

// Во сколько раз ускорена анимация фона (задание 3: «ускорить смену цвета»).
// При 1.0 полный цикл одного канала длится 2π ≈ 6.3 с, при 2.5 — около 2.5 с.
constexpr float kColorSpeed = 2.5f;

struct Rgb {
    float r, g, b;
};

// Каждый канал — своя синусоида со своей частотой, поэтому
// сочетания цветов почти не повторяются. sin даёт -1..1,
// переводим в 0..1 формулой 0.5 + 0.5 * sin(x).
Rgb backgroundAt(float seconds) {
    const float t = seconds * kColorSpeed;
    return {
        0.5f + 0.5f * std::sin(t),
        0.5f + 0.5f * std::sin(t * 0.7f + 2.0f),
        0.5f + 0.5f * std::sin(t * 1.3f + 4.0f),
    };
}

} // namespace

int main() {
    // vsync = false: кадры рисуются без ожидания монитора (задание 6),
    // поэтому FPS показывает реальную скорость видеокарты.
    GLFWwindow* window = gl_app::start("Week 1 - window", 1280, 720, false);
    if (window == nullptr) {
        return 1;
    }
    gl_app::Keyboard keys(window);
    std::cout << "Hold SPACE - white background, ESC - exit\n";

    int frames = 0;
    double secondStart = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (keys.held(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Пока SPACE зажат — белый; отпустили — снова анимация (задания 4 и 5).
        const Rgb bg = keys.held(GLFW_KEY_SPACE)
                           ? Rgb{1.0f, 1.0f, 1.0f}
                           : backgroundAt(static_cast<float>(glfwGetTime()));
        glClearColor(bg.r, bg.g, bg.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);

        // FPS: считаем кадры, раз в секунду печатаем и обнуляем (задание 7).
        ++frames;
        const double now = glfwGetTime();
        if (now - secondStart >= 1.0) {
            std::cout << "FPS: " << frames << '\n';
            frames = 0;
            secondStart = now;
        }
    }

    gl_app::finish(window);
    return 0;
}
