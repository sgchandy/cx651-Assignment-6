#include <string.h>
int search_count(char* filename, char* target);
struct count_result search_instance(char* filename, char* target);

struct count_result {
    int count;
    char** instances;
};

// Trim in place, preserving whitespace inside the line.
static inline void strip_line(char *line) {
    const char *whitespace = " \t\n\r\v\f";
    char *start = line + strspn(line, whitespace);
    char *end = start + strlen(start);
    while (end > start && strchr(whitespace, end[-1])) end--;
    *end = '\0';
    if (start != line) {
        memmove(line, start, end - start + 1);
    }
}
