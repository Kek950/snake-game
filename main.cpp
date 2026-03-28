// main.cpp
// A complete Win32 application demonstrating interactive UI and a game loop.
// Features: Custom styling, interactive buttons, and a Snake game.

// 1. DEFINE UNICODE: Ensures we use Wide-Character (W) versions of Win32 functions (e.g., TextOutW).
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>

// Link against the Common Controls library
#pragma comment(lib, "comctl32.lib")
#include <vector>
#include <deque>
#include <ctime>
#include <string>

// --- STYLE SHEET ---
// We define our colors and constants here to act as a "theme" for the app.
namespace Style {
    const COLORREF Background     = RGB(32, 33, 36);      // Dark Grey Background
    const COLORREF Text           = RGB(232, 234, 237);   // Off-white Text
    const COLORREF ButtonNormal   = RGB(138, 180, 248);   // Light Blue
    const COLORREF ButtonHover    = RGB(174, 203, 250);   // Lighter Blue on hover
    const COLORREF ButtonText     = RGB(32, 33, 36);      // Dark text for buttons
    const COLORREF SnakeBody      = RGB(76, 175, 80);     // Green
    const COLORREF Food           = RGB(244, 67, 54);     // Red
    const COLORREF GridLine       = RGB(60, 64, 67);      // Subtle grid lines
}

// --- GAME CONSTANTS ---
const int CELL_SIZE = 20;       // Size of one snake block in pixels
const int GRID_WIDTH = 25;      // Number of cells horizontally
const int GRID_HEIGHT = 25;     // Number of cells vertically
const int TIMER_ID = 1;         // ID for our game loop timer

// --- DATA STRUCTURES ---

// Represents a 2D point (x, y)
struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

// Represents a clickable button in our UI
struct Button {
    RECT rect;              // The position and size of the button
    std::wstring text;      // The text to display
    bool isHovered;         // Tracks if mouse is over the button

    // Helper to check if a coordinate is inside the button
    bool IsHit(int x, int y) const {
        return (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom);
    }
};

// --- GLOBAL STATE ---
// In a larger app, these would be in a class, but for a single-file Win32 app, globals are common.
enum AppState { MENU, GAME, GAME_OVER, SETTINGS };
AppState currentState = MENU;

Button playButton = { {150, 180, 350, 230}, L"PLAY SNAKE", false };
Button exitButton = { {150, 250, 350, 300}, L"EXIT", false };
Button settingsExitButton = { {150, 350, 350, 400}, L"EXIT", false };
Button settingsButton = { {150, 320, 350, 370}, L"SETTINGS", false };
Button settingsPlusButton = { {350, 200, 400, 250}, L"+", false };
Button settingsMinusButton = { {100, 200, 150, 250}, L"-", false };

// Snake Game State
std::deque<Point> snake;    // The snake's body (front is head)
Point food;                 // Position of the food
Point direction;            // Current movement direction (x, y)
int snakeSpeed = 100; // Timer interval in milliseconds
int score = 0;        // <--- FIX: Added missing score variable
DWORD lastTurnTime = 0; // Track time of last turn to prevent 180-degree bug

// --- FUNCTION DECLARATIONS ---
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void InitGame();
void UpdateGame();
void DrawGame(HDC hdc);
void DrawMenu(HDC hdc, HWND hwnd);
void SpawnFood();

// --- MAIN ENTRY POINT ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Initialize Common Controls (standard practice for GUI apps)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    if (!InitCommonControlsEx(&icex)) {
        MessageBox(NULL, L"InitCommonControlsEx Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Register the Window Class
    const wchar_t CLASS_NAME[] = L"SnakeGameWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;        // The function that handles messages
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); // Use standard arrow cursor
    wc.hbrBackground = CreateSolidBrush(Style::Background); // Set background color

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"RegisterClass Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Create the Window
    // We calculate the required window size to fit our game grid perfectly
    RECT windowRect = { 0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Native Win32 Snake", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        windowRect.right - windowRect.left, 
        windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        DWORD err = GetLastError();
        std::wstring errMsg = L"CreateWindowEx Failed! Error: " + std::to_wstring(err);
        MessageBox(NULL, errMsg.c_str(), L"Error", MB_ICONERROR);
        return 0;
    }

    // Seed random number generator for food placement
    srand(static_cast<unsigned int>(time(0)));

    // Message Loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// --- GAME LOGIC FUNCTIONS ---

