#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <string.h>

long long push(long long val);
long long pop();

static char *arena_base = NULL;
static size_t arena_size = 1024 * 1024 * 100; // 100MB pool
static size_t arena_offset = 0;

long long init_engine() {
    if (!arena_base) arena_base = malloc(arena_size);
    arena_offset = 0;
    printf("[Engine] Arena Initialized\n");
    return 0;
}

// The "Magic" function for SmallCore
long long take(long long bytes) {
    if (arena_offset + bytes > arena_size) return 0; // Out of memory
    void *ptr = arena_base + arena_offset;
    arena_offset += bytes;
    return (long long)ptr;
}

long long wipe() {
    arena_offset = 0; // Instant "free" of everything!
    return 0;
}

long long get_at(long long base, long long offset) {
    return *((long long*)(base + (offset * 8)));
}

long long set_at(long long base, long long offset, long long val) {
    *((long long*)(base + (offset * 8))) = val;
    return 0;
}

long long concat_stack() {
    char* str2 = (char*)pop(); // Second string
    char* str1 = (char*)pop(); // First string
    
    // Create a new buffer (on the heap so it doesn't disappear)
    char* result = malloc(strlen(str1) + strlen(str2) + 1);
    strcpy(result, str1);
    strcat(result, str2);
    
    push((long long)result); // Return the pointer to the new string
    return 0;
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
long clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    return 0;
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
long long sleep_ms(long long ms) {
    #ifdef _WIN32
        _sleep(ms);
    #else
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        nanosleep(&ts, NULL);
    #endif
    return 0;
}

long long say_part(char* s) {
    printf("%s", s);
    return 0;
}

long long say_num_part(long long n) {
    printf("%lld", n);
    return 0;
}

long long print_string(long long addr) {
    // Cast the number back to a string pointer and print it
    printf("Hello, %s!\n", (char*)addr);
    return 0;
}

// --- Value Stack for Params and Returns ---
static long long stack[256];
static int sp = 0; // Stack Pointer

long long push(long long val) {
    if (sp < 256) stack[sp++] = val;
    return 0;
}

long long pop() {
    if (sp > 0) return stack[--sp];
    return 0;
}

long long say_str(long long addr) {
    printf("%s\n", (char*)addr);
    return 0;
}

/**
 * MEMORY HELPERS
 * Helps SmallCore handle data structures.
 */

// Fills a block of memory with a specific byte (useful for clearing arrays)
long long mem_set(long long addr, long long val, long long size) {
    memset((void*)addr, (int)val, (size_t)size);
    return 0;
}
