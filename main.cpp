#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <cmath>
#include <SDL_ttf.h>
#include <chrono>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

enum GameState {
    MENU,
    GAME,
    WIN_GAME,
    LOSE_GAME
};

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gStartBgTexture = nullptr;
SDL_Texture* gStartButtonTexture = nullptr;
SDL_Texture* gGameBgTexture = nullptr;
SDL_Texture* gHookTexture = nullptr;
TTF_Font* font = nullptr;
GameState gState = MENU;
bool hookMovingDown = true;
bool hookStopped = false;
bool hookReturning = false;
int goldX = 500;
int goldY = 300;
bool goldReachedDestination = false; // Thêm biến cờ để kiểm tra khi gold.png đã đạt đến vị trí mong muốn
int diamondX = 350;
int diamondY = 400;
bool diamondReachedDestination = false;
int bigStoneX = 325;
int bigStoneY = 275;
bool bigStoneReachedDestination = false;
int smallStoneX = 600;
int smallStoneY = 350;
bool smallStoneReachedDestination = false;
long long score = 0;
long long goal = 3;
const int INITIAL_TIME = 20; // Initial time in seconds
int remainingTime = INITIAL_TIME;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf không thể khởi tạo! Lỗi SDL_ttf: " << TTF_GetError() << std::endl;
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

    font = TTF_OpenFont("VeraMoBd.ttf", 28); // Thay "your_font.ttf" bằng đường dẫn của font bạn muốn sử dụng
    if (font == nullptr) {
        std::cout << "Không thể tải font! Lỗi SDL_ttf: " << TTF_GetError() << std::endl;
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
    SDL_DestroyTexture(gHookTexture);
    gStartBgTexture = nullptr;
    gStartButtonTexture = nullptr;
    gGameBgTexture = nullptr;
    gHookTexture = nullptr;
    if (font != nullptr) {
        TTF_CloseFont(font);
        font = nullptr;
    }

    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = nullptr;
    gWindow = nullptr;

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            gState = MENU;
        } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            if (gState == MENU) {
                if (x >= 100 && x <= 300 && y >= 100 && y <= 200) {
                    gState = GAME;
                }
            }
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) {
                hookStopped = true;
            }
        }
    }
}

float hookAngle = 0.0f;
const float hookRotationSpeed = 0.25f;
const int hookYOffset = 125;
const int hookRange = 70;
const int hookStartX = 330 + 25;
const int hookStartY = 125 + 25;
int hookCenterX = hookStartX;
int hookCenterY = hookStartY;

bool checkCollision() {
    // Check for collision with an object at (500, 300) of size (50, 50)
    if (hookCenterX >= 500 && hookCenterX <= 550 && hookCenterY >= 300 && hookCenterY <= 350 && goldReachedDestination == false) {
        return true;
    }

    if (hookCenterX >= 350 && hookCenterX <= 400 && hookCenterY >= 400 && hookCenterY <= 450 && diamondReachedDestination == false) {
        return true;
    }

    if (hookCenterX >= 325 && hookCenterX <= 375 && hookCenterY >= 275 && hookCenterY <= 325 && bigStoneReachedDestination == false) {
        return true;
    }

    if (hookCenterX >= 600 && hookCenterX <= 650 && hookCenterY >= 350 && hookCenterY <= 400 && smallStoneReachedDestination == false) {
        return true;
    }
    // Check for collision with screen edges
    if (hookCenterX < -25 || hookCenterX > SCREEN_WIDTH-50 || hookCenterY < -25 || hookCenterY > SCREEN_HEIGHT-50) {
        return true;
    }
    return false;
}

