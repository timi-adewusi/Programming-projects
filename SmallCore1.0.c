#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* Three output buffers: headers, function definitions, main() body */
char headers[2000], funcs[20000], main_body[20000];
char *target  = main_body;  /* points to whichever buffer we're filling */
int  indent   = 1;          /* current indentation depth                 */

/* String variable registry — so `say` knows to use %s vs %lld */
char str_vars[100][32];
int  str_count = 0;

int is_str(char *name) {
    for (int i = 0; i < str_count; i++)
        if (!strcmp(str_vars[i], name)) return 1;
    return 0;
}

/* Append an indented, formatted line to the current target buffer */
void emit(int lvl, const char *fmt, ...) {
    char tmp[512];
    va_list ap; va_start(ap, fmt); vsnprintf(tmp, sizeof tmp, fmt, ap); va_end(ap);
    char *p = target + strlen(target);
    sprintf(p, "%*s%s", lvl * 4, "", tmp);
}

void compile_line(char *raw) {
    char *l = raw;
    while (isspace(*l)) l++;
    if (*l == '\0' || *l == '#') return;
    l[strcspn(l, "\n")] = '\0';  /* strip trailing newline */

    char tok[64] = {0};
    sscanf(l, "%63s", tok);

    if (!strcmp(tok, "use")) {
        char mod[64]; sscanf(l, "use %63s", mod);
        sprintf(headers + strlen(headers), "#include <%s.h>\n", mod);

    } else if (!strcmp(tok, "fn")) {
        char name[64]; sscanf(l, "fn %63s", name);
        target = funcs; indent = 1;
        sprintf(target + strlen(target), "void %s() {\n", name);

    } else if (!strcmp(tok, "call")) {
        char name[64]; sscanf(l, "call %63s", name);
        emit(indent, "%s();\n", name);

    } else if (!strcmp(tok, "let")) {
        emit(indent, "long long %s;\n", l + 4);

    } else if (!strcmp(tok, "listen")) {
        char var[64]; sscanf(l, "listen %63s", var);
        strcpy(str_vars[str_count++], var);
        emit(indent, "char %s[256]; scanf(\"%%255s\", %s);\n", var, var);

    } else if (!strcmp(tok, "ask")) {
        char var[64]; sscanf(l, "ask %63s", var);
        emit(indent, "scanf(\"%%lld\", &%s);\n", var);

    } else if (!strcmp(tok, "say")) {
        char *val = l + 4;
        char vname[64]; sscanf(val, "%63s", vname);
        if (val[0] == '"' || is_str(vname))
            emit(indent, "printf(\"%%s\\n\", %s);\n", val);
        else
            emit(indent, "printf(\"%%lld\\n\", %s);\n", val);

    } else if (!strcmp(tok, "loop") || !strcmp(tok, "if")) {
        char cond[128] = {0};
        sscanf(l + strlen(tok), " %127[^{]", cond);
        char *end = cond + strlen(cond) - 1;
        while (end > cond && isspace(*end)) *end-- = '\0';
        emit(indent, "%s (%s) {\n", !strcmp(tok, "loop") ? "while" : "if", cond);
        indent++;

    } else if (!strcmp(tok, "else")) {
        /* The prior `}` line was emitted at (indent) level after decrementing.
           We trim it off and replace with "} else {" at the same level. */
        char *p = target + strlen(target);
        /* Walk back past newline, }, and leading spaces on that line */
        if (p > target) p--;         /* '\n' */
        if (p > target) p--;         /* '}'  */
        while (p > target && (*(p-1) == ' ' || *(p-1) == '\t')) p--;
        *p = '\0';                   /* truncate buffer */
        emit(indent, "} else {\n");
        indent++;

    } else if (!strcmp(tok, "}")) {
        indent--;
        emit(indent, "}\n");
        if (indent == 0) { target = main_body; indent = 1; }

    } else if (!strcmp(tok, "stop")) {
        emit(indent, "break;\n");

    } else {
        emit(indent, "%s;\n", l);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: sc <file.sc>\n"); return 1; }
    FILE *in = fopen(argv[1], "r");
    if (!in) { perror(argv[1]); return 1; }

    char line[256];
    while (fgets(line, sizeof line, in)) compile_line(line);
    fclose(in);

    FILE *out = fopen("output.c", "w");
    if (!out) { perror("output.c"); return 1; }
    fprintf(out, "#include <stdio.h>\n%s\n%s\nint main() {\n%s\n    return 0;\n}\n",
            headers, funcs, main_body);
    fclose(out);
    return 0;
}
