#include "u_read.h"

#include "SDL_rwops.h"
#include "SDL_log.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

uint8_t* u_read_file(const char *const pUTF8Path, int64_t *pSizeOfFile) {
    *pSizeOfFile = 0;

    SDL_RWops* pRead = SDL_RWFromFile(pUTF8Path, "r");

    if(pRead == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Cannot open binary \"%s\" due to %s", pUTF8Path, SDL_GetError());
        return NULL;
    }

    int64_t size = SDL_RWseek(pRead, 0, RW_SEEK_END);

    if(size <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Cannot read binary \"%s\".", pUTF8Path);
        SDL_RWclose(pRead);
        return NULL;
    }

    uint8_t *pData = malloc(size * sizeof(uint8_t));

    if(pData == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Failed to allocate data for \"%s\" with size %li.", pUTF8Path, size);
        SDL_RWclose(pRead);
        return NULL;
    }

    if(SDL_RWseek(pRead, 0, RW_SEEK_SET) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Failed to get back to position for \"%s\" with size %li.", pUTF8Path, size);
        SDL_RWclose(pRead);
        free(pData);
        return NULL;
    }

    if(SDL_RWread(pRead, pData, size, 1) <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Failed to read buffer for \"%s\" with size %li.", pUTF8Path, size);
        SDL_RWclose(pRead);
        free(pData);
        return NULL;
    }

    SDL_RWclose(pRead);
    *pSizeOfFile = size;
    return pData;
}

void* u_read_qoi(const char *const pUTF8Path, qoi_desc *pDesc, int channels) {
    int64_t fileSize;
    uint8_t *pData = u_read_file(pUTF8Path, &fileSize);

    if(pData == NULL)
        return NULL;

    void *pPixelData = qoi_decode(pData, fileSize, pDesc, channels);

    free(pData);

    return pPixelData;
}
