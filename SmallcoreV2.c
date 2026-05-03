/*
 * sc.c  —  SmallCore Compiler  (transpiles .sc → .c, then calls gcc)
 * ─────────────────────────────────────────────────────────────────────
 *  TYPES         n=long long   f=double   s=char*   @=void*
 *  KEYWORDS      fn  st  if  el  lp  fr  rt  mk  pr  us
 *  GOD MODE (@)  @[p]  @[p:f]  @cast(T,p)  @size(T)  @addr(x)  @ref(p,T)
 *
 *  DECLARATION   name:type = expr    e.g.  x:n = 10   buf:@ = mk Node
 *  FUNCTION      fn name(a:n, b:f):s {
 *  STRUCT        st Name { field:type ... }
 *  IF / ELSE     if cond {  ...  } el {
 *  LOOP          lp cond {   /   fr init;cond;step {
 *  RETURN        rt expr
 *  PRINT         pr expr   /   pr "fmt" a b
 *  INCLUDE       us math   /   us "file.h"
 *  COMMENTS      # text
 *  PASS-THROUGH  any unrecognised line emits verbatim + auto-semicolon
 * ─────────────────────────────────────────────────────────────────────
 *  TWO PASSES:
 *    Pass 1 — collect structs + fn sigs → emit typedefs + forward decls
 *    Pass 2 — emit bodies line by line
 * ─────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── tiny helpers ───────────────────────────────────────────── */

static char *ltr(char *s) { while(isspace((unsigned char)*s)) s++; return s; }
static char *rtr(char *s) {
    char *e = s+strlen(s)-1;
    while(e>=s && isspace((unsigned char)*e)) *e--=0;
    return s;
}
static char *tr(char *s) { return rtr(ltr(s)); }
static void  ob(char *s) { char *p=strrchr(s,'{'); if(p)*p=0; rtr(s); } /* strip trailing { */

/* ── type map: SmallCore letter → C type string ─────────────── */

static const char *cty(const char *t) {
    if(!t||!*t)       return "long long";
    if(!strcmp(t,"n")) return "long long";
    if(!strcmp(t,"f")) return "double";
    if(!strcmp(t,"s")) return "char*";
    if(!strcmp(t,"@")) return "void*";
    if(!strcmp(t,"void")) return "void";
    return t; /* fallback: pass raw type name through (allows 'int', etc.) */
}

/* ── rewrite "name:type" param/decl → "ctype name" ─────────── */
/* e.g. "a:n" → "long long a",  "buf:@" → "void* buf"           */
static void rewrite_decl(const char *in, char *out, size_t sz) {
    char tmp[256]; strncpy(tmp, in, 255); tr(tmp);
    char *colon = strchr(tmp, ':');
    if (colon) {
        *colon = 0;
        char name[128]; char type[64];
        strncpy(name, tr(tmp), 127);
        strncpy(type, tr(colon+1), 63);
        /* strip trailing = and value if present in the type field */
        char *eq = strchr(type,'='); if(eq)*eq=0; rtr(type);
        snprintf(out, sz, "%s %s", cty(type), name);
    } else {
        /* no colon: default type is long long */
        snprintf(out, sz, "long long %s", tr(tmp));
    }
}

