#include "libtusk.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: test_libtusk <image_path>\n");
        return 1;
    }

    printf("Testing libtusk library with image: %s\n", argv[1]);
    printf("Input: image path string\n");
    printf("Output: JSON string\n\n");

    // Call the library: string in, string out
    char *json = libtusk_analyze(argv[1]);

    if (!json)
    {
        printf("Error: Failed to analyze image\n");
        return 1;
    }

    printf("=== Success! JSON Report ===\n");
    printf("%s\n", json);

    // Free the string
    libtusk_free(json);

    printf("\n=== Test completed ===\n");
    return 0;
}
