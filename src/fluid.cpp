#include "fluid.hpp"
#include <algorithm>
#include <cmath>

FluidSimulation::FluidSimulation(int width, int height, float diffusion, float viscosity, float dt)
    : width(width), height(height), dt(dt), diff(diffusion), visc(viscosity) {
    int size = width * height;
    densityR.resize(size, 0);
    densityG.resize(size, 0);
    densityB.resize(size, 0);
    Vx.resize(size, 0);
    Vy.resize(size, 0);
    Vx0.resize(size, 0);
    Vy0.resize(size, 0);
}

FluidSimulation::~FluidSimulation() {}

void FluidSimulation::step() {
    std::vector<float> p(width * height);
    std::vector<float> div(width * height);

    // Velocity step
    diffuse(1, Vx0, Vx, visc, dt);
    diffuse(2, Vy0, Vy, visc, dt);
    project(Vx0, Vy0, p, div);
    advect(1, Vx, Vx0, Vx0, Vy0, dt);
    advect(2, Vy, Vy0, Vx0, Vy0, dt);
    project(Vx, Vy, p, div);

    // Density step for each color channel
    std::vector<float> density0(width * height);
    
    // Red channel
    diffuse(0, density0, densityR, diff, dt);
    advect(0, densityR, density0, Vx, Vy, dt);
    
    // Green channel
    diffuse(0, density0, densityG, diff, dt);
    advect(0, densityG, density0, Vx, Vy, dt);
    
    // Blue channel
    diffuse(0, density0, densityB, diff, dt);
    advect(0, densityB, density0, Vx, Vy, dt);
}

void FluidSimulation::addDensity(int x, int y, float amount, int r, int g, int b) {
    int idx = IX(x, y);
    float normalizedAmount = amount / 255.0f;
    densityR[idx] += normalizedAmount * r;
    densityG[idx] += normalizedAmount * g;
    densityB[idx] += normalizedAmount * b;
}

void FluidSimulation::addVelocity(int x, int y, float amountX, float amountY) {
    int index = IX(x, y);
    Vx[index] += amountX;
    Vy[index] += amountY;
}

void FluidSimulation::createExplosion(int x, int y, float power, int r, int g, int b) {
    const float radius = 20.0f;  // Smaller, more concentrated radius
    const float densityAmount = power * 10.0f;  // More concentrated density
    const float velocityMultiplier = 50.0f;  // Much stronger outward force
    
    // Add density and velocity in a circular pattern
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            float dist = std::sqrt(i * i + j * j);
            if (dist <= radius) {
                int cx = x + i;
                int cy = y + j;
                
                // Ensure we're within bounds
                if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
                    // Calculate falloff based on distance from center
                    float falloff = (radius - dist) / radius;
                    falloff = falloff * falloff * falloff;  // Cubic falloff for sharper pressure point
                    
                    // Add density with color
                    addDensity(cx, cy, densityAmount * falloff, r, g, b);
                    
                    if (dist > 0.1f) {  // Avoid division by zero
                        // Add outward velocity with stronger force
                        float velX = (i / dist) * power * falloff * velocityMultiplier;
                        float velY = (j / dist) * power * falloff * velocityMultiplier;
                        addVelocity(cx, cy, velX, velY);
                    }
                }
            }
        }
    }
}

void FluidSimulation::diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt) {
    float a = dt * diff * (width - 2) * (height - 2);
    for (int k = 0; k < 20; k++) {
        for (int i = 1; i < width - 1; i++) {
            for (int j = 1; j < height - 1; j++) {
                x[IX(i, j)] = (x0[IX(i, j)] + a * (
                    x[IX(i+1, j)] + x[IX(i-1, j)] +
                    x[IX(i, j+1)] + x[IX(i, j-1)]
                )) / (1 + 4 * a);
            }
        }
        setBnd(b, x);
    }
}

