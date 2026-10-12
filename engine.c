#include <string.h>
#include "engine.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

int search_count(char *filename, char *target) {
   //returns the integer count of all occurrences of the target word in the file 
   FILE *file = fopen(filename, "r");
   if (!file) {
       return 0;
   }
   char line[MAX_LINE_LENGTH];
   int count = 0;
   while (fgets(line, sizeof(line), file)) {
       char *ptr = line;
       while ((ptr = strstr(ptr, target)) != NULL) {
           count++;
           ptr += strlen(target);
       }
   }
   fclose(file);
   return count;
}

struct count_result search_instance(char *filename,char *target){
    //returns the count and an array of string "instances" of the word. The string will be the string line where the word was found.
    // Each element of this array is a whitespace stripped string of the entire line which contained the target word.

    struct count_result result;
    result.count = 0;
    result.instances = NULL;

    if (!target || *target == '\0') {
        return result;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        return result;
    }
    char line[MAX_LINE_LENGTH];
    int slots = 0;
    while (fgets(line, sizeof(line), file)) {
        char *match = line;
        int matches = 0;
        while ((match = strstr(match, target)) != NULL) {
            matches++;
            match += strlen(target);
        }
        if (matches == 0) continue;

        char *start = line;
        while (isspace((unsigned char)*start)) start++;
        char *end = line + strlen(line);
        while (end > start && isspace((unsigned char)end[-1])) end--;
        *end = '\0';
        if (start != line) {
            memmove(line, start, end - start + 1);
        }

        for (int i = 0; i < matches; i++) {
            if (result.count == slots) {
                slots = slots ? slots * 2 : 16;
                char **grown = realloc(result.instances, slots * sizeof(*grown));
                if (!grown) {
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
                result.instances = grown;
            }
            result.instances[result.count] = strdup(line);
            if (!result.instances[result.count]) {
                perror("strdup");
                exit(EXIT_FAILURE);
            }
            result.count++;
        }
    }
    fclose(file);
    return result;
}