// Неделя 3. У каждой вершины свой цвет — внутри треугольника градиент.
//   C   — три разных цвета <-> один цвет на всех вершинах (градиент пропадает)
//   I   — инверсия цвета во фрагментном шейдере (1.0 - цвет)
//   V   — перевернуть треугольник в вершинном шейдере (y * -1)
//   E   — собрать шейдер с намеренной ошибкой и показать лог в консоли
//   ESC — выход

#include "gl_app.hpp"

#include <cstddef>
#include <iostream>

namespace {

// position и color — два атрибута вершины (location 0 и 1).
// color передаётся во фрагментный шейдер через out/in, и по дороге
// видеокарта смешивает (интерполирует) цвета трёх вершин.
const char* kVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
uniform float flipY;
out vec3 fragmentColor;
void main() {
    gl_Position = vec4(position.x, position.y * flipY, 0.0, 1.0);
    fragmentColor = color;
}
)glsl";

const char* kFragmentShader = R"glsl(
#version 330 core
in vec3 fragmentColor;
uniform bool invert;
out vec4 outColor;
void main() {
    vec3 c = fragmentColor;
    if (invert) {
        c = 1.0 - c;
    }
    outColor = vec4(c, 1.0);
}
)glsl";

// Тот же фрагментный шейдер, но после "vec3 c = fragmentColor" нет точки
// с запятой. Компилятор замечает это только на следующей строке, поэтому
// в логе будет «0:8» (строка 1 строкового литерала пустая, #version — строка 2).
const char* kBrokenFragmentShader = R"glsl(
#version 330 core
in vec3 fragmentColor;
uniform bool invert;
out vec4 outColor;
void main() {
    vec3 c = fragmentColor
    outColor = vec4(c, 1.0);
}
)glsl";

struct Vertex {
    float x, y;
    float r, g, b;
};

const Vertex kRainbow[3] = {
    {-0.55f, -0.50f,   1.0f, 0.0f, 0.0f},
    { 0.55f, -0.50f,   0.0f, 1.0f, 0.0f},
    { 0.00f,  0.60f,   0.0f, 0.0f, 1.0f},
};

const Vertex kSingleColor[3] = {
    {-0.55f, -0.50f,   0.2f, 0.7f, 0.9f},
    { 0.55f, -0.50f,   0.2f, 0.7f, 0.9f},
    { 0.00f,  0.60f,   0.2f, 0.7f, 0.9f},
};

} // namespace

int main() {
    GLFWwindow* window = gl_app::start("Week 3 - vertex colors", 1280, 720, true);
    if (window == nullptr) {
        return 1;
    }
    gl_app::Keyboard keys(window);

    const GLuint program = gl_app::buildProgram(kVertexShader, kFragmentShader);
    if (program == 0) {
        gl_app::finish(window);
        return 1;
    }
    const GLint flipLoc = glGetUniformLocation(program, "flipY");
    const GLint invertLoc = glGetUniformLocation(program, "invert");

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kRainbow), kRainbow, GL_DYNAMIC_DRAW);

    // Шаг между вершинами — sizeof(Vertex) = 5 float.
    // Позиция начинается с 0-го байта вершины, цвет — с поля r.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    bool singleColor = false;
    bool inverted = false;
    bool flipped = false;
    std::cout << "C - same color, I - invert, V - flip, E - broken shader demo, ESC - exit\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (keys.held(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (keys.pressed(GLFW_KEY_C)) {
            singleColor = !singleColor;
            // Новые цвета записываются поверх старых в тот же буфер.
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(kRainbow), singleColor ? kSingleColor : kRainbow);
            std::cout << "Vertex colors: " << (singleColor ? "same" : "red/green/blue") << '\n';
        }
        if (keys.pressed(GLFW_KEY_I)) {
            inverted = !inverted;
            std::cout << "Invert: " << (inverted ? "on" : "off") << '\n';
        }
        if (keys.pressed(GLFW_KEY_V)) {
            flipped = !flipped;
            std::cout << "Flip: " << (flipped ? "on" : "off") << '\n';
        }
        if (keys.pressed(GLFW_KEY_E)) {
            std::cout << "Building shader with a deliberate error...\n";
            const GLuint broken = gl_app::buildProgram(kVertexShader, kBrokenFragmentShader);
            if (broken == 0) {
                std::cout << "Build failed (see log above), the working shader is kept\n";
            } else {
                glDeleteProgram(broken);
            }
        }

        glClearColor(0.11f, 0.11f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);  // uniform задаётся активной программе
        glUniform1f(flipLoc, flipped ? -1.0f : 1.0f);
        glUniform1i(invertLoc, inverted ? 1 : 0);
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
