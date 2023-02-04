#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>


#include <SDL/SDL.h>

#include "draw.h"


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

enum {
    kScreenWidth = 800,
    kScreenHeight = 600,
    kBitDepth = 32,
    checkerSize = 16,
};

enum {
    NUM_BALLS = 7,
    MAX_SPEED = 5,
    MIN_R = 25,
    MAX_R = 60,
    MAX_FRAMES = -1,
    FRAME_DELAY_MS = 25,
};

// these are in RGBA (hi to lo byte) order.
const Pixel kColors[] = {
    // matchy palette.
    0xE2275Eff,
    0x752A2Eff,
    0x83B23Dff,
    0xD1100Fff,
    0xDD651Bff,

    // primary colors
    0xff0000ff,
    0x00ff00ff,
    0x0000ffff,
    0xff00ffff,
    0xffffffff,
    0x808080ff
};


typedef struct Ball_t {
    int32_t r, x, y, dx, dy;
    uint32_t color;
} Ball;

typedef struct GameState_t {
    LSBool finished;
    Ball balls[NUM_BALLS];
    SDL_Surface* screen;
    GraphicsBuffer* buffer;
    Uint32 backgroundColor1;
    Uint32 backgroundColor2;
    double currentTime, lastFrameTime;
    uint32_t frameCount;
} GameState;


static GameState stateRecord;

int32_t RandRange(int32_t min, int32_t max)
{
    int32_t range = max - min, result;
    assert(range > 0);

    result = rand() % range + min;
    return result;
}

void NewBall(Ball* ball, int32_t screenWidth, int32_t screenHeight)
{
    assert(ball != NULL);
    ball->r = RandRange(25, 60);

    // x and y position
    ball->x = RandRange(ball->r, screenWidth - ball->r - 1);
    ball->y = RandRange(ball->r, screenHeight - ball->r - 1);
    
    // delta x and y, the speed
    ball->dx = RandRange(- MAX_SPEED, MAX_SPEED);
    ball->dy = RandRange(- MAX_SPEED, MAX_SPEED);
    
    int nColors = sizeof(kColors) / sizeof(kColors[0]);
    assert(nColors > 0);
    int colorIdx = RandRange(0, nColors);
    assert(colorIdx >= 0 && colorIdx < nColors);
    ball->color = kColors[colorIdx];
}

void DrawBall(GameState* state, Ball* ball)
{
    uint8_t r, g, b, a;
    a = 255; // opaque
    PixelComponents(ball->color, &r, &g, &b);
    
    uint32_t fillColor = SDL_MapRGBA(state->screen->format, r, g, b, a);
    FillCircle(state->buffer,fillColor,  ball->x, ball->y, ball->r);
    DrawCircle(state->buffer,0xffffffff,  ball->x, ball->y, ball->r);
}

void PrintBall(int idx, Ball* ball)
{
    assert(ball);
    printf("%d: x = %d, y = %d, r = %d, dx = %d, dy = %d, c=%x\n",
         idx, ball->x, ball->y, ball->r, ball->dx, ball->dy, ball->color);
}

void MoveBall(Ball* ball, int32_t screenWidth, int32_t screenHeight)
{
    int32_t oldx = ball->x;
    int32_t oldy = ball->y;

    // Make the balls bounce off the walls by changing directions at the edges
    if(oldx + ball->r >= screenWidth || oldx - ball->r < 0) {
        ball->dx = -ball->dx;
    }
    if(oldy + ball->r >= screenHeight || oldy - ball->r < 0) {
        ball->dy = -ball->dy;
    }

    ball->x = oldx + ball->dx;
    ball->y = oldy + ball->dy;
}

void DrawBackground(GameState* state)
{
    assert(state);
    assert(state->buffer);
    for(int row=0; row < state->buffer->height; row++) {
        int yChk = (row / checkerSize) % 2;

        for(int col=0; col  < state->buffer->width; col++) {
            int xChk = (col / checkerSize) % 2;
            Uint32 color = (xChk == yChk) ? state->backgroundColor1 : state->backgroundColor2;

            state->buffer->ptr[row * state->buffer->width + col] = color;
        }
    }
}

