#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

FILE *out;
char strings[100][32]; 
int str_count = 0;
char vars[100][32];    // Symbol Table
int var_count = 0;
char func_body[20000] = {0}; 
char main_body[20000] = {0}; 
char *current_target = main_body; 
int indent_level = 0; // Scope Tracker

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

void compile_line(char *line) {
    line = trim(line);
    if (*line == '\0' || *line == '#') return;

    char line_copy[256];
    strcpy(line_copy, line);
    char *tok = strtok(line_copy, " \t\r\n");
    if (!tok) return;

    if (strcmp(tok, "fn") == 0) {
        char *name = strtok(NULL, " \t{");
        current_target = func_body; 
        emit("void f_%s() {\n", name);
        indent_level++;
    } 
    else if (strcmp(tok, "while") == 0 || strcmp(tok, "if") == 0) {
        char *a = strtok(NULL, " \t");
        char *op = strtok(NULL, " \t");
        char *b = strtok(NULL, " \t{");
        emit("    %s (v_%s %s %s) {\n", tok, a, op, b);
        indent_level++;
    }
    else if (strcmp(tok, "}") == 0) {
        indent_level--;
        emit("    }\n");
        if (indent_level == 0) current_target = main_body; 
    }
    else if (strcmp(tok, "let") == 0) {
        char *name = strtok(NULL, " \t=");
        char *expr = strchr(line, '=');
        int exists = 0;
        for(int i=0; i<var_count; i++) if(strcmp(vars[i], name) == 0) exists = 1;
        
        if (exists) emit("    v_%s = %s;\n", name, trim(expr + 1));
        else {
            strcpy(vars[var_count++], name);
            emit("    long long v_%s = %s;\n", name, trim(expr + 1));
        }
    }
    else if (strcmp(tok, "say") == 0) {
        char *val = trim(line + 3);
        if (val[0] == '"') emit("    printf(\"%%s\\n\", %s);\n", val);
        else {
            if (is_string_var(val)) emit("    printf(\"%%s\\n\", v_%s);\n", val);
            else emit("    printf(\"%%lld\\n\", v_%s);\n", val);
        }
    }
    else if (strcmp(tok, "ask") == 0) {
        char *name = strtok(NULL, " \t");
        emit("    long long v_%s; scanf(\"%%lld\", &v_%s);\n", name, name);
    }
    else if (strcmp(tok, "listen") == 0) {
        char *name = strtok(NULL, " \t");
        strcpy(strings[str_count++], name);
        emit("    char v_%s[256]; scanf(\"%%255s\", v_%s);\n", name, name);
    }
    else if (strcmp(tok, "call") == 0) {
        char *name = strtok(NULL, " \t\r\n");
        emit("    f_%s();\n", name);
    }
}

int main(int argc, char **argv) {
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
