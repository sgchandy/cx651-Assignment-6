#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 256

#define THREAD_COUNT 4

struct worker_args {
    char *filename;
    char *target;
    long start;
    long end;
    bool keep_instances;
    struct count_result result;
};

static void *search_worker(void *arg) {
    struct worker_args *args = arg;
    FILE *file = fopen(args->filename, "r");
    if (!file) return NULL;
    size_t capacity = MAX_LINE_LENGTH;
    char *line = malloc(capacity);
    if (!line) { perror("malloc"); exit(EXIT_FAILURE); }
    size_t target_length = strlen(args->target);
    int slots = 0;
    if (args->start > 0) {
        fseek(file, args->start - 1, SEEK_SET);
        if (fgetc(file) != '\n') getline(&line, &capacity, file);
    }
    long position = ftell(file);
    ssize_t length;
    while (position < args->end &&
           (length = getline(&line, &capacity, file)) != -1) {
        position += length;
        char *match = line;
        int matches = 0;
        while ((match = strstr(match, args->target)) != NULL) {
            matches++;
            match += target_length;
        }
        if (!args->keep_instances) {
            args->result.count += matches;
            continue;
        }
        if (matches == 0) continue;

        strip_line(line);
        for (int i = 0; i < matches; i++) {
            if (args->result.count == slots) {
                slots = slots ? slots * 2 : 16;
                char **grown = realloc(args->result.instances, slots * sizeof(char *));
                if (!grown) { perror("realloc"); exit(EXIT_FAILURE); }
                args->result.instances = grown;
            }
            char *copy = strdup(line);
            if (!copy) { perror("strdup"); exit(EXIT_FAILURE); }
            args->result.instances[args->result.count++] = copy;
        }
    }
    free(line);
    fclose(file);
    return NULL;
}

int search_count(char *filename, char *target) {
	struct stat info;
    if (*target == '\0' || stat(filename, &info) != 0) return 0;
    pthread_t threads[THREAD_COUNT];
    struct worker_args args[THREAD_COUNT];
    bool started[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i] = (struct worker_args) {
            filename, target, info.st_size * i / THREAD_COUNT,
            info.st_size * (i + 1) / THREAD_COUNT,
            false, {0, NULL}
        };
        started[i] = pthread_create(&threads[i], NULL, search_worker, &args[i]) == 0;
        if (!started[i]) search_worker(&args[i]);
    }
    int count = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (started[i]) pthread_join(threads[i], NULL);
        count += args[i].result.count;
    }
    return count;
}

struct count_result search_instance(char *filename,char *target){
	struct count_result result = {0, NULL};
    struct stat info;
    if (*target == '\0' || stat(filename, &info) != 0) return result;
    pthread_t threads[THREAD_COUNT];
    struct worker_args args[THREAD_COUNT];
    bool started[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i] = (struct worker_args) {
            filename, target, info.st_size * i / THREAD_COUNT,
            info.st_size * (i + 1) / THREAD_COUNT,
            true, {0, NULL}
        };
        started[i] = pthread_create(&threads[i], NULL, search_worker, &args[i]) == 0;
        if (!started[i]) search_worker(&args[i]);
    }
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (started[i]) pthread_join(threads[i], NULL);
        result.count += args[i].result.count;
    }
    if (result.count > 0) {
        result.instances = malloc(result.count * sizeof(char *));
        if (!result.instances) { perror("malloc"); exit(EXIT_FAILURE); }
    }
    // Merge in file order, after every worker has finished.
    int next = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        for (int j = 0; j < args[i].result.count; j++) {
            result.instances[next++] = args[i].result.instances[j];
        }
        free(args[i].result.instances);
    }
    return result;
}