void DrawCenterColumn(GameState* state)
{
    const int32_t colWidth = 100;
    const int32_t colHeight = 75;
    const int32_t thickness = 10;

    int32_t xc = state->buffer->width / 2;
    int32_t yc = state->buffer->height / 2;
    int32_t left = xc - colWidth / 2;
    int32_t top = yc - colHeight / 2;
    int32_t right = left + colHeight;
    int32_t bottom = top + colHeight;

    DrawRect(state->buffer, 0x80ff80ff,
        left, top, right, bottom );
    FillRectOpaque(state->buffer, 0xffffffff,
        left, top, right, bottom );
    FillRectOpaque(state->buffer, 0x808080ff,
        left+thickness, top+thickness, right-thickness, bottom-thickness );
}

void GameLoop(GameState* state)
{   
    SDL_Event event;

    GraphicsBuffer* buffer = state->buffer;
    assert(buffer);

    if(SDL_MUSTLOCK(state->screen ))
        SDL_LockSurface(state->screen );

    DrawBackground(state);
    // DrawCenterColumn(state);

    for(int j=0; j < NUM_BALLS; j++) {
        Ball* ball = state->balls + j;
        DrawBall(state, ball);
    }
    
    if(!state->finished) {
        if(state->currentTime - state->lastFrameTime >= FRAME_DELAY_MS && (MAX_FRAMES == -1 || state->frameCount < MAX_FRAMES)) {
            for(int j=0; j < NUM_BALLS; j++) {
                MoveBall(state->balls + j, kScreenWidth, kScreenHeight);
            }

            ++state->frameCount;
            state->lastFrameTime = state->currentTime;
        }
    }

    if(SDL_MUSTLOCK(state->screen ))
        SDL_UnlockSurface(state->screen );

    SDL_Flip(state->screen );

    if(SDL_PollEvent(&event)) {
        switch(event.type){
            case SDL_KEYDOWN:
                if(state->finished) {
                    printf("Key Pressed. Restarting!\n");
                    state->finished = 0;
                } else {
                    printf("Key Pressed. Finished!\n");
                    state->finished = 1;
                }            
                break;    
            default:
                break; // nothing to do....
                // printf("some other event\n");
        }
    }
}

EM_BOOL FrameCallback(double time, void* userData) 
{
    GameState* state = (GameState*)userData;
    state->currentTime = time;
    GameLoop(state);

    return EM_TRUE;
}

int main(int argc, const char** argv)
{
    srand(time(NULL));

#ifdef __EMSCRIPTEN__
    EM_ASM( main() );
#endif

    SDL_Init(SDL_INIT_VIDEO);

    GameState* state = &stateRecord;
    memset((void*)state, 0, sizeof(GameState));

    state->screen = SDL_SetVideoMode(kScreenWidth, kScreenHeight, kBitDepth, SDL_DOUBLEBUF | SDL_SWSURFACE);
    assert(state->screen);

    // Initialize graphics state
    state->buffer = NewGraphBuffer( 
        (Pixel*)state->screen->pixels,
        kScreenWidth, kScreenHeight, kScreenWidth,
        kScreenHeight * kScreenWidth * 4 );
    assert(state->buffer);

    // Create random balls.
    for(int j=0; j < NUM_BALLS; j++) {
        NewBall(state->balls + j, kScreenWidth, kScreenHeight);
        // PrintBall(j, state->balls + j);
    }

    int alpha = 255;
    state->backgroundColor1 = SDL_MapRGBA(state->screen->format, 0, 0, 80, alpha);
    state->backgroundColor2 = SDL_MapRGBA(state->screen->format, 40, 40, 40, alpha);

#ifdef __EMSCRIPTEN__
    // NOTE: use this instead for more flexibility: emscripten_set_main_loop
    emscripten_request_animation_frame_loop(FrameCallback, (void*)state);
#else
    while (!state->finished) {
        state->currentTime  = SDL_GetTicks();
        GameLoop(state);
        SDL_Delay(FRAME_DELAY_MS);
    }
#endif

#ifndef __EMSCRIPTEN__
    DeleteGraphBuffer(state->buffer);
    SDL_Quit();
#endif

    return 0;
}

// this is for use for debugging from javascript.
void NumString(const char* str, int num)
{
    printf("%s: %d\n", str == NULL ? "(null)" : str, num);
}