void FluidSimulation::project(std::vector<float>& velocX, std::vector<float>& velocY, std::vector<float>& p, std::vector<float>& div) {
    for (int i = 1; i < width - 1; i++) {
        for (int j = 1; j < height - 1; j++) {
            div[IX(i, j)] = -0.5f * (
                velocX[IX(i+1, j)] - velocX[IX(i-1, j)] +
                velocY[IX(i, j+1)] - velocY[IX(i, j-1)]
            ) / width;
            p[IX(i, j)] = 0;
        }
    }
    setBnd(0, div);
    setBnd(0, p);

    for (int k = 0; k < 20; k++) {
        for (int i = 1; i < width - 1; i++) {
            for (int j = 1; j < height - 1; j++) {
                p[IX(i, j)] = (div[IX(i, j)] + (
                    p[IX(i+1, j)] + p[IX(i-1, j)] +
                    p[IX(i, j+1)] + p[IX(i, j-1)]
                )) / 4;
            }
        }
        setBnd(0, p);
    }

    for (int i = 1; i < width - 1; i++) {
        for (int j = 1; j < height - 1; j++) {
            velocX[IX(i, j)] -= 0.5f * (p[IX(i+1, j)] - p[IX(i-1, j)]) * width;
            velocY[IX(i, j)] -= 0.5f * (p[IX(i, j+1)] - p[IX(i, j-1)]) * width;
        }
    }
    setBnd(1, velocX);
    setBnd(2, velocY);
}

void FluidSimulation::advect(int b, std::vector<float>& d, std::vector<float>& d0, std::vector<float>& velocX, std::vector<float>& velocY, float dt) {
    float i0, i1, j0, j1;
    float dtx = dt * (width - 2);
    float dty = dt * (height - 2);
    float s0, s1, t0, t1;
    float tmp1, tmp2, x, y;

    for (int i = 1; i < width - 1; i++) {
        for (int j = 1; j < height - 1; j++) {
            tmp1 = dtx * velocX[IX(i, j)];
            tmp2 = dty * velocY[IX(i, j)];
            x = i - tmp1;
            y = j - tmp2;

            if (x < 0.5f) x = 0.5f;
            if (x > width - 1.5f) x = width - 1.5f;
            i0 = std::floor(x);
            i1 = i0 + 1.0f;

            if (y < 0.5f) y = 0.5f;
            if (y > height - 1.5f) y = height - 1.5f;
            j0 = std::floor(y);
            j1 = j0 + 1.0f;

            s1 = x - i0;
            s0 = 1.0f - s1;
            t1 = y - j0;
            t0 = 1.0f - t1;

            int i0i = static_cast<int>(i0);
            int i1i = static_cast<int>(i1);
            int j0i = static_cast<int>(j0);
            int j1i = static_cast<int>(j1);

            d[IX(i, j)] =
                s0 * (t0 * d0[IX(i0i, j0i)] + t1 * d0[IX(i0i, j1i)]) +
                s1 * (t0 * d0[IX(i1i, j0i)] + t1 * d0[IX(i1i, j1i)]);
        }
    }
    setBnd(b, d);
}

void FluidSimulation::setBnd(int b, std::vector<float>& x) {
    for (int i = 1; i < width - 1; i++) {
        x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, height-1)] = b == 2 ? -x[IX(i, height-2)] : x[IX(i, height-2)];
    }
    for (int j = 1; j < height - 1; j++) {
        x[IX(0, j)] = b == 1 ? -x[IX(1, j)] : x[IX(1, j)];
        x[IX(width-1, j)] = b == 1 ? -x[IX(width-2, j)] : x[IX(width-2, j)];
    }

    x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, height-1)] = 0.5f * (x[IX(1, height-1)] + x[IX(0, height-2)]);
    x[IX(width-1, 0)] = 0.5f * (x[IX(width-2, 0)] + x[IX(width-1, 1)]);
    x[IX(width-1, height-1)] = 0.5f * (x[IX(width-2, height-1)] + x[IX(width-1, height-2)]);
}

void FluidSimulation::reset() {
    std::fill(densityR.begin(), densityR.end(), 0.0f);
    std::fill(densityG.begin(), densityG.end(), 0.0f);
    std::fill(densityB.begin(), densityB.end(), 0.0f);
    std::fill(Vx.begin(), Vx.end(), 0.0f);
    std::fill(Vy.begin(), Vy.end(), 0.0f);
    std::fill(Vx0.begin(), Vx0.end(), 0.0f);
    std::fill(Vy0.begin(), Vy0.end(), 0.0f);
}

void FluidSimulation::render(SDL_Renderer* renderer) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            int idx = IX(i, j);
            int r = static_cast<int>(std::min(255.0f, std::max(0.0f, densityR[idx] * 255)));
            int g = static_cast<int>(std::min(255.0f, std::max(0.0f, densityG[idx] * 255)));
            int b = static_cast<int>(std::min(255.0f, std::max(0.0f, densityB[idx] * 255)));
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, i, j);
        }
    }
}
