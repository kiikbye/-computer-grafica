#include "gl_app.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
int gWidth = 1280, gHeight = 720;
const glm::vec3 startPos(0.0f, 0.0f, 3.0f);
const glm::vec3 camUp(0.0f, 1.0f, 0.0f);
glm::vec3 camPos = startPos;
glm::vec3 camFront(0.0f, 0.0f, -1.0f);
float yaw = -90.0f, pitch = 0.0f, fov = 45.0f;
float sensitivity = 0.1f;
constexpr std::array<float, 3> sensitivities{0.03f, 0.1f, 0.5f};
double lastX = 0.0, lastY = 0.0;
bool firstMouse = true, invertY = false;

glm::vec3 directionFromAngles() {
    const float yawRad = glm::radians(yaw);
    const float pitchRad = glm::radians(pitch);
    return glm::normalize(glm::vec3(
        std::cos(yawRad) * std::cos(pitchRad),
        std::sin(pitchRad),
        std::sin(yawRad) * std::cos(pitchRad)));
}

void resetCamera() {
    camPos = startPos;
    camFront = glm::vec3(0.0f, 0.0f, -1.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    fov = 45.0f;
    firstMouse = true;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }
    const float xoffset = static_cast<float>(xpos - lastX);
    const float yoffset = static_cast<float>(lastY - ypos);
    lastX = xpos;
    lastY = ypos;
    yaw = std::remainder(yaw + xoffset * sensitivity, 360.0f);
    pitch = std::clamp(pitch + (invertY ? -yoffset : yoffset) * sensitivity,
                       -89.0f, 89.0f);
    camFront = directionFromAngles();
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    fov = std::clamp(fov - static_cast<float>(yoffset), 1.0f, 90.0f);
}

void printCamera() {
    std::printf("camPos   = (%.2f, %.2f, %.2f)\n"
                "camFront = (%.2f, %.2f, %.2f)\n"
                "yaw = %.1f   pitch = %.1f   fov = %.1f\n"
                "length = %.6f\n",
                camPos.x, camPos.y, camPos.z, camFront.x, camFront.y,
                camFront.z, yaw, pitch, fov, glm::length(camFront));
}

