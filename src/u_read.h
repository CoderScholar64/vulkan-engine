#ifndef READ_UTILITY_29
#define READ_UTILITY_29

#include "SDL_stdinc.h"


/**
 * Read a binary file.
 * @warning If this function returns a pointer you are responsiable for freeing it.
 * @param pUnicode8Filepath The path to where the binary file is. It is encoded with unicode.
 * @param pFileSize The file's size in bytes.
 * @return A valid pointer to the buffer that YOU MUST free() or a null on failure.
 */
Uint8* u_read_file(const char *const pUnicode8Filepath, Sint64 *pFileSize);

#endif // READ_UTILITY_29
