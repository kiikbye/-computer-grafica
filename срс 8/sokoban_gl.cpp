// Басқару: Бағыттауыш пернелер немесе WASD — жүру, U — жүрісті қайтару, R — деңгейді қайта бастау, N — келесі деңгей, ESC — шығу


#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>

using namespace std;

const int T = 32;   // Ұяшық өлшемі — пиксельмен


#define GLFN(type, name) static type p##name = nullptr;
GLFN(PFNGLCREATESHADERPROC,            glCreateShader)
GLFN(PFNGLSHADERSOURCEPROC,            glShaderSource)
GLFN(PFNGLCOMPILESHADERPROC,           glCompileShader)
GLFN(PFNGLGETSHADERIVPROC,             glGetShaderiv)
GLFN(PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog)
GLFN(PFNGLDELETESHADERPROC,            glDeleteShader)
GLFN(PFNGLCREATEPROGRAMPROC,           glCreateProgram)
GLFN(PFNGLATTACHSHADERPROC,            glAttachShader)
GLFN(PFNGLLINKPROGRAMPROC,             glLinkProgram)
GLFN(PFNGLGETPROGRAMIVPROC,            glGetProgramiv)
GLFN(PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog)
GLFN(PFNGLUSEPROGRAMPROC,              glUseProgram)
GLFN(PFNGLDELETEPROGRAMPROC,           glDeleteProgram)
GLFN(PFNGLGETUNIFORMLOCATIONPROC,      glGetUniformLocation)
GLFN(PFNGLUNIFORM2FPROC,               glUniform2f)
GLFN(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays)
GLFN(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray)
GLFN(PFNGLDELETEVERTEXARRAYSPROC,      glDeleteVertexArrays)
GLFN(PFNGLGENBUFFERSPROC,              glGenBuffers)
GLFN(PFNGLBINDBUFFERPROC,              glBindBuffer)
GLFN(PFNGLBUFFERDATAPROC,              glBufferData)
GLFN(PFNGLDELETEBUFFERSPROC,           glDeleteBuffers)
GLFN(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer)
GLFN(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)

static bool loadGL() {
#define L(type, name) p##name = (type)SDL_GL_GetProcAddress(#name); if (!p##name) return false;
    L(PFNGLCREATESHADERPROC,            glCreateShader)
    L(PFNGLSHADERSOURCEPROC,            glShaderSource)
    L(PFNGLCOMPILESHADERPROC,           glCompileShader)
    L(PFNGLGETSHADERIVPROC,             glGetShaderiv)
    L(PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog)
    L(PFNGLDELETESHADERPROC,            glDeleteShader)
    L(PFNGLCREATEPROGRAMPROC,           glCreateProgram)
    L(PFNGLATTACHSHADERPROC,            glAttachShader)
    L(PFNGLLINKPROGRAMPROC,             glLinkProgram)
    L(PFNGLGETPROGRAMIVPROC,            glGetProgramiv)
    L(PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog)
    L(PFNGLUSEPROGRAMPROC,              glUseProgram)
    L(PFNGLDELETEPROGRAMPROC,           glDeleteProgram)
    L(PFNGLGETUNIFORMLOCATIONPROC,      glGetUniformLocation)
    L(PFNGLUNIFORM2FPROC,               glUniform2f)
    L(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays)
    L(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray)
    L(PFNGLDELETEVERTEXARRAYSPROC,      glDeleteVertexArrays)
    L(PFNGLGENBUFFERSPROC,              glGenBuffers)
    L(PFNGLBINDBUFFERPROC,              glBindBuffer)
    L(PFNGLBUFFERDATAPROC,              glBufferData)
    L(PFNGLDELETEBUFFERSPROC,           glDeleteBuffers)
    L(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer)
    L(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)
    return true;
}


// Shaders (GLSL 3.30 core)

