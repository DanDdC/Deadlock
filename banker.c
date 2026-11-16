#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    COMMAND_REQUEST,
    COMMAND_RELEASE,
    COMMAND_PRINT
} CommandType;

typedef struct {
    CommandType type;
    int customer;
    int *resources;
} Command;

typedef struct {
    int resourceCount;
    int *resources;
    int customerCount;
    int **maximum;
    int *available;
    int **allocation;
    int **need;
    int commandCount;
    Command *commands;
} System;

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s <resource_count>...\n", program);
}

static bool parse_integer(const char *text, int *value, bool allowZero) {
    if (text == NULL || *text == '\0') {
        return false;
    }

    char *end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE) {
        return false;
    }

    if (parsed < 0 || parsed > INT_MAX) {
        return false;
    }

    if (!allowZero && parsed == 0) {
        return false;
    }

    *value = (int)parsed;
    return true;
}

static bool parse_positive_integer(const char *text, int *value) {
    return parse_integer(text, value, false);
}

static bool parse_nonnegative_integer(const char *text, int *value) {
    return parse_integer(text, value, true);
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

static int **allocate_matrix(int rows, int columns) {
    int **matrix = malloc((size_t)rows * sizeof(int *));

    if (matrix == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int row = 0; row < rows; row++) {
        matrix[row] = malloc((size_t)columns * sizeof(int));

        if (matrix[row] == NULL) {
            fprintf(stderr, "Error: memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }

    return matrix;
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

static bool parse_command_line(char *line, int resourceCount, int customerCount, Command *command) {
    char *token = strtok(line, " \t\r\n");

    if (token == NULL) {
        return false;
    }

    if (strcmp(token, "*") == 0) {
        if (strtok(NULL, " \t\r\n") != NULL) {
            return false;
        }

        command->type = COMMAND_PRINT;
        command->customer = -1;
        command->resources = NULL;
        return true;
    }

    if (strcmp(token, "RQ") == 0) {
        command->type = COMMAND_REQUEST;
    } else if (strcmp(token, "RL") == 0) {
        command->type = COMMAND_RELEASE;
    } else {
        return false;
    }

    token = strtok(NULL, " \t\r\n");
    int customer = 0;

    if (!parse_nonnegative_integer(token, &customer)) {
        return false;
    }

    if (customer >= customerCount) {
        return false;
    }

    int *resources = malloc((size_t)resourceCount * sizeof(int));

    if (resources == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int index = 0; index < resourceCount; index++) {
        token = strtok(NULL, " \t\r\n");

        if (!parse_nonnegative_integer(token, &resources[index])) {
            free(resources);
            return false;
        }
    }

    if (strtok(NULL, " \t\r\n") != NULL) {
        free(resources);
        return false;
    }

    command->customer = customer;
    command->resources = resources;
    return true;
}

static void read_commands(System *system) {
    FILE *file = fopen("commands.txt", "r");

    if (file == NULL) {
        fprintf(stderr, "Error: could not open commands.txt\n");
        exit(EXIT_FAILURE);
    }

    char line[4096];
    int lineNumber = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;
        Command command = {0};

        if (!parse_command_line(line, system->resourceCount, system->customerCount, &command)) {
            fprintf(stderr, "Error: commands.txt line %d has an invalid format\n", lineNumber);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        Command *newCommands = realloc(system->commands, (size_t)(system->commandCount + 1) * sizeof(Command));

        if (newCommands == NULL) {
            fprintf(stderr, "Error: memory allocation failed\n");
            free(command.resources);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        system->commands = newCommands;
        system->commands[system->commandCount] = command;
        system->commandCount++;
    }

    if (ferror(file)) {
        fprintf(stderr, "Error: could not read commands.txt\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);

    if (system->commandCount == 0) {
        fprintf(stderr, "Error: commands.txt is empty\n");
        exit(EXIT_FAILURE);
    }
}

static bool can_finish(const int *need, const int *work, int resourceCount) {
    for (int resource = 0; resource < resourceCount; resource++) {
        if (need[resource] > work[resource]) {
            return false;
        }
    }

    return true;
}

static bool is_safe_state(const System *system) {
    int *work = malloc((size_t)system->resourceCount * sizeof(int));
    bool *finished = calloc((size_t)system->customerCount, sizeof(bool));

    if (work == NULL || finished == NULL) {
        free(work);
        free(finished);
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int resource = 0; resource < system->resourceCount; resource++) {
        work[resource] = system->available[resource];
    }

    int completed = 0;

    while (completed < system->customerCount) {
        bool progress = false;

        for (int customer = 0; customer < system->customerCount; customer++) {
            if (finished[customer]) {
                continue;
            }

            if (!can_finish(system->need[customer], work, system->resourceCount)) {
                continue;
            }

            for (int resource = 0; resource < system->resourceCount; resource++) {
                work[resource] += system->allocation[customer][resource];
            }

            finished[customer] = true;
            completed++;
            progress = true;
        }

        if (!progress) {
            break;
        }
    }

    bool safe = completed == system->customerCount;
    free(work);
    free(finished);
    return safe;
}

static void initialize_state(System *system) {
    system->available = malloc((size_t)system->resourceCount * sizeof(int));

    if (system->available == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int resource = 0; resource < system->resourceCount; resource++) {
        system->available[resource] = system->resources[resource];
    }

    system->allocation = allocate_matrix(system->customerCount, system->resourceCount);
    system->need = allocate_matrix(system->customerCount, system->resourceCount);

    for (int customer = 0; customer < system->customerCount; customer++) {
        for (int resource = 0; resource < system->resourceCount; resource++) {
            system->allocation[customer][resource] = 0;
            system->need[customer][resource] = system->maximum[customer][resource];
        }
    }
}

static bool values_fit(const int *values, const int *limit, int resourceCount) {
    for (int resource = 0; resource < resourceCount; resource++) {
        if (values[resource] > limit[resource]) {
            return false;
        }
    }

    return true;
}

static void print_values(FILE *output, const int *values, int resourceCount) {
    for (int resource = 0; resource < resourceCount; resource++) {
        fprintf(output, " %d", values[resource]);
    }
}

static void print_values_with_trailing_space(FILE *output, const int *values, int resourceCount) {
    for (int resource = 0; resource < resourceCount; resource++) {
        fprintf(output, "%d ", values[resource]);
    }
}

static void process_request(System *system, int customer, const int *request, FILE *output) {
    if (!values_fit(request, system->need[customer], system->resourceCount)) {
        fprintf(output, "The customer %d request", customer);
        print_values(output, request, system->resourceCount);
        fprintf(output, " was denied because exceed its maximum allocation\n");
        return;
    }

    if (!values_fit(request, system->available, system->resourceCount)) {
        fprintf(output, "The resources ");
        print_values_with_trailing_space(output, system->available, system->resourceCount);
        fprintf(output, "are not enough to customer %d request ", customer);
        print_values_with_trailing_space(output, request, system->resourceCount);
        fputc('\n', output);
        return;
    }

    for (int resource = 0; resource < system->resourceCount; resource++) {
        system->available[resource] -= request[resource];
        system->allocation[customer][resource] += request[resource];
        system->need[customer][resource] -= request[resource];
    }

    if (!is_safe_state(system)) {
        for (int resource = 0; resource < system->resourceCount; resource++) {
            system->available[resource] += request[resource];
            system->allocation[customer][resource] -= request[resource];
            system->need[customer][resource] += request[resource];
        }

        fprintf(output, "The customer %d request", customer);
        print_values(output, request, system->resourceCount);
        fprintf(output, " was denied because result in an unsafe state\n");
        return;
    }

    fprintf(output, "Allocate to customer %d the resources", customer);
    print_values(output, request, system->resourceCount);
    fputc('\n', output);
}

static void process_release(System *system, int customer, const int *release, FILE *output) {
    if (!values_fit(release, system->allocation[customer], system->resourceCount)) {
        fprintf(output, "The customer %d release", customer);
        print_values(output, release, system->resourceCount);
        fprintf(output, " was denied because exceed its allocation\n");
        return;
    }

    for (int resource = 0; resource < system->resourceCount; resource++) {
        system->allocation[customer][resource] -= release[resource];
        system->need[customer][resource] += release[resource];
        system->available[resource] += release[resource];
    }

    fprintf(output, "Release from customer %d the resources", customer);
    print_values(output, release, system->resourceCount);
    fputc('\n', output);
}

static void run_command(System *system, const Command *command, FILE *output) {
    switch (command->type) {
        case COMMAND_REQUEST:
            process_request(system, command->customer, command->resources, output);
            break;
        case COMMAND_RELEASE:
            process_release(system, command->customer, command->resources, output);
            break;
        case COMMAND_PRINT:
            break;
    }
}

static void free_system(System *system) {
    free_matrix(system->maximum, system->customerCount);
    free_matrix(system->allocation, system->customerCount);
    free_matrix(system->need, system->customerCount);
    free(system->available);
    free(system->resources);

    for (int index = 0; index < system->commandCount; index++) {
        free(system->commands[index].resources);
    }

    free(system->commands);
}

int main(int argc, char **argv) {
    System system = {0};
    system.resources = read_resources(argc, argv, &system.resourceCount);
    read_customers(&system);
    read_commands(&system);
    initialize_state(&system);

    if (!is_safe_state(&system)) {
        fprintf(stderr, "Error: initial state is not safe\n");
        free_system(&system);
        return EXIT_FAILURE;
    }

    FILE *output = fopen("result.txt", "w");

    if (output == NULL) {
        fprintf(stderr, "Error: could not create result.txt\n");
        free_system(&system);
        return EXIT_FAILURE;
    }

    for (int index = 0; index < system.commandCount; index++) {
        run_command(&system, &system.commands[index], output);
    }

    fclose(output);
    free_system(&system);
    return EXIT_SUCCESS;
}
