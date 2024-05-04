#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <cmath>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

enum GameState {
    MENU,
    GAME
};

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gStartBgTexture = nullptr;
SDL_Texture* gStartButtonTexture = nullptr;
SDL_Texture* gGameBgTexture = nullptr;
GameState gState = MENU;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gWindow = SDL_CreateWindow("Gold Miner", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == nullptr) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == nullptr) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    return true;
}

SDL_Texture* loadTexture(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface == nullptr) {
        std::cout << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void close() {
    SDL_DestroyTexture(gStartBgTexture);
    SDL_DestroyTexture(gStartButtonTexture);
    SDL_DestroyTexture(gGameBgTexture);
    gStartBgTexture = nullptr;
    gStartButtonTexture = nullptr;
    gGameBgTexture = nullptr;

    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = nullptr;
    gWindow = nullptr;

    IMG_Quit();
    SDL_Quit();
}

void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            gState = GAME;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            if (gState == MENU) {
                if (x >= 100 && x <= 300 && y >= 100 && y <= 200) {
                    gState = GAME;
                }
            }
        }
    }
}

void update() {}

void drawMenu() {
    SDL_RenderClear(gRenderer);

    SDL_RenderCopy(gRenderer, gStartBgTexture, nullptr, nullptr);

    float scaleFactor = 1.5;
    int buttonWidth = 200 / scaleFactor;
    int buttonHeight = 100 / scaleFactor;

    SDL_Rect startButtonRect = { 140, 150, buttonWidth, buttonHeight };
    SDL_RenderCopy(gRenderer, gStartButtonTexture, nullptr, &startButtonRect);

    SDL_RenderPresent(gRenderer);
}

void drawGame() {
    SDL_RenderClear(gRenderer);

    SDL_RenderCopy(gRenderer, gGameBgTexture, nullptr, nullptr);

    SDL_Texture* minerActionTexture = loadTexture("mineraction.png");
    if (minerActionTexture != nullptr) {
        SDL_Rect minerActionRect = { 330, 55, 100, 100 };
        SDL_RenderCopy(gRenderer, minerActionTexture, nullptr, &minerActionRect);
        SDL_DestroyTexture(minerActionTexture);
    }

    static float hookAngle = 0.0f;
    static bool hookMovingDown = true;
    const float hookRotationSpeed = 0.25f;
    const int hookYOffset = 125;
    const int hookRange = 70;

    if (hookMovingDown) {
        hookAngle += hookRotationSpeed;
        if (hookAngle >= hookRange) {
            hookMovingDown = false;
        }
    }
    else {
        hookAngle -= hookRotationSpeed;
        if (hookAngle <= -hookRange) {
            hookMovingDown = true;
        }
    }

    SDL_Texture* hookTexture = loadTexture("hook.png");
    if (hookTexture != nullptr) {
        SDL_Rect hookRect = { 330, hookYOffset, 50, 50 };
        SDL_Point center = { 25, 0 }; // Điểm tâm của hình ảnh hook.png (dọc theo trục y)
        SDL_RenderCopyEx(gRenderer, hookTexture, nullptr, &hookRect, hookAngle, &center, SDL_FLIP_NONE);
        SDL_DestroyTexture(hookTexture);
    }

    SDL_RenderPresent(gRenderer);
}

int main(int argc, char* argv[]) {
    if (!init()) {
        std::cout << "Failed to initialize!" << std::endl;
        return 1;
    }

    gStartBgTexture = loadTexture("startBg.jpg");
    if (gStartBgTexture == nullptr) {
        std::cout << "Failed to load startBg.jpg!" << std::endl;
        close();
        return 1;
    }

    gStartButtonTexture = loadTexture("startButton.png");
    if (gStartButtonTexture == nullptr) {
        std::cout << "Failed to load startButton.png!" << std::endl;
        close();
        return 1;
    }

    gGameBgTexture = loadTexture("gameBg.png");
    if (gGameBgTexture == nullptr) {
        std::cout << "Failed to load gameBg.png!" << std::endl;
        close();
        return 1;
    }

    while (gState != GAME) {
        handleEvents();
        drawMenu();
    }

    while (gState == GAME) {
        handleEvents();
        update();
        drawGame();
    }

    close();
    return 0;
}
