#include "UI.hpp"
#include <iostream>

UI::UI(SDL_Renderer* renderer) : renderer(renderer), font(nullptr) {}

UI::~UI() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}

bool UI::init() {
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        return false;
    }

    // Load a default system font
    font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", FONT_SIZE);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return false;
    }

    return true;
}

void UI::drawText(const std::string& text, int x, int y, SDL_Color color, bool outline) {
    if (!font) {
        std::cerr << "Font not initialized" << std::endl;
        return;
    }

    if (outline) {
        // Draw black outline
        SDL_Color outlineColor = {0, 0, 0, 255};
        const int outlineOffset = 1;
        
        // Draw outline in 8 directions
        for(int dy = -outlineOffset; dy <= outlineOffset; dy++) {
            for(int dx = -outlineOffset; dx <= outlineOffset; dx++) {
                if (dx == 0 && dy == 0) continue; // Skip center
                
                SDL_Surface* outlineSurface = TTF_RenderText_Blended(font, text.c_str(), outlineColor);
                if (!outlineSurface) continue;

                SDL_Texture* outlineTexture = SDL_CreateTextureFromSurface(renderer, outlineSurface);
                if (outlineTexture) {
                    SDL_Rect outlineRect;
                    outlineRect.x = x + dx;
                    outlineRect.y = y + dy;
                    SDL_QueryTexture(outlineTexture, nullptr, nullptr, &outlineRect.w, &outlineRect.h);
                    SDL_RenderCopy(renderer, outlineTexture, nullptr, &outlineRect);
                    SDL_DestroyTexture(outlineTexture);
                }
                SDL_FreeSurface(outlineSurface);
            }
        }
    }

    // Draw main text
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect destRect;
    destRect.x = x;
    destRect.y = y;
    SDL_QueryTexture(texture, nullptr, nullptr, &destRect.w, &destRect.h);

    SDL_RenderCopy(renderer, texture, nullptr, &destRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
