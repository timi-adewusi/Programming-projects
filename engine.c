#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <string.h>

void push(long long val);
long long pop();

void concat_stack() {
    char* str2 = (char*)pop(); // Second string
    char* str1 = (char*)pop(); // First string
    
    // Create a new buffer (on the heap so it doesn't disappear)
    char* result = malloc(strlen(str1) + strlen(str2) + 1);
    strcpy(result, str1);
    strcat(result, str2);
    
    push((long long)result); // Return the pointer to the new string
}
/**
 * MATH WRAPPERS
 * These bridge C's 'double' functions to SmallCore's 'long long'.
 */

long long my_pow(long long base, long long exp) {
    return (long long)pow((double)base, (double)exp);
}

long long my_sqrt(long long n) {
    return (long long)sqrt((double)n);
}

/**
 * SYSTEM / UTILITY
 * Functions to give SmallCore more "OS-level" control.
 */

// Clears the terminal screen
void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Returns a random number between 0 and max
long long get_random(long long max) {
    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }
    if (max <= 0) return 0;
    return rand() % max;
}

// Pauses execution for X milliseconds
void sleep_ms(long long ms) {
    #ifdef _WIN32
        _sleep(ms);
    #else
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        nanosleep(&ts, NULL);
    #endif
}

void say_part(char* s) {
    printf("%s", s);
}

void say_num_part(long long n) {
    printf("%lld", n);
}

void print_string(long long addr) {
    // Cast the number back to a string pointer and print it
    printf("Hello, %s!\n", (char*)addr);
}

// --- Value Stack for Params and Returns ---
static long long stack[256];
static int sp = 0; // Stack Pointer

void push(long long val) {
    if (sp < 256) stack[sp++] = val;
}

long long pop() {
    if (sp > 0) return stack[--sp];
    return 0;
}

void say_str(long long addr) {
    printf("%s\n", (char*)addr);
}

/**
 * MEMORY HELPERS
 * Helps SmallCore handle data structures.
 */

// Fills a block of memory with a specific byte (useful for clearing arrays)
void mem_set(long long addr, long long val, long long size) {
    memset((void*)addr, (int)val, (size_t)size);
}