/* ── rewrite fn signature line ──────────────────────────────── */
/* "fn name(a:n, b:f):s" → "char* name(long long a, double b)"  */
static void rewrite_fn(const char *rest, char *out, size_t sz) {
    char tmp[512]; strncpy(tmp, rest, 511); ob(tmp); tr(tmp);

    /* split return type: last '):type' */
    char *rp = strrchr(tmp, ')');
    char rettype[32] = "long long";
    if (rp && *(rp+1)==':') { strncpy(rettype, rp+2, 31); tr(rettype); *rp=0; rp=NULL; }
    else if (rp) *rp=0;

    /* split name and params: name(params */
    char *pp = strchr(tmp, '(');
    char name[128]=""; char params[256]="";
    if (pp) { *pp=0; strncpy(name,tr(tmp),127); strncpy(params,tr(pp+1),255); }
    else strncpy(name,tr(tmp),127);

    /* rewrite each param */
    char out_params[512]="";
    if (*params) {
        char pbuf[256]; strncpy(pbuf,params,255);
        char *tok = strtok(pbuf, ",");
        int first=1;
        while (tok) {
            char pout[128]=""; rewrite_decl(tr(tok), pout, 128);
            if(!first) strcat(out_params, ", ");
            strcat(out_params, pout);
            first=0; tok=strtok(NULL,",");
        }
    }
    snprintf(out, sz, "%s %s(%s)", cty(rettype), name, out_params);
}

/* ── rewrite @ god-mode expressions ────────────────────────── */
/* Transforms @ operators in a line of text.                    */
static void rewrite_at(const char *in, char *out, size_t sz) {
    const char *r=in; char *w=out; size_t rem=sz-1;
    while (*r && rem>0) {
        if (*r=='@') {
            r++;
            char tmp[256]={0}; int n=0;
            /* @[ptr:type] or @[ptr]  — dereference */
            if (*r=='[') {
                r++;
                while(*r&&*r!=']'&&n<255) tmp[n++]=*r++;
                if(*r==']') r++;
                /* check for :type inside */
                char *col=strchr(tmp,':');
                if(col){ *col=0;
                    int k=snprintf(w,rem,"*((%s*)%s)",cty(col+1),tr(tmp));
                    w+=k;rem-=k;
                } else {
                    int k=snprintf(w,rem,"*((long long*)%s)",tr(tmp));
                    w+=k;rem-=k;
                }
            }
            /* @cast(Type, ptr) */
            else if (!strncmp(r,"cast(",5)) {
                r+=5;
                while(*r&&*r!=')'&&n<255) tmp[n++]=*r++;
                if(*r==')') r++;
                char *com=strchr(tmp,',');
                if(com){*com=0;
                    int k=snprintf(w,rem,"((%s*)%s)",tr(tmp),tr(com+1));
                    w+=k;rem-=k;
                } else {
                    int k=snprintf(w,rem,"((%s*)%s)",tr(tmp),tr(tmp));
                    w+=k;rem-=k;
                }
            }
            /* @size(Type) */
            else if (!strncmp(r,"size(",5)) {
                r+=5;
                while(*r&&*r!=')'&&n<255) tmp[n++]=*r++;
                if(*r==')') r++;
                int k=snprintf(w,rem,"sizeof(%s)",tr(tmp));
                w+=k;rem-=k;
            }
            /* @addr(x) */
            else if (!strncmp(r,"addr(",5)) {
                r+=5;
                while(*r&&*r!=')'&&n<255) tmp[n++]=*r++;
                if(*r==')') r++;
                int k=snprintf(w,rem,"(&%s)",tr(tmp));
                w+=k;rem-=k;
            }
            /* @ref(ptr,Type) — typed deref returning struct pointer */
            else if (!strncmp(r,"ref(",4)) {
                r+=4;
                while(*r&&*r!=')'&&n<255) tmp[n++]=*r++;
                if(*r==')') r++;
                char *com=strchr(tmp,',');
                if(com){*com=0;
                    int k=snprintf(w,rem,"((%s*)%s)",tr(com+1),tr(tmp));
                    w+=k;rem-=k;
                } else {
                    int k=snprintf(w,rem,"((void*)%s)",tr(tmp));
                    w+=k;rem-=k;
                }
            }
            /* bare @ — void* passthrough marker */
            else { *w++='@'; rem--; }
        } else { *w++=*r++; rem--; }
    }
    *w=0;
}

