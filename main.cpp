#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct Vec3 { float x, y, z; };
struct Particle { Vec3 pos; SDL_Color color; };

enum Mode { IMPLICIT, LORENZ, AIZAWA };

int WIDTH = 1280;
int HEIGHT = 800;
std::vector<Particle> particles;
Mode currentMode = LORENZ; // Default to the butterfly

struct Camera {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float dist = 120.0f;
};
Camera cam;

bool mouseDown = false;
int lastMouseX = 0;
int lastMouseY = 0;

// ------------------------------------------------
// ATTRACTOR GENERATORS
// ------------------------------------------------

void generateLorenz() {
    particles.clear();
    float x = 0.1f, y = 0.0f, z = 0.0f;
    float dt = 0.005f;

    for (int i = 0; i < 60000; i++) {
        float dx = 10.0f * (y - x);
        float dy = x * (28.0f - z) - y;
        float dz = x * y - (8.0f / 3.0f) * z;

        x += dx * dt;
        y += dy * dt;
        z += dz * dt;

        // Visual coloring: scale based on position
        Uint8 r = (Uint8)std::clamp(std::abs(x) * 10.0f, 0.0f, 255.0f);
        Uint8 g = (Uint8)std::clamp(std::abs(y) * 10.0f, 0.0f, 255.0f);
        Uint8 b = (Uint8)std::clamp(z * 5.0f, 0.0f, 255.0f);

        // Center Z around origin for easier rotation
        particles.push_back({ {x * 2.0f, y * 2.0f, (z - 25.0f) * 2.0f}, {r, g, b, 255} });
    }
}

void generateAizawa() {
    particles.clear();
    float x = 0.1f, y = 0.0f, z = 0.0f;
    float dt = 0.01f;
    float a = 0.95f, b = 0.7f, c = 0.6f, d = 3.5f, e = 0.25f, f = 0.1f;

    for (int i = 0; i < 60000; i++) {
        float dx = (z - b) * x - d * y;
        float dy = d * x + (z - b) * y;
        float dz = c + a * z - (pow(z, 3) / 3.0f) - (pow(x, 2) + pow(y, 2)) * (1.0f + e * z) + f * z * pow(x, 3);

        x += dx * dt;
        y += dy * dt;
        z += dz * dt;

        Uint8 r = (Uint8)std::clamp(100.0f + x * 70.0f, 0.0f, 255.0f);
        Uint8 g = (Uint8)std::clamp(150.0f + y * 70.0f, 0.0f, 255.0f);
        Uint8 b = (Uint8)std::clamp(200.0f + z * 70.0f, 0.0f, 255.0f);

        particles.push_back({ {x * 25.0f, y * 25.0f, z * 25.0f}, {r, g, b, 255} });
    }
}

// ------------------------------------------------
// RENDERING & INPUT (Condensed)
// ------------------------------------------------

void handleMouse(SDL_Event& e) {
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        mouseDown = true;
        lastMouseX = (int)e.button.x; lastMouseY = (int)e.button.y;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) mouseDown = false;
    if (e.type == SDL_EVENT_MOUSE_MOTION && mouseDown) {
        cam.yaw += (e.motion.x - lastMouseX) * 0.005f;
        cam.pitch += (e.motion.y - lastMouseY) * 0.005f;
        lastMouseX = (int)e.motion.x; lastMouseY = (int)e.motion.y;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL) cam.dist -= e.wheel.y * 5.0f;
}

void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 5, 5, 15, 255);
    SDL_RenderClear(renderer);

    float cY = cos(-cam.yaw), sY = sin(-cam.yaw);
    float cP = cos(-cam.pitch), sP = sin(-cam.pitch);

    for (auto& p : particles) {
        float rx = p.pos.x * cY - p.pos.z * sY;
        float rz_t = p.pos.x * sY + p.pos.z * cY;
        float ry = p.pos.y * cP - rz_t * sP;
        float rz = p.pos.y * sP + rz_t * cP + cam.dist;

        if (rz > 1.0f) {
            float sx = rx * 800.0f / rz + WIDTH / 2;
            float sy = ry * 800.0f / rz + HEIGHT / 2;
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
    SDL_Window* window = SDL_CreateWindow("3D Attractor Engine", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    generateLorenz(); // Start with Lorenz

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            handleMouse(e);
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_1) { currentMode = LORENZ; generateLorenz(); }
                if (e.key.key == SDLK_2) { currentMode = AIZAWA; generateAizawa(); }
            }
        }
        render(renderer);
    }
    SDL_Quit();
    return 0;
}
