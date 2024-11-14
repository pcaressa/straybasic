/// \file straybasic.c
/// \author Paolo Caressa <github.com/pcaressa>
/// \date 20241109

//  Old fashioned debug stuff...
//~ int debug_tab = 0;
//~ #define IN {++debug_tab;for(int i=0;i<debug_tab;++i)fputc('+',stderr);fprintf(stderr,"> %s:%s:%i\n", __func__, __FILE__, __LINE__);}
//~ #define OUT {for(int i=0;i<debug_tab;++i)fputc('+',stderr);fprintf(stderr,"< %s:%s:%i\n", __func__, __FILE__, __LINE__);--debug_tab;}
#define IN
#define OUT

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// /////////////////////////////// CONSTANTS /////////////////////////////// //

#define BUF_SIZE (256)
#define DATA_SIZE (4096)
#define ESTACK_SIZE (20)
#define PROG_SIZE (8192)
#define RAM_SIZE (65536)
#define RSTACK_SIZE (60)
#define STACK_SIZE (120)

enum {
    CODE_INTLIT = 128, CODE_NUMLIT, CODE_STRLIT, CODE_IDN, CODE_IDNS,
    CODE_STARTKEYWORD,
        CODE_BYE, CODE_CLOSE, CODE_DATA, CODE_DEF, CODE_DIM, CODE_ERROR,
        CODE_FOR, CODE_GOSUB, CODE_GOTO, CODE_IF, CODE_INPUT, CODE_LET,
        CODE_LINPUT, CODE_LIST, CODE_LOAD, CODE_MERGE, CODE_NEW, CODE_NEXT,
        CODE_ONERROR, CODE_OPEN, CODE_PRINT, CODE_READ, CODE_REPEAT,
        CODE_RESTORE, CODE_RETURN, CODE_RUN, CODE_SAVE, CODE_SKIP, CODE_STEP,
        CODE_STOP, CODE_SYS, CODE_THEN, CODE_TO,
    CODE_ENDKEYWORD,
    CODE_STARTOPERATOR,
        CODE_MUL, CODE_PLUS, CODE_MINUS, CODE_NEG, CODE_DIV, CODE_LT,
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
    "SKIP", "STEP", "STOP", "SYS", "THEN", "TO"
};

enum {
    ERROR_NONE, ERROR_CANNOT_OPEN_FILE,
    ERROR_CHANNEL_BUSY, ERROR_CHANNEL_CLOSED,
    ERROR_CLOSEPAR_WITHOUT_OPENPAR, ERROR_COMMA_EXPECTED,
    ERROR_COMMA_WITHOUT_OPENPAR, ERROR_DOMAIN,
    ERROR_EQ_EXPECTED, ERROR_HASH_EXPECTED,
    ERROR_IDENTIFIER_EXPECTED, ERROR_ILLEGAL_CHANNEL,
    ERROR_ILLEGAL_CONVERSION, ERROR_ILLEGAL_INPUT,
    ERROR_ILLEGAL_INSTRUCTION,
    ERROR_ILLEGAL_INSTRUCTION_OUTSIDE_PROGRAM,
    ERROR_ILLEGAL_LINE_NUMBER, ERROR_ILLEGAL_MODE,
    ERROR_OPENPAR_WITHOUT_CLOSEPAR, ERROR_OUT_OF_DATA,
    ERROR_OUT_OF_STRING_MEMORY, ERROR_OUT_OF_VARIABLE_MEMORY,
    ERROR_PROGRAM_STOPPED, ERROR_PROGRAM_TOO_LONG,
    ERROR_RETURN_WITHOUT_GOSUB, ERROR_STACK_OVERFLOW,
    ERROR_STACK_UNDERFLOW, ERROR_STRING_VARIABLE_EXPECTED,
    ERROR_SUBSCRIPT_ERROR,
    ERROR_SUBSCRIPT_EXPECTED, ERROR_TOO_MANY_GOSUB,
    ERROR_TYPE_MISMATCH, ERROR_UNDEFINED_VARIABLE,
    ERROR_VARIABLE_ALREADY_DEFINED
};

const char *ERROR[] = {
    "READY", "CANNOT OPEN FILE",
    "CHANNEL BUSY", "CHANNEL CLOSED",
    "\")\" WITHOUT \"(\"", "\",\" EXPECTED",
    "\",\" WITHOUT \"(\"", "DOMAIN ERROR",
    "\"=\" EXPECTED", "\"#\" EXPECTED",
    "IDENTIFIER EXPECTED", "ILLEGAL CHANNEL",
    "ILLEGAL CONVERSION", "ILLEGAL INPUT",
    "ILLEGAL INSTRUCTION",
    "ILLEGAL INSTRUCTION OUTSIDE PROGRAM",
    "ILLEGAL LINE NUMBER", "ILLEGAL MODE",
    "\"(\" WITHOUT \")\"", "OUT OF DATA",
    "NO MORE ROOM FOR STRINGS", "NO MORE ROOM FOR VARIABLES",
    "PROGRAM STOPPED", "PROGRAM TOO LONG",
    "RETURN WITHOUT GOSUB", "EXPRESSION TOO LONG",
    "MISSING VALUE", "STRING VARIABLE EXPECTED",
    "SUBSCRIPT OUT OF RANGE",
    "MISSING SUBSCRIPT", "TOO MANY NESTED GOSUBS",
    "TYPE MISMATCH", "UNDEFINED VARIABLE",
    "VARIABLE ALREADY DEFINED"
};

enum {
    VAR_NONE = 0, VAR_CHR = 1, VAR_NUM = 2, VAR_STR = 4, VAR_FOR = 8,
    VAR_VEC = 16, VAR_MAT = 32
};

// ///////////////////////////////// TYPES ///////////////////////////////// //

typedef uint8_t byte_t;     // 1 byte
typedef uint16_t addr_t;    // pointer to any byte (2 bytes)
typedef uint16_t str_t;     // pointer to string (2 bytes)
typedef float num_t;        // number (4 bytes)

#define NIL (0xFFFF)        // Used as NULL address.

// ///////////////////////////// RUNTIME CLASS ///////////////////////////// //

/** Class whose objects contain all data needed to store an instance of a
    virtual machine where Straybasic programs can be edited and run. */
typedef struct runtime_s
{
    byte_t ram[RAM_SIZE];

    /*  RAM[dp0:dp] contains string constants;
        RAM[dp:dt] contains temporary string constants;
        RAM[pp0:pp] contains the program;
        RAM[vp0:vp] contains the variables list;
        RAM[sp0:sp] contains the parameters stack;
        RAM[rsp0:rsp] contains the return stack;    */
    addr_t dp0, dp, dt, pp0, pp, vp0, vp, sp0, sp, rsp0, rsp;
    
    //  Object code and file buffers: buf0 is reserved to the terminal.
    addr_t obj, buf0, buf1, buf2, buf3, buf4;

    addr_t ip0;     ///< first byte of the current line (its "size byte).
    addr_t ip;      ///< first byte of next token in the current line.
    addr_t ipn;     ///< first byte of the next line (needed by GOTO etc.).

    jmp_buf err_buffer;

    int error;          ///< current error code: 0 means "no error".
    int error_routine;  ///< if != 0, on error a GOTO error_routine is executed.
    int prog_changed;   ///< 1 if the program was not saved since last changes.
    addr_t data;        ///< address of next line to scan for DATA constants.

    /** Operator stack: contains operator with priorities lower than the one
        under execution, inside an expression. */
    struct {
        void (*routine)(struct runtime_s*);
        int priority;
    } estack[ESTACK_SIZE];
    int estack_next;

    /** I/O files: channels[0] is always stdin or stdout, while channels[i]
        from i = 1 to 4 are NULL or handles to text files. */
    FILE *channels[5];

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
OUT}

