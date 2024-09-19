#include "u_read.h"

#include "SDL_rwops.h"
#include "SDL_log.h"

Uint8* u_read_file(const char *const pFilepath, Sint64 *pFileSize) {
    *pFileSize = 0;

    SDL_RWops* pFile = SDL_RWFromFile(pFilepath, "r");

    if(pFile == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Cannot open binary \"%s\" due to %s", pFilepath, SDL_GetError());
        return NULL;
    }

    Sint64 size = SDL_RWseek(pFile, 0, RW_SEEK_END);

    if(size <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Cannot read binary \"%s\".", pFilepath);
        SDL_RWclose(pFile);
        return NULL;
    }

    Uint8 *pData = malloc(size * sizeof(Uint8));

    if(pData == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Failed to allocate data for \"%s\" with size %li.", pFilepath, size);
        SDL_RWclose(pFile);
        return NULL;
    }

    if(SDL_RWseek(pFile, 0, RW_SEEK_SET) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Failed to get back to position for \"%s\" with size %li.", pFilepath, size);
        SDL_RWclose(pFile);
        free(pData);
        return NULL;
    }

    if(SDL_RWread(pFile, pData, size, 1) <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Failed to read buffer for \"%s\" with size %li.", pFilepath, size);
        SDL_RWclose(pFile);
        free(pData);
        return NULL;
    }

    SDL_RWclose(pFile);
    *pFileSize = size;
    return pData;
}