static const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec2 aPos;     // position in pixels
layout(location = 1) in vec3 aColor;   // per-vertex color
uniform vec2 uRes;                     // logical size of the level in pixels
out vec3 vColor;
void main() {
    vec2 ndc = vec2(aPos.x / uRes.x * 2.0 - 1.0,
                    1.0 - aPos.y / uRes.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = aColor;
}
)";

// Fragment Shader: объектінің түсін шығарады.

static const char* FRAG_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = pglCreateShader(type);
    pglShaderSource(s, 1, &src, nullptr);
    pglCompileShader(s);
    GLint ok = 0;
    pglGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        pglGetShaderInfoLog(s, sizeof(log), nullptr, log);
        fprintf(stderr, "Shader compile error:\n%s\n", log);
    }
    return s;
}

static GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint prog = pglCreateProgram();
    pglAttachShader(prog, vs);
    pglAttachShader(prog, fs);
    pglLinkProgram(prog);
    GLint ok = 0;
    pglGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        pglGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
    }
    pglDeleteShader(vs);
    pglDeleteShader(fs);
    return prog;
}

// ---------------------------------------------------------------------
// 3. Game data
// ---------------------------------------------------------------------
struct Col { float r, g, b; };
const Col LIGHT = {199 / 255.f, 240 / 255.f, 216 / 255.f};    // Nokia экранының түстері
const Col DARK  = { 67 / 255.f,  82 / 255.f,  61 / 255.f};

// Белгілер: # — қабырға, @ — ойыншы, $ — қорап, . — мақсат орны, * — мақсаттағы қорап, + — мақсат орнындағы ойыншы
const vector<vector<string>> LEVELS = {
    {
        "######",
        "#    #",
        "# $$ #",
        "# .. #",
        "# @  #",
        "######"
    },
    {
        "    #####",
        "    #   #",
        "    #$  #",
        "  ###  $##",
        "  #  $ $ #",
        "### # ## #   ######",
        "#   # ## #####  ..#",
        "# $  $          ..#",
        "##### ### #@##  ..#",
        "    #     #########",
        "    #######"
    },
    {
        "############",
        "#..  #     ###",
        "#..  # $  $  #",
        "#..  #$####  #",
        "#..    @ ##  #",
        "#..  # #  $ ##",
        "###### ##$ $ #",
        "  # $  $ $ $ #",
        "  #    #     #",
        "  ############"
    }
};

struct State {
    vector<string> grid;
    int px, py, moves;
};

static SDL_Window*   win = nullptr;
static SDL_GLContext ctx = nullptr;

static State cur;
static vector<State> history;
static vector<string> goals;
static int level = 0;
static bool won = false;
static bool allDone = false;
static int logicalW = 0, logicalH = 0;   


static GLuint prog = 0, vao = 0, vbo = 0;
static GLint  uResLoc = -1;


struct Vertex { float x, y, r, g, b; };
static vector<Vertex> batch;            


static void rect(float x, float y, float w, float h, Col c) {
    Vertex a = {x,     y,     c.r, c.g, c.b};
    Vertex b = {x + w, y,     c.r, c.g, c.b};
    Vertex d = {x + w, y + h, c.r, c.g, c.b};
    Vertex e = {x,     y + h, c.r, c.g, c.b};
    batch.push_back(a); batch.push_back(b); batch.push_back(d);   // triangle 1
    batch.push_back(a); batch.push_back(d); batch.push_back(e);   // triangle 2
}

static void drawWall(int x, int y) {
    rect(x, y, T, T, DARK);
    rect(x, y + 10, T, 2, LIGHT);        // brick lines
    rect(x, y + 21, T, 2, LIGHT);
    rect(x + 16, y, 2, 10, LIGHT);
    rect(x + 8,  y + 12, 2, 9, LIGHT);
    rect(x + 24, y + 12, 2, 9, LIGHT);
    rect(x + 16, y + 23, 2, 9, LIGHT);
}