/** Push the routine address r and relative priority p on the estack: while
    on top of it there are operators with higher priorities, execute them. */
void runtime_epush(runtime_t rt, void (*r)(runtime_t), int p)
{IN
    ON_ERROR(rt->estack_next >= ESTACK_SIZE, ERROR_STACK_OVERFLOW);
    rt->estack[rt->estack_next].routine = r;
    rt->estack[rt->estack_next].priority = p;
    ++ rt->estack_next;
OUT}

/** Initialize a virtual ram. */
void runtime_init(runtime_t rt)
{
    // Memory area limits, always set
    rt->dp0 = 0;
    rt->pp0 = rt->dp0 + DATA_SIZE;
    rt->vp0 = rt->pp0 + PROG_SIZE;
    rt->buf4 = RAM_SIZE - BUFSIZ;
    rt->buf3 = rt->buf4 - BUFSIZ;
    rt->buf2 = rt->buf3 - BUFSIZ;
    rt->buf1 = rt->buf2 - BUFSIZ;
    rt->buf0 = rt->buf1 - BUFSIZ;
    rt->obj = rt->buf0 - BUFSIZ;
    rt->rsp0 = rt->obj - RSTACK_SIZE;
    rt->sp0 = rt->rsp0 - STACK_SIZE;

    // Drop stacks.
    rt->rsp = rt->rsp0;
    rt->sp = rt->sp0;
    rt->estack_next = 0;
    rt->error = 0;
    rt->error_routine = 0;

    // Reset data pointer.
    rt->data = rt->pp0;

    // Drop variables.
    rt->vp = rt->vp0;
    
    // Drop data and program.
    rt->dt = rt->dp = rt->dp0;
    rt->pp = rt->pp0;
    rt->prog_changed = 0;
    for (int i = 0; i < 5; ++ i)
        rt->channels[i] = NULL;
}

/** Reset the virtual machine: if restart == 1 restart the machine, else
    preserves current program and data. Return the runtime. */
runtime_t runtime_reset(runtime_t rt, int restart)
{IN
    // Drop stacks.
    rt->rsp = rt->rsp0;
    rt->sp = rt->sp0;
    rt->estack_next = 0;
    rt->error = 0;
    rt->error_routine = 0;

    // Reset data pointer.
    rt->data = rt->pp0;

    // Drop variables.
    rt->vp = rt->vp0;
    
    if (restart) {
        // Also drop data and program.
        rt->dt = rt->dp = rt->dp0;
        rt->pp = rt->pp0;
        rt->prog_changed = 0;
        for (int i = 1; i < 5; ++ i)
            if (rt->channels[i] != NULL) {
                fclose(rt->channels[i]);
                rt->channels[i] = NULL;
            }
    }
OUT return rt;
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
    thus each element takes 6 bytes. If s == NIL then the item contains
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
OUT}

num_t pop_num(runtime_t rt)
{IN
    num_t n;
    str_t s;
    pop(rt, &n, &s);
    ON_ERROR(s != NIL, ERROR_TYPE_MISMATCH);
OUT return n;
}

str_t pop_str(runtime_t rt)
{IN
    num_t n;
    str_t s;
    pop(rt, &n, &s);
    ON_ERROR(s == NIL, ERROR_TYPE_MISMATCH);
OUT return s;
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
OUT}

#define push_num(rt, n) push(rt, n, NIL)
#define push_str(rt, s) { assert(s != NIL); push(rt, 0, s); }

/// Address of the number on top of the stack.
addr_t tos_num(runtime_t rt)
{
    ON_ERROR(rt->sp == rt->sp0, ERROR_STACK_UNDERFLOW);
    return rt->sp - sizeof(num_t);
}

/// Address of the string on top of the stack.
addr_t tos_str(runtime_t rt)
{
    ON_ERROR(rt->sp == rt->sp0, ERROR_STACK_UNDERFLOW);
    return rt->sp - sizeof(num_t) - sizeof(str_t);
}

/** Pop from the return stack the pair (ip0, offset) denoting a position inside
    a program line. */
void rpop(runtime_t rt, addr_t *ip0, byte_t *offset)
{IN
    ON_ERROR(rt->rsp == rt->rsp0, ERROR_RETURN_WITHOUT_GOSUB);
    rt->rsp -= 3;
    *ip0 = peek(RAM + rt->rsp);
    *offset = RAM[rt->rsp + 2];
OUT}

/** Push on the return stack the pair (ip0, offset) denoting a position inside a
    program line. Offset is stored as a single byte_t*/
void rpush(runtime_t rt, addr_t ip0, byte_t offset)
{IN
    // An item in the rstack takes 3 (sizeof addr + sizeof byte) bytes.
    ON_ERROR(rt->rsp - rt->rsp0 >= RSTACK_SIZE - 3, ERROR_TOO_MANY_GOSUB);
    poke(RAM + rt->rsp, ip0);
    RAM[rt->rsp + 2] = offset;
    rt->rsp += 3;
OUT}

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
    int k = -1;
    if (rt->dp + len + 1 < rt->pp0) {
        k = rt->dp;
        memcpy(RAM + k, p0, len);
        RAM[k + len] = '\0';
        rt->dp += len + 1;
    }
OUT return k;
}

/** Add a temporary string to the data area and return its address: if there's
    no more space then return a negative number. */
int data_add_temp(runtime_t rt, char *p, int len)
{IN
    int k = -1;
    if (rt->dt + len + 1 < rt->pp0) {
        k = rt->dt;
        memcpy(RAM + k, p, len);
        RAM[k + len] = '\0';
        rt->dt += len + 1;
    }
OUT return k;
}

/** Look for a string in the data area: if found then return its address,
    else a negative number. */
int data_find(runtime_t rt, char *p0, int len)
{IN
    str_t s = rt->dp0;
    while (s < rt->dp) {
        int len1 = strlen(RAM + s);
        if (len1 == len && memcmp(RAM + s, p0, len) == 0) {
OUT         return s;
        }
        s += len1 + 1;
    }
OUT return -1;
}

// //////////////////////////// PROGRAM EDITING //////////////////////////// //

/** A program line is stored as a sequence s,i1,i2,c1,...,cn of bytes, where
    s = 3 + n, i1+256*i2 is the line number, c1,...,cn are the bytecodes of
    the line. */

