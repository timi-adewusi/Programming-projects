/*
 * sc.c — SmallCore Compiler
 * Translates .sc source files into a single output.c
 *
 * Language features:
 *   use <module>          → #include <module.h>
 *   def NAME = VALUE      → text substitution (symbol/constant)
 *   ext func(val, str)    → declare external C function
 *   fn name {             → define a void function
 *   }                     → close fn or block
 *   let x = <expr>        → declare / assign a variable (long long)
 *   let x = call f(args)  → call returning a value into x
 *   listen x              → read a string  (char x[256])
 *   ask x                 → read a number  (scanf %lld)
 *   say x / "lit"         → print value or string literal
 *   if <cond> {           → if statement
 *   else                  → else branch
 *   loop <cond> {         → while loop
 *   stop                  → break
 *   call f(args)          → call a void function
 *   @(addr) = val         → write to raw memory address
 *   let x = @(addr)       → read from raw memory address
 *   free(x)               → free heap pointer
 *   <anything else>;      → emitted verbatim as a C statement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "engine.h"
#include <time.h>

/* ── Output buffers ─────────────────────────────────────────────────────── */
static char headers[4000];
static char funcs[200000];
static char body[100000];

/* ── Compiler state ─────────────────────────────────────────────────────── */
static char *out       = NULL; /* active write buffer                        */
static int   depth     = 1;    /* indentation depth (1 = inside main or fn)  */
static int   fn_braces = 0;    /* open-brace count since entering fn; 0=main */

/* ── Symbol table (def NAME = VALUE) ────────────────────────────────────── */
typedef struct { char name[64]; char value[128]; } Symbol;
static Symbol syms[128];
static int    sym_n = 0;

/* ── String-variable registry (for say formatting) ──────────────────────── */
static char strvars[128][64];
static int  strvar_n = 0;

/* ── Global-variable registry (variables used in both main and fns) ─────── */
static char gvars[256][64];
static int  gvar_n = 0;

/* ── All-declared-variables registry (prevents re-decl in nested scopes) ── */
static char decl_vars[512][64];
static int  decl_var_n = 0;

/* ═══════════════════════════════════════════════════════════════════════════
 * Helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Append a formatted, indented line to the active output buffer. */
static void emit(const char *fmt, ...) {
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);

    char line[1100];
    snprintf(line, sizeof line, "%*s%s", depth * 4, "", tmp);
    strncat(out, line, (out == funcs ? sizeof funcs : sizeof body) - strlen(out) - 1);
}

/* Append a string to the headers buffer. */
static void emit_header(const char *s) {
    strncat(headers, s, sizeof headers - strlen(headers) - 1);
}

/* Register a variable name as string-typed. */
static void register_strvar(const char *name) {
    for (int i = 0; i < strvar_n; i++)
        if (!strcmp(strvars[i], name)) return;
    strncpy(strvars[strvar_n++], name, 63);
}

/* Returns 1 if name is a known string variable. */
static int is_strvar(const char *name) {
    for (int i = 0; i < strvar_n; i++)
        if (!strcmp(strvars[i], name)) return 1;
    return 0;
}

/* Register a variable as global (shared between main and fns). */
static void register_gvar(const char *name) {
    for (int i = 0; i < gvar_n; i++)
        if (!strcmp(gvars[i], name)) return;
    strncpy(gvars[gvar_n++], name, 63);
}

/* Returns 1 if name is already a registered global variable. */
static int is_gvar(const char *name) {
    for (int i = 0; i < gvar_n; i++)
        if (!strcmp(gvars[i], name)) return 1;
    return 0;
}

/* Register a variable as declared (any scope). */
static void register_decl(const char *name) {
    for (int i = 0; i < decl_var_n; i++)
        if (!strcmp(decl_vars[i], name)) return;
    strncpy(decl_vars[decl_var_n++], name, 63);
}

/* Returns 1 if this variable has already been declared anywhere. */
static int is_decl(const char *name) {
    for (int i = 0; i < decl_var_n; i++)
        if (!strcmp(decl_vars[i], name)) return 1;
    return 0;
}

/* Expand def-symbols in a line. Uses a safe double-buffer swap.
 * 'line' must be LINESZ bytes.  */