void update() {

    static auto lastTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastTime).count() >= 1) {
        lastTime = currentTime;
        remainingTime--; // Corrected decrement operation
    }

    if (!hookStopped && !hookReturning) {
        if (hookMovingDown) {
            hookAngle += hookRotationSpeed;
            if (hookAngle >= hookRange) {
                hookMovingDown = false;
            }
        } else {
            hookAngle -= hookRotationSpeed;
            if (hookAngle <= -hookRange) {
                hookMovingDown = true;
            }
        }
    } else if (hookStopped) {
        float radAngle = (hookAngle + 90) * M_PI / 180.0;
        float directionX = cos(radAngle);
        float directionY = sin(radAngle);
        int hookSpeed = 2.0;
        hookCenterX += directionX * hookSpeed;
        hookCenterY += directionY * hookSpeed;

        if (checkCollision() || hookCenterX < 20 || hookCenterX > SCREEN_WIDTH - 20 || hookCenterY < 0 || hookCenterY > SCREEN_HEIGHT - 20) {
            hookReturning = true;
            hookStopped = false;
        }
    } else if (hookReturning) {
        long long hookSpeed = 1.25;
        if (hookCenterX > hookStartX) {
            hookCenterX -= hookSpeed;
        } else if (hookCenterX < hookStartX) {
            hookCenterX += hookSpeed;
        }

        if (hookCenterY > hookStartY) {
            hookCenterY -= hookSpeed;
        } else if (hookCenterY < hookStartY) {
            hookCenterY += hookSpeed;
        }

        if (abs(hookCenterX - hookStartX) < hookSpeed && abs(hookCenterY - hookStartY) < hookSpeed) {
            hookCenterX = hookStartX;
            hookCenterY = hookStartY;
            hookReturning = false;
            hookMovingDown = true;
            hookAngle = 0.0f;
        }
    }
    if (hookCenterX >= goldX && hookCenterX <= goldX + 50 && hookCenterY >= goldY && hookCenterY <= goldY + 50) {
        // Di chuyển gold.png về vị trí mong muốn (335, 150)
        int destinationX = 335;
        int destinationY = 150;
        int speed = 1.25; // Tốc độ di chuyển của gold.png

        // Xác định hướng di chuyển của gold.png
        int dx = destinationX - goldX;
        int dy = destinationY - goldY;
        float distance = sqrt(dx * dx + dy * dy);

        // Tính toán vận tốc của gold.png
        float moveX = dx / distance * speed;
        float moveY = dy / distance * speed;

        // Cập nhật vị trí của gold.png
        if (distance > speed) {
            goldX += moveX;
            goldY += moveY;
        } else {
            // Nếu khoảng cách nhỏ hơn tốc độ, di chuyển gold.png đến vị trí đích cuối cùng
            goldX = destinationX;
            goldY = destinationY;
            if (!goldReachedDestination) {
            score += 2; // Tăng điểm lên 1 đơn vị
            goldReachedDestination = true; // Đánh dấu là gold.png đã đạt đến vị trí mong muốn
        }
        }
    }

    // Di chuyển viên kim cương nếu chưa đạt đến vị trí mong muốn
    if (hookCenterX >= diamondX && hookCenterX <= diamondX + 50 && hookCenterY >= diamondY && hookCenterY <= diamondY + 50) {
        int destinationX = 335; // Vị trí mong muốn của kim cương trên trục X
        int destinationY = 150; // Vị trí mong muốn của kim cương trên trục Y
        int speed = 1.25; // Tốc độ di chuyển của kim cương

        // Xác định hướng di chuyển của kim cương
        int dx = destinationX - diamondX;
        int dy = destinationY - diamondY;
        float distance = sqrt(dx * dx + dy * dy);

        // Tính toán vận tốc của kim cương
        float moveX = dx / distance * speed;
        float moveY = dy / distance * speed;

        // Cập nhật vị trí của kim cương
        if (distance > speed) {
            diamondX += moveX;
            diamondY += moveY;
        } else {
            // Nếu khoảng cách nhỏ hơn tốc độ, di chuyển kim cương đến vị trí đích cuối cùng
            diamondX = destinationX;
            diamondY = destinationY;
            if (!diamondReachedDestination) {
            score += 3; // Tăng điểm lên 1 đơn vị
            diamondReachedDestination = true;
        }
        }
    }

   // Di chuyển bigStone.png nếu chưa đạt đến vị trí mong muốn
    if (hookCenterX >= bigStoneX && hookCenterX <= bigStoneX + 50 && hookCenterY >= bigStoneY && hookCenterY <= bigStoneY + 50) {
        int destinationX = 335; // Vị trí mong muốn của bigStone.png trên trục X
        int destinationY = 150; // Vị trí mong muốn của bigStone.png trên trục Y
        int speed = 1.25; // Tốc độ di chuyển của bigStone.png

        // Xác định hướng di chuyển của bigStone.png
        int dx = destinationX - bigStoneX;
        int dy = destinationY - bigStoneY;
        float distance = sqrt(dx * dx + dy * dy);

        // Tính toán vận tốc của bigStone.png
        float moveX = dx / distance * speed;
        float moveY = dy / distance * speed;

        // Cập nhật vị trí của bigStone.png
        if (distance > speed) {
            bigStoneX += moveX;
            bigStoneY += moveY;
        } else {
            // Nếu khoảng cách nhỏ hơn tốc độ, di chuyển bigStone.png đến vị trí đích cuối cùng
            bigStoneX = destinationX;
            bigStoneY = destinationY;
            if (!bigStoneReachedDestination) {
            score ++; // Tăng điểm lên 1 đơn vị
            bigStoneReachedDestination = true;
        }
        }
    }

    // Di chuyển bigStone.png nếu chưa đạt đến vị trí mong muốn
    if (hookCenterX >= smallStoneX && hookCenterX <= smallStoneX + 50 && hookCenterY >= smallStoneY && hookCenterY <= smallStoneY + 50) {
        int destinationX = 335; // Vị trí mong muốn của bigStone.png trên trục X
        int destinationY = 150; // Vị trí mong muốn của bigStone.png trên trục Y
        int speed = 1.25; // Tốc độ di chuyển của bigStone.png

        // Xác định hướng di chuyển của bigStone.png
        int dx = destinationX - smallStoneX;
        int dy = destinationY - smallStoneY;
        float distance = sqrt(dx * dx + dy * dy);

        // Tính toán vận tốc của bigStone.png
        float moveX = dx / distance * speed;
        float moveY = dy / distance * speed;

        // Cập nhật vị trí của bigStone.png
        if (distance > speed) {
            smallStoneX += moveX;
            smallStoneY += moveY;
        } else {
            // Nếu khoảng cách nhỏ hơn tốc độ, di chuyển bigStone.png đến vị trí đích cuối cùng
            smallStoneX = destinationX;
            smallStoneY = destinationY;
            if (!smallStoneReachedDestination) {
            score++; // Tăng điểm lên 1 đơn vị
            smallStoneReachedDestination = true;
        }
        }
    }
    if (score >= goal) {
    gState = WIN_GAME;
}
    if (remainingTime == 0) {
    gState = LOSE_GAME;
}
}