/** If there are unsaved changes in the current program, ask the user if they
    could be discarded: return 1 if there are no changes or the user agree to
    discard them, else return 0. */
int prog_check(runtime_t rt)
{IN
    int ok = 1;
    if (rt->prog_changed) {
        fputs("\nUNSAVED CHANGES IN CURRENT PROGRAM: DISCARD THEM (Y/N)? ",
            stdout);
        char c[2];
        fgets(c, sizeof(c), stdin);
        if (toupper(*c) != 'Y') ok = 0;
    }
OUT return ok;
}

/** Delete the line with number n: return 1 if the line doesn't exist, else 0. */
int prog_delete(runtime_t rt, int n)
{IN
    for (addr_t a = rt->pp0; a < rt->pp; a += RAM[a]) {
        addr_t n1 = peek(RAM + a + 2);
        if (n1 > n) break;
        if (n1 == n) {
            // Overwrite the line by the following ones.
            int size = RAM[a];
            memmove(RAM + a, RAM + a + size, rt->pp - (a + size));
            rt->pp -= size;
            rt->prog_changed = 1;
OUT         return 0;
    }}
OUT return 1;
}

/// Edit a line at ram[line] starting with a line number.
void prog_edit(runtime_t rt, addr_t line)
{IN
    extern void prog_insert(runtime_t rt, int n, addr_t a);
    // line = size; line+1 = CODE_INTLIT; line+2/+3 = integer; line+4 = line contents.
    int line_no = peek(RAM + line + 2);
    if (RAM[line + 4] == 0) {
        if (prog_delete(rt, line_no))
            printf("Line %i does not exist!\n", line_no);
    } else {
        // Overwrite (= delete + insert) the line.
        prog_delete(rt, line_no);
        prog_insert(rt, line_no, line);
    }
OUT}

/** Looks for a line with line number n: if found then its address is returned,
    else NIL. */
addr_t prog_find(runtime_t rt, int n)
{IN
    for (addr_t a = rt->pp0; a < rt->pp; a += RAM[a]) {
        addr_t n1 = peek(RAM + a + 2);
        if (n1 > n) break;
        if (n1 == n) {OUT return a;}
    }
OUT return NIL;
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
    if (rt->pp + size_new >= rt->vp0) rt->error = ERROR_PROGRAM_TOO_LONG;
    else {
        // If the new is not the last, makes room for it, shifting on the right.
        if (p != rt->pp) memmove(RAM + p + size_new, RAM + p, rt->pp - p);
        // Copy the line.
        memmove(RAM + p, RAM + a, size_new);
        rt->pp += size_new;
        rt->prog_changed = 1;
    }
OUT}

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
    }
OUT}

void prog_repl(runtime_t rt, FILE *f)
{IN
    extern int instr_exec(runtime_t);
    extern int tokenize(runtime_t rt);
    while (!feof(f)) {
dump_variables(rt);
        if (f == stdin) putchar('>');
        if (fgets(RAM + rt->buf0, BUF_SIZE, f) == NULL) break;
        tokenize(rt);
        // RAM[rt->obj] = line size.
        if (RAM[rt->obj + 1] == CODE_INTLIT) prog_edit(rt, rt->obj);
        else {
            rt->ip0 = rt->obj;
            rt->ip = rt->obj + 1;
            rt->ipn = NIL;
            if (instr_exec(rt) != ERROR_NONE) {
                if (rt->error > 0 && rt->error <= ERROR_VARIABLE_ALREADY_DEFINED)
                    puts(ERROR[rt->error]);
                else
                    printf("ERROR #%i\n", rt->error);
    }}}
OUT}

void prog_save(runtime_t rt, char *name)
{IN
    FILE *f = fopen(name, "w");
    if (f == NULL) rt->error = ERROR_CANNOT_OPEN_FILE;
    else {
        prog_print(rt, f);
        fclose(f);
        rt->prog_changed = 1;
    }
OUT}

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

/// Looks for a variable with name s and return its address, or NIL.
str_t var_find(runtime_t rt, str_t s)
{IN
    for (addr_t p = rt->vp0; p < rt->vp; p += peek(RAM + p))
        if (strcmp(RAM + p + sizeof(addr_t), RAM + s) == 0) {
OUT        return p;
        }
OUT return NIL;
}

/** Insert a new variable inside at rt->vp, updating rt->vp. According to the
    type, which is also returned as value, inserts the data from d1 on, while
    name and type, of course, are always inserted. */
int var_insert(runtime_t rt, addr_t name, int type, num_t d1, num_t d2,
               num_t step, addr_t ip0, addr_t offset)
{IN
    // Insert the variable: size, name, type, dim1 and possibly dim2 fields.
    addr_t var = rt->vp;
    rt->vp += sizeof(addr_t);   // Skip the size field, to be written later.
    poke(RAM + rt->vp, name);   // Write variable's name address.
    rt->vp += sizeof(addr_t);   // Skip the name field.
    RAM[rt->vp++] = type;       // Write the variable type and skip type field.
    if (type == VAR_NUM) { poke_num(RAM + rt->vp, 0); rt->vp += sizeof(num_t); }
    else if (type == VAR_STR) RAM[rt->vp++] = '\0';
    else if (type == VAR_FOR) {
        poke_num(RAM + rt->vp, d1); rt->vp += sizeof(num_t);
        poke_num(RAM + rt->vp, d2); rt->vp += sizeof(num_t);
        poke_num(RAM + rt->vp, step); rt->vp += sizeof(num_t);
        poke(RAM + rt->vp, ip0); rt->vp += sizeof(addr_t);
        poke(RAM + rt->vp, offset); rt->vp += sizeof(addr_t);
    } else {
        // array
        int numerical = type & VAR_NUM;
        // A string array is initialized by empty strings that take one byte.
        int size = d1 * d2 * (numerical ? sizeof(num_t) : 1);
        ON_ERROR(size >= rt->sp0 - rt->vp - 16, ERROR_OUT_OF_VARIABLE_MEMORY);
        poke(RAM + rt->vp, d1); rt->vp += sizeof(addr_t);
        if (type & VAR_MAT) { poke(RAM + rt->vp, d2); rt->vp += sizeof(addr_t); }
        // Allocates the actual array items.
        if (numerical) {
            for (int i = 0; i < d1*d2; ++ i) {
                poke_num(RAM + rt->vp, 0); rt->vp += sizeof(num_t);
            }
        } else {
            memset(RAM + rt->vp, 0, size);
            rt->vp += size;
    }}
    // Finally writes the size field of the variable.
    RAM[var] = rt->vp - var;
    return type;
OUT}

extern runtime_t expr(runtime_t);

/** Parse an array variable, thus "identifier(dim1, dim2)" and return all those
    data into: *name (address into the data area), *d1, *d2, while the type of
    the variable is returned as function value. Parse from IP which is updated
    to the token following che closed parenthesis. */
