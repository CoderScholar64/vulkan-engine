#include "u_read.h"

#include "SDL_rwops.h"
#include "SDL_log.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

uint8_t* u_read_file(const char *const pFilepath, int64_t *pFileSize) {
    *pFileSize = 0;

    SDL_RWops* pFile = SDL_RWFromFile(pFilepath, "r");

    if(pFile == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Cannot open binary \"%s\" due to %s", pFilepath, SDL_GetError());
        return NULL;
    }

    int64_t size = SDL_RWseek(pFile, 0, RW_SEEK_END);

    if(size <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "u_read_file: Cannot read binary \"%s\".", pFilepath);
        SDL_RWclose(pFile);
        return NULL;
    }

    uint8_t *pData = malloc(size * sizeof(uint8_t));

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

void* u_qoi_read(const char *const pUTF8Filepath, qoi_desc *pDesc, int channels) {
    int64_t fileSize;
    uint8_t *pData = u_read_file(pUTF8Filepath, &fileSize);

    if(pData == NULL)
        return NULL;

    void *pPixelData = qoi_decode(pData, fileSize, pDesc, channels);

    free(pData);

    return pPixelData;
}

static void* cgltfAllocFunc(void* user, cgltf_size size);
static void cgltfFreeFunc(void* user, void* ptr);
static cgltf_result cgltfFileRead(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data);
static void cgltfFileRelease(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, void* data);

cgltf_data* u_gltf_read(const char *const pUTF8Filepath, cgltf_result *pCGLTFResult) {
    cgltf_options options = {0};
    options.memory.alloc_func = cgltfAllocFunc;
    options.memory.free_func  = cgltfFreeFunc;
    options.file.read = cgltfFileRead;
    options.file.release = cgltfFileRelease;

    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, pUTF8Filepath, &data);

    if(pCGLTFResult != NULL)
        *pCGLTFResult = result;

    if (result == cgltf_result_success)
        return data;
    return NULL;
}

static void* cgltfAllocFunc(void* user, cgltf_size size) {
    return malloc(size);
}

static void cgltfFreeFunc(void* user, void* ptr) {
    free(ptr);
}

static cgltf_result cgltfFileRead(const struct cgltf_memory_options* memoryOptions, const struct cgltf_file_options* fileOptions, const char* path, cgltf_size* size, void** data) {
    int64_t fileSize;
    uint8_t *pData = u_read_file(path, &fileSize);

    if(pData == NULL)
        return cgltf_result_io_error;

    *size = fileSize;
    *data = pData;

    return cgltf_result_success;
}

static void cgltfFileRelease(const struct cgltf_memory_options* memoryOptions, const struct cgltf_file_options* fileOptions, void* data) {
    cgltfFreeFunc(NULL, data);
}
