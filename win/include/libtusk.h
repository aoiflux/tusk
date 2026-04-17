#ifndef LIBTUSK_H
#define LIBTUSK_H

#if defined(LIBTUSK_SHARED)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(LIBTUSK_BUILD_SHARED)
#define LIBTUSK_API __declspec(dllexport)
#else
#define LIBTUSK_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define LIBTUSK_API __attribute__((visibility("default")))
#else
#define LIBTUSK_API
#endif
#else
#define LIBTUSK_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Analyze a disk image and return filesystem report as JSON string.
     * Input: image file path
     * Output: JSON-formatted report (must be freed with libtusk_free)
     *
     * @param image_path Path to the disk image file
     * @return JSON string (caller must free with libtusk_free), or NULL on error
     */
    LIBTUSK_API char *libtusk_analyze(const char *image_path);

    /**
     * Free a string returned by libtusk_analyze.
     *
     * @param ptr Pointer to string to free
     */
    LIBTUSK_API void libtusk_free(char *ptr);

#ifdef __cplusplus
}
#endif

#endif // LIBTUSK_H