void drawMenu() {
    SDL_RenderClear(gRenderer);

    SDL_RenderCopy(gRenderer, gStartBgTexture, nullptr, nullptr);

    float scaleFactor = 1.5;
    int buttonWidth = 200 / scaleFactor;
    int buttonHeight = 100 / scaleFactor;

    SDL_Rect startButtonRect = {140, 150, buttonWidth, buttonHeight};
    SDL_RenderCopy(gRenderer, gStartButtonTexture, nullptr, &startButtonRect);

    SDL_RenderPresent(gRenderer);
}

void drawGame() {
    SDL_RenderClear(gRenderer);

    SDL_RenderCopy(gRenderer, gGameBgTexture, nullptr, nullptr);

    // Kiểm tra biến cờ goldReachedDestination để xác định xem có vẽ gold.png không
    if (!goldReachedDestination) {
        SDL_Texture* goldTexture = loadTexture("gold.png");
        if (goldTexture != nullptr) {
            SDL_Rect goldRect = { goldX, goldY, 50, 50 };
            SDL_RenderCopy(gRenderer, goldTexture, nullptr, &goldRect);
            SDL_DestroyTexture(goldTexture);
        }
    }

    SDL_Texture* diamondTexture = loadTexture("diamond.png");
    if (!diamondReachedDestination) {
        SDL_Texture* diamondTexture = loadTexture("diamond.png");
        if (diamondTexture != nullptr) {
            SDL_Rect diamondRect = { diamondX, diamondY, 50, 50 };
            SDL_RenderCopy(gRenderer, diamondTexture, nullptr, &diamondRect);
            SDL_DestroyTexture(diamondTexture);
        }
    }

    SDL_Texture* bigStoneTexture = loadTexture("bigStone.png");
    if (!bigStoneReachedDestination) {
        SDL_Texture* bigStoneTexture = loadTexture("bigStone.png");
    if (bigStoneTexture != nullptr) {
            SDL_Rect bigStoneRect = {bigStoneX, bigStoneY, 50, 50};
            SDL_RenderCopy(gRenderer, bigStoneTexture, nullptr, &bigStoneRect);
            SDL_DestroyTexture(bigStoneTexture);
        }
    }

    SDL_Texture* smallStoneTexture = loadTexture("smallStone.png");
    if (!smallStoneReachedDestination) {
        SDL_Texture* smallStoneTexture = loadTexture("smallStone.png");
    if (smallStoneTexture != nullptr) {
            SDL_Rect smallStoneRect = {smallStoneX, smallStoneY, 50, 50};
            SDL_RenderCopy(gRenderer, smallStoneTexture, nullptr, &smallStoneRect);
            SDL_DestroyTexture(smallStoneTexture);
        }
    }

    SDL_Texture* minerActionTexture = loadTexture("mineraction.png");
    if (minerActionTexture != nullptr) {
        SDL_Rect minerActionRect = {330, 55, 100, 100};
        SDL_RenderCopy(gRenderer, minerActionTexture, nullptr, &minerActionRect);
        SDL_DestroyTexture(minerActionTexture);
    }

    if (gHookTexture == nullptr) {
        gHookTexture = loadTexture("hook.png");
    }

    if (gHookTexture != nullptr) {
        SDL_Rect hookRect = {hookCenterX - 25, hookCenterY - 25, 50, 50};
        SDL_Point center = { 25, 0 };
        SDL_RenderCopyEx(gRenderer, gHookTexture, nullptr, &hookRect, hookAngle + 10, &center, SDL_FLIP_NONE);
    }

    SDL_Color textColor = {255, 255, 255, 255};
    std::string scoreText = "Score: " + std::to_string(score);
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
    SDL_Rect scoreRect = {10, 10, textSurface->w, textSurface->h};
    SDL_RenderCopy(gRenderer, textTexture, nullptr, &scoreRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    std::string goalText = "Goal: " + std::to_string(goal);
    SDL_Surface* goalSurface = TTF_RenderText_Solid(font, goalText.c_str(), textColor);
    SDL_Texture* goalTexture = SDL_CreateTextureFromSurface(gRenderer, goalSurface);
    SDL_Rect goalRect = {10, 50, goalSurface->w, goalSurface->h};
    SDL_RenderCopy(gRenderer, goalTexture, nullptr, &goalRect);
    SDL_FreeSurface(goalSurface);
    SDL_DestroyTexture(goalTexture);

    std::string timeText = "Time: " + std::to_string(remainingTime);
    SDL_Surface* timeSurface = TTF_RenderText_Solid(font, timeText.c_str(), textColor);
    SDL_Texture* timeTexture = SDL_CreateTextureFromSurface(gRenderer, timeSurface);
    SDL_Rect timeRect = {10, 90, timeSurface->w, timeSurface->h}; // Adjust position as needed
    SDL_RenderCopy(gRenderer, timeTexture, nullptr, &timeRect);
    SDL_FreeSurface(timeSurface);
    SDL_DestroyTexture(timeTexture);

    if (score >= goal) {
        SDL_Texture* endGameTexture = loadTexture("endGame.png");
        if (endGameTexture != nullptr) {
            SDL_RenderCopy(gRenderer, endGameTexture, nullptr, nullptr);
            SDL_DestroyTexture(endGameTexture);
        }
    }

    if (remainingTime <= 0) {
        SDL_Texture* loseGameTexture = loadTexture("loseGame.png");
        if (loseGameTexture != nullptr) {
            SDL_RenderCopy(gRenderer, loseGameTexture, nullptr, nullptr);
            SDL_DestroyTexture(loseGameTexture);
        }
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

    while (true) {
        handleEvents();
        if (gState == MENU) {
            drawMenu();
        } else if (gState == GAME) {
            update();
            drawGame();
        } else if (gState == WIN_GAME) {
            drawGame(); // Vẽ lại màn hình
        }  else if (gState == LOSE_GAME) {
            drawGame(); // Vẽ lại màn hình
        }

        // Nếu trạng thái trò chơi là END_GAME, không cần cập nhật hoặc xử lý sự kiện nữa
        if (gState != WIN_GAME) {
            // Kiểm tra điều kiện kết thúc trò chơi và thoát khỏi vòng lặp
            if (score == goal) {
                gState = WIN_GAME;
            } else if(remainingTime == 0) {
                gState = LOSE_GAME;
            }
        } else {
            SDL_Delay(1000); // Đợi một giây trước khi thoát khỏi trò chơi
        }
    }

    close();
    return 0;
}
