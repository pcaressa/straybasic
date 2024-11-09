/// \file straybasic.c
/// \author Paolo Caressa <github.com/pcaressa>
/// \date 20241109

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//  Old fashioned debug stuff...
//~ #define IN fprintf(stderr, "> %s:%s:%i\n", __func__, __FILE__, __LINE__);
#define IN

// /////////////////////////////// CONSTANTS /////////////////////////////// //

#define BUF_SIZE (256)
#define DATA_SIZE (1024)
#define FILE_NAME_SIZE (9)
#define PROG_SIZE (8192)
#define RAM_SIZE (65536)
#define RSTACK_SIZE (64)
#define SSTACK_SIZE (64)
#define STACK_SIZE (128)

enum {
    CODE_INTLIT = 128, CODE_NUMLIT, CODE_STRLIT, CODE_IDN, CODE_IDNS,
    CODE_STARTKEYWORD,
        CODE_BYE, CODE_CLOSE, CODE_DATA, CODE_DEF, CODE_DIM, CODE_ERROR,
        CODE_FOR, CODE_GOSUB, CODE_GOTO, CODE_IF, CODE_INPUT, CODE_LET,
        CODE_LINPUT, CODE_LIST, CODE_LOAD, CODE_MERGE, CODE_NEW, CODE_NEXT,
        CODE_ONERROR, CODE_OPEN, CODE_PRINT, CODE_READ, CODE_REPEAT,
        CODE_RESTORE, CODE_RETURN, CODE_RUN, CODE_SAVE, CODE_SKIP, CODE_STOP,
        CODE_SYS,
    CODE_ENDKEYWORD,
    CODE_STARTOPERATOR,
        CODE_AMP, CODE_MUL, CODE_PLUS, CODE_MINUS, CODE_NEG, CODE_DIV, CODE_LT,
        CODE_LEQ, CODE_NEQ, CODE_EQ, CODE_GT, CODE_GEQ, CODE_ABS, CODE_ACS,
        CODE_AND, CODE_ASC, CODE_ATN, CODE_CHRS, CODE_COS, CODE_EXP, CODE_INT,
        CODE_LEFTS, CODE_LEN, CODE_LOG, CODE_MIDS, CODE_NOT, CODE_OR,
        CODE_RIGHTS, CODE_RND, CODE_SIN, CODE_SQR, CODE_STRS, CODE_TAN,
        CODE_VAL, CODE_POW,
    CODE_ENDOPERATOR,
};

const char *KEYWORDS[] = {
    /*  Same ordering as the constant between CODE_STARTKEYWORD and
        CODE_ENDKEYWORD */
    "BYE", "CLOSE", "DATA", "DEF", "DIM", "ERROR", "FOR", "GOSUB", "GOTO", "IF",
    "INPUT", "LET", "LINPUT", "LIST", "LOAD", "MERGE", "NEW", "NEXT", "ONERROR",
    "OPEN", "PRINT", "READ", "REPEAT", "RESTORE", "RETURN", "RUN", "SAVE",
    "SKIP", "STOP", "SYS",
};

enum {
    ERROR_NONE,
    ERROR_CANNOT_OPEN_FILE,
    ERROR_CLOSEPAR_WITHOUT_OPENPAR,
    ERROR_COMMA_WITHOUT_OPENPAR,
    ERROR_DOMAIN,
    ERROR_EQ_EXPECTED,
    ERROR_ILLEGAL_CONVERSION,
    ERROR_ILLEGAL_INSTRUCTION, ERROR_ILLEGAL_LINE_NUMBER,
    ERROR_OPENPAR_WITHOUT_CLOSEPAR,
    ERROR_OUT_OF_STRING_MEMORY,
    ERROR_OUT_OF_VARIABLE_MEMORY,
    ERROR_PROGRAM_TOO_LONG,
    ERROR_STACK_OVERFLOW,
    ERROR_STACK_UNDERFLOW,
    ERROR_SUBSCRIPT_ERROR,
    ERROR_SUBSCRIPT_EXPECTED,
    ERROR_TYPE_MISMATCH,
    ERROR_UNDEFINED_VARIABLE,
    ERROR_VARIABLE_ALREADY_DEFINED
};

const char *ERROR[] = {
    "READY",
    "CANNOT OPEN FILE",
    "\")\" WITHOUT \"(\"",
    "\",\" WITHOUT \"(\"",
    "DOMAIN ERROR",
    "\"=\" EXPECTED",
    "ILLEGAL CONVERSION",
    "ILLEGAL INSTRUCTION",
    "ILLEGAL LINE NUMBER",
    "\"(\" WITHOUT \")\"",
    "NO MORE ROOM FOR STRINGS",
    "NO MORE ROOM FOR VARIABLES",
    "PROGRAM TOO LONG",
    "EXPRESSION TOO LONG",
    "MISSING VALUE",
    "SUBSCRIPT OUT OF RANGE",
    "MISSING SUBSCRIPT",
    "TYPE MISMATCH",
    "UNDEFINED VARIABLE",
    "VARIABLE ALREADY DEFINED"
};

#define STR_NULL (0xFFFF)   // can't be the address of any string.

enum {
    VAR_NULL = 0, VAR_CHR = 1, VAR_NUM = 2, VAR_STR = 4, VAR_FOR = 8,
    VAR_VEC = 16, VAR_MAT = 32
};

// ///////////////////////////////// TYPES ///////////////////////////////// //

typedef uint8_t byte_t;     // 1 byte
typedef uint16_t addr_t;    // pointer to any byte (2 bytes)
typedef uint16_t str_t;     // pointer to string (2 bytes)
typedef float num_t;        // number (4 bytes)

// ///////////////////////////// RUNTIME CLASS ///////////////////////////// //

/** A runtime object contains all data needed to store an instance of a
    virtual machine where Straybasic programs can be edited and run. */
typedef struct runtime_s
{
    byte_t ram[RAM_SIZE];

    addr_t dp0, dp, dt, pp0, pp, vp0, vp, sp0, sp, rsp0, rsp;
    addr_t obj, tib, buf1, buf2, buf3, buf4;

    addr_t ip0;     ///< first byte of the current line (its "size byte).
    addr_t ip;      ///< first byte of next token in the current line.
    addr_t ipn;     ///< first byte of the next line (needed by GOTO etc.).

    int prog_changed;   ///< 1 if the program was not saved since last changes.

    jmp_buf err_buffer;
    int error;

    /** Operator stack: contains operator with priorities lower than the one
        under execution, inside an expression. */
    struct { void (*routine)(struct runtime_s*); int priority; } estack[32];
    int estack_next;

} *runtime_t;

//  Common shortcuts: assume the runtime_t variable rt to be defined.
#define IP (rt->ip)
#define RAM (rt->ram)
#define CODE (RAM[IP])
#define ON_ERROR(c, e) if(c) longjmp(rt->err_buffer, rt->error = (e));
#define EXPECT(tok, msg) ON_ERROR(RAM[IP++] != tok, msg)

/** Pop and execute all routines on the rt->estack with priority > p. */
void runtime_epop(runtime_t rt, int p)
{IN
    while (rt->estack_next > 0 && rt->estack[rt->estack_next - 1].priority > p)
        (*rt->estack[--rt->estack_next].routine)(rt);
}

/** Push the routine address r and relative priority p on the estack: while
    on top of it there are operators with higher priorities, execute them. */
void runtime_epush(runtime_t rt, void (*r)(runtime_t), int p)
{IN
    ON_ERROR(rt->estack_next >= STACK_SIZE, ERROR_STACK_OVERFLOW);
    rt->estack[rt->estack_next].routine = r;
    rt->estack[rt->estack_next].priority = p;
    ++ rt->estack_next;
}