int var_array_parse(runtime_t rt, addr_t *name, addr_t *d1, addr_t *d2)
{
    int type = (CODE == CODE_IDN ? VAR_NUM : VAR_STR);
    // Parse the name.
    *name = peek(RAM + (++IP)); // Store the name address.
    IP += sizeof(addr_t);       // Skip the name address.
    // Parse the subscript(s).
    EXPECT('(', ERROR_SUBSCRIPT_EXPECTED);
    *d1 = pop_num(expr(rt));
    ON_ERROR(*d1 == 0, ERROR_SUBSCRIPT_ERROR);
    if (CODE == ',') {
        ++ IP;
        *d2 = pop_num(expr(rt));
        ON_ERROR(*d1 == 0, ERROR_SUBSCRIPT_ERROR);
        type |= VAR_MAT;
    } else {
        *d2 = 1;
        type |= VAR_VEC;
    }
    EXPECT(')', ERROR_OPENPAR_WITHOUT_CLOSEPAR);
OUT return type;
}

/** Parse the subscripts of a vector or matrix variable: type is the actual type
    of the variable, p is assumed to point to the variable's i (first dimension)
    field, the resulting address of the variable's item will be stored into a1
    and its scalar type returned. */
static int var_array_address(runtime_t rt, int type, addr_t p, addr_t *a1)
{IN
    int d1, d2, i, j = 1;
    int is_mat = type & VAR_MAT;
    d1 = peek(RAM + p); p += sizeof(addr_t);
    if (is_mat) { d2 = peek(RAM + p); p += sizeof(addr_t); }
    // Parse the subscript(s).
    EXPECT('(', ERROR_SUBSCRIPT_EXPECTED);
    i = pop_num(expr(rt));
    ON_ERROR(i < 1 || i > d1, ERROR_SUBSCRIPT_ERROR);
    if (is_mat) {
        EXPECT(',', ERROR_SUBSCRIPT_EXPECTED);
        j = pop_num(expr(rt));
        ON_ERROR(j < 1 || j > d2, ERROR_SUBSCRIPT_ERROR);
    }
    EXPECT(')', ERROR_OPENPAR_WITHOUT_CLOSEPAR);
    // According to the type, compute the address *a1 of the item.
    if (type & VAR_NUM) {
        *a1 = p + (i-1)*sizeof(num_t) + (j-1)*d1*sizeof(num_t);
OUT     return VAR_NUM;
    }
    *a1 = p + (i-1)*sizeof(str_t) + (j-1)*d1*sizeof(str_t);
OUT return VAR_STR;
}

/** Parse a variable, whose name (the CODE_IDN(S)) is pointed by IP and whose
    address in the variable's list is v, returning in *va the address of the
    value pointed by the variable expression (e.g. a$(1) points to the
    first string of the a$ array). On returning, IP points to the next token and
    the return value is the type of the variable. If a syntax error occurs, then
    an exception is raised. */
int var_address(runtime_t rt, addr_t v, addr_t *va)
{IN
    IP += 1 + sizeof(addr_t);           // Skip the variable's name
    addr_t p = v + 2*sizeof(addr_t);    // p points to the variable type.
    int type = RAM[p++];      
    if (type & (VAR_VEC|VAR_MAT)) {
OUT     return var_array_address(rt, type, p, va);
    }
    // Scalar or for variable: return it!
    *va = p;
OUT return type;
}

// //////////////////////// OPERATOR IMPLEMENTATIONS //////////////////////// //

//  Auxiliary functions.

/** Pop two values and compare them returning the result as an integer, the same
    as if strcmp or memcmp has been executed. */
static inline int oper_cmp(runtime_t rt)
{
    num_t n2;
    str_t s2;
    pop(rt, &n2, &s2);
    return (s2 == NIL) ? pop_num(rt) - n2
                            : strcmp(RAM + pop_str(rt), RAM + s2);
}

// Create a temporary string concatenating s1 and s2 and return it.
static str_t oper_concat(runtime_t rt, str_t s1, str_t s2)
{
    // Create the concatenation as temporary string.
    char *p1 = RAM + s1;
    unsigned l1 = strlen(p1);
    char *p2 = RAM + s2;
    unsigned l2 = strlen(p2);
    int addr = data_add_temp(rt, p1, l1 + l2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    strcpy(RAM + addr + l1, p2);
    return addr;
}

void OPER_ABS(runtime_t rt) {IN push_num(rt, fabs(pop_num(rt))); OUT}
void OPER_ACS(runtime_t rt) {IN push_num(rt, acos(pop_num(rt))); OUT}

void OPER_AND(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 && n2);
OUT}

void OPER_ASC(runtime_t rt) {IN push_num(rt, RAM[pop_str(rt)]); OUT}
void OPER_ATN(runtime_t rt) {IN push_num(rt, atan(pop_num(rt))); OUT}

void OPER_CHRS(runtime_t rt)
{IN
    char c[2];
    c[0] = pop_num(rt);
    c[1] = '\0';
    int addr = data_add_temp(rt, c, 1);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
OUT}

void OPER_COS(runtime_t rt) {IN push_num(rt, cos(pop_num(rt))); OUT}

void OPER_DIV(runtime_t rt)
{IN
    num_t n2 = pop_num(rt);
    push_num(rt, pop_num(rt) / n2);
OUT}

void OPER_EQ(runtime_t rt) {IN push_num(rt, oper_cmp(rt) == 0); OUT}
void OPER_EXP(runtime_t rt) {IN push_num(rt, exp(pop_num(rt))); OUT}
void OPER_GEQ(runtime_t rt) {IN push_num(rt, oper_cmp(rt) >= 0); OUT}
void OPER_GT(runtime_t rt) {IN push_num(rt, oper_cmp(rt) > 0); OUT}
void OPER_INT(runtime_t rt) {IN push_num(rt, floor(pop_num(rt))); OUT}

void OPER_LEFTS(runtime_t rt)
{IN
    int n2 = pop_num(rt);
    char *s1 = RAM + pop_str(rt);
    ON_ERROR(n2 < 0 || n2 > strlen(s1), ERROR_SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = data_add_temp(rt, s1, n2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
OUT}

void OPER_LEN(runtime_t rt) {IN push_num(rt, strlen(RAM + pop_str(rt))); OUT}
void OPER_LEQ(runtime_t rt)  {IN push_num(rt, oper_cmp(rt) <= 0); OUT}

void OPER_LOG(runtime_t rt)
{IN
    num_t n = pop_num(rt);
    ON_ERROR(n <= 0, ERROR_DOMAIN);
    push_num(rt, log(n));
OUT}

void OPER_LT(runtime_t rt) {IN push_num(rt, oper_cmp(rt) < 0); OUT}

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
OUT}

void OPER_MINUS(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 - n2);
OUT}

void OPER_MUL(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 * n2);
OUT}

void OPER_NEG(runtime_t rt) {IN push_num(rt, -pop_num(rt)); OUT}
void OPER_NEQ(runtime_t rt) {IN push_num(rt, oper_cmp(rt) != 0); OUT}
void OPER_NOT(runtime_t rt) {IN push_num(rt, !pop_num(rt)); OUT}

void OPER_OR(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 || n2);
OUT}

