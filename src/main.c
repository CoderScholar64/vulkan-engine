#include "context.h"
#include "v_init.h"

#include "SDL.h"

struct Context context = {"Hello World", 0, 0, 1920, 1080, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN};

void loop() {
    SDL_Event event;
    int run = 1;

    while(run) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                run = 0;
            }
        }

        if(v_draw_frame() < 0)
            return;
    }
}

int main(int argc, char **argv) {
    int returnCode;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed with %s", SDL_GetError());
        return -1;
    }

    context.pWindow = SDL_CreateWindow(context.title, context.x, context.y, context.w, context.h, context.flags);

    if(context.pWindow == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed with %s", SDL_GetError());
        return -2;
    }

    returnCode = v_init();

    if( returnCode < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Thus v_init() failed with code %s", SDL_GetError());
    }
    else {
        loop();
    }

    v_deinit();
    SDL_DestroyWindow(context.pWindow);

    return returnCode;
}