/** Reset the virtual machine. */
void runtime_reset(runtime_t rt)
{IN
    rt->dt = rt->dp = rt->dp0 = 0;
    rt->pp = rt->pp0 = rt->dp + DATA_SIZE;
    rt->vp = rt->vp0 = rt->pp + PROG_SIZE;
    
    rt->buf4 = RAM_SIZE - BUFSIZ;
    rt->buf3 = rt->buf4 - BUFSIZ;
    rt->buf2 = rt->buf3 - BUFSIZ;
    rt->buf1 = rt->buf2 - BUFSIZ;
    rt->tib = rt->buf1 - BUFSIZ;
    rt->obj = rt->tib - BUFSIZ;
    rt->rsp = rt->rsp0 = rt->obj - RSTACK_SIZE;
    rt->sp = rt->sp0 = rt->rsp0 - STACK_SIZE;
    
    rt->prog_changed = 0;
    rt->error = 0;
    
    rt->estack_next = 0;
}

// ////////////////////////////// MEMORY ACCESS ///////////////////////////// //

addr_t peek(byte_t *a) { return *a + (a[1] << 8); }
num_t peek_num(byte_t *a) { return *(num_t*)a; }
void poke(byte_t *a, addr_t i) { *a = i & 0xFF; a[1] = i >> 8; }
void poke_num(byte_t *a, num_t n) { *(num_t*)a = n; }

// ////////////////////////////// DEBUG STUFF ////////////////////////////// //

void dump_data(runtime_t rt)
{
    puts("DATA");
    for (addr_t p = rt->dp0; p < rt->dp; p += strlen(rt->ram + p) + 1)
        printf(" \"%s\"", rt->ram + p);
    putchar('\n');
    puts("TEMP DATA");
    for (addr_t p = rt->dp; p < rt->dt; p += strlen(rt->ram + p) + 1)
        printf(" \"%s\"", rt->ram + p);
    putchar('\n');
}

void dump_estack(runtime_t rt)
{
    fputs("ESTACK: ", stdout);
    for (int i = 0; i< rt->estack_next; ++ i)
        printf(" [%p,%i]", rt->estack[i].routine, rt->estack[i].priority);
    putchar('\n');
}

addr_t dump_line(runtime_t rt, addr_t a)
{
    addr_t a1 = a + RAM[a];
    printf("%04x:", a);
    for (; a < a1; ++ a)
        printf(" %i", RAM[a]);
    putchar('\n');
    return a1;
}

/// Dump the memory status.
void dump_memory(runtime_t rt)
{
    puts("MEMORY OCCUPATION STATUS:");
    printf("    obj       = % 5i (% 2i%%)\n", RAM[rt->obj], RAM[rt->obj] / BUF_SIZE);
    printf("    DATA      = % 5i (% 2i%%)\n", rt->dp - rt->dp0, (rt->dp - rt->dp0) / (rt->pp0 - rt->dp0));
    printf("    PROGRAM   = % 5i (% 2i%%)\n", rt->pp - rt->pp0, (rt->pp - rt->pp0) / (rt->vp0 - rt->pp0));
    printf("    VARIABLES = % 5i (% 2i%%)\n", rt->vp - rt->vp0, (rt->vp - rt->vp0) / (rt->sp - rt->vp0));
    printf("    STACK     = % 5i (% 2i%%)\n", rt->sp - rt->sp0, (rt->sp - rt->sp0) / STACK_SIZE);
    printf("    RSTACK    = % 5i (% 2i%%)\n", rt->rsp - rt->rsp0, (rt->rsp - rt->rsp0) / RSTACK_SIZE);
}

void dump_program(runtime_t rt)
{
    printf("PROGRAM: PP0 = %p, PP = %p\n", RAM + rt->pp0, RAM + rt->pp);
    for (addr_t a = rt->pp0; a < rt->pp; a = dump_line(rt, a))
        ;
}

void dump_variables(runtime_t rt)
{
    puts("VARIABLES:");
    for (addr_t p = rt->vp0; p < rt->vp; p += peek(RAM + p)) {
        fputs(RAM + peek(RAM + p + sizeof(addr_t)), stdout);
        addr_t p1 = p + 2*sizeof(addr_t);
        int type = RAM[p1++];
        switch (type) {
        case VAR_NUM:
            printf(" = %g\n", peek_num(RAM + p1));
            continue;
        case VAR_VEC:
            printf("(%i)\n", peek(RAM + p1));
            continue;
        case VAR_MAT:
            printf("(%i,%i)\n", peek(RAM + p1), peek(RAM + p1 + sizeof(addr_t)));
            continue;
        case VAR_STR:
            printf(" = \"%s\"\n", RAM + p1);
            continue;
        case VAR_FOR:
            printf(" = %g TO %g STEP %g\n", peek_num(RAM + p1),
                peek_num(RAM + p1 + sizeof(num_t)), peek_num(RAM + p1 + 2*sizeof(num_t)));
            continue;
        default:
            puts(" UNKNOWN!!!");
            return;
}}}

// ////////////////////////////// STACK ACCESS ///////////////////////////// //

/*  Stacks have fixed size, so they grow upward: the parameter stack starts
    at sp0 and ends at sp0 + STACK_SIZE - 1; the stack pointer sp contains the
    address of the first free item in the stack.

    Each stack item is a pair (n,s) where n is a number and s a string address:
    thus each element takes 6 bytes. If s == STR_NULL then the item contains
    a number, else a string. */

void pop(runtime_t rt, num_t *n, str_t *s)
{IN
//~ printf("sp0 = %i, sp = %i, *n = %p, *s = %p, n = %g, s = %u\n", rt->sp0, rt->sp, n, s, *n, *s);
    ON_ERROR(rt->sp == rt->sp0, ERROR_STACK_UNDERFLOW);
    rt->sp -= sizeof(num_t);
    *n = peek_num(RAM + rt->sp);
    rt->sp -= sizeof(str_t);
    *s = peek(RAM + rt->sp);
//~ printf("sp0 = %i, sp = %i, *n = %p, *s = %p, n = %g, s = %u\n", rt->sp0, rt->sp, n, s, *n, *s);
}

num_t pop_num(runtime_t rt)
{IN
    num_t n;
    str_t s;
    pop(rt, &n, &s);
    ON_ERROR(s != STR_NULL, ERROR_TYPE_MISMATCH);
    return n;
}

str_t pop_str(runtime_t rt)
{IN
    num_t n;
    str_t s;
    pop(rt, &n, &s);
    ON_ERROR(s == STR_NULL, ERROR_TYPE_MISMATCH);
    return s;
}

void push(runtime_t rt, num_t n, str_t s)
{IN
//~ printf("PUSH %g %04x\n", n, s);
    ON_ERROR(rt->sp - rt->sp0 >= STACK_SIZE - sizeof(num_t) - sizeof(str_t),
        ERROR_STACK_OVERFLOW);
    poke(RAM + rt->sp, s);
    rt->sp += sizeof(str_t);
    poke_num(RAM + rt->sp, n);
    rt->sp += sizeof(num_t);
}

#define push_num(rt, n) push(rt, n, STR_NULL)
#define push_str(rt, s) { assert(s != STR_NULL); push(rt, 0, s); }

addr_t rpop(runtime_t rt)
{IN
    ON_ERROR(rt->rsp == rt->rsp0, ERROR_STACK_UNDERFLOW);
    addr_t a = peek(RAM + rt->rsp + 1);
    rt->rsp += sizeof(addr_t);
    return a;
}

void rpush(runtime_t rt, addr_t a)
{IN
    ON_ERROR(rt->rsp - rt->rsp0 >= RSTACK_SIZE - sizeof(addr_t), ERROR_STACK_OVERFLOW);
    rt->sp -= sizeof(addr_t);
    poke(RAM + rt->sp + 1, a);
}

