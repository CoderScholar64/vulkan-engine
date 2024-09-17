#include "SDL.h"

struct Context {
    char title[64];
    int x, y;
    int w, h;
    Uint32 flags;
    SDL_Window *window_p;
} context = {"Hello World", 0, 0, 1920, 1080, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN};

int main(int argc, char **argv) {
    if(SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed with %s", SDL_GetError());
        return -1;
    }

    context.window_p = SDL_CreateWindow(context.title, context.x, context.y, context.w, context.h, context.flags);

    if(context.window_p == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed with %s", SDL_GetError());
        return -2;
    }

    SDL_Event event;
    int run = 1;

    while(run) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            run = 0;
        }
    }

    SDL_DestroyWindow(context.window_p);

    return 0;
}