/* ── variable type registry (for smart pr) ──────────────────── */
/* Stores declared types so `pr x` knows to use %g vs %lld etc. */
static struct { char name[64]; char type; } vtab[512];
static int nvars=0;
static void vset(const char*name,char type){
    for(int i=0;i<nvars;i++) if(!strcmp(vtab[i].name,name)){vtab[i].type=type;return;}
    if(nvars<512){strncpy(vtab[nvars].name,name,63);vtab[nvars].type=type;nvars++;}
}
static char vget(const char*name){
    for(int i=0;i<nvars;i++) if(!strcmp(vtab[i].name,name))return vtab[i].type;
    return 'n';
}

/* ── emit pr (smart print) ──────────────────────────────────── */
static void emit_pr(FILE*out,int d,const char*rest){
    for(int i=0;i<d*4;i++)fputc(' ',out);
    if(!*rest){fprintf(out,"printf(\"\\n\");\n");return;}
    /* string literal */
    if(rest[0]=='"'){
        const char*q=rest+1; while(*q&&*q!='"') q++;
        char fmt[256]={0}; strncpy(fmt,rest+1,q-rest-1);
        const char*args=*q?ltr((char*)q+1):"";
        if(*args && strchr(fmt,'%')){
            /* split args by space, rewrite each, comma-join */
            char awork[256]; strncpy(awork,args,255);
            char joined[512]={0};
            char *tok2=strtok(awork," \t"); int first2=1;
            while(tok2){
                char rb2[128]={0}; rewrite_at(tok2,rb2,sizeof rb2);
                if(!first2) strcat(joined,",");
                strcat(joined,rb2); first2=0; tok2=strtok(NULL," \t");
            }
            fprintf(out,"printf(\"%s\\n\",%s);\n",fmt,joined);
        } else fprintf(out,"printf(\"%s\\n\");\n",fmt);
        return;
    }
    /* variable/expression — look up type */
    char rb[256]={0}; rewrite_at(rest,rb,256);
    char t=vget(rest); /* look up bare name */
    if(t=='f') fprintf(out,"printf(\"%%g\\n\",(double)(%s));\n",rb);
    else if(t=='s') fprintf(out,"printf(\"%%s\\n\",(char*)(%s));\n",rb);
    else fprintf(out,"printf(\"%%lld\\n\",(long long)(%s));\n",rb);
}

/* ── pass 1: collect structs + fn sigs ──────────────────────── */
static void pass1(FILE*in,FILE*out){
    fprintf(out,"#include <stdio.h>\n#include <stdlib.h>\n"
                "#include <string.h>\n#include <math.h>\n#include <time.h>\n");
    char line[1024]; int in_st=0; char stbuf[2048]={0}; char stname[64]={0};
    /* forward decl storage */
    char fwds[128][512]; int nfwd=0;
    rewind(in);
    while(fgets(line,sizeof line,in)){
        char*t=tr(line); if(!*t||t[0]=='#') continue;
        char kw[16]={0},rest[1008]={0};
        sscanf(t,"%15s %1007[^\n]",kw,rest); tr(rest);

        if(!strcmp(kw,"us")){
            if(rest[0]=='"') fprintf(out,"#include %s\n",rest);
            else fprintf(out,"#include <%s.h>\n",rest);
        }
        /* struct: accumulate multi-line or parse single-line */
        else if(!strcmp(kw,"st")){
            char*ob2=strchr(rest,'{'),*cb=strchr(rest,'}');
            sscanf(rest,"%63s",stname); char*b=strchr(stname,'{');if(b)*b=0;
            if(ob2&&cb){ /* single line */
                fprintf(out,"typedef struct{");
                char fbuf[512]={0}; strncpy(fbuf,ob2+1,cb-ob2-1); tr(fbuf);
                char*tok=strtok(fbuf," \t");
                while(tok){char fd[128]={0};rewrite_decl(tok,fd,128);fprintf(out,"%s;",fd);tok=strtok(NULL," \t");}
                fprintf(out,"}%s;\n",stname);
            } else { in_st=1; snprintf(stbuf,sizeof stbuf,"typedef struct{"); }
        }
        else if(in_st){
            if(t[0]=='}'){
                char sb2[256]; snprintf(sb2,sizeof sb2,"%s}%s;\n",stbuf,stname);
                fprintf(out,"%s",sb2); in_st=0; memset(stbuf,0,sizeof stbuf);
            } else {
                char*tok=strtok(t," \t");
                while(tok){char fd[128]={0};rewrite_decl(tok,fd,128);
                    strncat(stbuf,fd,sizeof stbuf-strlen(stbuf)-4);
                    strncat(stbuf,";",sizeof stbuf-strlen(stbuf)-2);
                    tok=strtok(NULL," \t");}
            }
        }
        /* fn forward decl */
        else if(!strcmp(kw,"fn")&&nfwd<128){
            char sig[512]={0}; rewrite_fn(rest,sig,512);
            snprintf(fwds[nfwd++],512,"%s;",sig);
        }
    }
    fprintf(out,"\n");
    for(int i=0;i<nfwd;i++) fprintf(out,"%s\n",fwds[i]);
    fprintf(out,"\n");
}