#define LINESZ 2048
static void expand_syms(char *line) {
    static char tmp[LINESZ];
    for (int i = 0; i < sym_n; i++) {
        const char *nm  = syms[i].name;
        const char *val = syms[i].value;
        size_t      nl  = strlen(nm);
        size_t      vl  = strlen(val);
        const char *src = line;
        char       *dst = tmp;
        char       *lim = tmp + LINESZ - 1;
        while (*src && dst < lim) {
            char before = (src > line) ? *(src - 1) : ' ';
            if (strncmp(src, nm, nl) == 0) {
                char after = *(src + nl);
                if (!isalnum((unsigned char)before) && before != '_' &&
                    !isalnum((unsigned char)after)  && after  != '_') {
                    size_t room = (size_t)(lim - dst);
                    size_t n    = vl < room ? vl : room;
                    memcpy(dst, val, n);
                    dst += n;
                    src += nl;
                    continue;
                }
            }
            *dst++ = *src++;
        }
        *dst = '\0';
        strncpy(line, tmp, LINESZ - 1);
        line[LINESZ - 1] = '\0';
    }
}

/* Strip trailing whitespace from a string. */
static void rtrim(char *s) {
    int n = (int)strlen(s) - 1;
    while (n >= 0 && isspace((unsigned char)s[n])) s[n--] = '\0';
}

/* ═══════════════════════════════════════════════════════════════════════════
 * compile_line  — the heart of the compiler
 * ═══════════════════════════════════════════════════════════════════════════ */
