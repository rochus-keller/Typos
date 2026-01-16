#ifndef LUAOTFLOAD_IO_H
#define LUAOTFLOAD_IO_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the Kpathsea library wrapper.
 */
void luaotfload_io_init(void);

/**
 * @brief Resolve a filename using Kpathsea.
 * 
 * @param filename The name of the file (e.g., "lmroman10-regular.otf").
 * @param format The kpse format type (usually kpse_opentype_format or kpse_truetype_format).
 * @return Malloc'd string containing the absolute path, or NULL if not found.
 */
char *luaotfload_io_find_file(const char *filename);

/**
 * @brief Check if a file exists on the filesystem.
 */
bool luaotfload_io_file_exists(const char *path);

/**
 * @brief Read the entire contents of a file into a buffer.
 * Useful for reading luaotfload-names.lua.
 * 
 * @param path Absolute path to the file.
 * @param size Output pointer for the file size.
 * @return Pointer to the buffer (must be freed), or NULL on error.
 */
void *luaotfload_io_read_file(const char *path, size_t *size);

#endif // LUAOTFLOAD_IO_H