/* ── pass 2: emit bodies ────────────────────────────────────── */
static void pass2(FILE*in,FILE*out){
    char line[1024]; int d=0; int skip_st=0;
    rewind(in);
    while(fgets(line,sizeof line,in)){
        char buf[1024]; strncpy(buf,line,1023);
        char*t=tr(buf);
        if(!*t){fputc('\n',out);continue;}
        if(t[0]=='#'){fprintf(out,"//%s\n",t+1);continue;}

        char kw[16]={0},rest[1008]={0};
        sscanf(t,"%15s %1007[^\n]",kw,rest); tr(rest);

        /* skip — already handled in pass1 */
        if(!strcmp(kw,"us")) continue;
        if(!strcmp(kw,"st")){
            skip_st=strchr(t,'}')?0:1; /* single-line st: no skip */
            continue;
        }
        if(skip_st){ if(t[0]=='}')skip_st=0; continue; }

        /* } close block */
        if(!strcmp(kw,"}")){
            if(d>0)d--;
            for(int i=0;i<d*4;i++)fputc(' ',out);
            fprintf(out,"}\n"); continue;
        }
        /* fn */
        if(!strcmp(kw,"fn")){
            char sig[512]={0}; rewrite_fn(rest,sig,512);
            fprintf(out,"\n%s {\n",sig); d=1; continue;
        }
        /* if */
        if(!strcmp(kw,"if")){
            char rb[512]={0}; rewrite_at(rest,rb,512); ob(rb);
            for(int i=0;i<d*4;i++)fputc(' ',out);
            /* inline single-line: if cond { body } */
            char*ib=strchr(t,'{'),*ic=strrchr(t,'}');
            if(ib&&ic&&ic>ib){
                char cond[256]={0},body[256]={0};
                strncpy(cond,ltr(t+2),ib-t-2);rtr(cond);
                char cr[256]={0};rewrite_at(cond,cr,256);ob(cr);
                strncpy(body,ib+1,ic-ib-1);tr(body);
                char br[256]={0};rewrite_at(body,br,256);
                /* expand sc keywords inside single-line if body */
                char bkw[16]={0},brst[240]={0};
                sscanf(br,"%15s %239[^\n]",bkw,brst);
                if(!strcmp(bkw,"rt")){
                    char rb2[256]={0};rewrite_at(*brst?brst:"0",rb2,256);
                    fprintf(out,"if(%s){return %s;}\n",cr,rb2);
                } else {
                    char rb2[256]={0};rewrite_at(br,rb2,256);
                    fprintf(out,"if(%s){%s;}\n",cr,rb2);
                }
            } else { fprintf(out,"if(%s){\n",rb); d++; }
            continue;
        }
        /* el */
        if(!strcmp(kw,"el")){
            if(d>0)d--;
            for(int i=0;i<d*4;i++)fputc(' ',out);
            fprintf(out,"}else{\n"); d++; continue;
        }
        /* lp loop */
        if(!strcmp(kw,"lp")){
            char rb[512]={0}; rewrite_at(rest,rb,512); ob(rb);
            for(int i=0;i<d*4;i++)fputc(' ',out);
            fprintf(out,"while(%s){\n",rb); d++; continue;
        }
        /* fr for */
        if(!strcmp(kw,"fr")){
            char rb[512]={0}; rewrite_at(rest,rb,512); ob(rb);
            for(int i=0;i<d*4;i++)fputc(' ',out);
            fprintf(out,"for(%s){\n",rb); d++; continue;
        }
        /* rt return */
        if(!strcmp(kw,"rt")){
            for(int i=0;i<d*4;i++)fputc(' ',out);
            if(*rest){char rb[512]={0};rewrite_at(rest,rb,512);fprintf(out,"return %s;\n",rb);}
            else fprintf(out,"return 0;\n");
            continue;
        }
        /* mk make — alloc struct */
        if(!strcmp(kw,"mk")){
            for(int i=0;i<d*4;i++)fputc(' ',out);
            fprintf(out,"malloc(sizeof(%s));\n",rest); continue;
        }
        /* pr print */
        if(!strcmp(kw,"pr")){ emit_pr(out,d,rest); continue; }

        /* variable declaration: name:type = expr  OR  name:type */
        if(strchr(t,':') && t[0]!='@'){
            char*eq=strchr(t,'=');
            char lhs[128]={0}; char rhs[512]={0};
            if(eq){ strncpy(lhs,t,eq-t); tr(lhs); strncpy(rhs,eq+1,511); tr(rhs); }
            else   { strncpy(lhs,t,127); tr(lhs); }
            /* parse name:type from lhs */
            char*col=strchr(lhs,':');
            if(col){
                *col=0;
                char nm[64]={0},ty[32]={0};
                strncpy(nm,tr(lhs),63); strncpy(ty,tr(col+1),31);
                vset(nm,ty[0]); /* register type for pr */
                for(int i=0;i<d*4;i++)fputc(' ',out);
                if(*rhs){
                    char rb[512]={0};
                    if(!strncmp(ltr(rhs),"mk ",3)){
                        snprintf(rb,sizeof rb,"malloc(sizeof(%s))",ltr(rhs)+3);
                    } else rewrite_at(rhs,rb,sizeof rb);
                    fprintf(out,"%s %s = %s;\n",cty(ty),nm,rb);
                } else fprintf(out,"%s %s;\n",cty(ty),nm);
                continue;
            }
        }

        /* pass-through: rewrite @ ops, auto-semicolon */
        for(int i=0;i<d*4;i++)fputc(' ',out);
        char rb[1024]={0}; rewrite_at(t,rb,1024);
        int len=strlen(rb); char last=rb[len-1];
        int ns=(last!=';'&&last!='{'&&last!='}'&&rb[0]!='#');
        fprintf(out,"%s%s\n",rb,ns?";":"");
    }
}

/* ── main ───────────────────────────────────────────────────── */
int main(int argc, char**argv){
    if(argc<2){fprintf(stderr,"usage: sc file.sc\n");return 1;}
    FILE*in=fopen(argv[1],"r");
    if(!in){perror(argv[1]);return 1;}

    char out_c[256]; strncpy(out_c,argv[1],253);
    char*dot=strrchr(out_c,'.'); if(dot)strcpy(dot,".c"); else strcat(out_c,".c");

    FILE*out=fopen(out_c,"w");
    if(!out){perror(out_c);return 1;}

    pass1(in,out);
    pass2(in,out);

    fclose(in); fclose(out);
    fprintf(stderr,"sc → %s\n",out_c);
    return 0;
}