static void compile_line(char *raw) {
    /* Skip leading whitespace */
    char *l = raw;
    while (isspace((unsigned char)*l)) l++;

    /* Skip blank lines and comments */
    if (*l == '\0' || *l == '#') return;

    /* Strip newline */
    l[strcspn(l, "\n")] = '\0';
    rtrim(l);

    /* Extract first token */
    char tok[64] = {0};
    sscanf(l, "%63s", tok);

    /* ── def: text-substitution symbol ───────────────────────────────────── */
    if (!strcmp(tok, "def")) {
        char name[64] = {0}, value[128] = {0};
        sscanf(l, "def %63s = %127[^\n]", name, value);
        strncpy(syms[sym_n].name,  name,  63);
        strncpy(syms[sym_n].value, value, 127);
        sym_n++;
        return;
    }

    /* Apply symbol substitution before parsing anything else */
    char proc[LINESZ];
    strncpy(proc, l, LINESZ - 1);
    proc[LINESZ - 1] = '\0';
    expand_syms(proc);
    l = proc;
    sscanf(l, "%63s", tok);

    /* ── use: include a C header ──────────────────────────────────────────── */
    if (!strcmp(tok, "use")) {
        char mod[64] = {0};
        sscanf(l, "use %63s", mod);
        char buf[128];
        snprintf(buf, sizeof buf, "#include <%s.h>\n", mod);
        emit_header(buf);
        return;
    }

    /* ── fn: begin a function definition ─────────────────────────────────── */
    if (!strcmp(tok, "fn")) {
        char name[64] = {0};
        sscanf(l, "fn %63[^ {]", name);
        rtrim(name);
        out       = funcs;
        depth     = 1;
        fn_braces = 1;         /* fn's own { is one open brace */
        /* Reset local decl tracking for this new function scope */
        decl_var_n = 0;
        char buf[128];
        snprintf(buf, sizeof buf, "void %s() {\n", name);
        strncat(out, buf, sizeof funcs - strlen(out) - 1);
        return;
    }

    /* ── } close a block or function ─────────────────────────────────────── */
    if (!strcmp(tok, "}")) {
        depth--;
        emit("}\n");
        if (fn_braces > 0) {
            fn_braces--;
            if (fn_braces == 0) {
                /* Just closed the fn's outermost brace */
                out   = body;
                depth = 1;
            }
        }
        
        /* Check if 'else' is on the same line after the } */
        char *else_pos = strstr(l, "else");
        if (else_pos && else_pos > strchr(l, '}')) {
            /* We have } else on the same line, handle the else now */
            char *end = out + strlen(out);
            if (end > out && *(end-1) == '\n') end--;
            while (end > out && *(end-1) != '\n') end--;
            *end = '\0';
            emit("} else {\n");
            depth++;
            if (fn_braces > 0) fn_braces++;
            
            /* Check for { after else */
            char *brace_pos = strchr(else_pos, '{');
            if (brace_pos) {
                /* There's a { on the same line, don't need to parse it separately */
                return;
            }
        }
        return;
    }

    /* ── else ─────────────────────────────────────────────────────────────── */
    if (!strcmp(tok, "else")) {
        /* Undo the last "}\n" we emitted, then re-emit as "} else {\n".
         * depth is already at the if-body level; the } we are replacing
         * had already decremented it, so we need to put it back.          */
        char *end = out + strlen(out);
        if (end > out && *(end-1) == '\n') end--;
        while (end > out && *(end-1) != '\n') end--;  /* strip indented "}" line */
        *end = '\0';
        emit("} else {\n");
        /* The } we removed had depth-- and fn_braces--; undo those. */
        depth++;
        if (fn_braces > 0) fn_braces++;   /* count the new { in "else {" */
        return;
    }

    /* ── ext: external C function declaration ────────────────────────────── */
    if (!strcmp(tok, "ext")) {
        char name[64] = {0}, args[256] = {0};
        int  matched = sscanf(l, "ext %63[^(](%255[^)])", name, args);
        rtrim(name);

        /* Skip stdlib/math/time builtins already declared by their headers */
        static const char *builtins[] = {
            "pow","sqrt","log","rand","srand","time","printf","scanf",
            "malloc","calloc","realloc","free","memset","memcpy","strlen",
            "strcpy","strcat","strcmp",NULL
        };
        for (int bi = 0; builtins[bi]; bi++)
            if (!strcmp(name, builtins[bi])) return;

        char decl[512] = {0};
        snprintf(decl, sizeof decl, "extern long long %s(", name);

        if (matched == 2 && args[0] != '\0') {
            char argcopy[256];
            strncpy(argcopy, args, sizeof argcopy - 1);
            char *tok2 = strtok(argcopy, ",");
            int first = 1;
            while (tok2) {
                while (isspace((unsigned char)*tok2)) tok2++;
                if (!first) strncat(decl, ", ", sizeof decl - strlen(decl) - 1);
                if (strstr(tok2, "str"))
                    strncat(decl, "char*", sizeof decl - strlen(decl) - 1);
                else
                    strncat(decl, "long long", sizeof decl - strlen(decl) - 1);
                first = 0;
                tok2 = strtok(NULL, ",");
            }
        }
        strncat(decl, ");\n", sizeof decl - strlen(decl) - 1);
        emit_header(decl);
        return;
    }

    /* ── call: call a void function ──────────────────────────────────────── */
    if (!strcmp(tok, "call")) {
        char rest[256] = {0};
        sscanf(l, "call %255[^\n]", rest);
        
        /* Wrap string literals in (long long) cast */
        char processed[256] = {0};
        char *src = rest;
        char *dst = processed;
        int in_string = 0;
        
        while (*src && dst - processed < (int)sizeof(processed) - 20) {
            if (*src == '"') {
                if (!in_string) {
                    /* Starting a string, add (long long) before it */
                    dst += sprintf(dst, "(long long)\"");
                    src++;
                    in_string = 1;
                } else {
                    /* Ending a string */
                    *dst++ = '"';
                    src++;
                    in_string = 0;
                }
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
        
        strcpy(rest, processed);
        
        /* Add () if the call has no argument list */
        if (!strchr(rest, '('))
            emit("%s();\n", rest);
        else
            emit("%s;\n", rest);
        return;
    }

    /* ── let: variable declaration / assignment ───────────────────────────── */
    if (!strcmp(tok, "let")) {
        /* Parse:  let <name> = <rhs>   or   let <name>  */
        char name[64] = {0};
        sscanf(l + 4, " %63[^= \t]", name);

        char *eq  = strchr(l, '=');
        int already = is_gvar(name) || is_decl(name);

        if (!eq) {
            /* Declaration with no initialiser */
            if (!already) { register_decl(name); emit("long long %s = 0;\n", name); }
            else           emit("%s = 0;\n", name);
            return;
        }

        char *rhs = eq + 1;
        while (isspace((unsigned char)*rhs)) rhs++;

        if (!already) register_decl(name);

        /* let x = call f(...)  — function call returning a value */
        if (strncmp(rhs, "call ", 5) == 0) {
            if (already) emit("%s = %s;\n",           name, rhs + 5);
            else         emit("long long %s = %s;\n", name, rhs + 5);
            return;
        }

        /* let x = @(addr)  — raw memory read */
        if (rhs[0] == '@' && rhs[1] == '(') {
            char *close = strchr(rhs + 2, ')');
            if (close) {
                char addr[128] = {0};
                strncpy(addr, rhs + 2, (size_t)(close - rhs - 2));
                if (already) emit("%s = *((long long*)(%s));\n",           name, addr);
                else         emit("long long %s = *((long long*)(%s));\n", name, addr);
            }
            return;
        }

        /* let x = malloc(...) / calloc(...)  — heap allocation */
        if (strstr(rhs, "malloc") || strstr(rhs, "calloc")) {
            if (already) emit("%s = (long long)%s;\n",           name, rhs);
            else         emit("long long %s = (long long)%s;\n", name, rhs);
            return;
        }

        /* Generic expression */
        if (rhs[0] == '"') {
            if (already) emit("%s = (long long)%s;\n", name, rhs);
            else         emit("long long %s = (long long)%s;\n", name, rhs);
        } else {
            if (already) emit("%s = %s;\n", name, rhs);
            else         emit("long long %s = %s;\n", name, rhs);
        }
        return;
    }

    /* ── listen: read a string from stdin ───────────────────────────────── */
    if (!strcmp(tok, "listen")) {
        char name[64] = {0};
        sscanf(l, "listen %63s", name);
        register_strvar(name);
        emit("char %s[256]; scanf(\"%%255s\", %s);\n", name, name);
        return;
    }

    /* ── ask: read a number from stdin ───────────────────────────────────── */
    if (!strcmp(tok, "ask")) {
        char name[64] = {0};
        sscanf(l, "ask %63s", name);
        emit("scanf(\"%%lld\", &%s);\n", name);
        return;
    }

    /* ── say: print a value or string literal ─────────────────────────────── */
    if (!strcmp(tok, "say")) {
        char *val = l + 3;
        while (isspace((unsigned char)*val)) val++;

        if (val[0] == '"') {
            /* say "literal" */
            emit("printf(\"%%s\\n\", %s);\n", val);
        } else if (val[0] == '\0') {
            emit("printf(\"\\n\");\n");
        } else {
            /* say varname — check for explicit type prefix: say str x */
            char type[16] = {0}, vname[64] = {0};
            int matches = sscanf(val, "%15s %63s", type, vname);
            if (matches == 2 && !strcmp(type, "str")) {
                emit("printf(\"%%s\\n\", (char*)%s);\n", vname);
            } else {
                /* Default: treat as number */
                sscanf(val, "%63s", vname);
                emit("printf(\"%%lld\\n\", (long long)%s);\n", vname);
            }
        }
        return;
    }

    if (!strcmp(tok, "saynum")) {
        char vname[64] = {0};
        sscanf(l, "saynum %63s", vname);
        emit("printf(\"%%lld\\n\", %s);\n", vname);
        return;
    }

    /* ── show: print a number inline (no newline) ─────────────────────────── */
    if (!strcmp(tok, "show")) {
        char vname[64] = {0};
        sscanf(l, "show %63s", vname);
        emit("printf(\"%%lld \", (long long)%s);\n", vname);
        return;
    }

    /* ── showstr: print a string inline (no newline) ──────────────────────── */
    if (!strcmp(tok, "showstr")) {
        char *val = l + 7;
        while (isspace((unsigned char)*val)) val++;
        if (val[0] == '"')
            emit("printf(\"%%s\", %s);\n", val);
        else
            emit("printf(\"%%s\", (char*)%s);\n", val);
        return;
    }
    /* ── if / loop: open a block ─────────────────────────────────────────── */
    if (!strcmp(tok, "if") || !strcmp(tok, "loop")) {
        const char *kw = !strcmp(tok, "loop") ? "while" : "if";
        const char *after = l + strlen(tok);
        while (isspace((unsigned char)*after)) after++;

        /* Check for single-line form:  if cond { body } */
        char *open_brace  = strchr(after, '{');
        char *close_brace = open_brace ? strrchr(after, '}') : NULL;
        if (open_brace && close_brace && close_brace > open_brace) {
            /* Extract condition (between keyword and '{') */
            char cond[256] = {0};
            size_t clen = (size_t)(open_brace - after);
            if (clen >= sizeof cond) clen = sizeof cond - 1;
            strncpy(cond, after, clen);
            rtrim(cond);

            /* Extract body (between '{' and '}') */
            char body_stmt[512] = {0};
            size_t blen = (size_t)(close_brace - open_brace - 1);
            if (blen >= sizeof body_stmt) blen = sizeof body_stmt - 1;
            strncpy(body_stmt, open_brace + 1, blen);
            /* Trim leading/trailing whitespace from body */
            char *bs = body_stmt;
            while (isspace((unsigned char)*bs)) bs++;
            rtrim(bs);

            emit("%s (%s) {\n", kw, cond);
            depth++;
            if (fn_braces > 0) fn_braces++;
            /* Compile the inner statement */
            compile_line(bs);
            depth--;
            if (fn_braces > 0) fn_braces--;
            emit("}\n");
            return;
        }

        /* Multi-line form: condition is everything up to the optional '{' */
        char cond[256] = {0};
        strncpy(cond, after, sizeof cond - 1);
        char *end = cond + strlen(cond) - 1;
        while (end >= cond && (isspace((unsigned char)*end) || *end == '{'))
            *end-- = '\0';
        emit("%s (%s) {\n", kw, cond);
        depth++;
        if (fn_braces > 0) fn_braces++;
        return;
    }

    /* ── stop: break out of a loop ───────────────────────────────────────── */
    if (!strcmp(tok, "stop")) {
        emit("break;\n");
        return;
    }

    /* ── free: release heap memory ───────────────────────────────────────── */
    if (!strncmp(l, "free(", 5)) {
        char name[64] = {0};
        sscanf(l, "free(%63[^)])", name);
        emit("free((void*)%s);\n", name);
        return;
    }

    /* ── @(addr) = val: raw memory write ─────────────────────────────────── */
    if (l[0] == '@') {
        char *open  = strchr(l, '(');
        char *close = strchr(l, ')');
        char *eq    = strchr(l, '=');
        if (open && close && eq && eq > close) {
            char addr[128] = {0}, val[128] = {0};
            strncpy(addr, open + 1, (size_t)(close - open - 1));
            char *rhs = eq + 1;
            while (isspace((unsigned char)*rhs)) rhs++;
            strncpy(val, rhs, sizeof val - 1);
            emit("*((long long*)(%s)) = %s;\n", addr, val);
            return;
        }
    }

    /* ── x = @(addr): raw memory read (assignment form) ──────────────────── */
    char *eq = strchr(l, '=');
    if (eq) {
        char *rhs = eq + 1;
        while (isspace((unsigned char)*rhs)) rhs++;
        if (rhs[0] == '@' && rhs[1] == '(') {
            char *close = strchr(rhs + 2, ')');
            if (close) {
                char lhsname[64] = {0}, addr[128] = {0};
                strncpy(lhsname, l, (size_t)(eq - l));
                rtrim(lhsname);
                strncpy(addr, rhs + 2, (size_t)(close - rhs - 2));
                emit("%s = *((long long*)(%s));\n", lhsname, addr);
                return;
            }
        }
    }

    /* ── Fallback: emit verbatim as a C statement ─────────────────────────── */
    {
        int n = (int)strlen(l);
        if (n > 0 && l[n-1] == ';')
            emit("%s\n", l);   /* already has semicolon */
        else
            emit("%s;\n", l);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Global-variable promotion pass
 *
 * After the first compile pass, scan `body` for variables that also appear
 * inside `funcs`.  Promote those to global scope so both main() and user
 * functions can access them.
 * ═══════════════════════════════════════════════════════════════════════════ */
static void promote_globals(void) {
    /* Walk body looking for "long long <name>" declarations */
    char scan[sizeof body];
    strncpy(scan, body, sizeof scan - 1);
    char *p = scan;

    while ((p = strstr(p, "long long "))) {
        p += 10; /* skip "long long " */
        char name[64] = {0};
        int i = 0;
        while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < 63)
            name[i++] = *p++;
        if (!name[0]) continue;

        /* Does this name appear in funcs? */
        if (!strstr(funcs, name)) continue;

        /* Is it also declared as "long long <name>" in funcs? (already promoted) */
        char search[80];
        snprintf(search, sizeof search, "long long %s", name);
        if (strstr(funcs, search)) continue;

        /* Promote: strip "long long " prefix from body's declaration */
        register_gvar(name);
        char *decl = strstr(body, search);
        if (decl) {
            size_t skip = strlen("long long ");
            memmove(decl, decl + skip, strlen(decl + skip) + 1);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════════════════ */
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: sc <file.sc>\n");
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) { perror(argv[1]); return 1; }

    out = body;   /* start writing into main()'s body */

    char line[LINESZ];
    while (fgets(line, sizeof line, in))
        compile_line(line);
    fclose(in);

    /* Promote variables used in both main and functions to global scope */
    promote_globals();

    /* Build the globals declarations string */
    char globals_buf[8000] = {0};
    for (int i = 0; i < gvar_n; i++) {
        char tmp[96];
        snprintf(tmp, sizeof tmp, "long long %s = 0;\n", gvars[i]);
        strncat(globals_buf, tmp, sizeof globals_buf - strlen(globals_buf) - 1);
    }

    /* Write output.c */
    FILE *out_file = fopen("output.c", "w");
    if (!out_file) { perror("output.c"); return 1; }

    fprintf(out_file,
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include \"engine.h\"\n"
        "%s"        /* extra headers (use / ext) */
        "\n"
        "%s"        /* global variable declarations */
        "\n"
        "%s"        /* user-defined functions       */
        "int main() {\n"
        "%s"        /* main() body                  */
        "\n"
        "    return 0;\n"
        "}\n",
        headers,
        globals_buf,
        funcs,
        body);

    fclose(out_file);
    printf("Compiled to output.c\n");
    return 0;
}
