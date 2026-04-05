#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct Vec3
{
    float x, y, z;
};

struct Particle
{
    Vec3 pos;
    SDL_Color color;
};

enum Mode
{
    IMPLICIT,
    SDF_SHAPE,
    FRACTAL
};

int WIDTH = 1280;
int HEIGHT = 800;

std::vector<Particle> particles;

Mode currentMode = IMPLICIT;

struct Camera
{
    float yaw = 0.0f;
    float pitch = 0.0f;
    float dist = 120.0f;
};

Camera cam;

bool mouseDown = false;

int lastMouseX = 0;
int lastMouseY = 0;

//
// ------------------------------------------------
// MATH FUNCTIONS
// ------------------------------------------------
//

float implicitFunc(float x, float y, float z)
{
    return std::sin(x * 0.2f) +
        std::sin(y * 0.2f) +
        std::sin(z * 0.2f) +
        std::sin(x * y * 0.02f) -
        std::cos(z * x * 0.02f);
}

float sdf(Vec3 p)
{
    float sphere = sqrt(p.x * p.x + p.y * p.y + p.z * p.z) - 15.0f;

    float waves =
        sin(p.x * 0.3f) *
        sin(p.y * 0.3f) *
        sin(p.z * 0.3f);

    return sphere + waves * 4.0f;
}

float mandelbulb(Vec3 p)
{
    Vec3 z = p;
    float r = 0;

    for (int i = 0; i < 6; i++)
    {
        r = sqrt(z.x * z.x + z.y * z.y + z.z * z.z);

        if (r > 2.0f)
            break;

        float theta = acos(z.z / (r + 0.0001f));
        float phi = atan2(z.y, z.x);

        float power = 6.0f;

        float zr = pow(r, power);

        theta *= power;
        phi *= power;

        z =
        {
            zr * sin(theta) * cos(phi),
            zr * sin(theta) * sin(phi),
            zr * cos(theta)
        };

        z.x += p.x;
        z.y += p.y;
        z.z += p.z;
    }

    return r;
}

//
// ------------------------------------------------
// PARTICLE GENERATORS
// ------------------------------------------------
//

void generateImplicit()
{
    particles.clear();

    for (float x = -30; x < 30; x += 1.0f)
        for (float y = -30; y < 30; y += 1.0f)
            for (float z = -30; z < 30; z += 1.0f)
            {
                float v = implicitFunc(x, y, z);

                if (fabs(v) < 0.2f)
                {
                    Uint8 r = (Uint8)((sin(x * 0.1f) + 1) * 127);
                    Uint8 g = (Uint8)((sin(y * 0.1f) + 1) * 127);
                    Uint8 b = (Uint8)((sin(z * 0.1f) + 1) * 127);

                    particles.push_back({ {x,y,z},{r,g,b,255} });
                }
            }
}

void generateSDF()
{
    particles.clear();

    for (float x = -30; x < 30; x += 1.0f)
        for (float y = -30; y < 30; y += 1.0f)
            for (float z = -30; z < 30; z += 1.0f)
            {
                float d = sdf({ x,y,z });

                if (fabs(d) < 0.5f)
                {
                    Uint8 r = (Uint8)(255 - fabs(d) * 200);
                    Uint8 g = (Uint8)(100 + fabs(d) * 100);
                    Uint8 b = 255;

                    particles.push_back({ {x,y,z},{r,g,b,255} });
                }
            }
}

void generateFractal()
{
    particles.clear();

    for (float x = -2; x < 2; x += 0.1f)
        for (float y = -2; y < 2; y += 0.1f)
            for (float z = -2; z < 2; z += 0.1f)
            {
                float m = mandelbulb({ x,y,z });

                if (m < 2.0f)
                {
                    Uint8 c = (Uint8)(m * 120);

                    Particle p;

                    p.pos = { x * 20, y * 20, z * 20 };
                    p.color = { c,(Uint8)(255 - c),200,255 };

                    particles.push_back(p);
                }
            }
}

//
// ------------------------------------------------
// INPUT
// ------------------------------------------------
//

void handleMouse(SDL_Event& e)
{
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (e.button.button == SDL_BUTTON_LEFT)
        {
            mouseDown = true;
            lastMouseX = e.button.x;
            lastMouseY = e.button.y;
        }
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        if (e.button.button == SDL_BUTTON_LEFT)
            mouseDown = false;
    }

    if (e.type == SDL_EVENT_MOUSE_MOTION && mouseDown)
    {
        int dx = e.motion.x - lastMouseX;
        int dy = e.motion.y - lastMouseY;

        cam.yaw += dx * 0.005f;
        cam.pitch += dy * 0.005f;

        cam.pitch = std::clamp(cam.pitch, -1.5f, 1.5f);

        lastMouseX = e.motion.x;
        lastMouseY = e.motion.y;
    }

    if (e.type == SDL_EVENT_MOUSE_WHEEL)
    {
        cam.dist -= e.wheel.y * 5.0f;
        cam.dist = std::clamp(cam.dist, 10.0f, 500.0f);
    }
}

//
// ------------------------------------------------
// RENDER
// ------------------------------------------------
//

void render(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 5, 5, 10, 255);
    SDL_RenderClear(renderer);

    float cY = cos(-cam.yaw);
    float sY = sin(-cam.yaw);

    float cP = cos(-cam.pitch);
    float sP = sin(-cam.pitch);

    for (auto& p : particles)
    {
        float wx = p.pos.x;
        float wy = p.pos.y;
        float wz = p.pos.z;

        float rx = wx * cY - wz * sY;
        float rz_t = wx * sY + wz * cY;

        float ry = wy * cP - rz_t * sP;
        float rz = wy * sP + rz_t * cP + cam.dist;

        if (rz > 1.0f)
        {
            float sx = rx * 800.0f / rz + WIDTH / 2;
            float sy = ry * 800.0f / rz + HEIGHT / 2;

            if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT)
            {
                SDL_SetRenderDrawColor(renderer,
                    p.color.r,
                    p.color.g,
                    p.color.b,
                    255);

                SDL_RenderPoint(renderer, sx, sy);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

//
// ------------------------------------------------
// MAIN
// ------------------------------------------------
//

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "3D Math Geometry Engine",
        WIDTH,
        HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    generateImplicit();

    bool running = true;
    SDL_Event e;

    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                WIDTH = e.window.data1;
                HEIGHT = e.window.data2;
            }

            handleMouse(e);

            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_1)
                {
                    currentMode = IMPLICIT;
                    generateImplicit();
                }

                if (e.key.key == SDLK_2)
                {
                    currentMode = SDF_SHAPE;
                    generateSDF();
                }

                if (e.key.key == SDLK_3)
                {
                    currentMode = FRACTAL;
                    generateFractal();
                }

                if (e.key.scancode == SDL_SCANCODE_R)
                {
                    if (currentMode == IMPLICIT) generateImplicit();
                    if (currentMode == SDF_SHAPE) generateSDF();
                    if (currentMode == FRACTAL) generateFractal();
                }
            }
        }

        const bool* keys = SDL_GetKeyboardState(NULL);

        if (keys[SDL_SCANCODE_W]) cam.dist -= 3.0f;
        if (keys[SDL_SCANCODE_S]) cam.dist += 3.0f;

        cam.dist = std::clamp(cam.dist, 10.0f, 500.0f);

        render(renderer);
    }

    SDL_Quit();
    return 0;
}