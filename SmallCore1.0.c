#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

FILE *out;
char strings[100][32]; 
int str_count = 0;

typedef struct {
    char name[32];
    int is_array;
} Symbol;

Symbol vars[100];
int var_count = 0;

char func_body[20000] = {0}; 
char main_body[20000] = {0}; 
char *current_target = main_body; 
int indent_level = 0;

char* trim(char *s) {
    while(isspace((unsigned char)*s)) s++;
    return s;
}

int is_string_var(char *name) {
    for (int i = 0; i < str_count; i++) 
        if (strcmp(strings[i], name) == 0) return 1;
    return 0;
}

void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsprintf(buf, fmt, args);
    strcat(current_target, buf);
    va_end(args);
}

void emit_indented(const char *fmt, ...) {
    char indent[100] = "";
    // When in main_body and indent_level is 0, use level 1 for proper indentation inside main()
    // Otherwise use the current indent_level
    int level = (current_target == main_body && indent_level == 0) ? 1 : indent_level;
    for (int i = 0; i < level * 4; i++) indent[i] = ' ';
    strcat(current_target, indent);
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsprintf(buf, fmt, args);
    strcat(current_target, buf);
    va_end(args);
}

void compile_line(char *line) {
    // Strip trailing newline
    size_t len = strlen(line);
    if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
    
    line = trim(line);
    if (*line == '\0' || *line == '#') return;

    char line_copy[256];
    strcpy(line_copy, line);
    char *tok = strtok(line_copy, " \t\r\n");
    if (!tok) return;

    // Safety: If we aren't in a function and not starting one, we must be in main
    if (strcmp(tok, "fn") != 0 && indent_level == 0) {
        current_target = main_body;
    }

    if (strcmp(tok, "fn") == 0) {
        char *name = strtok(NULL, " \t{");
        current_target = func_body; 
        emit("void f_%s() {\n", name);
        indent_level = 1; 
    } 
    else if (strcmp(tok, "if") == 0 || strcmp(tok, "while") == 0) {
        char *a = strtok(NULL, " \t");
        char *op = strtok(NULL, " \t");
        char *b = strtok(NULL, " \t{");
        // Add v_ prefix only if operand doesn't start with a digit
        char *a_prefix = (isdigit((unsigned char)*a) || a[0] == '-') ? "" : "v_";
        char *b_prefix = (isdigit((unsigned char)*b) || b[0] == '-') ? "" : "v_";
        emit_indented("if (%s%s %s %s%s) {\n", a_prefix, a, op, b_prefix, b);
        indent_level++;
    }
    else if (strcmp(tok, "else") == 0) {
        // else on separate line - just emit else (closing brace already emitted)
        emit_indented("else {\n");
        indent_level++;  // Opening new block
    }
    else if (strcmp(tok, "}") == 0) {
        indent_level--;
        if (indent_level == 0) {
            // End of function - emit without indentation
            emit("}\n\n");
            current_target = main_body; 
        } else {
            // End of If/While block - emit with indentation
            emit_indented("}\n");
        }
    }
    else if (strcmp(tok, "let") == 0) {
        char *name = strtok(NULL, " \t=");
        char *expr = trim(strchr(line, '=') + 1);
        if (expr[0] == '[') {
            int size = atoi(expr + 1);
            emit_indented("long long v_%s[%d] = {0};\n", name, size);
            strcpy(vars[var_count].name, name);
            vars[var_count++].is_array = 1;
        } else {
            int exists = -1;
            for(int i=0; i<var_count; i++) if(strcmp(vars[i].name, name) == 0) exists = i;
            if (exists != -1) emit_indented("v_%s = %s;\n", name, expr);
            else {
                strcpy(vars[var_count].name, name);
                vars[var_count++].is_array = 0;
                emit_indented("long long v_%s = %s;\n", name, expr);
            }
        }
    }
    else if (strcmp(tok, "set") == 0) {
        char *name = strtok(NULL, " [");
        char *idx = strtok(NULL, " ]");
        strtok(NULL, "="); 
        char *val = trim(strtok(NULL, "\n\r"));
        emit_indented("v_%s[%s] = %s;\n", name, idx, val);
    }
    else if (strcmp(tok, "say") == 0) {
        char *val = trim(line + 3);
        if (val[0] == '"') emit_indented("printf(\"%%s\\n\", %s);\n", val);
        else {
            char *prefix = (strncmp(val, "v_", 2) == 0) ? "" : "v_";
            emit_indented("printf(\"%%lld\\n\", %s%s);\n", prefix, val);
        }
    }
    else if (strcmp(tok, "ask") == 0) {
        char *name = strtok(NULL, " \t");
        emit_indented("long long v_%s; scanf(\"%%lld\", &v_%s);\n", name, name);
    }
    else if (strcmp(tok, "call") == 0) {
        char *name = strtok(NULL, " \t\r\n");
        emit_indented("f_%s();\n", name);
    }
}

int main(int argc, char **argv) {
    memset(func_body, 0, sizeof(func_body));
    memset(main_body, 0, sizeof(main_body));

    if (argc < 2) return 1;
    FILE *in = fopen(argv[1], "r");
    if (!in) return 1;
    out = fopen("output.c", "w");
    
    char line[256];
    while (fgets(line, sizeof(line), in)) compile_line(line);
    
    fprintf(out, "#include <stdio.h>\n\n%s\n", func_body);
    fprintf(out, "int main() {\n%s\n    return 0;\n}\n", main_body);
    
    fclose(in); fclose(out);
    return 0;
}
