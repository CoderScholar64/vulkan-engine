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

    int movement[4] = {0};
    int dirtyModelView = 0;

    while(run) {
        currentTime = lastTime;
        lastTime = SDL_GetTicks64();
        delta = (lastTime - currentTime) * 0.001;

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
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    context.forceSwapChainRegen = 1;
                    break;
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION:
                if(SDL_GetRelativeMouseMode()) {
                    context.yaw   += event.motion.xrel * delta;
                    context.pitch += event.motion.yrel * delta;

                    context.yaw   =  Wrap(context.yaw,   -PI, PI);
                    context.pitch = Clamp(context.pitch,   0, PI);

                    dirtyModelView = 1;
                }
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    break;
                case SDL_SCANCODE_W:
                    movement[0] = 1;
                    break;
                case SDL_SCANCODE_A:
                    movement[1] = 1;
                    break;
                case SDL_SCANCODE_S:
                    movement[2] = 1;
                    break;
                case SDL_SCANCODE_D:
                    movement[3] = 1;
                    break;
                default: // Do nothing
                }
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_W:
                    movement[0] = 0;
                    break;
                case SDL_SCANCODE_A:
                    movement[1] = 0;
                    break;
                case SDL_SCANCODE_S:
                    movement[2] = 0;
                    break;
                case SDL_SCANCODE_D:
                    movement[3] = 0;
                    break;
                default: // Do nothing
                }
                break;
            default:
                break;
            }
        }

        if(movement[0] != movement[2] || movement[1] != movement[3]) {
            Vector2 move = {0};

            move.x = -8.0f * delta * (movement[0] - movement[2]);
            move.y = -8.0f * delta * (movement[1] - movement[3]);

            move = Vector2Rotate(move, -context.yaw);

            context.position.x += move.x;
            context.position.y += move.y;

            dirtyModelView = 1;
        }

        if(dirtyModelView) {
            Quaternion quaterion = QuaternionMultiply(QuaternionMultiply(QuaternionFromAxisAngle(zAxis, PI / 2.0), QuaternionFromAxisAngle(yAxis, context.pitch)), QuaternionFromAxisAngle(zAxis, context.yaw));
            quaterion = QuaternionNormalize(quaterion);
            context.modelView = MatrixMultiply(MatrixTranslate(context.position.x, context.position.y, context.position.z), QuaternionToMatrix(quaterion));
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

    SDL_SetRelativeMouseMode(SDL_TRUE);

    returnCode = v_init(&context);

    const static Vector3 yAxis = {0.0f, 1.0f, 0.0f};
    const static Vector3 zAxis = {0.0f, 0.0f, 1.0f};

    context.yaw   =  0;
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
