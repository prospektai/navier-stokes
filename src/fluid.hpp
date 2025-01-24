#pragma once
#include <vector>
#include <SDL2/SDL.h>

class FluidSimulation {
public:
    FluidSimulation(int width, int height, float diffusion, float viscosity, float dt);
    ~FluidSimulation();

    void step();
    void addDensity(int x, int y, float amount, int r, int g, int b);
    void addVelocity(int x, int y, float amountX, float amountY);
    void createExplosion(int x, int y, float power, int r, int g, int b);
    void render(SDL_Renderer* renderer);
    void reset();  // Reset simulation to initial state

private:
    int width;
    int height;
    float dt;
    float diff;
    float visc;

    std::vector<float> densityR;
    std::vector<float> densityG;
    std::vector<float> densityB;
    std::vector<float> Vx;
    std::vector<float> Vy;
    std::vector<float> Vx0;
    std::vector<float> Vy0;

    void diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt);
    void project(std::vector<float>& velocX, std::vector<float>& velocY, std::vector<float>& p, std::vector<float>& div);
    void advect(int b, std::vector<float>& d, std::vector<float>& d0, std::vector<float>& velocX, std::vector<float>& velocY, float dt);
    void setBnd(int b, std::vector<float>& x);
    int IX(int x, int y) { return x + y * width; }
};
