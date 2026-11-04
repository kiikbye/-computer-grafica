// Coursework keeps its implementation in main.cpp; reuse it without starting GLFW.
#define main week9_application_main
#include "../main.cpp"
#undef main
#include <cmath>

int main() {
    int failures = 0;
    const auto check = [&](bool ok, const char* name) {
        std::cout << (ok ? "PASS " : "FAIL ") << name << '\n';
        if (!ok) ++failures;
    };
    const auto close = [](const glm::vec3& a, const glm::vec3& b) {
        return glm::length(a - b) < 0.0001f;
    };
    // One second of movement must be independent of the number of frames.
    glm::vec3 results[2];
    const int rates[] = {30,144};
    for (int j = 0; j < 2; ++j) {
        camPos = glm::vec3(0,0,3);
        for (int i = 0; i < rates[j]; ++i)
            moveCamera(true,false,false,false,false,false,false,1.0f / rates[j]);
        results[j] = camPos;
    }
    check(close(results[0],results[1]) && close(results[0],glm::vec3(0,0,0.5f)),"30 vs 144 FPS");
    camPos = glm::vec3(0,0,3);
    moveCamera(true,false,false,false,false,false,true,1);
    check(close(camPos,glm::vec3(0,0,-2)),"Ctrl doubles distance");
    camPos = glm::vec3(0);
    moveCamera(false,false,true,false,false,false,false,1);
    check(close(camPos,glm::vec3(-2.5f,0,0)),"A moves left");
    moveCamera(false,false,false,true,false,false,false,1);
    moveCamera(false,false,false,false,true,false,false,1);
    check(close(camPos,glm::vec3(0,2.5f,0)),"D and Space");
    moveCamera(false,false,false,false,false,true,false,1);
    moveCamera(false,true,false,false,false,false,false,1);
    check(close(camPos,glm::vec3(0,0,2.5f)),"Shift and S");
    camFront = glm::normalize(glm::vec3(0,0.8f,-0.6f));
    camPos = glm::vec3(0);
    moveCamera(false,false,false,true,false,false,false,1);
    check(std::abs(glm::length(camPos) - 2.5f) < 0.0001f,"Normalized strafe when tilted");
    camFront = glm::vec3(0,0,-1);
    const auto v1 = glm::lookAt(glm::vec3(0,0,3),glm::vec3(0,0,2),camUp);
    const auto v2 = glm::lookAt(glm::vec3(-1,0,3),glm::vec3(-1,0,2),camUp);
    check(close(glm::vec3(v1[0]),glm::vec3(v2[0])) &&
          close(glm::vec3(v1[2]),glm::vec3(v2[2])),"Translation preserves camera direction");
    bool winding = true, faceColors = true;
    for (int t = 0; t < 12; ++t) {
        const auto position = [&](int i) { const float* p = vertices + (t * 3 + i) * 6; return glm::vec3(p[0],p[1],p[2]); };
        const auto a = position(0), b = position(1), c = position(2);
        winding &= glm::dot(glm::cross(b-a,c-a),(a+b+c) / 3.0f) > 0;
    }
    for (int face = 0; face < 6; ++face) {
        const float* base = vertices + face * 36 + 3;
        for (int i = 1; i < 6; ++i) {
            const float* color = base + i * 6;
            faceColors &= color[0] == base[0] && color[1] == base[1] && color[2] == base[2];
        }
    }
    check(winding,"All 12 triangles face outward CCW");
    check(faceColors,"Each face has a uniform color");
    check(close(positions[4],glm::vec3(-2.2f,1,-4)),"Week 8 scene positions");
    return failures ? 1 : 0;
}