void processInput(GLFWwindow* window, gl_app::Keyboard& keys, float dt) {
    if (keys.held(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

    const float speed = (keys.held(GLFW_KEY_LEFT_CONTROL) ? 5.0f : 2.5f) * dt;
    const glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (keys.held(GLFW_KEY_W)) camPos += camFront * speed;
    if (keys.held(GLFW_KEY_S)) camPos -= camFront * speed;
    if (keys.held(GLFW_KEY_A)) camPos -= right * speed;
    if (keys.held(GLFW_KEY_D)) camPos += right * speed;
    if (keys.held(GLFW_KEY_SPACE)) camPos += camUp * speed;
    if (keys.held(GLFW_KEY_LEFT_SHIFT)) camPos -= camUp * speed;

    if (keys.pressed(GLFW_KEY_I)) {
        invertY = !invertY;
        std::cout << "Invert Y: " << (invertY ? "on" : "off") << '\n';
    }
    if (keys.pressed(GLFW_KEY_R)) resetCamera();
    if (keys.pressed(GLFW_KEY_P)) printCamera();
    for (std::size_t i = 0; i < sensitivities.size(); ++i) {
        if (keys.pressed(GLFW_KEY_1 + static_cast<int>(i))) {
            sensitivity = sensitivities[i];
            std::cout << "Sensitivity: " << sensitivity << '\n';
        }
    }
}


const char* vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
)";
const char* fragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";
// 7-апта: 36 вершина, position + color. Әр қырдың өз түсі.
// Барлық үшбұрыштар сыртынан қарағанда CCW. EBO қажет емес.
const float vertices[] = {
    -0.5f,-0.5f,-0.5f, 1,0,0,  0.5f,0.5f,-0.5f, 1,0,0,  0.5f,-0.5f,-0.5f, 1,0,0,
    -0.5f,-0.5f,-0.5f, 1,0,0, -0.5f,0.5f,-0.5f, 1,0,0,  0.5f,0.5f,-0.5f, 1,0,0,
    -0.5f,-0.5f,0.5f, 0,1,0,  0.5f,-0.5f,0.5f, 0,1,0,  0.5f,0.5f,0.5f, 0,1,0,
    -0.5f,-0.5f,0.5f, 0,1,0,  0.5f,0.5f,0.5f, 0,1,0, -0.5f,0.5f,0.5f, 0,1,0,
    -0.5f,-0.5f,-0.5f, 0,0,1, -0.5f,-0.5f,0.5f, 0,0,1, -0.5f,0.5f,0.5f, 0,0,1,
    -0.5f,-0.5f,-0.5f, 0,0,1, -0.5f,0.5f,0.5f, 0,0,1, -0.5f,0.5f,-0.5f, 0,0,1,
     0.5f,-0.5f,-0.5f, 1,1,0,  0.5f,0.5f,-0.5f, 1,1,0,  0.5f,0.5f,0.5f, 1,1,0,
     0.5f,-0.5f,-0.5f, 1,1,0,  0.5f,0.5f,0.5f, 1,1,0,  0.5f,-0.5f,0.5f, 1,1,0,
    -0.5f,-0.5f,-0.5f, 1,0,1,  0.5f,-0.5f,-0.5f, 1,0,1,  0.5f,-0.5f,0.5f, 1,0,1,
    -0.5f,-0.5f,-0.5f, 1,0,1,  0.5f,-0.5f,0.5f, 1,0,1, -0.5f,-0.5f,0.5f, 1,0,1,
    -0.5f,0.5f,-0.5f, 0,1,1, -0.5f,0.5f,0.5f, 0,1,1,  0.5f,0.5f,0.5f, 0,1,1,
    -0.5f,0.5f,-0.5f, 0,1,1,  0.5f,0.5f,0.5f, 0,1,1,  0.5f,0.5f,-0.5f, 0,1,1
};
static_assert(sizeof(vertices) / sizeof(float) == 36 * 6);
// 8-аптадағы позициялар өзгеріссіз.
const glm::vec3 positions[] = {
    {0,0,0}, {2,0.5f,-2}, {-1.8f,-0.3f,-1}, {1.2f,-0.8f,-3.5f}, {-2.2f,1,-4}
};
struct Uniforms { GLint model, view, projection; };

void render(GLuint program, GLuint vao, const Uniforms& loc, float angle) {
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (gWidth <= 0 || gHeight <= 0) return;

    const float aspect = static_cast<float>(gWidth) / gHeight;
    const glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
    const glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
    // Exact-pole capture: leave the cleared frame when LookAt has no right vector.
    if (!std::isfinite(view[0][0])) return;

    glUseProgram(program);
    glUniformMatrix4fv(loc.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(loc.view, 1, GL_FALSE, glm::value_ptr(view));
    glBindVertexArray(vao);
    for (std::size_t i = 0; i < std::size(positions); ++i) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i]);
        model = glm::rotate(model, angle * (0.5f + 0.3f * i), glm::vec3(0.3f, 1.0f, 0.0f));
        glUniformMatrix4fv(loc.model, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}


bool saveFrame(const std::filesystem::path& path) {
    std::vector<unsigned char> pixels(gWidth * gHeight * 3);
    glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(0,0,gWidth,gHeight,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream file(path,std::ios::binary);
    file << "P6\n" << gWidth << ' ' << gHeight << "\n255\n";
    for (int y = gHeight - 1; y >= 0; --y)
        file.write(reinterpret_cast<char*>(pixels.data() + y * gWidth * 3),gWidth * 3);
    return file.good() && glGetError() == GL_NO_ERROR;
}
bool captureExamples(GLuint program, GLuint vao, const Uniforms& loc,
                     const std::filesystem::path& dir) {
    std::filesystem::create_directories(dir);
    const auto capture = [&](const std::string& name) {
        render(program, vao, loc, 0.9f);
        const bool saved = saveFrame(dir / (name + ".ppm"));
        std::cout << (saved ? "Saved " : "Failed ") << name << '\n';
        return saved;
    };

    bool ok = capture("01-default");
    camPos.y = -1.2f;
    mouse_callback(nullptr, 500.0, 400.0);
    ok &= pitch == 0.0f && yaw == -90.0f;
    mouse_callback(nullptr, 500.0, 180.0);
    ok &= pitch == 22.0f && camFront.y > 0.0f;
    ok &= capture("02-looking-up");

    resetCamera();
    scroll_callback(nullptr, 0.0, 20.0);
    ok &= fov == 25.0f;
    ok &= capture("03-zoomed");
    scroll_callback(nullptr, 0.0, -1000.0);
    ok &= fov == 90.0f;
    scroll_callback(nullptr, 0.0, 1000.0);
    ok &= fov == 1.0f;

    resetCamera();
    mouse_callback(nullptr, 0.0, 0.0);
    mouse_callback(nullptr, 0.0, -900.0);
    ok &= pitch == 89.0f && glm::length(glm::cross(camFront, camUp)) > 0.0f;
    ok &= std::abs(glm::length(camFront) - 1.0f) < 0.00001f;
    resetCamera();
    invertY = true;
    mouse_callback(nullptr, 0.0, 0.0);
    mouse_callback(nullptr, 0.0, -100.0);
    ok &= pitch == -10.0f;
    invertY = false;

    for (float value : sensitivities) {
        resetCamera();
        sensitivity = value;
        mouse_callback(nullptr, 0.0, 0.0);
        mouse_callback(nullptr, 100.0, 0.0);
        ok &= std::abs(yaw - (-90.0f + 100.0f * value)) < 0.00001f;
        std::cout << "Sensitivity " << value << ": 360 degrees = " << 360.0f / value
                  << " cursor units\n";
    }
    sensitivity = 0.1f;

    resetCamera();
    pitch = 90.0f;
    camFront = directionFromAngles();
    std::cout << "Unclamped pitch 90: cross length = "
              << glm::length(glm::cross(camFront, camUp)) << '\n';
    // At the exact mathematical pole, cos(90°) = 0. Float trig rounds it.
    camFront = camUp;
    const glm::mat4 brokenView = glm::lookAt(camPos, camPos + camFront, camUp);
    std::cout << "Exact pole pitch 90: view[0][0] = " << brokenView[0][0] << '\n';
    ok &= !std::isfinite(brokenView[0][0]);
    ok &= capture("04-broken");
    std::cout << (ok ? "Camera checks passed\n" : "Camera checks failed\n");
    return ok;
}
} // namespace

int main(int argc, char** argv) {
    const bool capture = argc == 3 && std::string(argv[1]) == "--capture";
    GLFWwindow* window = gl_app::start("Week 10 | Free camera", gWidth, gHeight, !capture);
    if (!window) return 1;
    if (capture) glfwHideWindow(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    framebuffer_size_callback(window, gWidth, gHeight);

    const GLuint program = gl_app::buildProgram(vertexShader, fragmentShader);
    if (!program) {
        gl_app::finish(window);
        return 1;
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    const Uniforms loc{glGetUniformLocation(program, "uModel"),
                       glGetUniformLocation(program, "uView"),
                       glGetUniformLocation(program, "uProjection")};

    bool ok = true;
    if (capture) {
        ok = captureExamples(program, vao, loc, argv[2]);
    } else {
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetWindowFocusCallback(window, [](GLFWwindow*, int) { firstMouse = true; });
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        gl_app::Keyboard keys(window);
        std::cout << "Mouse: look | Wheel: zoom | I: invert Y | P: camera state\n"
                  << "1/2/3: sensitivity | R: reset | WASD: move | Esc: exit\n"
                  << "Space/Left Shift: up/down | Left Ctrl: double speed\n";
        double previousTime = glfwGetTime();
        float angle = 0.0f;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            const double now = glfwGetTime();
            const float dt = static_cast<float>(now - previousTime);
            previousTime = now;
            if (gWidth <= 0 || gHeight <= 0) {
                glfwWaitEventsTimeout(0.05);
                continue;
            }
            processInput(window, keys, dt);
            char title[128];
            std::snprintf(title, sizeof(title), "Week 10 | yaw %.1f | pitch %.1f | FOV %.1f | sensitivity %.2f%s",
                          yaw, pitch, fov, sensitivity, invertY ? " | Invert Y" : "");
            glfwSetWindowTitle(window, title);
            angle += dt;
            render(program, vao, loc, angle);
            glfwSwapBuffers(window);
        }
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    gl_app::finish(window);
    return ok ? 0 : 1;
}