void InitGame() {
    snake.clear();
    snake.push_back({10, 10}); // Start in middle
    snake.push_back({9, 10});
    snake.push_back({8, 10});
    direction = {1, 0}; // Moving right
    score = 0; // Fixed: Now uses the globally declared score
    SpawnFood();
    currentState = GAME;
    lastTurnTime = GetTickCount(); // Reset turn timer
}

void SpawnFood() {
    // Simple logic to place food randomly
    // (In a real game, ensure it doesn't spawn inside the snake)
    food.x = rand() % GRID_WIDTH;
    food.y = rand() % GRID_HEIGHT;
}

void UpdateGame() {
    if (currentState != GAME) return;

    // Calculate new head position
    Point newHead = { snake.front().x + direction.x, snake.front().y + direction.y };

    // 1. Check Wall Collision
    if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
        currentState = GAME_OVER;
        return;
    }

    // 2. Check Self Collision
    for (const auto& segment : snake) {
        if (newHead == segment) {
            currentState = GAME_OVER;
            return;
        }
    }

    // Move Snake
    snake.push_front(newHead); // Add new head

    // 3. Check Food Collision
    if (newHead == food) {
        score += 10; // Fixed: Now uses the globally declared score
        SpawnFood(); // Don't remove tail, so snake grows
    } else {
        snake.pop_back(); // Remove tail to maintain length
    }
}

// --- DRAWING FUNCTIONS ---

