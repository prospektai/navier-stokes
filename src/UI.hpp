#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class UI {
public:
    UI(SDL_Renderer* renderer);
    ~UI();
    
    // Initialize the UI system (loads fonts, etc)
    bool init();
    
    // Draw text at specified position
    void drawText(const std::string& text, int x, int y, SDL_Color color = {255, 255, 255, 255}, bool outline = true);
    
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    static constexpr int FONT_SIZE = 24;
};
