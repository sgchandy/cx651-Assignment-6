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
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        return result;
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, target)) {
            result.count++;
            result.instances = realloc(result.instances, result.count * sizeof(char *));
            // stripping whitespace from the line before storing it
            char *start = line;
            while (isspace((unsigned char)*start)) start++;
            char *end = line + strlen(line) - 1;
            while (end > start && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';
            if (start != line) {
                memmove(line, start, end - start + 2);
            }
            // store the stripped line
            result.instances[result.count - 1] = strdup(line);
        }
    }
    fclose(file);
    return result;
}