static void drawGoal(int x, int y) {
    rect(x + 12, y + 12, 8, 8, DARK);
    rect(x + 14, y + 14, 4, 4, LIGHT);
}

static void drawBox(int x, int y, bool onGoal) {
    rect(x + 3, y + 3, T - 6, T - 6, DARK);
    if (onGoal) {
        rect(x + 12, y + 12, 8, 8, LIGHT);
    } else {
        rect(x + 6, y + 6, T - 12, T - 12, LIGHT);
        for (int i = 6; i < T - 6; i++) {          
            rect(x + i,         y + i, 2, 1, DARK);
            rect(x + T - 2 - i, y + i, 2, 1, DARK);
        }
    }
}

static void drawPlayer(int x, int y) {
    rect(x + 11, y + 4,  10, 9, DARK);    // head
    rect(x + 13, y + 7,  2,  2, LIGHT);   // eyes
    rect(x + 17, y + 7,  2,  2, LIGHT);
    rect(x + 9,  y + 14, 14, 9, DARK);    // body
    rect(x + 5,  y + 15, 4,  7, DARK);    // arms
    rect(x + 23, y + 15, 4,  7, DARK);
    rect(x + 10, y + 23, 5,  6, DARK);    // legs
    rect(x + 17, y + 23, 5,  6, DARK);
}

static void render() {
   // осы кадрға арналған төбелер тізімін дайындау
    batch.clear();
    for (size_t y = 0; y < cur.grid.size(); y++) {
        for (size_t x = 0; x < cur.grid[y].size(); x++) {
            int sx = (int)x * T, sy = (int)y * T;
            char c = cur.grid[y][x];
            bool goal = goals[y][x] == '.';
            if (c == '#') drawWall(sx, sy);
            else {
                if (goal) drawGoal(sx, sy);
                if (c == '$') drawBox(sx, sy, goal);
            }
        }
    }
    drawPlayer(cur.px * T, cur.py * T);

    // терезені тазалау
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    glViewport(0, 0, dw, dh);
    glClearColor(LIGHT.r, LIGHT.g, LIGHT.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

   // пропорцияны сақтау және көріністі ортаға орналастыру
    float scale = min((float)dw / logicalW, (float)dh / logicalH);
    int vw = (int)(logicalW * scale), vh = (int)(logicalH * scale);
    glViewport((dw - vw) / 2, (dh - vh) / 2, vw, vh);

   // төбелерді VBO-ға жіберіп, экранға салу
    pglUseProgram(prog);
    pglUniform2f(uResLoc, (float)logicalW, (float)logicalH);
    pglBindVertexArray(vao);
    pglBindBuffer(GL_ARRAY_BUFFER, vbo);
    pglBufferData(GL_ARRAY_BUFFER, batch.size() * sizeof(Vertex), batch.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)batch.size());

    SDL_GL_SwapWindow(win);
}

// ---------------------------------------------------------------------
// 5. Game logic
// ---------------------------------------------------------------------
static void updateTitle() {
    string t = "Sokoban Nokia (OpenGL)  |  Level " + to_string(level + 1) + "/" +
               to_string(LEVELS.size()) + "  |  Moves: " + to_string(cur.moves);
    if (allDone)  t = "Sokoban Nokia (OpenGL)  |  YOU WIN ALL LEVELS!  (ESC to quit)";
    else if (won) t += "  |  LEVEL COMPLETE! Press any key";
    SDL_SetWindowTitle(win, t.c_str());
}

static void loadLevel(int n) {
    level = n;
    won = false;
    history.clear();
    const auto& src = LEVELS[n];
    size_t w = 0;
    for (auto& r : src) w = max(w, r.size());
    cur.grid.assign(src.size(), string(w, ' '));
    goals.assign(src.size(), string(w, ' '));
    cur.moves = 0;
    for (size_t y = 0; y < src.size(); y++) {
        for (size_t x = 0; x < src[y].size(); x++) {
            char c = src[y][x], g = ' ';
            if (c == '.')      { g = '.'; c = ' '; }
            else if (c == '*') { g = '.'; c = '$'; }
            else if (c == '+') { g = '.'; c = '@'; }
            if (c == '@') { cur.px = (int)x; cur.py = (int)y; c = ' '; }
            cur.grid[y][x] = c;
            goals[y][x] = g;
        }
    }
    logicalW = (int)w * T;
    logicalH = (int)src.size() * T;
    SDL_SetWindowSize(win, logicalW, logicalH);
    SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    updateTitle();
}

