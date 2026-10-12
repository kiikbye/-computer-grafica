// Неделя 2. Два отдельных треугольника из одного массива (6 вершин).
//   L   — GL_TRIANGLES <-> GL_LINE_LOOP по всем 6 вершинам
//   F   — каркасный режим (glPolygonMode) вкл/выкл
//   ESC — выход

#include "gl_app.hpp"

#include <iostream>

namespace {

// Вершинный шейдер ничего не меняет: координаты уже в NDC (-1..1).
const char* kVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)glsl";

// Цвет задан прямо во фрагментном шейдере — оранжевый.
const char* kFragmentShader = R"glsl(
#version 330 core
out vec4 outColor;
void main() {
    outColor = vec4(1.0, 0.55, 0.1, 1.0);
}
)glsl";

// 6 вершин = 2 треугольника по 3 вершины, по 2 числа (x, y) на вершину.
const float kVertices[] = {
    // левый треугольник
    -0.80f, -0.40f,
    -0.20f, -0.40f,
    -0.50f,  0.45f,
    // правый треугольник
     0.20f,  0.40f,
     0.80f,  0.40f,
     0.50f, -0.45f,
};
constexpr GLsizei kVertexCount = 6;

} // namespace

int main() {
    GLFWwindow* window = gl_app::start("Week 2 - two triangles", 1280, 720, true);
    if (window == nullptr) {
        return 1;
    }
    gl_app::Keyboard keys(window);

    const GLuint program = gl_app::buildProgram(kVertexShader, kFragmentShader);
    if (program == 0) {
        gl_app::finish(window);
        return 1;
    }

    // VBO — массив вершин в памяти видеокарты.
    // VAO — запоминает, где лежат вершины и как их читать.
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    // атрибут 0: 2 float на вершину, вершины идут подряд (шаг 2 float)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    bool lineLoop = false;
    bool wireframe = false;
    std::cout << "L - GL_LINE_LOOP, F - wireframe, ESC - exit\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (keys.held(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (keys.pressed(GLFW_KEY_L)) {
            lineLoop = !lineLoop;
            std::cout << "Primitive: " << (lineLoop ? "GL_LINE_LOOP" : "GL_TRIANGLES") << '\n';
        }
        if (keys.pressed(GLFW_KEY_F)) {
            wireframe = !wireframe;
            std::cout << "Wireframe: " << (wireframe ? "on" : "off") << '\n';
        }

        glClearColor(0.12f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

        glUseProgram(program);
        glBindVertexArray(vao);
        // Один вызов на все 6 вершин. GL_TRIANGLES берёт их тройками,
        // а GL_LINE_LOOP соединяет все 6 по порядку в ОДНУ замкнутую ломаную —
        // поэтому треугольники выглядят как одна фигура.
        glDrawArrays(lineLoop ? GL_LINE_LOOP : GL_TRIANGLES, 0, kVertexCount);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    gl_app::finish(window);
    return 0;
}
