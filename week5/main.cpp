// Неделя 5. EBO (индексный буфер) и glDrawElements.
//   1 — квадрат: 4 вершины, 6 индексов
//   2 — рисуем только первые 3 индекса {0, 1, 2}: остаётся один треугольник
//   3 — пятиугольник: 5 вершин, 9 индексов (3 треугольника)
//   4 — два квадрата из одного VAO/VBO/EBO, место задаёт glUniform2f
//   5 — итог: два квадрата летают по кругу и пульсируют
//   W / S — (удерживать) быстрее / медленнее, ESC — выход
//
// Все фигуры лежат в ОДНОМ буфере вершин и ОДНОМ буфере индексов;
// какую фигуру рисовать, решают count и смещение в glDrawElements.

#include "gl_app.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kCircleRadius = 0.55f;

const char* kVertexShader = R"glsl(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
uniform vec2 center;   // куда поставить фигуру
uniform float size;    // во сколько раз увеличить
uniform float aspect;  // высота / ширина окна
out vec3 fragmentColor;
void main() {
    vec2 p = position * size + center;
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

struct Vertex {
    float x, y;
    float r, g, b;
};

// Часть общего буфера индексов: сколько индексов и с какого начинать.
struct Shape {
    GLsizei count;
    GLsizei firstIndex;
};

// Вершины 0..3 — квадрат, 4..8 — пятиугольник.
// Индексы 0..5 — квадрат, 6..14 — пятиугольник.
void buildGeometry(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                   Shape& quad, Shape& pentagon) {
    //  3 ---- 2
    //  |    / |
    //  |  /   |
    //  0 ---- 1
    vertices = {
        {-0.2f, -0.2f,   0.95f, 0.30f, 0.35f},
        { 0.2f, -0.2f,   0.95f, 0.80f, 0.20f},
        { 0.2f,  0.2f,   0.25f, 0.80f, 0.55f},
        {-0.2f,  0.2f,   0.35f, 0.45f, 0.95f},
    };
    // Первые три индекса — {0, 1, 2}: это и есть «сломанный» вариант из задания 1.
    indices = {0, 1, 2,   2, 3, 0};
    quad = {6, 0};

    // Пятиугольник: 5 вершин на окружности, первая — сверху.
    const GLuint base = static_cast<GLuint>(vertices.size());  // = 4
    for (int i = 0; i < 5; ++i) {
        const float a = kPi / 2.0f + i * 2.0f * kPi / 5.0f;
        const float t = i / 4.0f;  // цвет плавно от жёлтого к фиолетовому
        vertices.push_back({0.25f * std::cos(a), 0.25f * std::sin(a), 1.0f - 0.4f * t, 0.85f - 0.6f * t, 0.2f + 0.7f * t});
    }
    // Веер из вершины 0 пятиугольника: (0,1,2), (0,2,3), (0,3,4).
    // В общем буфере его вершины начинаются с номера base, поэтому + base.
    pentagon = {9, static_cast<GLsizei>(indices.size())};
    for (GLuint i = 1; i <= 3; ++i) {
        indices.insert(indices.end(), {base, base + i, base + i + 1});
    }
}

void draw(const Shape& shape) {
    // Последний аргумент — смещение в БАЙТАХ от начала буфера индексов.
    glDrawElements(GL_TRIANGLES, shape.count, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(shape.firstIndex * sizeof(GLuint)));
}

} // namespace

int main() {
    GLFWwindow* window = gl_app::start("Week 5 - EBO", 1280, 720, true);
    if (window == nullptr) {
        return 1;
    }
    gl_app::Keyboard keys(window);

    const GLuint program = gl_app::buildProgram(kVertexShader, kFragmentShader);
    if (program == 0) {
        gl_app::finish(window);
        return 1;
    }
    const GLint centerLoc = glGetUniformLocation(program, "center");
    const GLint sizeLoc = glGetUniformLocation(program, "size");
    const GLint aspectLoc = glGetUniformLocation(program, "aspect");

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    Shape quad{};
    Shape pentagon{};
    buildGeometry(vertices, indices, quad, pentagon);
    const Shape firstTriangle{3, 0};  // индексы {0, 1, 2}

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    // EBO привязываем, пока привязан VAO: VAO запоминает свой EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);  // EBO отвязывать до этого нельзя — VAO его «забудет»

    std::cout << "Vertices: " << vertices.size() << ", indices: " << indices.size() << '\n'
              << "1-5 - task, W/S - speed, ESC - exit\n";

    int task = 1;
    float speed = 1.2f;  // рад/с
    float angle = 0.0f;
    double prevFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - prevFrameTime);
        prevFrameTime = now;

        if (keys.held(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        for (int k = 1; k <= 5; ++k) {
            if (keys.pressed(GLFW_KEY_0 + k)) {
                task = k;
                std::cout << "Task " << task << '\n';
            }
        }
        // Скорость меняется «в секунду», поэтому тоже через dt.
        if (keys.held(GLFW_KEY_W)) {
            speed += 1.0f * dt;
        }
        if (keys.held(GLFW_KEY_S)) {
            speed -= 1.0f * dt;
        }
        speed = std::clamp(speed, 0.0f, 6.0f);
        angle += speed * dt;

        char title[96];
        std::snprintf(title, sizeof(title), "Week 5 - EBO | task %d | speed %.2f rad/s", task, speed);
        glfwSetWindowTitle(window, title);

        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        if (w == 0 || h == 0) {
            continue;
        }

        glClearColor(0.09f, 0.10f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glUniform1f(aspectLoc, static_cast<float>(h) / static_cast<float>(w));
        glUniform1f(sizeLoc, 1.0f);
        glUniform2f(centerLoc, 0.0f, 0.0f);
        glBindVertexArray(vao);

        switch (task) {
        case 1:
            draw(quad);
            break;
        case 2:
            draw(firstTriangle);
            break;
        case 3:
            draw(pentagon);
            break;
        case 4:
            // Один и тот же квадрат дважды: меняется только uniform center.
            glUniform2f(centerLoc, -0.45f, 0.0f);
            draw(quad);
            glUniform2f(centerLoc, 0.45f, 0.0f);
            draw(quad);
            break;
        default: {
            // Два квадрата в противоположных точках круга; размер
            // колеблется от 0.6 до 1.4 со сдвигом фазы на половину периода.
            const float x = kCircleRadius * std::cos(angle);
            const float y = kCircleRadius * std::sin(angle);
            const float pulse = 0.4f * std::sin(static_cast<float>(now) * 3.0f);
            glUniform1f(sizeLoc, 1.0f + pulse);
            glUniform2f(centerLoc, x, y);
            draw(quad);
            glUniform1f(sizeLoc, 1.0f - pulse);
            glUniform2f(centerLoc, -x, -y);
            draw(quad);
            break;
        }
        }

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    gl_app::finish(window);
    return 0;
}