// /////////////////////////// DATA (STRING) AREA /////////////////////////// //

/**
    The data area, starting at dp0 and ending at pp0, contains all string
    constants parsed in a program, along with all identifiers. It is maintained
    as a linked list, so to look for something in it requires linear time :(

    The first free item is pointed by dp: this is also considered to point to
    the first temporary strings, strings built during expression evaluations,
    while dt points to the first byte after all temporary strings. Since dt
    is set to dp after any expression evaluation, all temporary strings are
    discarded when another expression will be evaluated.
*/

/** Add a new string to the data area and return its address: if there's no
    more space then return a negative number. */
int data_add(runtime_t rt, char *p0, int len)
{IN
    if (rt->dp + len + 1 >= rt->pp0)
        return -1;
    memcpy(RAM + rt->dp, p0, len);
    RAM[rt->dp + len] = '\0';
    str_t k = rt->dp;
    rt->dp += len + 1;
    return k;
}

/** Add a temporary string to the data area and return its address: if there's
    no more space then return a negative number. */
int data_add_temp(runtime_t rt, char *p0, int len)
{IN
    if (rt->dt + len + 1 >= rt->pp0)
        return -1;
    memcpy(RAM + rt->dt, p0, len);
    RAM[rt->dt + len] = '\0';
    str_t k = rt->dt;
    rt->dt += len + 1;
    return k;
}

/** Look for a string in the data area: if found then return its address,
    else a negative number. */
int data_find(runtime_t rt, char *p0, int len)
{IN
    str_t s = rt->dp0;
    while (s < rt->dp) {
        int len1 = strlen(RAM + s);
        if (len1 == len && memcmp(RAM + s, p0, len) == 0)
            //~ return s - dp0;
            return s;
        s += len1 + 1;
    }
    return -1;
}

// //////////////////////////// PROGRAM EDITING //////////////////////////// //

/** A program line is stored as a sequence s,i1,i2,c1,...,cn of bytes, where
    s = 3 + n, i1+256*i2 is the line number, c1,...,cn are the bytecodes of
    the line. */

/** Delete the line with number n: return 1 if the line doesn't exist, else 0. */
int prog_delete(runtime_t rt, int n)
{IN
    for (addr_t a = rt->pp0; a < rt->pp; a += RAM[a]) {
        addr_t n1 = peek(RAM + a + 2);
        if (n1 > n) return 1;
        if (n1 == n) {
            // Overwrite the line by the following ones.
            int size = RAM[a];
            memmove(RAM + a, RAM + a + size, rt->pp - (a + size));
            rt->pp -= size;
            return 0;
    }}
    return 1;
}

/** Looks for a line with line number n: if found then its address is returned,
    else 0. */
int prog_find(runtime_t rt, int n)
{IN
    for (addr_t a = rt->pp0; a < rt->pp; a += RAM[a]) {
        addr_t n1 = peek(RAM + a + 2);
        if (n1 > n) return 0;
        if (n1 == n) return a;
    }
    return 0;
}

/** Insert line a with line number n (assume no such line exists). */
void prog_insert(runtime_t rt, int n, addr_t a)
{IN
    // Get in p the address of the first line with number > n.
    addr_t p = rt->pp0;
    while (p < rt->pp && peek(RAM + p + 2) < n)
        p += RAM[p];
    assert(p == rt->pp || peek(RAM + p + 2) != n);
    
    int size_new = RAM[a];
    if (rt->pp + size_new >= rt->vp0) {
        rt->error = ERROR_PROGRAM_TOO_LONG;
        return;
    }
    // If the new is not the last, makes room for it, shifting on the right.
    if (p != rt->pp) {
        memmove(RAM + p + size_new, RAM + p, rt->pp - p);
    }
    // Copy the line.
    memmove(RAM + p, RAM + a, size_new);
    rt->pp += size_new;
}

void prog_print(runtime_t rt, FILE *f)
{IN
    extern addr_t token_dump(runtime_t rt, addr_t a, FILE *f);
    addr_t p = rt->pp0;
    while (p < rt->pp) {
        fprintf(f, "%i ", peek(RAM + p + 2));
        p += 4;
        while (RAM[p] != 0)
            p = token_dump(rt, p, f);
        fputc('\n', f);
        ++ p;
}}

void prog_save(runtime_t rt, char *name)
{IN
    char name_ext[FILE_NAME_SIZE + 4];
    FILE *f = fopen(strcat(strcpy(name_ext, name), ".BAS"), "w");
    if (f == NULL) rt->error = ERROR_CANNOT_OPEN_FILE;
    else {
        prog_print(rt, f);
        fclose(f);
}}

/// Edit a line at ram[line] starting with a line number.
void prog_edit(runtime_t rt, addr_t line)
{IN
    // line = size; line+1 = CODE_INTLIT; line+2/+3 = integer; line+4 = line contents.
    int line_no = peek(RAM + line + 2);
    if (RAM[line + 4] == 0) {
        if (prog_delete(rt, line_no))
            printf("Line %i does not exist!\n", line_no);
    } else {
        // Overwrite (= delete + insert) the line.
        prog_delete(rt, line_no);
        prog_insert(rt, line_no, line);
}}

// /////////////////////////////// VARIABLES /////////////////////////////// //

/**
    Variables are stored from vp0 to vp-1 in the RAM in one of the following
    formats, that depend on the type of variable

        size, name, VAR_NUM, n
        size, name, VAR_NUM|VAR_VEC, i, n1, ..., ni
        size, name, VAR_NUM|VAR_MAT, i, j, n11, ..., n1i, ..., nj1, ..., nji
        size, name, VAR_STR, s
        size, name, VAR_STR|VAR_VEC, i, s1, ..., si
        size, name, VAR_STR|VAR_MAT, i, j, s11, ..., s1i, ..., sj1, ..., sji
        size, name, VAR_FOR, n (value), n1 (bound), n2 (step), ip0, ip1 - ip0

    Here size is a 16 bit unsigned, name the address of a string, n, n1, ...
    numbers, s, s1, ... C-strings (NOT addresses!), i and j 16 bit unsigned.
    A VAR_FOR includes the current value, the bound and the step, plus the
    address of the line where NEXT should jump and the offset in the line
    where NEXT should jump.
*/

/// Looks for a variable with name s and return its address, or STR_NULL.
str_t var_find(runtime_t rt, str_t s)
{IN
    for (addr_t p = rt->vp0; p < rt->vp; p += peek(RAM + p))
        if (strcmp(RAM + p + 2, RAM + s) == 0)
            return p;
    return STR_NULL;
}

/** Parse the subscripts of a vector or matrix variable: type is the actual type
    of the variable, p is assumed to point to the variable's i (first dimension)
    field, the resulting address of the variable's item will be stored into a1
    and its scalar type returned. */
static int var_array_parse(runtime_t rt, int type, addr_t p, addr_t *a1)
{IN
    extern void expr(runtime_t);
    int d1, d2, i, j = 1;
    int is_mat = type & VAR_MAT;
    d1 = peek(RAM + p); p += sizeof(addr_t);
    if (is_mat) { d2 = peek(RAM + p); p += sizeof(addr_t); }
    // Parse the subscript(s).
    EXPECT('(', ERROR_SUBSCRIPT_EXPECTED);
    expr(rt);
    i = pop_num(rt);
    ON_ERROR(i < 1 || i > d1, ERROR_SUBSCRIPT_ERROR);
    if (is_mat) {
        EXPECT(',', ERROR_SUBSCRIPT_EXPECTED);
        expr(rt);
        j = pop_num(rt);
        ON_ERROR(j < 1 || j > d2, ERROR_SUBSCRIPT_ERROR);
        EXPECT(')', ERROR_OPENPAR_WITHOUT_CLOSEPAR);
    }
    // According to the type, compute the address *a1 of the item.
    if (type & VAR_NUM) {
        *a1 = p + (i-1)*sizeof(num_t) + (j-1)*d1*sizeof(num_t);
        return VAR_NUM;
    }
    *a1 = p + (i-1)*sizeof(str_t) + (j-1)*d1*sizeof(str_t);
    return VAR_STR;
}

