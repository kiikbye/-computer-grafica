#include "gl_app.hpp"

#include <iostream>
#include <string>

namespace gl_app {

namespace {

void printGlfwError(int code, const char* text) {
    std::cerr << "GLFW error " << code << ": " << text << '\n';
}

void resizeViewport(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// Возвращает id шейдера или 0, если компиляция не прошла.
GLuint compile(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(logLength > 0 ? logLength : 1, '\0');
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    std::cerr << (type == GL_VERTEX_SHADER ? "[vertex shader] " : "[fragment shader] ")
              << "compile error:\n" << log.c_str() << '\n';
    glDeleteShader(shader);
    return 0;
}

} // namespace

GLFWwindow* start(const char* title, int width, int height, bool vsync) {
    glfwSetErrorCallback(printGlfwError);
    if (!glfwInit()) {
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "GLAD: could not load OpenGL functions\n";
        finish(window);
        return nullptr;
    }

    glfwSetFramebufferSizeCallback(window, resizeViewport);
    glfwSwapInterval(vsync ? 1 : 0);

    std::cout << "GPU:    " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n'
              << "OpenGL: " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << '\n';
    return window;
}

void finish(GLFWwindow* window) {
    if (window != nullptr) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

GLuint buildProgram(const char* vsText, const char* fsText) {
    const GLuint vs = compile(GL_VERTEX_SHADER, vsText);
    const GLuint fs = compile(GL_FRAGMENT_SHADER, fsText);
    if (vs == 0 || fs == 0) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);  // код уже скопирован в программу
    glDeleteShader(fs);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength > 0 ? logLength : 1, '\0');
        glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "[program] link error:\n" << log.c_str() << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

} // namespace gl_app
