#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int resourceCount;
    int *resources;
    int customerCount;
    int **maximum;
} System;

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

static void free_matrix(int **matrix, int rows) {
    if (matrix == NULL) {
        return;
    }

    for (int row = 0; row < rows; row++) {
        free(matrix[row]);
    }

    free(matrix);
}

static bool parse_customer_line(char *line, int resourceCount, int *row) {
    int count = 0;
    char *token = strtok(line, ",\r\n");

    while (token != NULL) {
        if (count >= resourceCount) {
            return false;
        }

        char *end = NULL;
        errno = 0;
        long value = strtol(token, &end, 10);

        if (end == token || *end != '\0' || errno == ERANGE || value < 0 || value > INT_MAX) {
            return false;
        }

        row[count] = (int)value;
        count++;
        token = strtok(NULL, ",\r\n");
    }

    return count == resourceCount;
}

static void read_customers(System *system) {
    FILE *file = fopen("customer.txt", "r");

    if (file == NULL) {
        fprintf(stderr, "Error: could not open customer.txt\n");
        exit(EXIT_FAILURE);
    }

    char line[4096];
    int lineNumber = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;
        int *row = malloc((size_t)system->resourceCount * sizeof(int));

        if (row == NULL) {
            fprintf(stderr, "Error: memory allocation failed\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        if (!parse_customer_line(line, system->resourceCount, row)) {
            fprintf(stderr, "Error: customer.txt line %d has an invalid format\n", lineNumber);
            free(row);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        int **newMaximum = realloc(system->maximum, (size_t)(system->customerCount + 1) * sizeof(int *));

        if (newMaximum == NULL) {
            fprintf(stderr, "Error: memory allocation failed\n");
            free(row);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        system->maximum = newMaximum;
        system->maximum[system->customerCount] = row;
        system->customerCount++;
    }

    if (ferror(file)) {
        fprintf(stderr, "Error: could not read customer.txt\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);

    if (system->customerCount == 0) {
        fprintf(stderr, "Error: customer.txt is empty\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    System system = {0};
    system.resources = read_resources(argc, argv, &system.resourceCount);
    read_customers(&system);

    free_matrix(system.maximum, system.customerCount);
    free(system.resources);
    return EXIT_SUCCESS;
}