/** Parse a variable and get its address, writing it into *a0, and the address
    of the value pointed by the variable expression (e.g. a$(1) points to the
    first string of the a$ array). On returning, IP points to the next token and
    the return value is the type of the variable (VAR_CHR, VAR_NUM, VAR_STR).
    If the variable doesn't exist, VAR_NULL is returned and IP points to the
    variable's name (to the CODE_IDN/CODE_IDNS byte).
    If a syntax error occurs, then an exception is raised. */
int var_parse(runtime_t rt, addr_t *a0, addr_t *a1)
{IN
    extern void expr(runtime_t rt);
    // Looks for the string at IP + 1.
    *a0 = var_find(rt, IP + 1);
    if (*a0 == STR_NULL) return VAR_NULL;
    IP += 1 + sizeof(addr_t);
    addr_t p = *a0 + 2*sizeof(addr_t);  // p points to the variable type.
    int type = RAM[p++];
    if (type & (VAR_VEC|VAR_MAT)) return var_array_parse(rt, type, p, a1);
    // Scalar or for variable: return it!
    *a1 = p;
    return type;
}

// //////////////////////// OPERATOR IMPLEMENTATIONS //////////////////////// //

/** Pop two values and compare them returning the result as an integer, the same
    as if strcmp or memcmp has been executed. */
static int cmp(runtime_t rt)
{IN
    num_t n2;
    str_t s2;
    pop(rt, &n2, &s2);
    return (s2 == STR_NULL) ? pop_num(rt) - n2
                            : strcmp(RAM + pop_str(rt), RAM + s2);
}

void OPER_ABS(runtime_t rt) {IN push_num(rt, fabs(pop_num(rt))); }
void OPER_ACS(runtime_t rt) {IN push_num(rt, acos(pop_num(rt))); }

void OPER_AMP(runtime_t rt)
{IN
    char *s2 = RAM + pop_str(rt);
    char *s1 = RAM + pop_str(rt);
    unsigned l1 = strlen(s1), l2 = strlen(s2);
//~ printf("s1=%s, s2=%s, l1=%u, l2 =%u\n", s1, s2, l1, l2);
    // Create the concatenation as temporary string.
    int addr = data_add_temp(rt, s1, l1 + l2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    strcpy(s1 + l1, s2);
    push_str(rt, (byte_t*)s1 - RAM);
}

void OPER_AND(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 && n2);
}

void OPER_ASC(runtime_t rt) {IN push_num(rt, RAM[pop_str(rt)]); }
void OPER_ATN(runtime_t rt) {IN push_num(rt, atan(pop_num(rt))); }