// Draws a button with our custom style
void DrawButton(HDC hdc, Button& btn) {
    // Choose color based on hover state
    COLORREF color = btn.isHovered ? Style::ButtonHover : Style::ButtonNormal;
    HBRUSH brush = CreateSolidBrush(color);
    
    // Fill the button rectangle
    FillRect(hdc, &btn.rect, brush);
    DeleteObject(brush);

    // Draw Text
    SetBkMode(hdc, TRANSPARENT); // Transparent background for text
    SetTextColor(hdc, Style::ButtonText);
    DrawText(hdc, btn.text.c_str(), -1, &btn.rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawMenu(HDC hdc, HWND hwnd) {
    // Draw Title
    RECT titleRect = { 0, 50, 500, 150 };
    SetTextColor(hdc, Style::Text);
    SetBkMode(hdc, TRANSPARENT);
    
    // Create a larger font for the title
    HFONT hFont = CreateFont(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    DrawText(hdc, L"SNAKE GAME", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    // Draw Buttons
    DrawButton(hdc, playButton);
    DrawButton(hdc, exitButton);
    DrawButton(hdc, settingsButton);     
}

void DrawGame(HDC hdc) {
    // 1. Draw Game Border
    // We draw a rectangle around the entire grid area
    RECT gameRect = { 0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE };
    HBRUSH borderBrush = CreateSolidBrush(Style::GridLine);
    FrameRect(hdc, &gameRect, borderBrush); // FrameRect draws just the outline
    DeleteObject(borderBrush);

    // 2. Draw Food
    RECT foodRect = { food.x * CELL_SIZE, food.y * CELL_SIZE, 
                      (food.x + 1) * CELL_SIZE, (food.y + 1) * CELL_SIZE };
    // Shrink slightly to see the grid/background between cells
    InflateRect(&foodRect, -2, -2); 
    HBRUSH foodBrush = CreateSolidBrush(Style::Food);
    FillRect(hdc, &foodRect, foodBrush);
    DeleteObject(foodBrush);

    // 3. Draw Snake
    HBRUSH snakeBrush = CreateSolidBrush(Style::SnakeBody);
    for (const auto& segment : snake) {
        RECT segRect = { segment.x * CELL_SIZE, segment.y * CELL_SIZE, 
                         (segment.x + 1) * CELL_SIZE, (segment.y + 1) * CELL_SIZE };
        // Add a small border by shrinking the rect slightly
        InflateRect(&segRect, -1, -1);
        FillRect(hdc, &segRect, snakeBrush);
    }
    DeleteObject(snakeBrush);

    // 4. Draw Score
    std::wstring scoreText = L"Score: " + std::to_wstring(score); // Fixed: Now uses the globally declared score
    SetTextColor(hdc, Style::Text);
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, 10, 10, scoreText.c_str(), scoreText.length());
}

void DrawGameOver(HDC hdc) {
    RECT rect = { 0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE };
    
    SetTextColor(hdc, Style::Food); // Red text
    SetBkMode(hdc, TRANSPARENT);
    
    HFONT hFont = CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    DrawText(hdc, L"GAME OVER", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    // Draw "Click to Menu" below
    RECT subRect = rect;
    subRect.top += 40;
    SetTextColor(hdc, Style::Text);
    DrawText(hdc, L"Click to return to Menu", -1, &subRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void DrawSettings(HDC hdc, HWND hwnd) {
    // Draw Title
    RECT titleRect = { 0, 50, GRID_WIDTH * CELL_SIZE, 100 }; // Title at the top
    SetTextColor(hdc, Style::Food); 
    SetBkMode(hdc, TRANSPARENT);
    
    HFONT hFont = CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    DrawText(hdc, L"SETTINGS", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // CALCULATE A USER-FRIENDLY SPEED LEVEL (1-36)
    // The range is 160ms (Slowest) to 20ms (Fastest). Total range 140ms.
    // (160 - snakeSpeed) / 4 = Level (0-35). Add 1 to make it 1-36.
    int speedLevel = (160 - snakeSpeed) / 4 + 1;
    // Cap it for safety/display
    if (speedLevel < 1) speedLevel = 1;
    if (speedLevel > 36) speedLevel = 36;
    
    // Draw Speed Text - Center it between the +/- buttons
    RECT speedRect = { settingsMinusButton.rect.right + 10, 
                        settingsMinusButton.rect.top, 
                        settingsPlusButton.rect.left - 10, 
                        settingsMinusButton.rect.bottom };

    std::wstring speedText = L"Speed Level: " + std::to_wstring(speedLevel);
    SetTextColor(hdc, Style::Text); // Use normal text color
    DrawText(hdc, speedText.c_str(), -1, &speedRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont); 

    DrawButton(hdc, settingsPlusButton);
    DrawButton(hdc, settingsMinusButton);
    DrawButton(hdc, settingsExitButton);
}

// --- WINDOW PROCEDURE ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Set a timer to trigger WM_TIMER messages every 100ms (10 FPS)
            SetTimer(hwnd, TIMER_ID, snakeSpeed, nullptr);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID);
            PostQuitMessage(0);
            return 0;

        case WM_TIMER:
            if (currentState == GAME) {
                UpdateGame();
                InvalidateRect(hwnd, nullptr, FALSE); // Request redraw
            }
            return 0;

        case WM_MOUSEMOVE: {
            // Handle hover effects for buttons
            if (currentState == MENU) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                
                bool needsRedraw = false;
                if (playButton.isHovered != playButton.IsHit(x, y)) {
                    playButton.isHovered = !playButton.isHovered;
                    needsRedraw = true;
                }
                if (exitButton.isHovered != exitButton.IsHit(x, y)) {
                    exitButton.isHovered = !exitButton.isHovered;
                    needsRedraw = true;
                }
                if (needsRedraw) InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            // Handle Mouse Clicks
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (currentState == MENU) {
                if (playButton.IsHit(x, y)) {
                    InitGame();
                } else if (exitButton.IsHit(x, y)) {
                    PostQuitMessage(0);
                } else if (settingsButton.IsHit(x, y)) {
                    currentState = SETTINGS;
                    InvalidateRect(hwnd, nullptr, TRUE);
                }
            } else if (currentState == SETTINGS) {
                if (settingsExitButton.IsHit(x, y)) {
                    currentState = MENU;
                    InvalidateRect(hwnd, nullptr, TRUE);
                } else if (settingsPlusButton.IsHit(x, y)) {
                    // Clicking '+' makes the snake FASTER (smaller interval)
                    if (snakeSpeed > 20) { // Clamp minimum interval to 20ms
                        snakeSpeed -= 4;
                        
                        // CRUCIAL: Kill and reset the timer with the new speed!
                        KillTimer(hwnd, TIMER_ID);
                        SetTimer(hwnd, TIMER_ID, snakeSpeed, nullptr);
                        
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                } else if (settingsMinusButton.IsHit(x, y)) {
                    // Clicking '-' makes the snake SLOWER (larger interval)
                    if (snakeSpeed < 160) { // Clamp maximum interval to 160ms
                        snakeSpeed += 4;
                        
                        // CRUCIAL: Kill and reset the timer with the new speed!
                        KillTimer(hwnd, TIMER_ID);
                        SetTimer(hwnd, TIMER_ID, snakeSpeed, nullptr);
                        
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                }
            } else if (currentState == GAME_OVER) {
                currentState = MENU;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }

        case WM_KEYDOWN:
            // Handle Keyboard Input
            if (currentState == GAME) {
                // Check delay to prevent 180-degree turns (User requested 0.2s delay)
                DWORD currentTime = GetTickCount();
                if (currentTime - lastTurnTime < 20) break;

                Point newDir = direction;
                switch (wParam) {
                    case VK_UP: 
                        // Only change to UP if not currently moving DOWN
                        if(direction.y != 1) newDir = {0, -1}; 
                        break;
                    case VK_DOWN: 
                        // Only change to DOWN if not currently moving UP
                        if(direction.y != -1) newDir = {0, 1}; 
                        break;
                    case VK_LEFT: 
                        // Only change to LEFT if not currently moving RIGHT
                        if(direction.x != 1) newDir = {-1, 0}; 
                        break;
                    case VK_RIGHT: 
                        // Only change to RIGHT if not currently moving LEFT
                        if(direction.x != -1) newDir = {1, 0}; 
                        break;
                }
                
                // Only update if direction actually changed
                if (!(newDir == direction)) {
                    direction = newDir;
                    lastTurnTime = currentTime;
                }
            }
            return 0;

        case WM_PAINT: { // <-- Start of WM_PAINT block
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Double Buffering to prevent flickering
            // 1. Create an off-screen DC
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            // 2. Fill background
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            HBRUSH bgBrush = CreateSolidBrush(Style::Background);
            FillRect(memDC, &clientRect, bgBrush);
            DeleteObject(bgBrush);

            // 3. Draw content based on state
            if (currentState == MENU) {
                DrawMenu(memDC, hwnd);
            
            } else if (currentState == SETTINGS) {
                DrawSettings(memDC, hwnd);
            
            } else if (currentState == GAME) {
                DrawGame(memDC);
            } else if (currentState == GAME_OVER) {
                DrawGame(memDC); // Draw game state behind game over text
                DrawGameOver(memDC);
            }
            
            // 4. Copy from off-screen DC to screen (Blit/Transfer)
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

            // Cleanup
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        } // <-- End of WM_PAINT block
    } // <-- FIX: Closing brace for the switch (uMsg) statement

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
} // <-- Closing brace for the WindowProc function