void OPER_PLUS(runtime_t rt)
{IN
    num_t n1, n2;
    str_t s1, s2;
    pop(rt, &n2, &s2);
    pop(rt, &n1, &s1);
    if (s1 == NIL) {
        ON_ERROR(s2 != NIL, ERROR_TYPE_MISMATCH);
        push_num(rt, n1 + n2);
    } else {
        ON_ERROR(s2 == NIL, ERROR_TYPE_MISMATCH);
        push_str(rt, oper_concat(rt, s1, s2));
    }
OUT}

void OPER_POW(runtime_t rt)
{IN
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    ON_ERROR(n1 <= 0, ERROR_DOMAIN);
    push_num(rt, pow(n1, n2));
OUT}

void OPER_RIGHTS(runtime_t rt)
{IN
    int n2 = pop_num(rt);
    char *s1 = RAM + pop_str(rt);
    ON_ERROR(n2 < 0 || n2 > strlen(s1), ERROR_SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = data_add_temp(rt, s1 + strlen(s1) - n2, n2);
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
OUT}

void OPER_RND(runtime_t rt)
{IN
    pop_num(rt);
    push_num(rt, rand());
OUT}

void OPER_SIN(runtime_t rt) {IN push_num(rt, sin(pop_num(rt))); OUT}

void OPER_SQR(runtime_t rt)
{IN
    num_t n = pop_num(rt);
    ON_ERROR(n < 0, ERROR_DOMAIN);
    push_num(rt, sqrt(n));
OUT}

void OPER_STRS(runtime_t rt)
{IN
    char s[32];
    sprintf(s, "%g", pop_num(rt));
    int addr = data_add_temp(rt, s, strlen(s));
    ON_ERROR(addr < 0, ERROR_OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
OUT}

void OPER_TAN(runtime_t rt) {IN push_num(rt, tan(pop_num(rt))); OUT}

void OPER_VAL(runtime_t rt)
{IN
    char *p = RAM + pop_str(rt);
    errno = 0;
    num_t n = strtod(p, NULL);
    ON_ERROR(errno, ERROR_ILLEGAL_CONVERSION);
    push_num(rt, n);
OUT}

/** The table of all operators: each operator has a name, an OPER_*** function
    implementing it, a flag binary true if the operator is infix, as a + b, or
    prefix as RIGHT$(x$,i), and a priority, used to pop it from the operator
    stack at the appropriated moment.

    To add a new operator FOO, just add it to the following table, insert the
    corresponding CODE_FOO constant in the appropriated place and implement the
    corresponding OPER_FOO function. */
const struct {
    char name[8];
    void (*routine)(runtime_t);
    short binary;
    short priority;
} OPERATORS[] = {
    // Ordered according to the name field.
    {"*", OPER_MUL, 1, 60},
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

/** Evaluate an expression at rt->ip and leave on the stack the value: return
    the runtime, so that it can be used in expressions (e.g. pop_num(expr(rt))).
    */
runtime_t expr(runtime_t rt)
{IN
    addr_t v, va;
    int open_close_match = 0;   // match between opened and closed parentheses.
    rt->dt = rt->dp;            // Reset temporary string area.
    for (;;) {
        // Prefix operators are the non binary ones (or '-' used as prefix).
        while (CODE == CODE_MINUS || CODE > CODE_STARTOPERATOR && CODE < CODE_ENDOPERATOR
        && !OPERATORS[CODE - CODE_STARTOPERATOR - 1].binary) {
            // Convert CODE_MINUS to CODE_NEG when considered as unary.
            int code = (CODE == CODE_MINUS ? CODE_NEG : CODE) - CODE_STARTOPERATOR - 1;
            // Execute operators on the stack with higher priority
            runtime_epop(rt, OPERATORS[code].priority);
            // Push the operator on the stack.
            runtime_epush(rt, OPERATORS[code].routine, OPERATORS[code].priority);
            ++ IP;
        }
        /** Operand. */
//~ printf("OPERAND: CODE = %i\n", CODE);
        switch (CODE) {
        case CODE_IDN: {
            ON_ERROR((v = var_find(rt, IP + 1)) == NIL, ERROR_UNDEFINED_VARIABLE);
            var_address(rt, v, &va);
            push_num(rt, peek_num(RAM + va));
            break;
        } case CODE_IDNS: {
            ON_ERROR((v = var_find(rt, IP + 1)) == NIL, ERROR_UNDEFINED_VARIABLE);
            var_address(rt, v, &va);
            // Make a temporary copy of string va.
            int s = data_add_temp(rt, RAM + va, strlen(RAM + va));
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
            // A closed parenthesis may delimit the end of an expression.
            if (rt->estack_next == 0) {
OUT             return rt;
            }
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
    }
OUT return rt;
}

// /////////////////////// ASSIGNMENT IMPLEMENTATIONS /////////////////////// //

//  To assign a number value use poke_num!!!

/** Assign to the string variable v the string s: the address of the string to
    overwrite is expected in va, while v contains the first byte of the variable
    in the variable's list (its size field). */
void assign_string(runtime_t rt, addr_t v, str_t va, str_t s)
{IN
    // va points to the first character of the string to overwrite with s.
    unsigned len_v = strlen(RAM + va);
    unsigned len_s = strlen(RAM + s);
    int delta = len_s - len_v;
    ON_ERROR(delta >= rt->sp0 - rt->vp, ERROR_OUT_OF_VARIABLE_MEMORY);
    if (delta != 0) {
        // Reduce/Augment the space for the string.
        memmove(RAM + va + len_s, RAM + va + len_v, rt->vp - (va + len_v));
        // Adjust the pointer to the first free byte in the variable area.
        rt->vp += delta;
        // Adjust the size of the variable containing the string.
        poke(RAM + v, peek(RAM + v) + delta);
    }
    // Finally copy string s on string a1.
    strcpy(RAM + va, RAM + s);
OUT}

//~ /** Assign to the variable with given type, located at v and whose value is at
    //~ address va, the value at address val). */
//~ void assign(runtime_t rt, int type, addr_t v, addr_t va, addr_t val)
//~ {IN
    //~ if (type & VAR_NUM) {
        //~ poke_num(RAM + va, peek_num(RAM + val));
    //~ } else {
        //~ assert(type & VAR_STR);
        //~ assign_string(rt, v, va, val);
    //~ }
//~ OUT}

/** Parse "= expr" and assign the value to the variable of given type, at
    address v and whose value is at address va. */
void assign_expr(runtime_t rt, int type, addr_t v, addr_t va)
{IN
    ON_ERROR(type == VAR_NONE, ERROR_UNDEFINED_VARIABLE);
    EXPECT(CODE_EQ, ERROR_EQ_EXPECTED);
    expr(rt);
    if (type & VAR_NUM) {
        poke_num(RAM + va, pop_num(rt));
    } else {
        assert(type & VAR_STR);
        assign_string(rt, v, va, pop_str(rt));
    }
OUT}

// ////////////////////// INSTRUCTION IMPLEMENTATIONS ////////////////////// //

//  Auxiliary functions.

/// Set rt->ip and rt->ipn to execute the line at rt->ip0.
static inline void instr_line(runtime_t rt)
{
    rt->ipn = rt->ip0 + RAM[rt->ip0];   // next line
    IP = rt->ip0 + 4;               // skip line length + CODE_INTLIT + integer.
}

/** Parse a possible channel and return the corresponding file and buffer into
    *file and *buffer. */
static inline void instr_channel(runtime_t rt, FILE **file, addr_t *buffer)
{
    *buffer = rt->buf0; // Terminal buffer
    if (CODE == '#') {
        ++ IP;
        int ch = pop_num(expr(rt));
        ON_ERROR(ch < 0 || ch > 4, ERROR_ILLEGAL_CHANNEL);
        ON_ERROR(rt->channels[ch] == NULL, ERROR_CHANNEL_CLOSED);
        *file = rt->channels[ch];
        *buffer += ch * BUF_SIZE;
        EXPECT(',', ERROR_COMMA_EXPECTED);
}}

void INSTR_BYE(runtime_t rt)
{IN
    if (prog_check(rt)) {
        puts("BYE.");
        exit(EXIT_SUCCESS);
    }
OUT}

void INSTR_CLOSE(runtime_t rt)
{IN
    // CLOSE #channel
    EXPECT('#', ERROR_HASH_EXPECTED);
    int ch = pop_num(expr(rt));
    ON_ERROR(ch < 1 || ch > 4, ERROR_ILLEGAL_CHANNEL);
    ON_ERROR(rt->channels[ch] == NULL, ERROR_CHANNEL_CLOSED);
    fclose(rt->channels[ch]);
    rt->channels[ch] = NULL;
OUT}

void INSTR_DATA(runtime_t rt)
{IN
    ON_ERROR(rt->ipn == NIL, ERROR_ILLEGAL_INSTRUCTION_OUTSIDE_PROGRAM);
    // Skip to the next line.
    rt->ip0 = rt->ipn;
    instr_line(rt);
OUT}

void INSTR_DEF(runtime_t rt) {IN assert(!"TODO!"); OUT}

void INSTR_DIM(runtime_t rt)
{IN
    for (;;) {
        ON_ERROR(CODE != CODE_IDN && CODE != CODE_IDNS, ERROR_IDENTIFIER_EXPECTED);
        addr_t d1, d2;
        str_t name;
        int type = var_array_parse(rt, &name, &d1, &d2);
        var_insert(rt, name, type, d1, d2, 0, 0, 0);
        if (CODE != ',') break;
        ++ IP;
    }
OUT}

void INSTR_ERROR(runtime_t rt) {IN ON_ERROR(1, pop_num(expr(rt))); OUT}

void INSTR_FOR(runtime_t rt) {IN assert(!"TODO!"); OUT}

void INSTR_GOSUB(runtime_t rt)
{IN
    addr_t a = prog_find(rt, pop_num(expr(rt)));
    ON_ERROR(a == NIL, ERROR_ILLEGAL_LINE_NUMBER);
    rpush(rt, rt->ip0, IP - rt->ip0);
    rt->ip0 = a;
    instr_line(rt);
OUT}

void INSTR_GOTO(runtime_t rt)
{IN
    rt->ip0 = prog_find(rt, pop_num(expr(rt)));
    ON_ERROR(rt->ip0 == NIL, ERROR_ILLEGAL_LINE_NUMBER);
    instr_line(rt);
OUT}

void INSTR_IF(runtime_t rt)
{IN
    // If the integer value of the expression is false, skip to next line.
    ON_ERROR(!(int)pop_num(expr(rt)), ERROR_NONE);
OUT}

void INSTR_INPUT(runtime_t rt)
{IN
    FILE *f = stdin;
    addr_t b = rt->buf0;    // Terminal buffer
    instr_channel(rt, &f, &b);
    ON_ERROR(fgets(RAM + b, BUF_SIZE, f) == NULL, ERROR_ILLEGAL_INPUT);
    char *p = strchr(RAM + rt->buf0, '\n');
    if (p != NULL) *p = '\0';   // Drop the ending '\n' if any.
    // Now parse the text in the buffer and the list of variables.
    for (byte_t *p = RAM + b; p < RAM + b + BUF_SIZE && *p != '\n' && *p != '\0'; ) {
        p += strspn(p, " \t");  // skip blanks
        addr_t v = var_find(rt, IP + 1);
        ON_ERROR(v == NIL, ERROR_UNDEFINED_VARIABLE);
        addr_t va;
        int type = var_address(rt, v, &va);
        if (type & VAR_NUM) {
            char *p0 = p;
            num_t n = strtod(p0, &p);
            ON_ERROR(p0 == p, ERROR_ILLEGAL_INPUT);
            poke_num(RAM + va, n);
        } else {
            char *p1 = strpbrk(p, ",\n");
            if (p1 == NULL) assign_string(rt, v, va, p - RAM);
            else {
                int c = *p1;
                *p1 = '\0';
                assign_string(rt, v, va, p - RAM);
                *p1 = c;
            }
            p = p1;
        }
        if (CODE != ',') break;
        ON_ERROR(*p != ',', ERROR_ILLEGAL_INPUT);
        ++ IP;
        ++ p;
    }
OUT}

void INSTR_LET(runtime_t rt)
{IN
    for (;;) {
        addr_t v = var_find(rt, IP + 1);
//~ printf("%u %s\n", v, RAM + peek(RAM + IP + 1));
        ON_ERROR(v != NIL, ERROR_VARIABLE_ALREADY_DEFINED);
        v = rt->vp;     // The variable will be inserted here.
        int type = (CODE == CODE_IDN) ? VAR_NUM : VAR_STR;
        var_insert(rt, peek(RAM + IP + 1), type, 0, 0, 0, 0, 0);
        addr_t va;
        var_address(rt, v, &va);
        assign_expr(rt, type, v, va);
        if (CODE != ',') break;
        ++ IP;
    }
OUT}

void INSTR_LINPUT(runtime_t rt)
{IN
    FILE *f = stdin;
    addr_t b = rt->buf0;    // Terminal buffer
    instr_channel(rt, &f, &b);
    addr_t v = var_find(rt, IP + 1);
    ON_ERROR(v == NIL, ERROR_UNDEFINED_VARIABLE);
    addr_t va;
    int type = var_address(rt, v, &va);
    ON_ERROR(!(type & VAR_STR), ERROR_STRING_VARIABLE_EXPECTED);
    ON_ERROR(fgets(RAM + b, BUF_SIZE, f) == NULL, ERROR_ILLEGAL_INPUT);
    char *p = strchr(RAM + b, '\n');
    if (p != NULL) *p = '\0';   // Drop the ending '\n' if any.
    // Assign to variable v the value of the string k.
    assign_string(rt, v, va, b);
OUT}

void INSTR_LIST(runtime_t rt) {IN prog_print(rt, stdout); OUT}

void INSTR_LOAD(runtime_t rt)
{IN
    extern void INSTR_MERGE(runtime_t);
    if (prog_check(rt)) INSTR_MERGE(runtime_reset(rt, 1));
    else (void) pop_str(expr(rt));  // skip the program name.
OUT}

void INSTR_MERGE(runtime_t rt)
{IN
    FILE *f = fopen(RAM + pop_str(expr(rt)), "r");
    ON_ERROR(f == NULL, ERROR_CANNOT_OPEN_FILE);
    prog_repl(rt, f);
    fclose(f);
    rt->prog_changed = 0;
    // The rt->obj buffer has been corrupted loading the file.
    longjmp(rt->err_buffer, 0);
OUT}

void INSTR_NEW(runtime_t rt) {IN if (prog_check(rt)) runtime_reset(rt, 1); OUT}

void INSTR_NEXT(runtime_t rt) {IN assert(!"TODO!"); OUT}

void INSTR_ONERROR(runtime_t rt)
{IN
    int n = pop_num(expr(rt));
    ON_ERROR(prog_find(rt, n) == NIL, ERROR_ILLEGAL_LINE_NUMBER);
    rt->error_routine = n;
OUT}

void INSTR_OPEN(runtime_t rt)
{IN
    // OPEN #channel, name, mode
    // channel and mode are integers, name is a string.
    EXPECT('#', ERROR_HASH_EXPECTED);
    int ch = pop_num(expr(rt));
    ON_ERROR(ch < 1 || ch > 4, ERROR_ILLEGAL_CHANNEL);
    ON_ERROR(rt->channels[ch] != NULL, ERROR_CHANNEL_BUSY);
    EXPECT(',', ERROR_COMMA_EXPECTED);
    str_t name = pop_str(expr(rt));
    EXPECT(',', ERROR_COMMA_EXPECTED);
    int mode = pop_num(expr(rt));
    ON_ERROR(mode < 0 || mode > 2, ERROR_ILLEGAL_MODE);
    rt->channels[ch] = fopen(RAM + name, mode == 0 ? "r" : mode == 1 ? "w" : "a");
    ON_ERROR(rt->channels[ch] == NULL, ERROR_CANNOT_OPEN_FILE);
OUT}

void INSTR_PRINT(runtime_t rt)
{IN
    FILE *f = stdout;
    addr_t b = rt->buf0;    // Terminal buffer
    instr_channel(rt, &f, &b);
    int newline = 1;
    while (CODE != 0 && CODE != ':') {
        if (CODE == ',' || CODE == ';') {
            if (CODE == ',') fputc('\t', f);
            ++ IP;
            newline = 0;
            continue;
        }
        num_t n;
        str_t s;
        pop(expr(rt), &n, &s);
        if (s == NIL) fprintf(f, "%g", n); else fputs(RAM + s, f);
        newline = 1;
    }
    if (newline) fputc('\n', f);
OUT}

void INSTR_READ(runtime_t rt)
{IN
    // Looks for the next DATA instruction from rt->data.
    while (rt->data < rt->vp0) {
        if (RAM[rt->data + 4] == CODE_DATA) {
            // OK: read from this line a sequence of data
            assert(!"TODO!");
        }
        rt->data += RAM[rt->data];
    }
    ON_ERROR(1, ERROR_OUT_OF_DATA);
OUT}

void INSTR_REPEAT(runtime_t rt)
{IN
    ON_ERROR(rt->ipn == NIL, ERROR_ILLEGAL_INSTRUCTION_OUTSIDE_PROGRAM);
    IP = rt->ip0 + 4;
OUT}

void INSTR_RESTORE(runtime_t rt) {IN rt->data = rt->pp0; OUT}

void INSTR_RETURN(runtime_t rt)
{IN
    byte_t offset;
    rpop(rt, &rt->ip0, &offset);
    IP = rt->ip0 + offset;
    rt->ipn = rt->ip0 + RAM[rt->ip0];
OUT}

void INSTR_RUN(runtime_t rt)
{IN
    extern int instr_exec(runtime_t);
    // Saves ip0, ip and ipn
    addr_t ip0_saved = rt->ip0;
    addr_t ip_saved = rt->ip;
    addr_t ipn_saved = rt->ipn;
    // Reset just variables and stacks.
    runtime_reset(rt, 0);
    // The increment rt->ipn is set inside the loop.
    for (rt->ip0 = rt->pp0; rt->ip0 < rt->pp; rt->ip0 = rt->ipn) {
        instr_line(rt);
        if (instr_exec(rt)) {
            printf("LINE %i: ", peek(RAM + rt->ip0 + 2));
            break;
    }}
    rt->ip0 = ip0_saved;
    rt->ip = ip_saved;
    rt->ipn = ipn_saved;
OUT}

void INSTR_SAVE(runtime_t rt) {IN prog_save(rt, RAM + pop_str(expr(rt))); OUT}

void INSTR_SKIP(runtime_t rt)
{IN
    ON_ERROR(rt->ipn == NIL, ERROR_ILLEGAL_INSTRUCTION_OUTSIDE_PROGRAM);
    // Skip to the next line and skip it!
    rt->ip0 = rt->ipn;
    instr_line(rt);
OUT}

void INSTR_STEP(runtime_t rt) {IN ON_ERROR(1, ERROR_ILLEGAL_INSTRUCTION); OUT}
void INSTR_STOP(runtime_t rt) {IN ON_ERROR(1, ERROR_PROGRAM_STOPPED); OUT}
void INSTR_SYS(runtime_t rt) {IN system(RAM + pop_str(expr(rt))); OUT}
void INSTR_THEN(runtime_t rt) {IN OUT}
void INSTR_TO(runtime_t rt) {IN ON_ERROR(1, ERROR_ILLEGAL_INSTRUCTION); OUT}

/// Table of all routines implementing instructions.
void (*INSTRUCTION_CODE[])(runtime_t) = {
    /*  Same ordering as the constant between CODE_STARTKEYWORD and
        CODE_ENDKEYWORD */
    INSTR_BYE, INSTR_CLOSE, INSTR_DATA, INSTR_DEF, INSTR_DIM, INSTR_ERROR,
    INSTR_FOR, INSTR_GOSUB, INSTR_GOTO, INSTR_IF, INSTR_INPUT, INSTR_LET,
    INSTR_LINPUT, INSTR_LIST, INSTR_LOAD, INSTR_MERGE, INSTR_NEW, INSTR_NEXT,
    INSTR_ONERROR, INSTR_OPEN, INSTR_PRINT, INSTR_READ, INSTR_REPEAT,
    INSTR_RESTORE, INSTR_RETURN, INSTR_RUN, INSTR_SAVE, INSTR_SKIP, INSTR_STEP,
    INSTR_STOP, INSTR_SYS, INSTR_THEN, INSTR_TO
};

/** Execute the line at IP0, starting from instruction IP. Return the value of
    rt->error. */
int instr_exec(runtime_t rt)
{IN
    // Save exception buffer
    jmp_buf error_saved;
    memcpy(error_saved, rt->err_buffer, sizeof(error_saved));
Again:
    rt->error = 0;
    if (setjmp(rt->err_buffer) == 0) {
        volatile byte_t opcode;
        while ((opcode = CODE) != 0 && opcode != '\'') {
            // Skip instruction separators.
            while (CODE == ':') ++ IP;
            if ((opcode = CODE) > CODE_STARTKEYWORD && opcode < CODE_ENDKEYWORD) {
                ++ IP; (*INSTRUCTION_CODE[opcode - CODE_STARTKEYWORD - 1])(rt);
            } else if (opcode == CODE_IDN || opcode == CODE_IDNS) {
                addr_t v = var_find(rt, IP + 1);
                ON_ERROR(v == NIL, ERROR_UNDEFINED_VARIABLE);
                addr_t va;
                int type = var_address(rt, v, &va);
                assign_expr(rt, type, v, va);
            } else {
                ON_ERROR(1, ERROR_ILLEGAL_INSTRUCTION);
    }}} else {
        // If some ONERROR instruction has been issued, execute it.
        if (rt->error_routine != 0) {
            rt->ip0 = prog_find(rt, rt->error_routine);
            // If the ONERROR line is wrong, do nothing.
            if (rt->ip0 != NIL) {
                instr_line(rt);
                goto Again;
    }}}
    // Restore exception buffer.
    memcpy(rt->err_buffer, error_saved, sizeof(error_saved));
    return rt->error;
OUT}

// //////////////////////////// LEXICAL ANALIZER //////////////////////////// //

/// Print a token at address a on file f: return the address of next token.
addr_t token_dump(runtime_t rt, addr_t a, FILE *f)
{
IN  static int space = 0;   // True if a space is the last dumped character.
    byte_t b = RAM[a++];
    if (b == CODE_IDN || b == CODE_IDNS) {
        fprintf(f, "%s ", RAM + peek(RAM + a));
        a += 2;
        space = 1;
    } else if (b == CODE_INTLIT) {
        fprintf(f, "%i", peek(RAM + a));
        a += 2;
        space = 0;
    } else if (b == CODE_NUMLIT) {
        fprintf(f, "%g", peek_num(RAM + a));
        a += sizeof(num_t);
        space = 0;
    } else if (b == CODE_STRLIT) {
        fprintf(f, "\"%s\"", RAM + peek(RAM + a));
        a += 2;
        space = 0;
    } else if (b == '\'') {
        fputs(RAM + a - 1, f);
        a += strlen(RAM + a) + 1;
        space = 0;
    } else if (b == CODE_IDN || b == CODE_IDNS) {
        fprintf(f, "%s", RAM + peek(RAM + a));
        a += 2;
        space = 0;
    } else if (b > CODE_STARTKEYWORD && b < CODE_ENDKEYWORD) {
        fprintf(f, "%s ", KEYWORDS[b - CODE_STARTKEYWORD - 1]);
        space = 1;
    } else if (b > CODE_STARTOPERATOR && b < CODE_ENDOPERATOR) {
        if (!space) fputc(' ', f);
        fprintf(f, "%s ", OPERATORS[b - CODE_STARTOPERATOR - 1].name);
        space = 1;
    } else if (b != 0) {
        if (b == '(' || b == ')') {
            if (space) fputc('\b', f);
            fputc(b, f);
            space = 0;
        } else {
            //~ if (!space) fputc(' ', f);
            fputc(b, f);
            fputc(' ', f);
            space = 1;
        }
    }
OUT return a;
}

/** Try to match a keyword with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding keyword code is returned, else 0. */
int token_keyword(char *p0, unsigned len)
{
IN for (int i = 0; i < CODE_ENDKEYWORD - CODE_STARTKEYWORD - 1; ++ i) {
        int cmp = memcmp(KEYWORDS[i], p0, len);
        if (cmp == 0 && len == strlen(KEYWORDS[i])) {
OUT         return (CODE_STARTKEYWORD + 1) + i;
        }
        if (cmp > 0) break;
    }
OUT return 0;
}

/** Try to match an operator with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding operator code is returned, else 0. */
int token_operator(char *p0, unsigned len)
{
IN  for (int i = 0; i < CODE_ENDOPERATOR - CODE_STARTOPERATOR - 1; ++ i) {
        int cmp = memcmp(OPERATORS[i].name, p0, len);
        if (cmp == 0 && OPERATORS[i].name[len] == '\0') {
OUT         return (CODE_STARTOPERATOR + 1) + i;
        }
        if (cmp > 0) break;
    }
OUT return 0;
}

///  Tokenize the contents of buf0 and store it at obj: return 0 on error.
int tokenize(runtime_t rt)
{
IN  int k, len;
    // Encode the buf0 string at obj.
    byte_t *p = RAM + rt->buf0;
    byte_t *q0 = RAM + rt->obj; // The line size will be stored here.
    byte_t *q = q0 + 1;             // The line will be stored here.
    byte_t b;
    while (*p != '\n' && *p != '\0') {
        p += strspn(p, " \t");  // Skip blanks.
        if (*p > 127) {
            printf("INVALID ASCII CODE %i\n", *p);
OUT         return 0;
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
//~ printf("int %i\n", peek(q+1));
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
            if (*p == '$') { *q1 = '$'; ++ p; }
            len = p - p0;
            if ((k = token_keyword(q, len)) > 0) *q++ = k;
            else if ((k = token_operator(q, len)) > 0) *q++ = k;
            else {
                if ((k = data_find(rt, q, len)) < 0
                && (k = data_add(rt, q, len)) < 0) {
                    puts("TOO MANY IDENTIFIERS");
OUT                 return 0;
                }
                *q = (*q1 == '$') ? CODE_IDNS : CODE_IDN;
                poke(q + 1, k);
//~ printf("idn %s\n", ram + peek(q+1));
                q += 3;
            }
        } else if (*p == '\'') {    // Comment.
            /*  A comment is compiled as '\'' followed by the string
                with the comment text. */
            len = strlen(p) + 1;
            strcpy(q, p);
            q += len;
            p += len;
        } else if (*p == '"') {
            len = strcspn(++p, "\"");
            if (p[len] != '"') {
                puts("END OF LINE INSIDE STRING");
OUT             return 0;
            }
            if ((k = data_find(rt, p, len)) < 0
            && (k = data_add(rt, p, len)) < 0) {
                puts("TOO MANY STRING CONSTANTS");
OUT             return 0;
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
OUT return 1;
}

// ////////////////////////////////// MAIN ////////////////////////////////// //

int main(int npar, char **pars)
{
IN  struct runtime_s rt;
    runtime_init(&rt);
    if (npar > 2) {
        puts("USAGE: straybasic [file.bas]");
        return EXIT_FAILURE;
    }
    if (npar == 2) {
        FILE *f = fopen(pars[1], "r");
        if (f == NULL) puts(ERROR[ERROR_CANNOT_OPEN_FILE]);
        else {
            printf("LOAD PROGRAM %s\n", pars[1]);
            prog_repl(&rt, f);
            fclose(f);
            rt.prog_changed = 0;
    }}
    //fputs("\033[2J\033[92m", stdout);   // clear screen, green ink.
    puts("StrayBasic");
    puts("(c) 2024 by Paolo Caressa");
    puts("[Type BYE to quit]\n");
    prog_repl(&rt, stdin);
OUT return EXIT_SUCCESS;
}
