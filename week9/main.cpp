#include "gl_app.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// 7-апта: framebuffer өлшемі aspect үшін жаңартылады.
int gWidth = 1280, gHeight = 720;
glm::vec3 camPos(0.0f, 0.0f, 3.0f);
glm::vec3 camFront(0.0f, 0.0f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);
struct Settings {
    int scene = 1; // 1: бес куб; 2: бір куб; 3: үш куб; 4: Z-fighting
    bool ortho = false, depth = true, reverse = false, paused = false;
    int culling = 1; // 0: off; 1: GL_BACK; 2: GL_FRONT
    float fov = 45, nearPlane = 0.1f, farPlane = 100;
} settings;
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    gWidth = w; gHeight = h;
    glViewport(0, 0, w, h);
}
void moveCamera(bool w, bool s, bool a, bool d, bool up, bool down, bool run, float dt) {
    float speed = 2.5f * dt;
    if (run) speed *= 2.0f;
    if (w) camPos += speed * camFront;
    if (s) camPos -= speed * camFront;
    const glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (a) camPos -= speed * right;
    if (d) camPos += speed * right;
    // Әлемнің Y осімен: камера еңкейсе де биіктік тік өзгереді.
    if (up) camPos += speed * glm::vec3(0,1,0);
    if (down) camPos -= speed * glm::vec3(0,1,0);
}
void processInput(GLFWwindow* window, float dt) {
    const auto held = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    if (held(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);
    moveCamera(held(GLFW_KEY_W), held(GLFW_KEY_S), held(GLFW_KEY_A), held(GLFW_KEY_D),
        held(GLFW_KEY_SPACE), held(GLFW_KEY_LEFT_SHIFT), held(GLFW_KEY_LEFT_CONTROL), dt);
    if (held(GLFW_KEY_R)) camPos = glm::vec3(0,0,3);
    static std::array<bool, GLFW_KEY_LAST + 1> previous{};
    const auto pressed = [&](int key) {
        bool now = held(key), was = previous[key]; previous[key] = now; return now && !was;
    };
    if (pressed(GLFW_KEY_P)) std::cout << "camPos: " << camPos.x << ", " << camPos.y << ", " << camPos.z << '\n';
    bool changed = false;
    for (int i = 0; i < 4; ++i) if (pressed(GLFW_KEY_1 + i)) {
        settings = Settings{}; settings.scene = i + 1;
        if (settings.scene == 3) settings.fov = 60; // Үш куб кадрға толық сыяды.
        camPos = glm::vec3(0,0,3); changed = true;
    }
    if (pressed(GLFW_KEY_O)) { settings.ortho = !settings.ortho; changed = true; }
    if (pressed(GLFW_KEY_B)) { settings.depth = !settings.depth; changed = true; }
    if (pressed(GLFW_KEY_C)) { settings.culling = (settings.culling + 1) % 3; changed = true; }
    if (pressed(GLFW_KEY_T)) { settings.reverse = !settings.reverse; changed = true; }
    if (pressed(GLFW_KEY_F)) { settings.paused = !settings.paused; changed = true; }
    const float fovs[] = {30,45,90,120};
    for (int i = 0; i < 4; ++i) if (pressed(GLFW_KEY_F1 + i)) { settings.fov = fovs[i]; changed = true; }
    if (pressed(GLFW_KEY_F5)) {
        const float v[] = {0.1f,1,2.5f,4}; int next = 0;
        for (int i = 0; i < 4; ++i) if (settings.nearPlane == v[i]) next = (i + 1) % 4;
        settings.nearPlane = v[next]; changed = true;
    }
    if (pressed(GLFW_KEY_F6)) {
        const float v[] = {100,5,2}; int next = 0;
        for (int i = 0; i < 3; ++i) if (settings.farPlane == v[i]) next = (i + 1) % 3;
        settings.farPlane = v[next]; changed = true;
    }
    if (pressed(GLFW_KEY_Z)) {
        settings.scene = 4; settings.ortho = false; settings.depth = true;
        settings.nearPlane = 0.0001f; settings.farPlane = 1000; changed = true;
    }
    if (pressed(GLFW_KEY_X)) { settings.nearPlane = 0.1f; settings.farPlane = 100; changed = true; }
    if (changed) {
        const std::string title = "Week 09 | scene=" + std::to_string(settings.scene) +
            (settings.ortho ? " ortho" : " perspective") + " fov=" + std::to_string(settings.fov) +
            " near=" + std::to_string(settings.nearPlane) + " far=" + std::to_string(settings.farPlane) +
            " depth=" + std::to_string(settings.depth) + " cull=" + std::to_string(settings.culling);
        glfwSetWindowTitle(window, title.c_str()); std::cout << title << '\n';
    }
}
const char* vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTint;
out vec3 vColor;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = mix(aColor, vec3(1.0) - aColor, uTint);
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
struct Uniforms { GLint model, view, projection, tint; };
void render(GLuint program, GLuint vao, const Uniforms& loc, float angle) {
    // Қалыпты күй main-де циклге дейін орнатылады. Тек диагностикалық ауыстыруды өңдеу.
    static int oldDepth = 1, oldCulling = 1;
    if (oldDepth != settings.depth) {
        if (settings.depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        oldDepth = settings.depth;
    }
    if (oldCulling != settings.culling) {
        if (!settings.culling) glDisable(GL_CULL_FACE);
        else { glEnable(GL_CULL_FACE); glCullFace(settings.culling == 1 ? GL_BACK : GL_FRONT); }
        oldCulling = settings.culling;
    }
    glClearColor(0.06f,0.08f,0.12f,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // near >= far болса, жарамсыз frustum құрмаймыз.
    if (gWidth <= 0 || gHeight <= 0 || settings.nearPlane >= settings.farPlane) return;
    const float aspect = static_cast<float>(gWidth) / gHeight;
    const glm::mat4 projection = settings.ortho
        ? glm::ortho(-aspect,aspect,-1.0f,1.0f,settings.nearPlane,settings.farPlane)
        : glm::perspective(glm::radians(settings.fov),aspect,settings.nearPlane,settings.farPlane);
    const glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
    glUseProgram(program);
    glUniformMatrix4fv(loc.projection,1,GL_FALSE,glm::value_ptr(projection));
    glUniformMatrix4fv(loc.view,1,GL_FALSE,glm::value_ptr(view));
    glBindVertexArray(vao);
    const glm::vec3 three[] = {{-1.5f,0,0},{0,0,-1},{1.5f,0,0}};
    const int count = settings.scene == 1 ? 5 : settings.scene == 3 ? 3 : settings.scene == 4 ? 2 : 1;
    for (int n = 0; n < count; ++n) {
        const int i = settings.reverse ? count - 1 - n : n;
        glm::vec3 position(0);
        if (settings.scene == 1) position = positions[i];
        if (settings.scene == 3) position = three[i];
        if (settings.scene == 4) position.z = i * 0.001f;
        // Әр объект үшін жаңа model: түрлендірулер үстемеленбейді.
        glm::mat4 model = glm::translate(glm::mat4(1),position);
        const glm::vec3 axis = settings.scene == 1 || settings.scene == 4
            ? glm::vec3(0.3f,1,0) : glm::vec3(0,1,0);
        const float rate = settings.scene == 4 ? 1.0f : 0.5f + i * 0.3f;
        model = glm::rotate(model,angle * rate,axis);
        glUniformMatrix4fv(loc.model,1,GL_FALSE,glm::value_ptr(model));
        glUniform1f(loc.tint,settings.scene == 4 && i == 1 ? 1.0f : 0.0f);
        glDrawArrays(GL_TRIANGLES,0,36);
    }
}
// Нақты OpenGL framebuffer-ден есепке арналған бірдей кадрларды сақтау.
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
bool captureExamples(GLuint program, GLuint vao, const Uniforms& loc, const std::string& dir) {
    std::filesystem::create_directories(dir);
    const auto capture = [&](const std::string& name, float angle = 0.9f) {
        render(program,vao,loc,angle);
        bool ok = saveFrame(std::filesystem::path(dir) / (name + ".ppm"));
        std::cout << (ok ? "Saved: " : "FAILED: ") << name << '\n'; return ok;
    };
    bool ok = true;
    settings = Settings{}; ok &= capture("01-five-cubes");
    settings.culling = 0; ok &= capture("11-depth-on");
    settings.depth = false; ok &= capture("02-depth-off");
    settings.reverse = true; ok &= capture("03-depth-off-reverse");
    settings = Settings{}; settings.scene = 2; ok &= capture("04-perspective");
    settings.ortho = true; ok &= capture("05-orthographic");
    settings = Settings{}; settings.scene = 3; settings.fov = 60; ok &= capture("06-three-cubes");
    settings = Settings{}; settings.scene = 4; settings.nearPlane = 0.0001f; settings.farPlane = 1000;
    ok &= capture("07-zfighting");
    settings.nearPlane = 0.1f; settings.farPlane = 100; ok &= capture("08-zfighting-fixed");
    settings = Settings{}; settings.culling = 2; ok &= capture("09-cull-front");
    settings = Settings{}; camPos.x = -1; ok &= capture("10-camera-left");
    camPos = glm::vec3(0,0,3); settings.scene = 2;
    for (float fov : {30.0f,45.0f,90.0f,120.0f}) {
        settings.fov = fov; ok &= capture("fov-" + std::to_string(static_cast<int>(fov)),0);
    }
    settings.fov = 45;
    for (float nearPlane : {0.1f,1.0f,2.5f,4.0f}) {
        settings.nearPlane = nearPlane; ok &= capture("near-" + std::to_string(nearPlane),0);
    }
    settings.nearPlane = 0.1f;
    for (float farPlane : {100.0f,5.0f,2.0f}) {
        settings.farPlane = farPlane; ok &= capture("far-" + std::to_string(static_cast<int>(farPlane)),0);
    }
    return ok;
}
int main(int argc, char** argv) {
    const bool capture = argc == 3 && std::string(argv[1]) == "--capture";
    GLFWwindow* window = gl_app::start("Week 09 | 7: MVP -> 8: depth/culling -> 9: camera",gWidth,gHeight,!capture);
    if (!window) return 1;
    if (capture) glfwHideWindow(window);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
    glfwGetFramebufferSize(window,&gWidth,&gHeight);
    framebuffer_size_callback(window,gWidth,gHeight);
    GLuint program = gl_app::buildProgram(vertexShader,fragmentShader);
    if (!program) { gl_app::finish(window); return 1; }
    GLuint vao,vbo;
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo); glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6 * sizeof(float),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6 * sizeof(float),reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);
    // Uniform орындары циклге дейін бір рет кэштеледі.
    const Uniforms loc{glGetUniformLocation(program,"uModel"),glGetUniformLocation(program,"uView"),
        glGetUniformLocation(program,"uProjection"),glGetUniformLocation(program,"uTint")};
    int depthBits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_DEPTH,GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,&depthBits);
    std::cout << "Depth bits: " << depthBits << '\n'
        << "WASD: move | Space/Left Shift: up/down | Left Ctrl: 2x | R: reset | P: position\n"
        << "1: five cubes | 2: single | 3: three | 4: overlapping\n"
        << "O: projection | F1-F4: FOV | F5: near | F6: far | F: pause\n"
        << "B: depth | C: culling | T: draw order | Z: Z-fighting | X: fix | Esc: exit\n";
    bool ok = depthBits > 0;
    if (capture) ok &= captureExamples(program,vao,loc,argv[2]);
    else {
        double lastTime = glfwGetTime(); float angle = 0;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents(); const double now = glfwGetTime();
            float dt = static_cast<float>(now - lastTime); lastTime = now;
            if (gWidth <= 0 || gHeight <= 0) { glfwWaitEventsTimeout(0.05); continue; }
            processInput(window,dt);
            if (!settings.paused) angle += dt;
            render(program,vao,loc,angle); glfwSwapBuffers(window);
        }
    }
    glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao); glDeleteProgram(program);
    gl_app::finish(window); return ok ? 0 : 1;
}
