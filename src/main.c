#include "context.h"

#include "v_init.h"
#include "v_render.h"

#include "SDL.h"


struct Context context = {"Hello World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN, NULL, 0, {0, 0, -2}};

void loop() {
    VEngineResult vResult;
    SDL_Event event;
    int run = 1;
    int isWindowMinimized = 0;
    Uint64 currentTime = SDL_GetTicks64();
    Uint64 lastTime = currentTime;
    float delta = 0;

    const static Vector3 yAxis = {0.0f, 1.0f, 0.0f};
    const static Vector3 zAxis = {0.0f, 0.0f, 1.0f};

    while(run) {
        currentTime = lastTime;
        lastTime = SDL_GetTicks64();
        delta = (lastTime - currentTime) * 0.001;

        context.time += delta;

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_QUIT:
                run = 0;
                break;
            case SDL_WINDOWEVENT:
                switch(event.window.event) {
                case SDL_WINDOWEVENT_MINIMIZED:
                    isWindowMinimized = 1;
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    isWindowMinimized = 0;
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    context.forceSwapChainRegen = 1;
                    break;
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION:
                context.yaw   += event.motion.xrel * delta;
                context.pitch += event.motion.yrel * delta;

                context.yaw   = Wrap(context.yaw,   -PI, PI);
                context.pitch = Wrap(context.pitch, -PI, PI);

                Quaternion quaterion = QuaternionMultiply(QuaternionMultiply(QuaternionFromAxisAngle(zAxis, PI / 2.0), QuaternionFromAxisAngle(yAxis, context.pitch)), QuaternionFromAxisAngle(zAxis, context.yaw));
                quaterion = QuaternionNormalize(quaterion);
                context.modelView = MatrixMultiply(MatrixTranslate(context.position.x, context.position.y, context.position.z), QuaternionToMatrix(quaterion));
                break;
            default:
                break;
            }
        }

        if(!isWindowMinimized) {
            vResult = v_draw_frame(&context, delta);

            if(vResult.type < 0)
                return;
        }
    }
}

int main(int argc, char **argv) {
    VEngineResult returnCode;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed with %s", SDL_GetError());
        return -1;
    }

    context.pWindow = SDL_CreateWindow(context.title, context.x, context.y, context.w, context.h, context.flags);

    if(context.pWindow == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed with %s", SDL_GetError());
        return -2;
    }

    returnCode = v_init(&context);

    const static Vector3 yAxis = {0.0f, 1.0f, 0.0f};
    const static Vector3 zAxis = {0.0f, 0.0f, 1.0f};

    context.yaw   = -PI / 2.0;
    context.pitch =  PI / 2.0;

    Quaternion quaterion = QuaternionMultiply(QuaternionMultiply(QuaternionFromAxisAngle(zAxis, PI / 2.0), QuaternionFromAxisAngle(yAxis, context.pitch)), QuaternionFromAxisAngle(zAxis, context.yaw));
    quaterion = QuaternionNormalize(quaterion);
    context.modelView = MatrixMultiply(MatrixTranslate(context.position.x, context.position.y, context.position.z), QuaternionToMatrix(quaterion));

    if( returnCode.type < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Thus v_init() failed with SDL code %s, or return code %i point %i", SDL_GetError(), returnCode.type, returnCode.point);
    }
    else {
        loop();
    }

    v_deinit(&context);
    SDL_DestroyWindow(context.pWindow);

    return returnCode.type;
}
