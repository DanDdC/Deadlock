#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s <resource_count>...\n", program);
}

static bool parse_positive_integer(const char *text, int *value) {
    if (text == NULL || *text == '\0') {
        return false;
    }

    char *end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE) {
        return false;
    }

    if (parsed <= 0 || parsed > INT_MAX) {
        return false;
    }

    *value = (int)parsed;
    return true;
}

static int *read_resources(int argc, char **argv, int *resourceCount) {
    if (argc < 2) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    *resourceCount = argc - 1;
    int *resources = malloc((size_t)*resourceCount * sizeof(int));

    if (resources == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int index = 0; index < *resourceCount; index++) {
        if (!parse_positive_integer(argv[index + 1], &resources[index])) {
            fprintf(stderr, "Error: resource count '%s' must be a positive integer\n", argv[index + 1]);
            free(resources);
            exit(EXIT_FAILURE);
        }
    }

    return resources;
}

int main(int argc, char **argv) {
    int resourceCount = 0;
    int *resources = read_resources(argc, argv, &resourceCount);

    (void)resourceCount;
    free(resources);
    return EXIT_SUCCESS;
}
