#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <omp.h>
#include <ctype.h>
#include "engine.h"

#define MAX_LINE_LENGTH 256

#define THREAD_COUNT 4

int search_count(char *filename, char *target) {
   struct stat info;
    if (*target == '\0' || stat(filename, &info) != 0) return 0;
    int count = 0;
    #pragma omp parallel for num_threads(THREAD_COUNT) reduction(+:count)
    for (int i = 0; i < THREAD_COUNT; i++) {
        long start = info.st_size * i / THREAD_COUNT;
        long end = info.st_size * (i + 1) / THREAD_COUNT;
        FILE *file = fopen(filename, "r");
        if (!file) continue;
        setvbuf(file, NULL, _IOFBF, 64 * 1024);
        size_t capacity = MAX_LINE_LENGTH;
        char *line = malloc(capacity);
        if (!line) { perror("malloc"); exit(EXIT_FAILURE); }
        size_t target_length = strlen(target);

        if (start > 0) {
            fseek(file, start - 1, SEEK_SET);
            if (fgetc(file) != '\n') getline(&line, &capacity, file);
        }
        long position = ftell(file);
        ssize_t length;
        while (position < end &&
               (length = getline(&line, &capacity, file)) != -1) {
            position += length;
            char *match = line;
            while ((match = strstr(match, target)) != NULL) {
                count++;
                match += target_length;
            }
        }
        free(line);
        fclose(file);
    }
    return count;
}


struct count_result search_instance(char *filename,char *target){
    struct count_result result = {0, NULL};
    struct stat info;
    if (*target == '\0' || stat(filename, &info) != 0) return result;
    struct count_result partial[THREAD_COUNT] = {0};

    #pragma omp parallel for num_threads(THREAD_COUNT)
    for (int i = 0; i < THREAD_COUNT; i++) {
        long start = info.st_size * i / THREAD_COUNT;
        long end = info.st_size * (i + 1) / THREAD_COUNT;
        struct count_result result = {0, NULL};
        FILE *file = fopen(filename, "r");
        if (!file) continue;
        setvbuf(file, NULL, _IOFBF, 64 * 1024);
        size_t capacity = MAX_LINE_LENGTH;
        char *line = malloc(capacity);
        if (!line) { perror("malloc"); exit(EXIT_FAILURE); }
        size_t target_length = strlen(target);
        int slots = 0;

        if (start > 0) {
            fseek(file, start - 1, SEEK_SET);
            if (fgetc(file) != '\n') getline(&line, &capacity, file);
        }
        long position = ftell(file);
        ssize_t length;
        while (position < end &&
               (length = getline(&line, &capacity, file)) != -1) {
            position += length;
            char *match = line;
            while ((match = strstr(match, target)) != NULL) {
                if (result.count == slots) {
                    slots = slots ? slots * 2 : 16;
                    char **grown = realloc(result.instances, slots * sizeof(char *));
                    if (!grown) { perror("realloc"); exit(EXIT_FAILURE); }
                    result.instances = grown;
                }
                char *copy = strdup(line);
                if (!copy) { perror("strdup"); exit(EXIT_FAILURE); }
                strip_line(copy);
                result.instances[result.count] = copy;
                result.count++;
                match += target_length;
            }
        }
        free(line);
        fclose(file);
        partial[i] = result;
    }
    // The parallel loop finishes before results are merged in file order.
    for (int i = 0; i < THREAD_COUNT; i++) {
        result.count += partial[i].count;
    }
    if (result.count > 0) {
        result.instances = malloc(result.count * sizeof(char *));
        if (!result.instances) { perror("malloc"); exit(EXIT_FAILURE); }
    }
    int next = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        for (int j = 0; j < partial[i].count; j++) {
            result.instances[next++] = partial[i].instances[j];
        }
        free(partial[i].instances);
    }
    return result;
}