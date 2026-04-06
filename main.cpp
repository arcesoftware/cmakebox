#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct Vec3 { float x, y, z; };
struct Particle { Vec3 pos; SDL_Color color; };

enum Mode { LORENZ, AIZAWA, ROSSLER };

// Global Settings
int WIDTH = 1280;
int HEIGHT = 800;
std::vector<Particle> particles;
Mode currentMode = LORENZ;

// Simulation State
Vec3 currentPos = { 0.1f, 0.0f, 0.0f };
float timer = 0.0f;

struct Camera {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float dist = 150.0f;
    Vec3 target = { 0.0f, 0.0f, 0.0f }; // Look-at point
};
Camera cam;

bool mouseDown = false;
int lastMouseX = 0, lastMouseY = 0;

// ------------------------------------------------
// INTEGRATORS (Step-by-Step Math)
// ------------------------------------------------

void stepSimulation() {
    float dt = 0.01f;
    float dx = 0, dy = 0, dz = 0;

    if (currentMode == LORENZ) {
        dx = 10.0f * (currentPos.y - currentPos.x);
        dy = currentPos.x * (28.0f - currentPos.z) - currentPos.y;
        dz = currentPos.x * currentPos.y - (8.0f / 3.0f) * currentPos.z;
    }
    else if (currentMode == AIZAWA) {
        float a = 0.95f, b = 0.7f, c = 0.6f, d = 3.5f, e = 0.25f, f = 0.1f;
        dx = (currentPos.z - b) * currentPos.x - d * currentPos.y;
        dy = d * currentPos.x + (currentPos.z - b) * currentPos.y;
        dz = c + a * currentPos.z - (pow(currentPos.z, 3) / 3.0f) -
            (pow(currentPos.x, 2) + pow(currentPos.y, 2)) * (1.0f + e * currentPos.z) + f * currentPos.z * pow(currentPos.x, 3);
    }
    else if (currentMode == ROSSLER) {
        float a = 0.2f, b = 0.2f, c = 5.7f;
        dx = -currentPos.y - currentPos.z;
        dy = currentPos.x + a * currentPos.y;
        dz = b + currentPos.z * (currentPos.x - c);
    }

    currentPos.x += dx * dt;
    currentPos.y += dy * dt;
    currentPos.z += dz * dt;

    // Scaling for visualization
    float s = (currentMode == AIZAWA) ? 30.0f : (currentMode == ROSSLER ? 5.0f : 2.0f);
    float zOff = (currentMode == LORENZ) ? 25.0f : 0.0f;

    Uint8 r = (Uint8)std::clamp(128.0f + currentPos.x * 10.0f, 0.0f, 255.0f);
    Uint8 g = (Uint8)std::clamp(128.0f + currentPos.y * 10.0f, 0.0f, 255.0f);
    Uint8 b = (Uint8)std::clamp(currentPos.z * 5.0f, 0.0f, 255.0f);

    particles.push_back({ {currentPos.x * s, currentPos.y * s, (currentPos.z - zOff) * s}, {r, g, b, 255} });

    // Keep buffer manageable
    if (particles.size() > 100000) particles.erase(particles.begin());
}

// ------------------------------------------------
// INPUT & CAMERA
// ------------------------------------------------

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) running = false;

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { mouseDown = true; }
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) { mouseDown = false; }
        if (e.type == SDL_EVENT_MOUSE_MOTION && mouseDown) {
            cam.yaw += e.motion.xrel * 0.005f;
            cam.pitch += e.motion.yrel * 0.005f;
        }
        if (e.type == SDL_EVENT_MOUSE_WHEEL) cam.dist -= e.wheel.y * 5.0f;

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_1) { currentMode = LORENZ; particles.clear(); currentPos = { 0.1, 0, 0 }; }
            if (e.key.key == SDLK_2) { currentMode = AIZAWA; particles.clear(); currentPos = { 0.1, 0, 0 }; }
            if (e.key.key == SDLK_3) { currentMode = ROSSLER; particles.clear(); currentPos = { 0.1, 0, 0 }; }
            if (e.key.key == SDLK_R) { particles.clear(); currentPos = { 0.1, 0, 0 }; }
        }
    }

    const bool* keys = SDL_GetKeyboardState(NULL);
    float camSpeed = 1.0f;
    if (keys[SDL_SCANCODE_W]) cam.target.y -= camSpeed;
    if (keys[SDL_SCANCODE_S]) cam.target.y += camSpeed;
    if (keys[SDL_SCANCODE_A]) cam.target.x -= camSpeed;
    if (keys[SDL_SCANCODE_D]) cam.target.x += camSpeed;
}

// ------------------------------------------------
// RENDER
// ------------------------------------------------

void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderClear(renderer);

    float cY = cos(-cam.yaw), sY = sin(-cam.yaw);
    float cP = cos(-cam.pitch), sP = sin(-cam.pitch);

    for (const auto& p : particles) {
        // Translate relative to camera target (Panning)
        float tx = p.pos.x - cam.target.x;
        float ty = p.pos.y - cam.target.y;
        float tz = p.pos.z - cam.target.z;

        // Rotation
        float rx = tx * cY - tz * sY;
        float rz_t = tx * sY + tz * cY;
        float ry = ty * cP - rz_t * sP;
        float rz = ty * sP + rz_t * cP + cam.dist;

        if (rz > 1.0f) {
            float fov = 800.0f;
            float sx = rx * fov / rz + WIDTH / 2;
            float sy = ry * fov / rz + HEIGHT / 2;

            if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT) {
                SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, 255);
                SDL_RenderPoint(renderer, sx, sy);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Chaos Engine v2", WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    bool running = true;
    while (running) {
        handleInput(running);

        // Add 50 new points per frame for smooth growth
        for (int i = 0; i < 50; i++) stepSimulation();

        render(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
