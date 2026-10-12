// Неделя 4. Треугольник летает по кругу. Скорость задана в радианах
// в секунду, поэтому угол растёт на speed * dt, где dt — время кадра.
//   T   — сравнение: угол растёт на 0.02 за КАДР (без dt) <-> с dt
//   V   — VSync вкл/выкл (меняет FPS, чтобы увидеть разницу)
//   ESC — выход

#include "gl_app.hpp"

#include <cmath>
#include <cstdio>
#include <iostream>

namespace {

constexpr float kSpeed = 2.0f;         // рад/с
constexpr float kStepPerFrame = 0.02f; // рад за кадр — вариант без dt
constexpr float kRadius = 0.55f;

// Вершины треугольника заданы вокруг (0, 0); center сдвигает его на круг.
// aspect (= высота/ширина окна) сжимает x, чтобы круг не был овалом.
const char* kVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
uniform vec2 center;
uniform float aspect;
out vec3 fragmentColor;
void main() {
    vec2 p = position + center;
    gl_Position = vec4(p.x * aspect, p.y, 0.0, 1.0);
    fragmentColor = color;
}
)glsl";

const char* kFragmentShader = R"glsl(
#version 330 core
in vec3 fragmentColor;
out vec4 outColor;
void main() {
    outColor = vec4(fragmentColor, 1.0);
}
)glsl";

// x, y, r, g, b
const float kTriangle[] = {
    -0.12f, -0.09f,   0.95f, 0.35f, 0.30f,
     0.12f, -0.09f,   0.30f, 0.90f, 0.45f,
     0.00f,  0.14f,   0.35f, 0.50f, 1.00f,
};

} // namespace

int main() {
    bool vsync = false;
    GLFWwindow* window = gl_app::start("Week 4 - delta time", 1280, 720, vsync);
    if (window == nullptr) {
        return 1;
    }
    gl_app::Keyboard keys(window);

    const GLuint program = gl_app::buildProgram(kVertexShader, kFragmentShader);
    if (program == 0) {
        gl_app::finish(window);
        return 1;
    }
    // Номера uniform-ов узнаём один раз, до цикла.
    const GLint centerLoc = glGetUniformLocation(program, "center");
    const GLint aspectLoc = glGetUniformLocation(program, "aspect");

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kTriangle), kTriangle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    bool useDt = true;
    float angle = 0.0f;
    // Берём текущее время, а не 0: иначе первый dt = всё время запуска.
    double prevFrameTime = glfwGetTime();
    double fpsStart = prevFrameTime;
    int frames = 0;
    std::cout << "T - with/without dt, V - vsync, ESC - exit\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - prevFrameTime);
        prevFrameTime = now;

        if (keys.held(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (keys.pressed(GLFW_KEY_T)) {
            useDt = !useDt;
            std::cout << (useDt ? "angle += speed * dt  (same speed at any FPS)\n"
                                : "angle += 0.02 per frame  (speed depends on FPS)\n");
        }
        if (keys.pressed(GLFW_KEY_V)) {
            vsync = !vsync;
            glfwSwapInterval(vsync ? 1 : 0);
            std::cout << "VSync: " << (vsync ? "on" : "off") << '\n';
        }

        // Обновление: сначала меняем состояние...
        angle += useDt ? kSpeed * dt : kStepPerFrame;

        ++frames;
        if (now - fpsStart >= 0.5) {
            char title[128];
            std::snprintf(title, sizeof(title), "Week 4 - delta time | %s | %d FPS",
                          useDt ? "with dt" : "WITHOUT dt", static_cast<int>(frames / (now - fpsStart)));
            glfwSetWindowTitle(window, title);
            frames = 0;
            fpsStart = now;
        }

        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        if (w == 0 || h == 0) {
            continue;
        }

        // ...потом рисуем его.
        glClearColor(0.10f, 0.10f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glUniform2f(centerLoc, kRadius * std::cos(angle), kRadius * std::sin(angle));
        glUniform1f(aspectLoc, static_cast<float>(h) / static_cast<float>(w));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    gl_app::finish(window);
    return 0;
}
