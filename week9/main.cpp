#include "gl_app.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Камера әлем координаттарында. camFront осы аптада тұрақты.
glm::vec3 camPos(0.0f, 0.0f, 3.0f);
glm::vec3 camFront(0.0f, 0.0f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 2.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        speed *= 2.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    const glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= speed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += speed * right;

    // Әлемнің Y осі: камера еңкейсе де биіктік тік бағытта өзгереді.
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camPos += speed * worldUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * worldUp;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camPos = glm::vec3(0, 0, 3);

    // P басылғанда бір рет шығару, консольді әр кадр сайын толтырмау.
    static bool pWasDown = false;
    const bool pDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pDown && !pWasDown)
        std::cout << "camPos: " << camPos.x << ", " << camPos.y << ", " << camPos.z << '\n';
    pWasDown = pDown;
}

const char* vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 color;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    color = aColor;
}
)";
const char* fragmentShader = R"(
#version 330 core
in vec3 color;
out vec4 FragColor;
void main() { FragColor = vec4(color, 1.0); }
)";

int main() {
    GLFWwindow* window = gl_app::start("Week 09 - LookAt camera | WASD Space Shift Ctrl R P", 1280, 720, true);
    if (!window) return 1;
    const GLuint program = gl_app::buildProgram(vertexShader, fragmentShader);
    if (!program) { gl_app::finish(window); return 1; }

    // Сегіз төбе: позиция және түс. Индекстер сыртынан қарағанда CCW.
    const float vertices[] = {
        -0.5f,-0.5f,-0.5f, 0.2f,0.4f,0.9f,
         0.5f,-0.5f,-0.5f, 0.2f,0.8f,0.9f,
         0.5f, 0.5f,-0.5f, 0.9f,0.8f,0.2f,
        -0.5f, 0.5f,-0.5f, 0.9f,0.3f,0.3f,
        -0.5f,-0.5f, 0.5f, 0.5f,0.3f,0.9f,
         0.5f,-0.5f, 0.5f, 0.2f,0.9f,0.5f,
         0.5f, 0.5f, 0.5f, 1.0f,0.7f,0.2f,
        -0.5f, 0.5f, 0.5f, 0.9f,0.3f,0.6f
    };
    const unsigned int indices[] = {
        0,2,1, 0,3,2, // артқы
        4,5,6, 4,6,7, // алдыңғы
        0,4,7, 0,7,3, // сол
        1,2,6, 1,6,5, // оң
        0,1,5, 0,5,4, // астыңғы
        3,7,6, 3,6,2  // үстіңгі
    };
    const glm::vec3 cubePositions[] = {
        {0.0f,0.0f,0.0f}, {-2.0f,0.0f,-1.0f}, {2.0f,0.0f,-1.5f},
        {-1.0f,1.0f,-3.0f}, {1.5f,-0.5f,-4.0f}
    };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const GLint modelLoc = glGetUniformLocation(program, "model");
    const GLint viewLoc = glGetUniformLocation(program, "view");
    const GLint projectionLoc = glGetUniformLocation(program, "projection");
    std::cout << "WASD: move | Space/Left Shift: up/down | Left Ctrl: 2x speed\n"
                 "R: reset | P: position | Esc: exit\n";
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        processInput(window, dt);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) continue;
        glViewport(0, 0, width, height);
        glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        // Әр кадрда қайта есептеледі. Мақсат нүктесі камерамен бірге жылжиды.
        const glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            static_cast<float>(width) / height, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(vao);
        for (int i = 0; i < 5; ++i) {
            // Кубтар қозғалмайды: тек камераның view матрицасы өзгереді.
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            model = glm::rotate(model, glm::radians(12.0f * i), glm::vec3(1.0f, 0.3f, 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
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
