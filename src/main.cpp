#include <SDL2/SDL.h>
#include <iostream>
#include "fluid.hpp"
#include "UI.hpp"
#include <string>
#include <array>
#include <sstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int SIMULATION_WIDTH = 300;
const int SIMULATION_HEIGHT = 200;

enum class Tool {
    Fluid,
    Explosion
};

typedef struct {
    std::string name;
    int r;
    int g;
    int b;
} Color;

template<int N>
void cycleColor(const std::array<Color, N>& colorArray, Color& currentDrawColor, int& currentColorIdx) {
    // Increment the color index, cycling back to 0 if it exceeds the array size
    currentColorIdx = (currentColorIdx + 1) % colorArray.size();

    // Update the current draw color
    currentDrawColor = colorArray[currentColorIdx];
}

int main(int argc, char* argv[]) {
    std::array<Color, 4> const colors = {{
        {"Blue", 0, 0, 255},
        {"Red", 255, 0, 0},
        {"Green", 0, 255, 0},
        {"Purple", 255, 0, 255}
    }};

    int currentColorIdx = 0;
    Color currentDrawColor = colors[currentColorIdx];
    Tool currentTool = Tool::Fluid;
    bool showHelp = false;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Navier-Stokes Fluid Simulation",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize UI system
    UI ui(renderer);
    if (!ui.init()) {
        std::cerr << "UI initialization failed" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Set up fluid simulation
    FluidSimulation fluid(SIMULATION_WIDTH, SIMULATION_HEIGHT, 0.0000001f, 0.0000001f, 0.016f);

    bool running = true;
    SDL_Event event;
    int mouseX, mouseY;
    bool mouseDown = false;
    int prevMouseX = 0, prevMouseY = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_t:
                            currentTool = (currentTool == Tool::Fluid) ? Tool::Explosion : Tool::Fluid;
                            break;
                        case SDLK_c:
                            fluid.reset();
                            break;
                        case SDLK_h:
                            showHelp = !showHelp;
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if(event.button.button == SDL_BUTTON_LEFT) {
                        SDL_GetMouseState(&mouseX, &mouseY);
                        int simX = (mouseX * SIMULATION_WIDTH) / WINDOW_WIDTH;
                        int simY = (mouseY * SIMULATION_HEIGHT) / WINDOW_HEIGHT;
                        
                        if (currentTool == Tool::Explosion) {
                            // Create explosion on click
                            fluid.createExplosion(simX, simY, 100.0f, currentDrawColor.r, currentDrawColor.g, currentDrawColor.b);
                        } else {
                            mouseDown = true;
                            prevMouseX = mouseX;
                            prevMouseY = mouseY;
                        }
                    } else if(event.button.button == SDL_BUTTON_RIGHT) {
                        cycleColor<4>(colors, currentDrawColor, currentColorIdx);
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    mouseDown = false;
                    break;
                case SDL_MOUSEMOTION:
                    if (mouseDown) {
                        SDL_GetMouseState(&mouseX, &mouseY);
                        // Scale mouse coordinates to simulation size
                        int simX = (mouseX * SIMULATION_WIDTH) / WINDOW_WIDTH;
                        int simY = (mouseY * SIMULATION_HEIGHT) / WINDOW_HEIGHT;
                        int prevSimX = (prevMouseX * SIMULATION_WIDTH) / WINDOW_WIDTH;
                        int prevSimY = (prevMouseY * SIMULATION_HEIGHT) / WINDOW_HEIGHT;
                        
                        // Only handle fluid tool during motion
                        if (currentTool == Tool::Fluid) {
                            // Add density at mouse position with current color
                            fluid.addDensity(simX, simY, 100, currentDrawColor.r, currentDrawColor.g, currentDrawColor.b);
                            
                            // Add velocity based on mouse movement
                            float velX = (mouseX - prevMouseX) * 2.0f;
                            float velY = (mouseY - prevMouseY) * 2.0f;
                            fluid.addVelocity(simX, simY, velX, velY);
                        }
                        
                        prevMouseX = mouseX;
                        prevMouseY = mouseY;
                    }
                    break;
            }
        }

        // Update simulation
        fluid.step();

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Scale rendering to window size
        SDL_RenderSetScale(renderer, 
            static_cast<float>(WINDOW_WIDTH) / SIMULATION_WIDTH,
            static_cast<float>(WINDOW_HEIGHT) / SIMULATION_HEIGHT);

        // Render fluid
        fluid.render(renderer);
        
        // Reset scale for UI rendering
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);

        // Draw UI elements
        std::stringstream ss;
        ss << "Color: " << currentDrawColor.name;
        ui.drawText(ss.str(), 10, 10);
        ui.drawText("Tool: " + std::string(currentTool == Tool::Fluid ? "Fluid" : "Explosion"), 10, 40);
        ui.drawText("Help (H)", WINDOW_WIDTH - 100, 10);

        if (showHelp) {
            // Semi-transparent black background for help popup
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            const int margin = 50;  // Margin from window edges
            const int textPadding = 30;  // Padding for text from popup edges
            const int popupWidth = WINDOW_WIDTH - (margin * 2);
            const int popupHeight = 250;  // Fixed height for the popup
            const int popupX = margin;
            const int popupY = (WINDOW_HEIGHT - popupHeight) / 2;  // Centered vertically

            SDL_Rect helpRect = {popupX, popupY, popupWidth, popupHeight};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &helpRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            // Help text
            int y = popupY + textPadding;
            ui.drawText("Controls:", popupX + textPadding, y);
            y += 35;
            ui.drawText("Left click and drag - Draw fluid", popupX + textPadding, y);
            y += 35;
            ui.drawText("Right click - Cycle colors", popupX + textPadding, y);
            y += 35;
            ui.drawText("T - Switch between fluid/explosion tools", popupX + textPadding, y);
            y += 35;
            ui.drawText("C - Clear simulation", popupX + textPadding, y);
            y += 35;
            ui.drawText("H - Toggle help", popupX + textPadding, y);
        }

        // Present render
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
