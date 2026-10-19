
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>

static const char* VS = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uTransform;
out vec3 vColor;
void main() {
    gl_Position = uTransform * vec4(aPos, 1.0);
    vColor = aColor;
})";

static const char* FS = R"(#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); })";

static int mode = 1;      // 1: T*R, 2: R*T, 3: бұрыштан айналу, 4: үш объект
static float speed = 1.0f;

static unsigned compile(GLenum type, const char* src) {
    unsigned s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::printf("Shader error: %s\n", log); }
    return s;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(800, 800, "Week 6 - Transformation Matrices", nullptr, nullptr);
    if (!win) { std::printf("Window error\n"); return -1; }
    glfwMakeContextCurrent(win);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // Shader program
    unsigned vs = compile(GL_VERTEX_SHADER, VS);
    unsigned fs = compile(GL_FRAGMENT_SHADER, FS);
    unsigned shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Төртбұрыш: позиция (x,y,z) + түс (r,g,b), жартылай өлшемі 0.5
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  1.0f, 0.2f, 0.2f,
         0.5f, -0.5f, 0.0f,  0.2f, 1.0f, 0.2f,
         0.5f,  0.5f, 0.0f,  0.2f, 0.4f, 1.0f,
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.2f,
    };
    unsigned indices[] = { 0, 1, 2, 2, 3, 0 };

    unsigned vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Uniform орнын циклге дейін бір рет кэштейміз
    int locTransform = glGetUniformLocation(shader, "uTransform");

    float angle = 0.0f;
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);
        if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) mode = 1;
        if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) mode = 2;
        if (glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS) mode = 3;
        if (glfwGetKey(win, GLFW_KEY_4) == GLFW_PRESS) mode = 4;

        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) speed += 1.5f * dt;
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) speed -= 1.5f * dt;
        angle += speed * dt;   // радиан

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        glBindVertexArray(vao);

        const glm::vec3 axisZ(0.0f, 0.0f, 1.0f);
        glm::vec3 orbit(std::cos(angle) * 0.4f, std::sin(angle) * 0.4f, 0.0f);

        if (mode == 4) {
            // 5.3: бір геометрия, үш матрица
            for (int i = 0; i < 3; i++) {
                glm::mat4 m = glm::mat4(1.0f);   // цикл ІШІНДЕ
                m = glm::translate(m, glm::vec3(-0.5f + i * 0.5f, 0.0f, 0.0f));
                m = glm::rotate(m, angle * (i + 1), axisZ);
                m = glm::scale(m, glm::vec3(0.4f, 0.4f, 1.0f));
                glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m));
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        } else {
            glm::mat4 m = glm::mat4(1.0f);       // цикл ІШІНДЕ
            if (mode == 1) {                     // T * R
                m = glm::translate(m, orbit);
                m = glm::rotate(m, angle, axisZ);
                m = glm::scale(m, glm::vec3(0.6f, 0.6f, 1.0f));
            } else if (mode == 2) {              // R * T
                m = glm::rotate(m, angle, axisZ);
                m = glm::translate(m, orbit);
                m = glm::scale(m, glm::vec3(0.6f, 0.6f, 1.0f));
            } else {                             // сол төменгі бұрыштан айналу
                m = glm::translate(m, glm::vec3(-0.3f, -0.3f, 0.0f)); // 3-ші: кері қайтару
                m = glm::rotate(m, angle, axisZ);                     // 2-ші: бұру
                m = glm::translate(m, glm::vec3( 0.3f,  0.3f, 0.0f)); // 1-ші: бұрышты басына апару
                m = glm::scale(m, glm::vec3(0.6f, 0.6f, 1.0f));       // ең алдымен: өлшем
            }
            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(m));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        glfwSwapBuffers(win);
    }
    glfwTerminate();
    return 0;
}