void OPER_CHRS(runtime_t rt)
{IN
    char c[2];
    c[0] = pop_num(rt);
    c[1] = '\0';
    int addr = data_add_temp(rt, &c, 1);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_COS(runtime_t rt) {IN push_num(rt, cos(pop_num(rt))); }

void OPER_DIV(runtime_t rt)
{IN
    num_t n2 = pop_num(rt);
    push_num(rt, pop_num(rt) / n2);
}

void OPER_EQ(runtime_t rt) {IN push_num(rt, cmp(rt) == 0); }
void OPER_EXP(runtime_t rt) {IN push_num(rt, exp(pop_num(rt))); }
void OPER_GEQ(runtime_t rt) {IN push_num(rt, cmp(rt) >= 0); }
void OPER_GT(runtime_t rt) {IN push_num(rt, cmp(rt) > 0); }
void OPER_INT(runtime_t rt) {IN push_num(rt, floor(pop_num(rt))); }

void OPER_LEFTS(runtime_t rt)
{IN
    int n2 = pop_num(rt);
    char *s1 = RAM + pop_str(rt);
    ON_ERROR(n2 < 0 || n2 > strlen(s1), ERROR_SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = data_add_temp(rt, s1, n2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_LEN(runtime_t rt) {IN push_num(rt, strlen(RAM + pop_str(rt))); }
void OPER_LEQ(runtime_t rt)  {IN push_num(rt, cmp(rt) <= 0); }

void OPER_LOG(runtime_t rt)
{IN
    num_t n = pop_num(rt);
    ON_ERROR(n <= 0, ERROR_DOMAIN);
    push_num(rt, log(n));
}

void OPER_LT(runtime_t rt) {IN push_num(rt, cmp(rt) < 0); }

void OPER_MIDS(runtime_t rt)
{IN
    int n2 = pop_num(rt);
    int n1 = pop_num(rt) - 1;
    char *s1 = RAM + pop_str(rt);
    ON_ERROR(n2 < 0 || n1 + n2 > strlen(s1), ERROR_SUBSCRIPT_ERROR);
    ON_ERROR(n1 < 0 || n1 >= strlen(s1), ERROR_SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = data_add_temp(rt, s1 + n1, n2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_MINUS(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 - n2);
}

void OPER_MUL(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 * n2);
}

void OPER_NEG(runtime_t rt) {IN push_num(rt, -pop_num(rt)); }

void OPER_NEQ(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 != n2);
}

void OPER_NOT(runtime_t rt) {IN push_num(rt, !pop_num(rt)); }

void OPER_OR(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 || n2);
}

void OPER_PLUS(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 + n2);
}

void OPER_POW(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    ON_ERROR(n1 <= 0, ERROR_DOMAIN);
    push_num(rt, pow(n1, n2));
}

void OPER_RIGHTS(runtime_t rt)
{IN
    int n2 = pop_num(rt);
    char *s1 = RAM + pop_str(rt);
    ON_ERROR(n2 < 0 || n2 > strlen(s1), ERROR_SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = data_add_temp(rt, s1 + strlen(s1) - n2, n2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_RND(runtime_t rt)
{IN
    pop_num(rt);
    push_num(rt, rand());
}

void OPER_SIN(runtime_t rt) {IN push_num(rt, sin(pop_num(rt))); }

void OPER_SQR(runtime_t rt)
{IN
    num_t n = pop_num(rt);
    ON_ERROR(n < 0, ERROR_DOMAIN);
    push_num(rt, sqrt(n));
}

void OPER_STRS(runtime_t rt)
{IN
    char s[32];
    sprintf(s, "%g", pop_num(rt));
    int addr = data_add_temp(rt, s, strlen(s));
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_TAN(runtime_t rt) {IN push_num(rt, tan(pop_num(rt))); }

void OPER_VAL(runtime_t rt)
{IN
    char *p = RAM + pop_str(rt);
    errno = 0;
    num_t n = strtod(p, NULL);
    ON_ERROR(errno, ERROR_ILLEGAL_CONVERSION);
    push_num(rt, n);
}

/** The table of all operators: each operator has a name, an OPER_*** function
    implementing it, a flag binary true if the operator is infix, as a + b, or
    prefix as RIGHT$(x$,i), and a priority, used to pop it from the operator
    stack at the appropriated moment. */
const struct {
    char name[8];
    void (*routine)(runtime_t);
    short binary;
    short priority;
} OPERATORS[] = {
    // Ordered according to the name field.
    {"&", OPER_AMP, 1, 50}, {"*", OPER_MUL, 1, 60},
    {"+", OPER_PLUS, 1, 50}, {"-", OPER_MINUS, 1, 50},
    {"-", OPER_NEG, 0, 70}, {"/", OPER_DIV, 1, 60},
    {"<", OPER_LT, 1, 30}, {"<=", OPER_LEQ, 1, 30},
    {"<>", OPER_NEQ, 1, 30}, {"=", OPER_EQ, 1, 30},
    {">", OPER_GT, 1, 30}, {">=", OPER_GEQ, 1, 30},
    {"ABS", OPER_ABS, 0, 100}, {"ACS", OPER_ACS, 0, 100},
    {"AND", OPER_AND, 1, 10}, {"ASC", OPER_ASC, 0, 100},
    {"ATN", OPER_ATN, 0, 100}, {"CHR$", OPER_CHRS, 0, 100},
    {"COS", OPER_COS, 0, 100}, {"EXP", OPER_EXP, 0, 100},
    {"INT", OPER_INT, 0, 100}, {"LEFT$", OPER_LEFTS, 0, 100},
    {"LEN", OPER_LEN, 0, 100}, {"LOG", OPER_LOG, 0, 100}, 
    {"MID$", OPER_MIDS, 0, 100}, {"NOT", OPER_NOT, 0, 20},
    {"OR", OPER_OR, 1, 10}, {"RIGHT$", OPER_RIGHTS, 0, 100},
    {"RND", OPER_RND, 0, 100}, {"SIN", OPER_SIN, 0, 100},
    {"SQR", OPER_SQR, 0, 100}, {"STR$", OPER_STRS, 0, 100},
    {"TAN", OPER_TAN, 0, 100}, {"VAL", OPER_VAL, 0, 100},
    {"^", OPER_POW, 1, 80},
};

// ///////////////////////// EXPRESSION EVALUATION ///////////////////////// //

/**
    An expression is evaluated by pushing operands on the stack and operators on
    the runtime estack. Whenever a new operator is parsed, if its priority is
    less than the priority of the operator on top of the estack, the latter is
    executed. Else the operator is pushed on the estack, too.

    When the expression has been completely parsed, all remaining operators on
    the estack are popped and executed.
*/

/** Evaluate an expression at rt->ip and leave on the stack the value. */
void expr(runtime_t rt)
{IN
    int open_close_match = 0;   // match between opened and closed parentheses.
    rt->dt = rt->dp;            // Reset temporary string area.
    for (;;) {
        // Prefix operators.
        while (CODE == CODE_MINUS || CODE > CODE_STARTOPERATOR && CODE < CODE_ENDOPERATOR
        && !OPERATORS[CODE - CODE_STARTOPERATOR - 1].binary) {
            // Convert CODE_MINUS to CODE_NEG when considered as unary.
            int code = (CODE == CODE_MINUS ? CODE_NEG : CODE) - CODE_STARTOPERATOR - 1;
            runtime_epop(rt, OPERATORS[code].priority);
            runtime_epush(rt, OPERATORS[code].routine, OPERATORS[code].priority);
            ++ IP;
        }
        /** Operand. */
//~ printf("OPERAND: CODE = %i\n", CODE);
        switch (CODE) {
        case CODE_IDN: {
            addr_t a0, a1;
            ON_ERROR(var_parse(rt, &a0, &a1) == VAR_NULL, ERROR_UNDEFINED_VARIABLE);
            push_num(rt, peek_num(RAM + a1));
            break;
        } case CODE_IDNS: {
            addr_t a0, a1;
            ON_ERROR(var_parse(rt, &a0, &a1) == VAR_NULL, ERROR_UNDEFINED_VARIABLE);
            // Make a temporary copy of string a1.
            int s = data_add_temp(rt, RAM + a1, strlen(RAM + a1));
            ON_ERROR(s < 0, ERROR_OUT_OF_STRING_MEMORY);
            push_str(rt, s);
            break;
        } case CODE_INTLIT:
            push_num(rt, peek(RAM + IP + 1));
            IP += 1 + sizeof(addr_t);
            break;
        case CODE_NUMLIT:
            push_num(rt, peek_num(RAM + IP + 1));
            IP += 1 + sizeof(num_t);
            break;
        case CODE_STRLIT:
            push_str(rt, peek(RAM + IP + 1));
            IP += 1 + sizeof(addr_t);
            break;
        case '(':
            /*  Push a fake operator with priority 0: all operators below it
                won't be executed until this one is not popped, and that
                happens when ')' is parsed. */
            ++ open_close_match;
            ++ IP;
            runtime_epush(rt, NULL, 0);
            continue;
        default:
            ON_ERROR(1, ERROR_STACK_UNDERFLOW);
        }
        // Check against closed parentheses
        while (CODE == ')') {
            // Pop and execute anything on the stack with priority >= 1.
            runtime_epop(rt, 0);
            ON_ERROR(rt->estack_next == 0, ERROR_CLOSEPAR_WITHOUT_OPENPAR);
            // Drop the (NULL, 0) item on top of the operator stack.
            -- rt->estack_next;
            -- open_close_match;
            ++ IP;
        }
//~ printf("OPERATOR?: CODE = %i, paren = %i\n", CODE, open_close_match);
        // Check against a binary operator
        if (CODE > CODE_STARTOPERATOR && CODE < CODE_ENDOPERATOR
        && OPERATORS[CODE - CODE_STARTOPERATOR - 1].binary) {
            int code = CODE - CODE_STARTOPERATOR - 1;
            runtime_epop(rt, OPERATORS[code].priority);
            runtime_epush(rt, OPERATORS[code].routine, OPERATORS[code].priority);
            ++ IP;
            continue;   // evaluate the second operand.
        }
        // A list of comma separated expressions inside ( and ) is legal.
        if (CODE == ',' && open_close_match > 0) {
            // Pop and execute anything on the stack with priority >= 1.
            runtime_epop(rt, 0);
            ON_ERROR(rt->estack_next == 0, ERROR_COMMA_WITHOUT_OPENPAR);
            // Continue to the next item of the comma separated list.
            ++ IP;
            continue;
        }
        // End of the expression: break
        ON_ERROR(open_close_match != 0, ERROR_OPENPAR_WITHOUT_CLOSEPAR);
        runtime_epop(rt, 0);
        break;
}}

/** Assign to the variable located at a0 and with value at a1 the value popped
    from the stack. It expects the '=' symbol at IP. */
void expr_assign(runtime_t rt, int type, addr_t a0, addr_t a1)
{IN
    EXPECT(CODE_EQ, ERROR_EQ_EXPECTED);
    expr(rt);
    switch (type) {
    case VAR_NULL: ON_ERROR(1, ERROR_UNDEFINED_VARIABLE);
    case VAR_NUM:
        poke_num(RAM + a1, pop_num(rt));
        break;
    case VAR_STR: {
        str_t s = pop_str(rt);
        // a1 points to the first character of the string to overwrite with s.
        unsigned len_a = strlen(RAM + a1);
        unsigned len_s = strlen(RAM + s);
        int delta = len_s - len_a;
        // Check space (if len_s > len_a).
        ON_ERROR(delta >= rt->sp0 - rt->vp, ERROR_OUT_OF_VARIABLE_MEMORY);
        if (delta != 0) {
//~ printf("a1 = %u, vp = %u\n", a1, rt->vp);
//~ printf("lena = %u, lens  = %u\n", len_a, len_s);
//~ printf("memmove(%p, %p, %i);\n",RAM + a1 + len_s, RAM + a1 + len_a, rt->vp - a1 + delta);
            // Reduce the space for the string.
            memmove(RAM + a1 + len_s, RAM + a1 + len_a, rt->vp - (a1 + len_a));
            // Adjust the pointer to the first free byte in the variable area.
            rt->vp += delta;
            // Adjust the size of the variable containing the string.
            poke(RAM + a0, peek(RAM + a0) + delta);
        }
        // Finally copy string s on string a1.
        strcpy(RAM + a1, RAM + s);
        break;
    }
    default:
        assert(!"BUG!");
}}

// ////////////////////// INSTRUCTION IMPLEMENTATIONS ////////////////////// //

void INSTR_BYE(runtime_t rt)
{IN
    if (rt->prog_changed) {
        fputs("Unsaved changes: are you sure to leave? (Y/N)", stdout);
        if (toupper(getchar()) != 'Y') return;
    }
    puts("Bye.");
    exit(EXIT_SUCCESS);
}

void INSTR_CLOSE(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_DATA(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_DEF(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_DIM(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_ERROR(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_FOR(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_GOSUB(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_GOTO(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_IF(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_INPUT(runtime_t rt) {IN assert(!"TODO!"); }

void INSTR_LET(runtime_t rt)
{IN
    extern void execute_assignment(runtime_t rt);
    addr_t a0, a1;
    int type = var_parse(rt, &a0, &a1);
    ON_ERROR(type != VAR_NULL, ERROR_VARIABLE_ALREADY_DEFINED);
    // Insert the variable in the variables list.
    addr_t p0 = rt->vp;
    addr_t p = p0 + sizeof(addr_t);
    poke(RAM + p, peek(RAM + IP + 1));  // insert the variable's name.
    p += sizeof(addr_t);
    int size = 2*sizeof(addr_t) + 1;    // at least size,name,type
    // Write the byte with the type code.
    if (CODE == CODE_IDN) {
        RAM[p++] = type = VAR_NUM;
        size += sizeof(num_t);
    } else {
        RAM[p++] = type = VAR_STR;
        // Initialize the variable to "".
        RAM[p] = 0;
        size += 1;
    }
    poke(RAM + p0, size);
    rt->vp += size;
    // Now the variable has been defined: assign the value to it.
    IP += 1 + sizeof(str_t);    // skip the variable name in the object code.
//~ printf("IP = %04x ", IP); dump_line(rt, rt->ip0);
    expr_assign(rt, type, p0, p);
}

void INSTR_LINPUT(runtime_t rt) {IN assert(!"TODO!"); }

void INSTR_LIST(runtime_t rt) {IN prog_print(rt, stdout); }

void INSTR_LOAD(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_MERGE(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_NEW(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_NEXT(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_ONERROR(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_OPEN(runtime_t rt) {IN assert(!"TODO!"); }

void INSTR_PRINT(runtime_t rt)
{IN
    FILE *f = stdout;
    if (CODE == '#') {
        assert(!"TODO!");
    }
    int newline = 1;
    while (CODE != 0 && CODE != ':') {
        if (CODE == ',' || CODE == ';') {
            if (CODE == ',') fputc('\t', f);
            ++ IP;
            newline = 0;
            continue;
        }
        expr(rt);
        num_t n;
        str_t s;
        pop(rt, &n, &s);
        if (s == STR_NULL) fprintf(f, "%g", n); else fputs(RAM + s, f);
        newline = 1;
    }
    if (newline) fputc('\n', f);
}

void INSTR_READ(runtime_t rt) {IN assert(!"TODO!"); }

void INSTR_REPEAT(runtime_t rt)
{IN
    IP = rt->ip0 + 1;
    if (CODE == CODE_INTLIT) IP += 3;
}

void INSTR_RESTORE(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_RETURN(runtime_t rt) {IN assert(!"TODO!"); }

void INSTR_RUN(runtime_t rt)
{IN
    extern int execute(runtime_t);
    // Reset variables and stacks.
    rt->vp = rt->vp0;
    rt->sp = rt->sp0;
    rt->rsp = rt->rsp0;
    rt->error = 0;
    // The increment rt->ipn is set inside the loop.
    for (rt->ip0 = rt->pp0; rt->ip0 < rt->pp; rt->ip0 = rt->ipn) {
        rt->ipn = rt->ip0 + RAM[rt->ip0];   // next line
        IP = rt->ip0 + 4;       // skip line length + CODE_INTLIT + integer.
        if (execute(rt)) break;
    }
    if (rt->error) printf("At line %i: ", peek(RAM + rt->ip0 + 2));
}

void INSTR_SAVE(runtime_t rt) {IN expr(rt); prog_save(rt, RAM + pop_str(rt)); }
void INSTR_SKIP(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_STOP(runtime_t rt) {IN assert(!"TODO!"); }
void INSTR_SYS(runtime_t rt) {IN assert(!"TODO!"); }

/// Table of all routines implementing instructions.
void (*INSTRUCTION_CODE[])(runtime_t) = {
    /*  Same ordering as the constant between CODE_STARTKEYWORD and
        CODE_ENDKEYWORD */
    INSTR_BYE, INSTR_CLOSE, INSTR_DATA, INSTR_DEF, INSTR_DIM, INSTR_ERROR,
    INSTR_FOR, INSTR_GOSUB, INSTR_GOTO, INSTR_IF, INSTR_INPUT, INSTR_LET,
    INSTR_LINPUT, INSTR_LIST, INSTR_LOAD, INSTR_MERGE, INSTR_NEW, INSTR_NEXT,
    INSTR_ONERROR, INSTR_OPEN, INSTR_PRINT, INSTR_READ, INSTR_REPEAT,
    INSTR_RESTORE, INSTR_RETURN, INSTR_RUN, INSTR_SAVE, INSTR_SKIP, INSTR_STOP,
    INSTR_SYS,
};

// /////////////////////////////// EXECUTION /////////////////////////////// //

void execute_assignment(runtime_t rt)
{IN
    addr_t a0, a1;
    /** Parse a variable and leave the address of its first byte in a0, thus
        a0[0] and a0[1] is its size, while in a1 leave the address of the
        variable's value. In type return the type of variable, VAR_NULL if the
        variable is not defined. */
    int type = var_parse(rt, &a0, &a1);
    EXPECT(CODE_EQ, ERROR_EQ_EXPECTED);
    expr(rt);
    switch (type) {
    case VAR_NULL: ON_ERROR(1, ERROR_UNDEFINED_VARIABLE);
    case VAR_CHR: RAM[a1] = RAM[pop_str(rt)]; break;
    case VAR_NUM: poke_num(RAM + a1, pop_num(rt)); break;
    case VAR_STR: {
        str_t s = pop_str(rt);
        // a1 points to the first character of the string to overwrite with s.
        unsigned len_a = strlen(RAM + a1);
        unsigned len_s = strlen(RAM + s);
        int delta = len_a - len_s;
        // Check space (if len_s > len_a).
        ON_ERROR(len_s - len_a >= rt->sp0 - rt->vp, ERROR_OUT_OF_VARIABLE_MEMORY);
        if (len_a != len_s) {
            // Augment/Reduce the space for the string.
            memmove(RAM + a1 + len_s, RAM + a1 + len_a, rt->vp - a1 - len_a);
            // Adjust the pointer to the first free byte in the variable area.
            rt->vp += len_s - len_a;
            // Adjust the size of the variable containing the string.
            poke(RAM + a0, peek(RAM + a0) + len_s - len_a);
        }
        // Finally copy string s on string a1.
        strcpy(RAM + a1, RAM + s);
}}}

/** Execute the line at IP0, starting from instruction IP. Return the value of
    rt->error. */
int execute(runtime_t rt)
{IN
    rt->error = 0;
    if (setjmp(rt->err_buffer) == 0) {
        volatile byte_t opcode;
//~ int i = 0;
//~ rt->ipn = rt->ip0 + RAM[rt->ip0];
        while ((opcode = CODE) != 0) {
//~ ++ i; if (i > 10) break;
//~ printf("error = %i, IP0 = %u, IP = %u, IPN = %u\n", rt->error, rt->ip0, rt->ip, rt->ipn);
            if (opcode > CODE_STARTKEYWORD && opcode < CODE_ENDKEYWORD) {
                ++ IP; (*INSTRUCTION_CODE[opcode - CODE_STARTKEYWORD - 1])(rt);
            } else if (opcode == CODE_IDN || opcode == CODE_IDNS) {
                addr_t a0, a1;
                int type = var_parse(rt, &a0, &a1);
                ON_ERROR(type == VAR_NULL, ERROR_UNDEFINED_VARIABLE);
                expr_assign(rt, type, a0, a1);
                //~ execute_assignment(rt);
            } else {
                ON_ERROR(1, ERROR_ILLEGAL_INSTRUCTION);
    }}}
    return rt->error;
}

// //////////////////////////// LEXICAL ANALIZER //////////////////////////// //

/// Print a token at address a on file f: return the address of next token.
addr_t token_dump(runtime_t rt, addr_t a, FILE *f)
{IN
    byte_t b = RAM[a++];
    if (b != 0) {
        if (b == CODE_IDN || b == CODE_IDNS) {
            fprintf(f, "%s ", RAM + peek(RAM + a));
            a += 2;
        } else if (b == CODE_INTLIT) {
            fprintf(f, "%i", peek(RAM + a));
            a += 2;
        } else if (b == CODE_NUMLIT) {
            fprintf(f, "%g", peek_num(RAM + a));
            a += sizeof(num_t);
        } else if (b == CODE_STRLIT) {
            fprintf(f, "\"%s\"", RAM + peek(RAM + a));
            a += 2;
        } else if (b == CODE_IDN || b == CODE_IDNS) {
            fprintf(f, "%s ", RAM + peek(RAM + a));
            a += 2;
        } else if (b > CODE_STARTKEYWORD && b < CODE_ENDKEYWORD) {
            fprintf(f, "%s ", KEYWORDS[b - CODE_STARTKEYWORD - 1]);
        } else if (b > CODE_STARTOPERATOR && b < CODE_ENDOPERATOR) {
            fprintf(f, "%s ", OPERATORS[b - CODE_STARTKEYWORD - 1].name);
        } else {
            if (b != ')') fputc(' ', f);
            fputc(b, f);
            if (b != '(') fputc(' ', f);
        }
    }
    return a;
}

/** Try to match a keyword with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding keyword code is returned, else 0. */
int token_keyword(char *p0, unsigned len)
{IN
    for (int i = 0; i < CODE_ENDKEYWORD - CODE_STARTKEYWORD - 1; ++ i) {
        int cmp = memcmp(KEYWORDS[i], p0, len);
        if (cmp == 0 && len == strlen(KEYWORDS[i]))
            return (CODE_STARTKEYWORD + 1) + i;
        if (cmp > 0) return 0;
    }
    return 0;
}

/** Try to match an operator with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding operator code is returned, else 0. */
int token_operator(char *p0, unsigned len)
{IN
    for (int i = 0; i < CODE_ENDOPERATOR - CODE_STARTOPERATOR - 1; ++ i) {
        int cmp = memcmp(OPERATORS[i].name, p0, len);
//~ printf("cmp |%s|, |%s|, len = %i\n", OPERATORS[i].name, p0, len);
        if (cmp == 0 && OPERATORS[i].name[len] == '\0') {
//~ puts("OK!");
            return (CODE_STARTOPERATOR + 1) + i;
        }
        if (cmp > 0) return 0;
    }
    return 0;
}

///  Tokenize the contents of tib and store it at obj: return 0 on error.
int tokenize(runtime_t rt)
{IN
    int k, len;
    // Encode the tib string at obj.
    byte_t *p = RAM + rt->tib;
    byte_t *q0 = RAM + rt->obj; // The line size will be stored here.
    byte_t *q = q0 + 1;             // The line will be stored here.
    byte_t b;
    while (*p != '\n' && *p != '\0') {
        p += strspn(p, " \t");  // Skip blanks.
        if (*p > 127) {
            printf("Invalid ASCII code %i\n", *p);
            return 0;
        }
        if (*p < 32) ++ p;   // skip non printable
        //~ else if (isdigit(*p) || *p == '-' && isdigit(p[1])) {
        else if (isdigit(*p)) {
            char *p1, *p2;
            int i = strtol(p, &p1, 10);
            num_t n = strtod(p, &p2);
            if (p1 == p2 && i >= -32768 && i < 32768) {
                *q = CODE_INTLIT;
                poke(q + 1, i);
//~ printf("intero %i\n", peek(q+1));
                q += 3;
                p = p1;
            } else {
                *q = CODE_NUMLIT;
                poke_num(q + 1, n);
//~ printf("float %g\n", peek_num(q+1));
                q += 1 + sizeof(num_t);
                p = p2;
            }
        } else if (isalpha(*p)) {
            // Scan the string and create a new constant.
            byte_t *p0 = p;
            byte_t *q1 = q;
            do {
                *q1++ = toupper(*p);
                ++ p;
            } while (isalnum(*p));
            //~ for (*q1++ = toupper(*p); ++ p, isalnum(*p); *q1++ = toupper(*p))
                //~ ;
            if (*p == '$') { *q1 = '$'; ++ p; }
            len = p - p0;
            if ((k = token_keyword(q, len)) > 0) *q++ = k;
            else if ((k = token_operator(q, len)) > 0) *q++ = k;
            else {
                if ((k = data_find(rt, q, len)) < 0
                && (k = data_add(rt, q, len)) < 0) {
                    puts("Too many identifiers");
                    return 0;
                }
                *q = (*q1 == '$') ? CODE_IDNS : CODE_IDN;
                poke(q + 1, k);
//~ printf("idn %s\n", ram + peek(q+1));
                q += 3;
            }
        } else if (*p == '"') {
            len = strcspn(++p, "\"");
            if (p[len] != '"') {
                puts("End of line inside string");
                return 0;
            }
            if ((k = data_find(rt, p, len)) < 0
            && (k = data_add(rt, p, len)) < 0) {
                puts("Too many string constants");
                return 0;
            }
            *q = CODE_STRLIT;
            poke(q + 1, k);
            q += 3;
            p += len + 1;
        } else if (*p == '<' && p[1] == '=') { *q++ = CODE_LEQ; p += 2; }
        else if (*p == '<' && p[1] == '>') { *q++ = CODE_NEQ; p += 2; }
        else if (*p == '>' && p[1] == '=') { *q++ = CODE_GEQ; p += 2; }
        else if ((k = token_operator(p, 1)) > 0) { *q++ = k; ++ p; }
        else *q++ = *p++;
    }
    *q++ = 0;       // End of line marker
    *q0 = q - q0;   // Line size
    return 1;
}

// ////////////////////////////////// MAIN ////////////////////////////////// //

int main(int npar, char **pars)
{IN
    struct runtime_s rt;
    runtime_reset(&rt);
    for (;;) {  // REPL
        putchar('>');
        fgets(rt.ram + rt.tib, BUF_SIZE, stdin);
        tokenize(&rt);
        // RAM[obj] = line size.
        if (rt.ram[rt.obj + 1] == CODE_INTLIT) prog_edit(&rt, rt.obj);
        else {
            rt.ip0 = rt.obj;
            rt.ip = rt.obj + 1;
            puts(ERROR[execute(&rt)]);
        }
dump_data(&rt);
dump_variables(&rt);

//~ printf("ERROR = %i\n", error);
//~ for (int i = 0; i < rt.ram[rt.obj]; ++ i)
    //~ // i = token_dump(obj + i, stdout);
    //~ printf(" %i", rt.ram[rt.obj + i]);
//~ putchar('\n');
//~ dump_memory();
//~ dump_program(&rt);
    }
}

// ///////////////////////////// DOCUMENTATION ///////////////////////////// //

/**

This interpreter simulates an old fashioned BASIC system, a "street Basic"
as Kemeny and Kurtz called them, which runs on top of a 16-bit computer.

The techniques used are primitive and imitate the ones employed by 70s
interpreters, obsessed with space saving at the expense of time and
maintenability. (It may well be that that some unconscious adolescent
reminiscence of my past with the ZX Spectrum influenced the development
of this code.)

All data (program, symbol table and data) are stored inside a 64K virtual
ram, which is subdivided as follows:

    - Data area, from dp0 to pp0-1.
    - Program area, from pp0 to vp0-1.
    - Variable area, from vp0 to vp-1.
    - Free space from vp to sp-1.
    - Stack from sp to sp0-1.
    - Return stack from sp0 to rsp0-1.
    - Object buffer, from rsp0 to tib-1.
    - Terminal input buffer, from tib to buf1-1.
    - Buffer for file #1, from buf1 to buf2-1.
    - Buffer for file #2, from buf2 to buf3-1.
    - Buffer for file #3, from buf3 to buf4-1.
    - Buffer for file #4, from buf4 to 65536.

Inside some areas there are pointers pointing to the first free item in the
area:
    - dp points to the first free byte in the Data area.
    - dt points to the first free byte after temporary strings in Data area
    - pp points to the first free byte in the Program area.
    - vp points to the first free byte in the Variable area.
    - sp points to the first free byte in the Stack.
    - tsp points to the first free byte in the Type Stack.
    - rsp points to the first free byte in the Return stack.

Buffers have no pointers, data inside them are stored either with a size
prefix, such as obj, or by an ending 0, as in text file buffers.

Obj is the buffer that contains the immediate statements under execution,
thus statements typed during the REPL of the interpreter.
Tib, buf#1, buf#2, buf#3, buf#4 are buffers used to exchange data with the
terminal and the files possibly associated to the four available channels.
Data is a memory area where string constants are stored, such as variable's
names and costant strings.
The program is stored as a sequence of lines of the form:

    | s | n1 | n2 | c1 | ... | cn |

where s is the lengh of the all sequence, bytes n1 and n2 do form the line
number n1 + 256 * n2, and bytes c1, ..., cn are ASCII codes or special codes
denoting keywords etc.

The text of a line is not a string but it is encoded as follows:
    - keywords are encoded in single bytes with code >= 128.
    - Numbers are encoded as CODE_NUMLIT plus the 4 bytes of the number.
    - Strings are encoded as CODE_STRLIT plus the string itself.
    - Identifiers are encoded as CODE_IDN(S) plus the string representing them.
    - Characters, such as ",", "+", etc. are represented by themselves.
Each of the previous item is called a token: an instruction is a sequence
of tokens ending with ":" or CODE_EOL.

A pointer prog_next points to the first free byte of the program area, that
is 8192 bytes long.

Symbols are just variables that are stored as follows:

    | n1 | n2 | c1 | ... | cn | t | b1 | ... | bn |

where n1 and n2 do form the size of the rest of the symbol: thus, if p
is the address of n1, to jump to the next symbol just put p += n1 + 256*n2.
Bytes c1...cn form symbol name, t is a type code byte, denoting the
type of variable (num, str, for, numvec, strvec, nummat, strmat) and b1
... bn the bytes with the variable data and metadata, as follows:

- If t = 1 (num) then b1b2b3b4 contains the variable's value
- If t = 2 (str) then b1...bn is the content of the string that is
    0-terminated as in C
- If t = 3 (for) then b1...bn are as follows:
    - a number v which is the value of the variable
    - a number b which is the bound of the loop
    - a number s which is the step of the loop
    - 

    


The Basic language understood by StreetBasic is described as follows:

    v1 = e1, ... , vn = en
        Assign the result of evaluation of e1 to v1 and so on until vn:
        the variables should already exist, else an error is raised.
    CLOSE #e
        Closes the file associated to channel e.
    DATA e1, ..., en
        A series of constants, given by the evaluation of the expressions
        e1, ..., en is defined, to be READ. A data_next pointer points to
        the next expression to evaluate.
    DEF FN v(v1, ..., vn) = e
        Defines a function FNl to be evaluated when invoked inside an
        expression: when inside an expression FN v(e1, ..., en) is called,
        e1, ..., en are pushed on the stack and the line with the DEF FN
        is looked for; when found, variables v1, ..., vn are parsed in
        reverse order and assigned to the values on the stack, next e
        is evaluated and the result returned on the stack.
    DIM v1(i1,j1), ..., vn(in,jn)
        Create new array variables v1, ..., vn. If some vi already exists,
        an error is raised.
    ERROR e
        Raise an error with code e.
    FOR v = e1 TO e2 STEP e3
        Perform v = e1 (if v does not exist create it), evaluate e1 and e2
        and store them somewhere along with the the program pointer.
    GOSUB e
        Push the instruction pointer on the return stack and perform GOTO e
    GOTO e
        Evaluate e to a number, take the integer part, consider it as a
        line number and jump to it. If the line number does not exist, an
        error is raised.
    IF e THEN
        Evaluates e: if it is zero, skip to the next line. THEN is optional.
    INPUT #e, v1, ..., vn
        Read the contents of variables v1, ..., vn from the file associated
        to channel e. Values are delimited by commas, spaces or end of
        lines, unless they are strings enclosed between " and ".
        For example |1 2 3| can be read by INPUT#e, v1, v2, v3 as |1,2,3|
        can. However, |a, b, c| is read by INPUT#e, v1$, v2$, v3$ as
        |a b c| is. However |"a b" c| is a sequence of two strings.
    INPUT v1, ..., vn
        Same as INPUT#e, v1, ..., vn but reads from the terminal.
    LET v1 = e1, ... , vn = en
        Create new scalar variables v1, ..., vn and initialize them with
        values resulting from the evaluation of e1, ..., en. If some vi
        already exists, an error is raised.
    LINPUT #e, v
        Read a line from file associated to channel e and store it into
        string variable v: the end of line is dropped.
    LINPUT v
        Same as LINPUT#e, v but reads from the terminal.
    NEXT v
        Perform v = v + e2: if e3 < 0 then check against e2 <= v, else
        check against v <= e2; if the check is true then jump to the saved
        program pointer, pointing right after the corresponding FOR.
    ONERROR e1, e2
        Whenever an error with error code e1 occurs, a GOTO e2 is performed.
    OPEN #e1, e2, e3
        Try to open a text file with name e2, to assign it to channel e1
        and in mode e3: e3 = 0 means "read", e1 = 1 means "write", e2 = 2
        means "append".
    POP v1, ..., vm
        Delete variables v1, ..., vn from the symbol table: if at least
        one of them is not defined, an error is raised.
    PUSH v1 = e1, ... , vn = en
        Same as DEF but no error is raised if a symbol already exists,
        rather it is redefined shadowing the previous one.
    PRINT #e, e1 sep ... en sep
        Print the values of expressions e1, ..., en to the file associated
        to channel e. Expressions are printed on a single line which is
        dumped on the file, unless a final separator sep is present.
        Separators can be: "," which inserts a tabulation, ";" which put
        data one after another.
    PRINT e1 sep ... en sep
        Same as PRINT#e, e1 sep ... en sep but prints on the terminal.
    READ v1, ..., vn
        Evaluate, if any, the next expression from a DATA statement and
        assign it to variable v1; continues until vn.
    REPEAT
        repeat the current program line from the beginning.
    RESTORE
        Reset the data_next pointer to beginning of the first DATA
        instruction.
    RETURN
        Pop the instruction pointer from the return stack
    SKIP
        skip the next program line
    STOP
        Terminates the program with a STOP message
    SYS e
        Evaluates e to a string which is executed in the shell of the
        hosting OS, if any.

The interpreter

A REPL is executed, that get a line into tib and encode it at LOW, next
the encoded text is copied in tib and interpreted.

The interpreter uses a pointer IP set to the first byte of the instruction

*/

