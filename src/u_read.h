#ifndef READ_UTILITY_29
#define READ_UTILITY_29

#include <stdint.h>

#define QOI_NO_STDIO // Use u_qoi_read instead.
#include "qoi.h"

/**
 * Read a binary file.
 * @warning If this function returns a pointer you are responsiable for freeing the returned pointer.
 * @param pUTF8Filepath The path to where the binary file is. It is encoded with unicode.
 * @param pFileSize The file's size in bytes.
 * @return A valid pointer to the buffer that YOU MUST free() or a null on failure.
 */
uint8_t* u_read_file(const char *const pUTF8Filepath, int64_t *pFileSize);

/**
 * Read an image file and place into into a buffer.
 * @warning If this function returns a pointer you are responsiable for freeing the returned pointer.
 * @param pUTF8Filepath The path to where the binary file is. It is encoded with unicode.
 * @param pDesc Returns a qoi descriptor with width, height, channels and encoding type
 * @param channels Valid values. 0 means alpha is optional, 3 means alpha channel is excluded, 4 means there is always an alpha channel.
 * @return A valid pointer to the buffer that YOU MUST free() or a null on failure.
 */
void* u_read_qoi(const char *const pUTF8Filepath, qoi_desc *pDesc, int channels);

#endif // READ_UTILITY_29
