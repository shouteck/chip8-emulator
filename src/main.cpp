#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdio>
#include "chip8.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// top of main.cpp, above main():
int mapKey(SDL_Keycode k) {
    switch (k) {
        case SDLK_1: return 0x1;  case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;  case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;  case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;  case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;  case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;  case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;  case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;  case SDLK_v: return 0xF;
        default: return -1;
    }
}

// Everything one frame needs, bundled so the browser version can
// hand it to a callback the way the native while-loop does.
struct App {
    Chip8 chip8;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    uint32_t pixels[64 * 32] = {};
    bool running = true;
};

// One frame: pump input events, run instructions, tick timers, render.
static void frame(App& app) {
    if (!app.running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            app.running = false;
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            SDL_Keycode sym = e.key.keysym.sym;
            if (sym == SDLK_ESCAPE) {
                app.running = false;
            }
            else {
                int k = mapKey(sym);
                if (k >= 0) {
                    if (e.type == SDL_KEYDOWN) {
                        app.chip8.keyDown(k);
                    } else {
                        app.chip8.keyUp(k);
                    }
                }
            }
        }
    }

    for (int i = 0; i < 10; ++i) {
        app.chip8.cycle();
    }
    app.chip8.tickTimers();

    for (int i = 0; i < 64 * 32; ++i) {
        app.pixels[i] = app.chip8.display[i] ? 0xFFFFFFFF : 0xFF000000;
    }
    SDL_UpdateTexture(app.texture, nullptr, app.pixels, 64 * sizeof(uint32_t));

    SDL_SetRenderDrawColor(app.renderer, 20, 20, 30, 255);
    SDL_RenderClear(app.renderer);
    SDL_RenderCopy(app.renderer, app.texture, nullptr, nullptr);
    SDL_RenderPresent(app.renderer);
}

#ifdef __EMSCRIPTEN__
// emscripten_set_main_loop_arg needs a plain void(*)(void*) — this
// unpacks the App pointer and forwards to frame().
static void frameAdapter(void* arg) {
    frame(*static_cast<App*>(arg));
}
#endif

int main(int argc, char** argv) {
    const char* rom = (argc > 1) ? argv[1] : "roms/Pong.ch8";

    App app;
    if (!app.chip8.loadRom(rom)) {
        std::fprintf(stderr, "Failed to load ROM\n");
        return 1;
    }
    /*
    std::printf("First 8 ROM bytes: ");
    for (int i = 0; i < 8; ++i) {
        std::printf("%02X ", app.chip8.peek(0x200 + i));
    }
    std::printf("\n");
    */

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 320, SDL_WINDOW_SHOWN);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    app.renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    app.texture = SDL_CreateTexture(
        app.renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, 64, 32);
    if (!app.texture) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(app.renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

#ifdef __EMSCRIPTEN__
    // The browser owns the frame loop: frameAdapter runs once per
    // requestAnimationFrame (~60fps -> tickTimers stays at 60Hz).
    emscripten_set_main_loop_arg(frameAdapter, &app, 0, true);
#else
    while (app.running) {
        frame(app);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(app.texture);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}
