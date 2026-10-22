

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

const int T = 32;  // tile size in pixels

// Nokia LCD palette
const SDL_Color LIGHT = {199, 240, 216, 255};
const SDL_Color DARK  = {67, 82, 61, 255};

// Legend: # wall, @ player, $ box, . goal, * box on goal, + player on goal
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
static SDL_Renderer* ren = nullptr;

static State cur;
static vector<State> history;
static vector<string> goals;
static int level = 0;
static bool won = false;
static bool allDone = false;

static void setColor(SDL_Color c) { SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a); }

static void fillRect(int x, int y, int w, int h, SDL_Color c) {
    setColor(c);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(ren, &r);
}

static void updateTitle() {
    string t = "Tausogar  |  Level " + to_string(level + 1) + "/" + to_string(LEVELS.size()) +
               "  |  Moves: " + to_string(cur.moves);
    if (allDone)      t = "Tausogar  |  YOU WIN ALL LEVELS!  (ESC to quit)";
    else if (won)     t += "  |  LEVEL COMPLETE! Press any key";
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
    SDL_SetWindowSize(win, (int)w * T, (int)src.size() * T);
    SDL_RenderSetLogicalSize(ren, (int)w * T, (int)src.size() * T);
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

// ---------- drawing ----------

static void drawWall(int x, int y) {
    fillRect(x, y, T, T, DARK);
    // brick lines
    fillRect(x, y + 10, T, 2, LIGHT);
    fillRect(x, y + 21, T, 2, LIGHT);
    fillRect(x + 16, y, 2, 10, LIGHT);
    fillRect(x + 8,  y + 12, 2, 9, LIGHT);
    fillRect(x + 24, y + 12, 2, 9, LIGHT);
    fillRect(x + 16, y + 23, 2, 9, LIGHT);
}

static void drawGoal(int x, int y) {
    fillRect(x + 12, y + 12, 8, 8, DARK);
    fillRect(x + 14, y + 14, 4, 4, LIGHT);
}

static void drawBox(int x, int y, bool onGoal) {
    fillRect(x + 3, y + 3, T - 6, T - 6, DARK);
    if (onGoal) {
        // solid box with small light square = "done"
        fillRect(x + 12, y + 12, 8, 8, LIGHT);
    } else {
        fillRect(x + 6, y + 6, T - 12, T - 12, LIGHT);
        // diagonal cross
        setColor(DARK);
        for (int i = 6; i < T - 6; i++) {
            SDL_RenderDrawPoint(ren, x + i, y + i);
            SDL_RenderDrawPoint(ren, x + i + 1, y + i);
            SDL_RenderDrawPoint(ren, x + T - 1 - i, y + i);
            SDL_RenderDrawPoint(ren, x + T - 2 - i, y + i);
        }
    }
}

static void drawPlayer(int x, int y) {
    fillRect(x + 11, y + 4,  10, 9,  DARK);   // head
    fillRect(x + 13, y + 7,  2,  2,  LIGHT);  // eyes
    fillRect(x + 17, y + 7,  2,  2,  LIGHT);
    fillRect(x + 9,  y + 14, 14, 9,  DARK);   // body
    fillRect(x + 5,  y + 15, 4,  7,  DARK);   // arms
    fillRect(x + 23, y + 15, 4,  7,  DARK);
    fillRect(x + 10, y + 23, 5,  6,  DARK);   // legs
    fillRect(x + 17, y + 23, 5,  6,  DARK);
}

static void render() {
    setColor(LIGHT);
    SDL_RenderClear(ren);
    for (size_t y = 0; y < cur.grid.size(); y++) {
        for (size_t x = 0; x < cur.grid[y].size(); x++) {
            int sx = (int)x * T, sy = (int)y * T;
            char c = cur.grid[y][x];
            bool goal = goals[y][x] == '.';
            if (c == '#')        drawWall(sx, sy);
            else {
                if (goal) drawGoal(sx, sy);
                if (c == '$') drawBox(sx, sy, goal);
            }
        }
    }
    drawPlayer(cur.px * T, cur.py * T);
    SDL_RenderPresent(ren);
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    win = SDL_CreateWindow("Tausogar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           400, 300, SDL_WINDOW_RESIZABLE);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!win || !ren) return 1;
    SDL_RenderSetIntegerScale(ren, SDL_FALSE);

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

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