static bool inside(int x, int y) {
    return y >= 0 && y < (int)cur.grid.size() && x >= 0 && x < (int)cur.grid[y].size();
}

static bool isWon() {
    for (size_t y = 0; y < cur.grid.size(); y++)
        for (size_t x = 0; x < cur.grid[y].size(); x++)
            if (goals[y][x] == '.' && cur.grid[y][x] != '$') return false;
    return true;
}

static void tryMove(int dx, int dy) {
    int nx = cur.px + dx, ny = cur.py + dy;
    if (!inside(nx, ny) || cur.grid[ny][nx] == '#') return;
    State backup = cur;
    if (cur.grid[ny][nx] == '$') {
        int bx = nx + dx, by = ny + dy;
        if (!inside(bx, by)) return;
        char t = cur.grid[by][bx];
        if (t == '#' || t == '$') return;
        cur.grid[by][bx] = '$';
        cur.grid[ny][nx] = ' ';
    }
    cur.px = nx; cur.py = ny;
    cur.moves++;
    history.push_back(backup);
    if (isWon()) won = true;
    updateTitle();
}

static void undo() {
    if (history.empty()) return;
    cur = history.back();
    history.pop_back();
    updateTitle();
}

// ---------------------------------------------------------------------
// 6. main
// ---------------------------------------------------------------------
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;

  
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    win = SDL_CreateWindow("Sokoban Nokia (OpenGL)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           400, 300, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!win) { fprintf(stderr, "Window error: %s\n", SDL_GetError()); return 1; }
    ctx = SDL_GL_CreateContext(win);
    if (!ctx) { fprintf(stderr, "GL context error: %s\n", SDL_GetError()); return 1; }
    SDL_GL_SetSwapInterval(1);

    if (!loadGL()) { fprintf(stderr, "Could not load OpenGL 3.3 functions\n"); return 1; }

    // Shader program
    prog = createProgram();
    uResLoc = pglGetUniformLocation(prog, "uRes");


    pglGenVertexArrays(1, &vao);
    pglGenBuffers(1, &vbo);
    pglBindVertexArray(vao);
    pglBindBuffer(GL_ARRAY_BUFFER, vbo);
    pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);                    
    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));  
    pglEnableVertexAttribArray(1);

    loadLevel(0);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_WaitEventTimeout(&e, 50)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            if (e.type != SDL_KEYDOWN) continue;
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_ESCAPE) { running = false; break; }
            if (allDone) continue;
            if (won) {
                if (e.key.repeat) continue;
                if (level + 1 < (int)LEVELS.size()) loadLevel(level + 1);
                else { allDone = true; updateTitle(); }
                continue;
            }
            switch (k) {
                case SDLK_UP:    case SDLK_w: tryMove(0, -1); break;
                case SDLK_DOWN:  case SDLK_s: tryMove(0, 1);  break;
                case SDLK_LEFT:  case SDLK_a: tryMove(-1, 0); break;
                case SDLK_RIGHT: case SDLK_d: tryMove(1, 0);  break;
                case SDLK_u:     undo(); break;
                case SDLK_r:     loadLevel(level); break;
                case SDLK_n:     if (level + 1 < (int)LEVELS.size()) loadLevel(level + 1); break;
            }
        }
        render();
    }

    
    pglDeleteBuffers(1, &vbo);
    pglDeleteVertexArrays(1, &vao);
    pglDeleteProgram(prog);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
