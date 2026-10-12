// Неделя 2, доп. задание. Пирамида из N рядов треугольников:
// в первом (верхнем) ряду 1 треугольник, во втором 2, ..., в N-м — N.
// Всего N*(N+1)/2 треугольников.
//
// Число рядов: аргумент командной строки (week2_pyramid 6)
// или ввод с клавиатуры в консоли. Допустимо 1..100.
//   TAB — каркасный режим вкл/выкл
//   ESC — выход

#include "gl_app.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kMinRows = 1;
constexpr int kMaxRows = 100;
const float kRowHeight = std::sqrt(3.0f) / 2.0f;  // высота равностороннего треугольника со стороной 1

// Вершины задаём в «единицах треугольника» (сторона = 1),
// в NDC их переводит uniform scale — так треугольники остаются
// равносторонними при любом размере окна.
const char* kVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec2 position;
uniform vec2 scale;
void main() {
    gl_Position = vec4(position * scale, 0.0, 1.0);
}
)glsl";

const char* kFragmentShader = R"glsl(
#version 330 core
uniform vec3 color;
out vec4 outColor;
void main() {
    outColor = vec4(color, 1.0);
}
)glsl";

int askRowCount(int argc, char** argv) {
    if (argc > 1) {
        const int fromArgs = std::atoi(argv[1]);
        if (fromArgs >= kMinRows && fromArgs <= kMaxRows) {
            return fromArgs;
        }
    }
    while (true) {
        std::cout << "Rows (" << kMinRows << ".." << kMaxRows << "): ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return 0;  // ввод закрыт — выходим
        }
        const int rows = std::atoi(line.c_str());
        if (rows >= kMinRows && rows <= kMaxRows) {
            return rows;
        }
        std::cout << "Enter a number from " << kMinRows << " to " << kMaxRows << '\n';
    }
}

// Пирамида центрирована в (0, 0). Ряд r (0 = верхний) содержит r+1
// треугольников, ряд сдвинут влево на половину своей ширины.
std::vector<float> pyramidVertices(int rows) {
    std::vector<float> v;
    v.reserve(static_cast<size_t>(rows) * (rows + 1) / 2 * 6);
    const float top = rows * kRowHeight / 2.0f;
    for (int r = 0; r < rows; ++r) {
        const float rowTop = top - r * kRowHeight;
        const float rowBottom = rowTop - kRowHeight;
        const float rowLeft = -(r + 1) / 2.0f;
        for (int i = 0; i <= r; ++i) {
            const float x = rowLeft + i;
            v.insert(v.end(), {x, rowBottom,   x + 1.0f, rowBottom,   x + 0.5f, rowTop});
        }
    }
    return v;
}

} // namespace

int main(int argc, char** argv) {
    const int rows = askRowCount(argc, argv);
    if (rows == 0) {
        return 1;
    }
    const std::vector<float> vertices = pyramidVertices(rows);
    const GLsizei pointCount = static_cast<GLsizei>(vertices.size() / 2);
    std::cout << "Rows: " << rows << ", triangles: " << pointCount / 3 << '\n';

    const std::string title = "Week 2 - pyramid: " + std::to_string(rows) + " rows, " +
                              std::to_string(pointCount / 3) + " triangles";
    GLFWwindow* window = gl_app::start(title.c_str(), 1280, 720, true);
    if (window == nullptr) {
        return 1;
    }
    gl_app::Keyboard keys(window);

    const GLuint program = gl_app::buildProgram(kVertexShader, kFragmentShader);
    if (program == 0) {
        gl_app::finish(window);
        return 1;
    }
    const GLint scaleLoc = glGetUniformLocation(program, "scale");
    const GLint colorLoc = glGetUniformLocation(program, "color");

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    bool wireframe = false;
    std::cout << "TAB - wireframe, ESC - exit\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (keys.held(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (keys.pressed(GLFW_KEY_TAB)) {
            wireframe = !wireframe;
        }

        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        if (w == 0 || h == 0) {
            continue;  // окно свёрнуто
        }

        // Пирамида шириной rows и высотой rows*kRowHeight должна влезть
        // в 90% окна. Пиксель по x и по y в NDC разный, поэтому scale.x
        // дополнительно умножаем на h / w.
        const float aspect = static_cast<float>(w) / static_cast<float>(h);
        const float fit = 1.8f / std::max(rows * kRowHeight, rows / aspect);

        glClearColor(0.10f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

        glUseProgram(program);
        glUniform2f(scaleLoc, fit / aspect, fit);
        glUniform3f(colorLoc, 0.95f, 0.75f, 0.25f);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, pointCount);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    gl_app::finish(window);
    return 0;
}
