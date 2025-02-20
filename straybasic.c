/// \file straybasic.c
/// \author Paolo Caressa <github.com/pcaressa>
/// \date 20241231
/// \todo Assign substrings as in LET X$(2 TO 3) = ...
/// \todo Calling external programs.
/// \todo MAT instructions
/// \todo More functions, such as SPACE$(n), TIME(n) etc.
/// \todo Multiple line DEF FNs.

#define VERSION "STRAYBASIC 1.0"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/// \defgroup CONSTANTS Global Constants
/// \{

#define BUF_NUM (4)         ///< Number of file buffer (included stdin/out).
#define BUF_SIZE (256)      ///< Total length of a file buffer.
#define CSTR_SIZE (4096)    ///< Size of string area.
#define ESTACK_SIZE (20)    ///< Numbers of items in the expression-stack.
#define LINE_MIN (1)        ///< Minimum line number.
#define LINE_MAX (9999)     ///< Maximum line number.
#define PROG_SIZE (8192)    ///< Size of program area.
#define RAM_SIZE (65536)    ///< Total RAM size, <= 65536.
#define RSTACK_SIZE (60)    ///< Numbers of items in the return-stack.
#define STACK_SIZE (120)    ///< Numbers of items in the stack.

/** Token codes: keyword and operator codes are in the same ordering as the
    corresponding items in the Operators and Instructions tables are. */
enum {
    CODE_INTLIT = 128, CODE_NUMLIT, CODE_STRLIT, CODE_IDN, CODE_IDNS,
    CODE_STARTKEYWORD,  // Fake code, used as delimiter
#   define I(name) CODE_##name,
#   include "straybasic.h"
    CODE_ENDKEYWORD,    // Fake code, used as delimiter
    CODE_STARTOPERATOR, // Fake code, used as delimiter
#   define O(name, label, arity, isinfix, priority) CODE_##label,
#   include "straybasic.h"
    CODE_ENDOPERATOR,   // Fake code, used as delimiter
};

/** Keywords, in the same ordering as the constant between CODE_STARTKEYWORD and
    CODE_ENDKEYWORD and items in Instructions[] are. */
const char *Keywords[] = {
#   define I(name) #name,
#   include "straybasic.h"
#   undef I
};

/// Error codes: same ordering as the Errors[] costants.
enum {
#   define E(code, message) ERROR_##code,
#   include "straybasic.h"
};

/// Error messages corresponding to error codes.
const char *Errors[] = {
#   define E(code, message) message,
#   include "straybasic.h"
};

/// Variable types: they are bits in a byte, so & and | apply.
enum {
    VAR_NONE = 0, VAR_CHR = 1, VAR_NUM = 2, VAR_STR = 4, VAR_FOR = 8,
    VAR_VEC = 16, VAR_MAT = 32
};

/// \}
/// \defgroup TYPES Data Types
/// \{

typedef uint8_t byte_t;     ///< Byte.
typedef uint16_t addr_t;    ///< Pointer to any RAM byte.
typedef uint16_t str_t;     ///< Pointer to string.
typedef float num_t;        ///< Number.

#define NIL (0xFFFF)        ///< Used as NULL address for strings.

/// \}
/// \defgroup RUNTIME Runtime Class
/// \{

/** Object contain global variables, rt means "runtime". */
struct {
    /** All stuff (constants, programs, variables, stacks, buffers) are stored
        in a 16-bit addressable RAM memory. */
    byte_t ram[RAM_SIZE];

    addr_t csp0;    ///< RAM[csp0:csp] contains string constants.
    addr_t csp;     ///< RAM[csp0:csp] contains string constants.
    addr_t tsp;     ///< RAM[csp:tsp] contains temporary string constants.
    addr_t pp0;     ///< RAM[pp0:pp] contains the program.
    addr_t pp;      ///< RAM[pp0:pp] contains the program.
    addr_t vp0;     ///< RAM[vp0:vp] contains the variables list.
    addr_t vp;      ///< RAM[vp0:vp] contains the variables list.
    addr_t sp0;     ///< RAM[sp0:sp] contains the parameters stack.
    addr_t sp;      ///< RAM[sp0:sp] contains the parameters stack.
    addr_t rsp0;    ///< RAM[rsp0:rsp] contains the return stack.
    addr_t rsp;     ///< RAM[rsp0:rsp] contains the return stack.
    
    /** Object code buffer: a source line is converted in inner form here,
        before being inserted in the program or executed. */
    addr_t obj;
    
    /// File buffers: buf[0] is reserved to the terminal.
    addr_t buf[1 + BUF_NUM];

    /** I/O files: channels[0] is always stdin or stdout, while channels[i]
       from i = 1 to BUF_NUM are NULL or handles to opened text files. */
    FILE *channels[1 + BUF_NUM];

    addr_t ip0;         ///< First byte of the current line (its size byte).
    addr_t ip;          ///< First byte of next token in the current line.
    addr_t data_next;   ///< Address of item in a DATA statement to READ from.

    jmp_buf err_buffer; ///< Exception handler.

    int error;          ///< Current error code: 0 means "no error".
    int err;            /**< If ON ERROR is set, error is copied to err, so that
                            the ERR function will return it, even if error is
                            reset. */
    addr_t on_error;    ///< If != NIL, instruction where to jump on error.
    int prog_changed;   ///< 1 if the program was not saved since last changes.
    int trace;          ///< 1 if runnig the program lines are printed.

    /** Operator stack: contains operator with priorities lower than the one
        under execution, inside an expression. */
    struct { void (*routine)(void); int priority; } estack[ESTACK_SIZE];
    int estack_next;    ///< 0 it means "stack empty".
    
    time_t t0;          ///< Interpreter launch time.
} rt;

//  Common shortcuts: assume the rt_t variable rt to be defined.
#define IP (rt.ip)
#define RAM (rt.ram)
#define CODE (RAM[IP])
#define ERROR(e) longjmp(rt.err_buffer, rt.error = (ERROR_##e));
#define EXPECT(tok, msg) if (RAM[IP++] != tok) ERROR(msg)

void rt_ctrlbreak(int sig) {
    // ERROR works only inside a program...
    if (rt.ip0 == rt.obj) IP = NIL;
    else {
        switch (sig) {
            case SIGINT: ERROR(BREAK);
}}}

/** Initialize a virtual ram. */
void rt_init(void) {
    // Memory area limits, always set
    rt.csp0 = 0;
    rt.pp0 = rt.csp0 + CSTR_SIZE;
    rt.vp0 = rt.pp0 + PROG_SIZE;
    
    // Upper part of the memory contains 256-sized buffers.
    rt.buf[BUF_NUM] = RAM_SIZE - BUF_SIZE;
    for (int i = BUF_NUM - 1; i >= 0; -- i)
        rt.buf[i] = rt.buf[i+1] - BUF_SIZE;
    rt.obj = rt.buf[0] - BUF_SIZE;
    rt.rsp0 = rt.obj - RSTACK_SIZE;
    rt.sp0 = rt.rsp0 - STACK_SIZE;

    // Drop stacks.
    rt.rsp = rt.rsp0;
    rt.sp = rt.sp0;
    rt.estack_next = 0;
    rt.error = 0;
    rt.err = 0;
    rt.on_error = NIL;
    rt.trace = 0;

    // Reset data pointer: points to the first token of the first line.
    rt.data_next = rt.pp0 + 2 + sizeof(addr_t);

    // Drop variables.
    rt.vp = rt.vp0;
    
    // Drop data and program.
    rt.tsp = rt.csp = rt.csp0;
    rt.pp = rt.pp0;
    rt.prog_changed = 0;
    for (int i = 0; i < 1 + BUF_NUM; ++ i)
        rt.channels[i] = NULL;

    // Set the initial time.
    rt.t0 = time(NULL);

    signal(SIGINT, rt_ctrlbreak);
}

/// Constants used as parameters in the rt_reset() function.
enum {
    RT_RESET_PROG = 1,  ///< Reset program and strings
    RT_RESET_VARS = 2,  ///< Reset variables
    RT_RESET_FILES = 4,
    RT_RESET_ALL = 255
};

/** Reset the virtual machine: according to the value of flags, reset the
    corresponding item. */
void rt_reset(int flags) {
    if (flags == RT_RESET_ALL) {
        rt.csp = rt.csp0;
    }
    if (flags & RT_RESET_VARS) {
        // Reset data pointer: points to the first token of the first line.
        rt.data_next = rt.pp0 + 2 + sizeof(addr_t);
        rt.vp = rt.vp0;     // Drop all variables.
    }
    if (flags & RT_RESET_PROG) {
        rt.pp = rt.pp0;
        rt.prog_changed = 0;
        rt.on_error = NIL;
        rt.trace = 0;
        flags |= RT_RESET_FILES;
    }
    if (flags & RT_RESET_FILES) {
        for (int i = 1; i <= BUF_NUM; ++ i) {
            if (rt.channels[i] != NULL) {
                fclose(rt.channels[i]);
                rt.channels[i] = NULL;
    }}}
    // Some globals are always reset.
    rt.estack_next = 0; // Reset operator stack.
    rt.sp = rt.sp0;     // Reset operand stack.
    rt.tsp = rt.csp;    // Reset temporary string area.
    rt.error = 0;       // Reset the error status.
    rt.err = 0;         // Reset the err status.
}

/** Push the routine address r and relative priority p on the estack: while
    on top of it there are operators with higher priorities, execute them. */
void rt_epush(void (*r)(void), int p) {
    if (rt.estack_next >= ESTACK_SIZE) ERROR(EXPRESSION_TOO_LONG);
    rt.estack[rt.estack_next].routine = r;
    rt.estack[rt.estack_next].priority = p;
    ++ rt.estack_next;
}

/** Pop and execute all routines on the rt.estack with priority >= p. */
void rt_epop(int p) {
    /*  priority >= p implies operators associate to the left (with > p they
        would associate to the right). */
    while (rt.estack_next > 0 && rt.estack[rt.estack_next - 1].priority >= p)
        (*rt.estack[--rt.estack_next].routine)();
}

/// \}
/// \defgroup MEMORY Memory Access
/// \{

addr_t peek(byte_t *a) { return *a + (a[1] << 8); }
num_t peek_num(byte_t *a) { return *(num_t*)a; }
void poke(byte_t *a, addr_t i) { *a = i & 0xFF; a[1] = i >> 8; }
void poke_num(byte_t *a, num_t n) { *(num_t*)a = n; }

#define PEEK(a) (peek(RAM + (a)))
#define PEEK_NUM(a) (peek_num(RAM + (a)))
#define POKE(a,v) poke(RAM + (a), v)
#define POKE_NUM(a,v) poke_num(RAM + (a), v)

/// \}
/// \defgroup DUMP Debug stuff
/// \{

void dump_channels(void) {
    fputs("CHANNELS:\n   ", stdout);
    for (int i = 0; i < 1 + BUF_NUM; ++ i)
        printf(" #%i %s.", i, rt.channels[i] == NULL ? "FREE" : "BUSY");
    putchar('\n');
}

void dump_cstr(void) {
    puts("STRINGS:");
    for (addr_t p = rt.csp0; p < rt.csp; p += strlen(rt.ram + p) + 1)
        printf(" \"%s\"", rt.ram + p);
    if (rt.csp0 < rt.csp) putchar('\n');
}

void dump_memory(void) {
    fputs("MEMORY:\n   ", stdout);
    printf("STRINGS = %i/%i (%2i%%);", rt.csp - rt.csp0, rt.pp0 - rt.csp0, (int)(100.0*(rt.csp - rt.csp0) / (rt.pp0 - rt.csp0)));
    printf(" PROGRAM = %i/%i (%2i%%);", rt.pp - rt.pp0, rt.vp0 - rt.pp0, (int)(100.0*(rt.pp - rt.pp0) / (rt.vp0 - rt.pp0)));
    printf(" VARIABLES = %i/%i (%2i%%)\n", rt.vp - rt.vp0, rt.sp - rt.vp0, (int)(100.0*(rt.vp - rt.vp0) / (rt.sp - rt.vp0)));
    puts("MEMORY MAP:\n    | strings | program | variables | free space | stacks | buffers |");
    printf("  %04X      %04X      %04X        %04X         %04X     %04X      FFFF\n",
        rt.csp0, rt.pp0, rt.vp0, rt.vp, rt.sp0, rt.obj);
    printf("REGISTERS:\n    IP = %04X, PP = %04X, VP = %04X, "
        "SP = %04X, RP = %04X\n", rt.ip, rt.pp, rt.vp, rt.sp, rt.rsp);
}

void dump_variables(void) {
    puts("VARIABLES:");
    for (addr_t p = rt.vp0; p < rt.vp; p += PEEK(p)) {
        fputc(' ', stderr);
        fputs(RAM + PEEK(p + sizeof(addr_t)), stderr);  // Name.
        addr_t p1 = p + 2*sizeof(addr_t);
        int type = RAM[p1++];
        if (type & VAR_VEC) {
            int d1 = PEEK(p1);
            p1 += sizeof(addr_t);
            fprintf(stderr, "(%i) = |", d1);
            for (int i = 0; i < d1; ++ i) {
                if (i > 2 && i < d1 - 1) {
                    fputs(" ... ", stderr);
                    i = d1 - 2;
                    continue;
                }
                if (type & VAR_NUM) {
                    fprintf(stderr, " %g", PEEK_NUM(p1));
                    p1 += sizeof(num_t);
                } else {
                    fprintf(stderr, " \"%s\"", RAM + p1);
                    p1 += strlen(RAM + p1) + 1;
            }}
            fputs("|\n", stderr);
        } else if (type & VAR_MAT) {
            int d1 = PEEK(p1), d2 = PEEK(p1 + sizeof(addr_t));
            p1 += 2*sizeof(addr_t);
            fprintf(stderr, "(%i,%i) = |", d1, d2);
            for (int i = 0; i < d1; ++ i) {
                if (i > 2 && i < d1 - 1) {
                    fputs(" ... ", stderr);
                    i = d1 - 2;
                } else
                for (int j = 0; j < d2; ++ j) {
                    if (j > 2 && j < d2 - 1) {
                        fputs(" ... ", stderr);
                        j = d2 - 2;
                        continue;
                    }
                    if (type & VAR_NUM) {
                        fprintf(stderr, " %g", PEEK_NUM(p1));
                        p1 += sizeof(num_t);
                    } else {
                        fprintf(stderr, " \"%s\"", RAM + p1);
                        p1 += strlen(RAM + p1) + 1;
                }}
                fputs(" ;", stderr);
            }
            fputs("|\n", stderr);
        } else if (type == VAR_NUM) {
            fprintf(stderr, " = %g\n", PEEK_NUM(p1));
        } else if (type == VAR_FOR) {
            fprintf(stderr, " = %g TO %g STEP %g\n", PEEK_NUM(p1),
                PEEK_NUM(p1 + sizeof(num_t)), PEEK_NUM(p1 + 2*sizeof(num_t)));
        } else if (type == VAR_STR) {
            fprintf(stderr, " = \"%s\"\n", RAM + p1);
        } else {
            fputs(" UNKNOWN!!!\n", stderr);
}}}

/// \}
/** \defgroup STACK Stacks

    Stacks have fixed size, so they grow upward: the parameter stack starts
    at sp0 and ends at sp0 + STACK_SIZE - 1; the stack pointer sp contains the
    address of the first free item in the stack.

    Each stack item is a pair (n,s) where n is a number and s a string address:
    thus each element takes 6 bytes. If s == NIL then the item contains
    a number, else a string. */
/// \{

void pop(num_t *n, str_t *s) {
    if (rt.sp == rt.sp0) ERROR(VALUE);
    rt.sp -= sizeof(num_t);
    *n = PEEK_NUM(rt.sp);
    rt.sp -= sizeof(str_t);
    *s = PEEK(rt.sp);
}

num_t pop_num(void) {
    num_t n;
    str_t s;
    pop(&n, &s);
    if (s != NIL) ERROR(TYPE);
    return n;
}

str_t pop_str(void) {
    num_t n;
    str_t s;
    pop(&n, &s);
    if (s == NIL) ERROR(TYPE);
    return s;
}

void push(num_t n, str_t s) {
    if (rt.sp - rt.sp0 >= STACK_SIZE - sizeof(num_t) - sizeof(str_t))
        ERROR(EXPRESSION_TOO_LONG);
    POKE(rt.sp, s);
    rt.sp += sizeof(str_t);
    POKE_NUM(rt.sp, n);
    rt.sp += sizeof(num_t);
}

#define push_num(n) push(n, NIL)
#define push_str(s) { assert(s != NIL); push(0, s); }

/// Address of the number on top of the stack.
addr_t tos_num(void) {
    if (rt.sp == rt.sp0) ERROR(VALUE);
    return rt.sp - sizeof(num_t);
}

/// Address of the string on top of the stack.
addr_t tos_str(void) {
    if (rt.sp == rt.sp0) ERROR(VALUE);
    return rt.sp - sizeof(num_t) - sizeof(str_t);
}

/** Pop from the return stack the pair (ip0, ip) denoting a position inside
    a program line. */
void rpop(addr_t *ip0, addr_t *ip) {
    if (rt.rsp == rt.rsp0) ERROR(RETURN);
    rt.rsp -= 2*sizeof(addr_t);
    *ip0 = PEEK(rt.rsp);
    *ip = PEEK(rt.rsp + sizeof(addr_t));
}

/** Push on the return stack the pair (ip0, ip) denoting a position inside a
    program line. Offset is stored as a single byte_t*/
void rpush(addr_t ip0, addr_t ip) {
    // An item in the rstack takes 4 (2*sizeof addr) bytes.
    if (rt.rsp - rt.rsp0 >= RSTACK_SIZE - 2*sizeof(addr_t))
        ERROR(TOO_MANY_GOSUB);
    POKE(rt.rsp, ip0);
    POKE(rt.rsp + sizeof(addr_t), ip);
    rt.rsp += 2*sizeof(addr_t);
}

/// \}
/** \defgroup CSTR String Constants Area

    The constant string area, starting at rt.csp0 and ending at rt.pp0 - 1,
    contains all string constants parsed in a program, along with all
    identifiers. It is maintained as a linked list, so to look for something in
    it, it requires linear time :-(

    The first free byte in the string area is pointed by rt.csp, which is also
    the pointer to the first temporary string: temporary strings may be built
    during expression evaluations, while rt.tsp points to the first byte after
    all temporary strings. Since rt.tsp is reset to rt.csp whenever an
    instruction starts to executing, all temporary strings are discarded when
    another expression will be evaluated. */
/// \{

#define STRCMP(s1, s2) (strcmp(RAM + (s1), RAM + (s2)))
#define STREQ(s1, s2) (strcmp(RAM + (s1), RAM + (s2)) == 0)

/** Add a new string to the data area and return its address: if there's no
    more space then return a negative number. */
int cstr_add(char *p0, int len) {
    int k = -1;
    if (rt.csp + len + 1 < rt.pp0) {
        k = rt.csp;
        memcpy(RAM + k, p0, len);
        RAM[k + len] = '\0';
        rt.csp += len + 1;
    }
    return k;
}

/** Add a temporary string to the data area and return its address: if there's
    no more space then return a negative number.
    \param rt runtime object. \param p string to add \param len length of p.
    \return the addr_t of the new temporary string.
    \exception If the string can't be allocated. */
int cstr_add_temp(char *p, int len) {
    if (rt.tsp + len + 1 >= rt.pp0) ERROR(OUT_OF_STRINGS);
    int k = rt.tsp;
    rt.tsp += len + 1;
    memcpy(RAM + k, p, len);
    RAM[k + len] = '\0';
    return k;
}

/** Look for a string in the data area: if found then return its address,
    else a negative number. */
int cstr_find(char *p0, int len) {
    str_t s = rt.csp0;
    while (s < rt.csp) {
        int len1 = strlen(RAM + s);
        if (len1 == len && memcmp(RAM + s, p0, len) == 0) {
            return s;
        }
        s += len1 + 1;
    }
    return -1;
}

/// \}
/** \defgroup OPER Operators Implementation

    Operators retrieve their parameters from the stack, thus the expression is
    implicitly translated into postfix Polish form when evaluated: however, this
    is done by maintaining two stack, the stack at rt.sp0 for data (each item
    being a pair (number, string), that is a number if string is NIL else it is
    a string, thus a pointer to a string in the constant string area), and the
    stack rt.estack for operators. Operators are executed or pushed on the
    stack according to their priorities, expressed in the Operators[] table
    hereinafter. Constants and variables are immediately evaluated and pushed
    on the stack, instead. */
/// \{

//  Auxiliary functions.

/** Pop two values and compare them returning the result as an integer, the same
    as if strcmp or memcmp has been executed. */
int oper_cmp(void) {
    num_t n1, n2;
    str_t s1, s2;
    pop(&n2, &s2);
    pop(&n1, &s1);
    if ((s1 == NIL) ^ (s2 == NIL)) ERROR(TYPE);
    return (s1 == NIL) ? ((n1 > n2) ? 1 : (n1 == n2) ? 0 : -1) : STRCMP(s1, s2);
}

/// Press the empty string on the stack.
void oper_empty_string(void)
{
    // As empty string, use the ending '\0' of the last string in data area.
    // If the string list is empty (almost impossible), push an empty string.
    if (rt.csp == rt.csp0) { RAM[rt.csp++] = '\0'; }
    push_str(rt.csp - 1);
}

/// Create a temporary string concatenating s1 and s2 and return it.
str_t oper_concat(str_t s1, str_t s2) {
    // Create the concatenation as temporary string.
    char *p1 = RAM + s1;
    unsigned l1 = strlen(p1);
    char *p2 = RAM + s2;
    unsigned l2 = strlen(p2);
    int addr = cstr_add_temp(p1, l1 + l2);
    strcpy(RAM + addr + l1, p2);
    return addr;
}

#include <termios.h>
///  Get pressed key: Linux specific!
char oper_inkey(void) {
    static int c1 = -1, c2 = -1;
    int c;
    if (c1 > -1) {
        c = c1;
        c1 = c2;
        c2 = -1;
    } else {
        struct termios prev, curr;
        tcgetattr(0, &prev);
        curr = prev;                        // Init new settings.
        curr.c_lflag &= ~(ICANON | ECHO);   // Change settings.
        tcsetattr(0, TCSANOW, &curr);       // Set up non waiting mode.
        c = getchar();
        if (c == 27) { c1 = getchar(); c2 = getchar(); }
        tcsetattr(0, TCSANOW, &prev);       // Restore waiting mode.
    }
    return c;
}

/// Check against ctrl-c: if pressed, 1 is returned, else 0.
int oper_ctrlc(void) {
    struct termios prev, curr;
    tcgetattr(0, &prev);
    curr = prev;                        // Init new settings.
    curr.c_lflag &= ~(ICANON | ECHO);   // Change settings.
    curr.c_cc[VMIN] = 0;
    curr.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &curr);       // Set new mode.
    int c = getchar();
    tcsetattr(0, TCSANOW, &prev);       // Restore old mode.
    return c;
}


void OPER_ABS(void) { push_num(fabs(pop_num())); }

void OPER_ACS(void) {
    num_t n = pop_num();
    if (n < -1 || n > 1) ERROR(DOMAIN);
    push_num(acos(n));
}

void OPER_AND(void) {
    num_t n2 = pop_num(), n1 = pop_num();
    push_num(n1 && n2);
}

void OPER_ASC(void) { push_num(RAM[pop_str()]); }

void OPER_ASN(void) {
    num_t n = pop_num();
    if (n < -1 || n > 1) ERROR(DOMAIN);
    push_num(asin(n));
}

void OPER_AT(void) {
    // AT(i,j) is used for its side effect to position the cursor at row i and
    // column j; it returns the empty string.
    char *w = getenv("COLUMNS"), *h = getenv("LINES");
    const int WIDTH = w != NULL ? atoi(w) : 80;
    const int HEIGHT = h != NULL ? atoi(h) : 24;
    int col = (int)pop_num() % WIDTH;
    int row = (int)pop_num() % HEIGHT;
    printf("\033[%i;%if", row, col);
    oper_empty_string();
}

void OPER_ATN(void) { push_num(atan(pop_num())); }

void OPER_CHRS(void) {
    char c[2];
    c[0] = pop_num();
    c[1] = '\0';
    push_str(cstr_add_temp(c, 1));
}

void OPER_CONCAT(void) {
    str_t s2 = pop_str(), s1 = pop_str();
    push_str(oper_concat(s1, s2));
}

void OPER_COL(void) {
    char *c = getenv("COLUMNS");
    push_num(c != NULL ? atoi(c) : 80);
}

void OPER_COS(void) { push_num(cos(pop_num())); }

void OPER_DIV(void) {
    num_t n2 = pop_num();
    if (n2 == 0) ERROR(ZERO);
    push_num(pop_num() / n2);
}

void OPER_EOF(void) {
    // EOF(c) = 0 if c is opened and not yet ended, 1 if it is ended.
    int ch = pop_num();
    if (ch < 0 || ch > BUF_NUM) ERROR(ILLEGAL_CHANNEL);
    if (rt.channels[ch] == NULL) ERROR(CHANNEL_CLOSED);
    push_num(feof(rt.channels[ch]));
}

void OPER_EQ(void) { push_num(oper_cmp() == 0); }
void OPER_ERR(void) { push_num(rt.err); }
void OPER_EXP(void) { push_num(exp(pop_num())); }
void OPER_GEQ(void) { push_num(oper_cmp() >= 0); }
void OPER_GT(void) { push_num(oper_cmp() > 0); }

void OPER_IDIV(void) {
    int n2 = floor(pop_num()), n1 = floor(pop_num());
    if (n2 == 0) ERROR(ZERO);
    div_t d = div(n1, n2);
    push_num(d.quot);
}

void OPER_INKEYS(void) {
    push_num(oper_inkey());
    OPER_CHRS();
}

void OPER_INT(void) { push_num(floor(pop_num())); }

void OPER_LEFTS(void) {
    int n2 = pop_num();
    char *s1 = RAM + pop_str();
    if (n2 < 0 || n2 > strlen(s1)) ERROR(SUBSCRIPT_RANGE);
    // Create the substring as temporary string.
    push_str(cstr_add_temp(s1, n2));
}

void OPER_LEN(void) { push_num(strlen(RAM + pop_str())); }
void OPER_LEQ(void)  { push_num(oper_cmp() <= 0); }

void OPER_LOG(void) {
    num_t n = pop_num();
    if (n <= 0) ERROR(DOMAIN);
    push_num(log(n));
}

void OPER_LT(void) { push_num(oper_cmp() < 0); }

void OPER_MIDS(void) {
    int n2 = pop_num();
    int n1 = pop_num() - 1;
    char *s1 = RAM + pop_str();
    if(n2 < 0 || n1 + n2 > strlen(s1) || n1 < 0 || n1 >= strlen(s1))
        ERROR(SUBSCRIPT_RANGE);
    // Create the substring as temporaty string.
    push_str(cstr_add_temp(s1 + n1, n2));
}

void OPER_MINUS(void) {
    num_t n2 = pop_num(), n1 = pop_num();
    push_num(n1 - n2);
}

void OPER_MOD(void) {
    int n2 = floor(pop_num()), n1 = floor(pop_num());
    if (n2 == 0) ERROR(ZERO);
    div_t d = div(n1, n2);
    push_num(d.rem);
}

void OPER_MUL(void) {
    num_t n2 = pop_num(), n1 = pop_num();
    push_num(n1 * n2);
}

void OPER_NEG(void) { push_num(-pop_num()); }
void OPER_NEQ(void) { push_num(oper_cmp() != 0); }
void OPER_NOT(void) { push_num(!pop_num()); }

void OPER_OR(void) {
    num_t n2 = pop_num(), n1 = pop_num();
    push_num(n1 || n2);
}

void OPER_PLUS(void) {
    num_t n1, n2;
    str_t s1, s2;
    pop(&n2, &s2);
    pop(&n1, &s1);
    if (s1 == NIL) {
        if (s2 != NIL) ERROR(TYPE);
        push_num(n1 + n2);
    } else {
        if (s2 == NIL) ERROR(TYPE);
        push_str(oper_concat(s1, s2));
}}

void OPER_POW(void) {
    num_t n2 = pop_num(), n1 = pop_num();
    if (n1 == 0 && n2 <= 0) ERROR(DOMAIN);
    errno = 0;
    n1 = pow(n1, n2);
    if (errno) ERROR(DOMAIN);
    push_num(n1);
}

void OPER_RIGHTS(void) {
    int n2 = pop_num();
    char *s1 = RAM + pop_str();
    if (n2 < 0 || n2 > strlen(s1)) ERROR(SUBSCRIPT_RANGE);
    // Create the substring as temporaty string.
    push_str(cstr_add_temp(s1 + strlen(s1) - n2, n2));
}

void OPER_RND(void) { push_num((double)rand()/RAND_MAX); }

void OPER_ROW(void) {
    char *l = getenv("LINES");
    push_num(l != NULL ? atoi(l) : 24);
}

void OPER_SGN(void) {
    num_t n = pop_num();
    push_num(n > 0 ? 1 : n == 0 ? 0 : -1);
}

void OPER_SIN(void) { push_num(sin(pop_num())); }

void OPER_SQR(void) {
    num_t n = pop_num();
    if (n < 0) ERROR(DOMAIN);
    push_num(sqrt(n));
}

void OPER_STRS(void) {
    char s[32];
    sprintf(s, "%g", pop_num());
    push_str(cstr_add_temp(s, strlen(s)));
}

void OPER_SUBS(void) {
    // SUB$(x$, i, j) = substring of x$ from i-th to j-th character (included).
    int j = pop_num() - 1;
    int i = pop_num() - 1;
    char *s = RAM + pop_str();
    int len = strlen(s);
    // X$(I TO J) is illegal if I < 1 or J > LEN X$; if J < I it is "".
    if (i < 0 || j >= len) ERROR(SUBSCRIPT_RANGE);
    if (j < i) {
        // i points to the last 0 character so an empty string will result.
        i = len;
        j = i + 1;
    }
    // Create the substring as temporaty string.
    push_str(cstr_add_temp(s + i, j - i + 1));
}

void OPER_TAB(void) {
    // TAB(i) is used for its side effect to position the cursor at column j;
    // it returns the empty string.
    char *w = getenv("COLUMNS");
    const int WIDTH = w != NULL ? atoi(w) : 80;
    printf("\033[%iG", (int)pop_num() % WIDTH);
    oper_empty_string();
}

void OPER_TAN(void) { push_num(tan(pop_num())); }
void OPER_TIME(void) { push_num(time(NULL) - rt.t0); }

void OPER_VAL(void) {
    char *p = RAM + pop_str();
    errno = 0;
    num_t n = strtod(p, NULL);
    if (errno) ERROR(ILLEGAL_CONVERSION);
    push_num(n);
}

/** The table of all operators: each operator has a name, an OPER_*** function
    implementing it, an arity value which is the number of parameters of the
    operator plus a flag which is set if the operator is infix binary and a
    priority, used to pop it from the operator stack at the appropriated moment.
    To add a new operator FOO, just add it to the following table, insert the
    corresponding CODE_FOO constant in the constants list and implement the
    corresponding OPER_FOO function. */
const struct {
    char name[8];
    void (*routine)(void);
    byte_t arity;       // # of operator parameters.
    byte_t infix;       // 1 if the operator is binary infix, else 0.
    byte_t priority;    // operator priority.
} Operators[] = {
    // Ordered according to the name field.
#   define O(n,r,a,i,p) {n, OPER_##r, a, i, p},
#   include "straybasic.h"
#   undef O
};

/// \}
/** \defgroup VAR Variables Management

    Variables are stored from vp0 to vp-1 in the RAM in one of the following
    formats, that depend on the type of variable

        size, name, VAR_NUM, n
        size, name, VAR_NUM|VAR_VEC, i, n1, ..., ni
        size, name, VAR_NUM|VAR_MAT, i, j, n11, ..., n1i, ..., nj1, ..., nji
        size, name, VAR_STR, s
        size, name, VAR_STR|VAR_VEC, i, s1, ..., si
        size, name, VAR_STR|VAR_MAT, i, j, s11, ..., s1i, ..., sj1, ..., sji
        size, name, VAR_FOR, n (value), n1 (bound), n2 (step), ip0, ip

    Here size is a 16 bit unsigned, name the address of a string, n, n1, ...
    numbers, s, s1, ... C-strings (NOT addresses!), i and j 16 bit unsigned.
    A VAR_FOR includes the current value, the bound and the step, plus the
    address of the line where NEXT should jump and the offset in the line
    where NEXT should jump. */
/// \{

// Some shortcuts: they retrieve the address/value of a variable's field.
#define VAR_SIZE(v) (PEEK((v)))
#define VAR_NAME(v) ((v) + sizeof(addr_t))
#define VAR_TYPE(v) (RAM[(v) + 2*sizeof(addr_t)])
#define VAR_ADDR(v) ((v) + 2*sizeof(addr_t) + 1)
#define VAR_TO(v) ((v) + 2*sizeof(addr_t) + 1 + sizeof(num_t))
#define VAR_STEP(v) ((v) + 2*sizeof(addr_t) + 1 + 2*sizeof(num_t))

/** Create a new variable at the address rt.vp. According to the provided type,
    which is also returned as value, inserts the data from d1 on, while name and
    type, of course, are always inserted. The pointer rt.csp is updated to the
    first byte following the created variable. */
int var_create(addr_t name, int type, num_t d1, num_t d2,
               num_t step, addr_t ip0, addr_t ip) {
    // Insert the variable: size, name, type, dim1 and possibly dim2 fields.
    addr_t v = rt.vp;
    if (rt.sp0 - v < 2*sizeof(addr_t) + 1 + sizeof(num_t)) goto Error;
    POKE(VAR_NAME(v), name);
    VAR_TYPE(v) = type;
    rt.vp = VAR_ADDR(v);
    if (type == VAR_NUM) { POKE_NUM(rt.vp, 0); rt.vp += sizeof(num_t); }
    else if (type == VAR_STR) RAM[rt.vp++] = '\0';
    else if (type == VAR_FOR) {
        if (rt.sp0 - v < 4*sizeof(addr_t) + 1 + 3*sizeof(num_t)) goto Error;
        // 5 consecutive values to store: value, to, step, ip0, ip.
        POKE_NUM(rt.vp, d1); rt.vp += sizeof(num_t);
        POKE_NUM(rt.vp, d2); rt.vp += sizeof(num_t);
        POKE_NUM(rt.vp, step); rt.vp += sizeof(num_t);
        POKE(rt.vp, ip0); rt.vp += sizeof(addr_t);
        POKE(rt.vp, ip); rt.vp += sizeof(addr_t);
    } else {
        // array
        int numerical = type & VAR_NUM;
        // A string array is initialized by empty strings that take one byte.
        int size = d1 * d2;
        if (numerical) size *= sizeof(num_t);
        if (rt.sp0 - rt.vp < size + 2*sizeof(addr_t) + 1 + 2*sizeof(num_t))
            goto Error;
        POKE(rt.vp, d1); rt.vp += sizeof(addr_t);
        if (type & VAR_MAT) { POKE(rt.vp, d2); rt.vp += sizeof(addr_t); }
        // Allocates the actual array items.
        if (numerical) {
            for (int i = 0; i < d1*d2; ++ i) {
                POKE_NUM(rt.vp, 0);
                rt.vp += sizeof(num_t);
            }
        } else {
            memset(RAM + rt.vp, 0, size);
            rt.vp += size;
    }}
    // Finally writes the size field of the variable.
    POKE(v, rt.vp - v);
    return type;
Error:
    rt.vp = v;
    ERROR(OUT_OF_VARIABLES);
}

/// Looks for a variable with name s and return its address, or NIL.
str_t var_find(str_t s) {
    for (addr_t v = rt.vp0; v < rt.vp; v += VAR_SIZE(v)) {
        if (STREQ(PEEK(VAR_NAME(v)), s)) {
           return v;
    }}
    return NIL;
}

/// Return 1 if the termination condition on the FOR variable is false.
int var_for_check(addr_t v) {
    num_t value = PEEK_NUM(VAR_ADDR(v));
    num_t to = PEEK_NUM(VAR_TO(v));
    return (PEEK_NUM(VAR_STEP(v)) > 0) ? value <= to : to <= value;
}

/** Applies the step to the provided FOR variable and set rt.ip0 and IP
    to the beginning of the loop if the bound has not yet been surpassed. */
void var_for_next(addr_t v) {
    addr_t va = VAR_ADDR(v);
    // Increase the value by the step
    POKE_NUM(va, PEEK_NUM(va) + PEEK_NUM(VAR_STEP(v)));
    // If the bound has not been surpassed, jump to ip.
    if (var_for_check(v)) {
        rt.ip0 = PEEK(va + 3*sizeof(num_t));
        IP = PEEK(va + 3*sizeof(num_t) + sizeof(addr_t));
}}

/** If a scalar variable name (that should be the address of the address
    of the actual string) exists then its value is returned, else a new variable
    is created, of the type inferred from the name and its value is returned.
    An error, if there's no more space for the variable, can be raised. */
addr_t var_insert(addr_t name) {
    int type = RAM[name - 1] == CODE_IDN ? VAR_NUM : VAR_STR;
    str_t s = PEEK(name);
    addr_t v = var_find(s);
    if (v == NIL) {
        v = rt.vp; // The next instruction will create a variable here.
        var_create(s, type, 0, 0, 0, 0, 0);
    }
    return v;
}

/** Parse an array variable, thus "identifier(dim1, dim2)" and return all those
    data into: *name (address into the constant string area), *d1, *d2, while
    the type of the variable is returned as function value. Parse from IP which
    is updated to the token following che closed parenthesis. */
int var_array_parse(addr_t *name, addr_t *d1, addr_t *d2) {
    extern num_t expr_num(void);
    int type = (CODE == CODE_IDN ? VAR_NUM : VAR_STR);
    *name = PEEK(IP + 1);       // Store the name address.
    IP += 1 + sizeof(addr_t);   // Skip the name address.
    // Parse the subscript (s).
    EXPECT('(', SUBSCRIPT);
    *d1 = expr_num();
    if (*d1 == 0) ERROR(SUBSCRIPT_RANGE);
    if (CODE == ',') {
        ++ IP;
        *d2 = expr_num();
        if (*d1 == 0) ERROR(SUBSCRIPT_RANGE);
        type |= VAR_MAT;
    } else {
        *d2 = 1;
        type |= VAR_VEC;
    }
    EXPECT(')', OPENPAR_WITHOUT_CLOSEPAR);
    return type;
}

/** Parse the subscript s of a vector or matrix variable: type is the actual
    type of the variable, p is assumed to point to the variable's i (first
    dimension) field, the resulting address of the variable's item will be
    stored into a1 and its scalar type returned. */
int var_array_address(int type, addr_t p, addr_t *a1) {
    extern num_t expr_num(void);
    int d2 = 1, j = 1;
    int is_mat = type & VAR_MAT;
    int d1 = PEEK(p); p += sizeof(addr_t);
    if (is_mat) { d2 = PEEK(p); p += sizeof(addr_t); }
    // Parse the subscript (s).
    EXPECT('(', SUBSCRIPT);
    int i = expr_num();
    if (i < 1 || i > d1) ERROR(SUBSCRIPT_RANGE);
    if (is_mat) {
        EXPECT(',', SUBSCRIPT);
        j = expr_num();
        if (j < 1 || j > d2) ERROR(SUBSCRIPT_RANGE);
    }
    EXPECT(')', OPENPAR_WITHOUT_CLOSEPAR);
    // Now p points to the first item in the vector or matrix.
    // According to the type, compute the address *a1 of the item.
    type &= VAR_NUM | VAR_STR;
    if (type == VAR_NUM) {
        *a1 = p + ((i-1)*d2 + j-1)*sizeof(num_t);
    } else {
        int n = (i-1)*d2 + (j-1);   // Number of strings to skip.
        // Skip n strings.
        while (n-- > 0) {
            // To skip a string skip its contents + the final 0
            p += strlen(RAM + p) + 1;
        }
        *a1 = p;
    }
    return type;
}

/** Parse a variable, whose name (the CODE_IDN(S)) is pointed by IP and whose
    address in the variable's list is v, returning in *va the address of the
    value pointed by the variable expression (e.g. a$(1) points to the
    first string of the a$ array). On returning, IP points to the next token and
    the return value is the type of the variable. If a syntax error occurs, then
    an exception is raised. */
int var_address(addr_t v, addr_t *va) {
    IP += 1 + sizeof(addr_t);           // Skip the variable's name
    int type = VAR_TYPE(v);
    if (type & (VAR_VEC|VAR_MAT)) {
        type = var_array_address(type, VAR_ADDR(v), va);
    } else {
        *va = VAR_ADDR(v);              // Scalar or FOR variable: return it!
    }
    return type;
}

/// \}
/** \defgroup EXPR Expression Evaluator

    An expression is evaluated by pushing operands on the stack and operators on
    the runtime estack. Whenever a new operator is parsed, if its priority is
    less than the priority of the operator on top of the estack, the latter is
    executed. Else the operator is pushed on the estack, too.

    When the expression has been completely parsed, all remaining operators on
    the estack are popped and executed.

    Prefixed operators and binary operators are dealt with separately. */
/// \{

extern void expr(void);

/// Parse an expression, retrieve the result as a number and return it.
num_t expr_num(void) {
    expr();
    return pop_num();
}

/// Parse an expression, retrieve the result as a string and return it.
str_t expr_str(void) {
    expr();
    return pop_str();
}

/** Compile a sequence of prefix (unary) operators, if any. If the compilation
    ends in an operand on the stack, 0 is returned, else 1. */
int expr_prefix_operators(void) {
Again:; // Tail recursion goto target.
    // Here CODE_MINUS it is infix but should be converted to CODE_NEG.
    int code = (CODE == CODE_MINUS ? CODE_NEG : CODE) - CODE_STARTOPERATOR - 1;
    // Check against a built-in function or non infix operator.
    if (code >= 0 && CODE < CODE_ENDOPERATOR && !Operators[code].infix) {
        ++ IP;  // Skip the operator.
        // Execute operators on the stack with higher priority
        rt_epop(Operators[code].priority);
        // Push the operator on the stack.
        rt_epush(Operators[code].routine, Operators[code].priority);
        // Parse and push on the stack the function parameters.
        int arity = Operators[code].arity;
        // Unary operators are followed by an operand, e.g. LEN X$.
        if (arity == 1) goto Again; // tail recursion.
        // 0-ary operators need no parameter.
        if (arity > 0) {
            // Parentheses always needed for arity > 1.
            EXPECT('(', OPENEDPAR);
            for (int i = 1; i < arity; ++ i) {
                expr();
                EXPECT(',', COMMA);
            }
            expr();
            EXPECT(')', CLOSEDPAR);
        }
        // In any case we pushed an operand on the stack.
        return 0;
    }
    // Here no operand has been pushed on the stack.
    return 1;
}

/** Try to parse an operand from IP: if it fails an error is raised. */
void expr_operand(void) {
    switch (CODE) {
    case '(': {         // Subexpression.
        ++ IP;
        expr();
        EXPECT(')', CLOSEDPAR);
        break;
    } case CODE_IDN: case CODE_IDNS: {  // Variable or function.
        extern int fn_eval(str_t);
        int code = CODE;
        str_t name = PEEK(IP + 1);
        addr_t v = var_find(name);
        if (v == NIL) { if (!fn_eval(name)) ERROR(UNDEFINED_VARIABLE); }
        else {
            addr_t va;
            var_address(v, &va);
            if (code == CODE_IDN) {
                push_num(PEEK_NUM(va));
            } else {
                // Make a temporary copy of string va.
                push_str(cstr_add_temp(RAM + va, strlen(RAM + va)));
        }}
        break;
    } case CODE_INTLIT: {
        push_num(PEEK(IP + 1));
        IP += 1 + sizeof(addr_t);
        break;
    } case CODE_NUMLIT: {
        push_num(PEEK_NUM(IP + 1));
        IP += 1 + sizeof(num_t);
        break;
    } case CODE_STRLIT: {
        push_str(PEEK(IP + 1));
        IP += 1 + sizeof(str_t);
        break;
    } default:
        ERROR(VALUE);
    }
}

/// Used after an operand to deal with a possible string subscript.
void expr_str_subscript(void) {
    // x$(i TO j) is the same as SUB$(x$, i, j)
    // x$(i TO) is the same as x$(i TO LEN x$)
    // x$(TO j) is the same as x$(1 TO j)
    // x$(i) is the same as x$(i TO i)
    // Compute or use the default for both i and j
    ++ IP;
    if (CODE == CODE_TO) {  // x$(TO j).
        push_num(1);    // Missing i set to i by default.
        ++ IP;              // Skip "TO".
        expr();           // Push j.
    } else {
        int len = strlen(RAM + PEEK(tos_str()));
        expr();           // Push i
        if (CODE == ')') {  // x$(i)
            // x$(i) = x$(i TO i), so duplicate number on top of stack.
            push_num(PEEK_NUM(tos_num()));
        } else {
            EXPECT(CODE_TO, TO_EXPECTED);
            // x$(i TO j) or x$(i TO)
            if (CODE != ')') expr(); else push_num(len);
    }}
    EXPECT(')', OPENPAR_WITHOUT_CLOSEPAR);
    rt_epush(OPER_SUBS,
        Operators[CODE_SUBS - CODE_STARTOPERATOR - 1].priority);
}

/** Evaluate an expression at rt.ip and leave on the stack the value. */
void expr(void) {
    /*  Push a fake 0-priority operator on the operators stack to avoid
        to accidentally use operators already on the stack before executing
        the current expression (since expressions can be nested). */
    rt_epush(NULL, 0);
Again:
    // If no operand is produced by expr_prefix_operators, produce it!
    if (expr_prefix_operators())
        expr_operand();
    // A string on the top of stack and a "(" means a string subscript.
    if (CODE == '(' && PEEK(tos_str()) != NIL)
        expr_str_subscript();
    // Check against an infix operator followed by another operand etc.
    int code = CODE - CODE_STARTOPERATOR - 1;
    if (code >= 0 && CODE < CODE_ENDOPERATOR && Operators[code].infix) {
        rt_epop(Operators[code].priority);
        rt_epush(Operators[code].routine, Operators[code].priority);
        ++ IP;          // Skip the operator.
        goto Again;     // Repeat to evaluate the second operand.
    }
    // Parsed operators may be on the rt.estack: unroll them.
    rt_epop(1); // Not 0, else the NULL function is invoked!!!
    // Drop the fake NULL operator.
    if (rt.estack_next == 0) ERROR(SYNTAX);
    -- rt.estack_next;
}

/// \}
/** \defgroup ASSIGN Assignments Implementation

    To assign a number value use poke_num!!! */
/// \{

/** Assign to the string variable v the string s: the address of the string to
    overwrite is expected in va, while v contains the first byte of the variable
    in the variable's list (its size field). */
void assign_string(addr_t v, str_t va, str_t s) {
    // va points to the first character of the string to overwrite with s.
    unsigned len_v = strlen(RAM + va) + 1;
    unsigned len_s = strlen(RAM + s) + 1;
    int delta = len_s - len_v;  // Shall increase the length of v by delta.
    if (delta >= rt.sp0 - rt.vp) ERROR(OUT_OF_VARIABLES);
    if (delta != 0) {
        // Reduce/Augment the space for the string.
        memmove(RAM + va + len_s, RAM + va + len_v, rt.vp - (va + len_v));
        // Adjust the pointer to the first free byte in the variable area.
        rt.vp += delta;
        // Adjust the size of the variable containing the string.
        POKE(v, PEEK(v) + delta);
    }
    // Finally copy string s on string va.
    strcpy(RAM + va, RAM + s);
}

/** Parse "= expr" and assign the value to the variable of given type, at
    address v and whose value is at address va. */
void assign_expr(int type, addr_t v, addr_t va) {
    addr_t token_dump(addr_t, FILE*);
    if (type == VAR_NONE) ERROR(UNDEFINED_VARIABLE);
    EXPECT(CODE_EQ, ASSIGNMENT);
    expr();
    if (type & (VAR_NUM|VAR_FOR)) {
        POKE_NUM(va, pop_num());
    } else {
        assert(type & VAR_STR);
        assign_string(v, va, pop_str());
}}

/** Scan buffer b matching a constant (number, comma ending string or string
    delimited by double quotes) to the variable at IP: if no error occurs,
    assigns to the parsed variable the a value scanned from b. The updated
    pointer to the buffer is returned, pointing to ',' or '\0': if neither ','
    nor '\0' is found after the constant, NIL is returned. */
addr_t assign_item(addr_t b) {
    // We'll use C string standard functions.
    byte_t *p = RAM + b;
    byte_t *p1;
    addr_t v = var_insert(IP + 1);
    addr_t va;
    int type = var_address(v, &va);
    p += strspn(p, " \t\r\f\n");    // skip blanks
    if (type & (VAR_NUM|VAR_FOR)) {
        p1 = p;
        num_t n = strtod((char*)p1, (char**)&p);
        if (p1 == p) ERROR(ILLEGAL_INPUT);
        POKE_NUM(va, n);
    } else if (*p == '"') {
        // String between double quotes.
        ++ p;
        if ((p1 = strchr(p, '"')) == NULL) ERROR(EOL_INSIDE_STRING);
        // assign_string expects a C-string.
        *p1 = '\0'; assign_string(v, va, p - RAM); *p1 = '"';
        p = p1 + 1;
    } else {
        // String ending with the line or the next comma.
        if ((p1 = strchr(p, ',')) == NULL) {
            assign_string(v, va, p - RAM);
            p += strlen(p);
        } else {
            // Transform the ',' into '\0' to get a C-string.
            *p1 = '\0'; assign_string(v, va, p - RAM); *p1 = ',';
            p = p1;
    }}
    p += strspn(p, " \t\r\f\n");    // skip blanks
    return (*p == ',' || *p == '\0') ? p - RAM : NIL;
}

/// \}
/// \defgroup TOKEN Lexical Analyzer
/// \{

/// Print a token at address a on file f: return the address of next token.
addr_t token_dump(addr_t a, FILE *f) {
    static int space = 0;   // 1 if a space should be printed in advance.
    byte_t b = RAM[a++];
    if (b == CODE_IDN || b == CODE_IDNS) {
        if (space) fputc(' ', f);
        fprintf(f, "%s", RAM + PEEK(a));
        a += 2;
        space = 0;
    } else if (b == CODE_INTLIT) {
        if (space) fputc(' ', f);
        fprintf(f, "%i", PEEK(a));
        a += 2;
        space = 0;
    } else if (b == CODE_NUMLIT) {
        if (space) fputc(' ', f);
        fprintf(f, "%g", PEEK_NUM(a));
        a += sizeof(num_t);
        space = 0;
    } else if (b == CODE_STRLIT) {
        if (space) fputc(' ', f);
        fprintf(f, "\"%s\"", RAM + PEEK(a));
        a += 2;
        space = 0;
    } else if (b == '\'') {
        if (space) fputc(' ', f);
        fputs(RAM + a - 1, f);
        a += strlen(RAM + a);   // points to the ending '\0'
        space = 0;
    } else if (b > CODE_STARTOPERATOR && b < CODE_ENDOPERATOR) {
        fprintf(f, " %s", Operators[b - CODE_STARTOPERATOR - 1].name);
        space = 1;
    } else if (b > CODE_STARTKEYWORD && b < CODE_ENDKEYWORD) {
        if (f == stderr) fputs("\033[1m", stderr);  // Bold blue
        fprintf(f, " %s", Keywords[b - CODE_STARTKEYWORD - 1]);
        if (f == stderr) fputs("\033[22m", stderr);  // Not bold
        if (b == CODE_DATA || b == CODE_REM) {
            fputs(RAM + a, f);
            a += strlen(RAM + a);   // points to the ending '\0'
            space = 0;
        } else {
            space = 1;
        }
    } else if (b != 0) {
        if (b == '(' || b == ')') {
            fputc(b, f);
            space = 0;
        } else if (b == ',' || b == ';' || b == ':') {
            fputc(b, f);
            space = 1;
        } else {
            fputc(b, f);
            space = 0;
    }} else {
        space = 0;
    }
    return a;
}

/** Try to match a keyword with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding keyword code is returned, else 0. */
int token_keyword(char *p0, unsigned len) {
    for (int i = 0; i < CODE_ENDKEYWORD - CODE_STARTKEYWORD - 1; ++ i) {
        int cmp = memcmp(Keywords[i], p0, len);
        if (cmp == 0 && len == strlen(Keywords[i])) {
            return (CODE_STARTKEYWORD + 1) + i;
        }
        if (cmp > 0) break;
    }
    return 0;
}

/** Try to match an operator with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding operator code is returned, else 0. */
int token_operator(char *p0, unsigned len) {
    // In the following loop, j is an alias for i - (CODE_STARTOPERATOR + 1).
    for (int i = CODE_STARTOPERATOR + 1, j = 0; i < CODE_ENDOPERATOR; ++ i, ++ j) {
        int cmp = memcmp(Operators[j].name, p0, len);
        if (cmp == 0 && Operators[j].name[len] == '\0') return i;
        if (cmp > 0) break;
    }
    return 0;
}

///  Tokenize the contents of buf[0] and store it at obj: return 0 on error.
int tokenize(void) {
    int k, len;
    // Encode the buf[0] string at obj.
    byte_t *p = RAM + rt.buf[0];
    byte_t *q0 = RAM + rt.obj; // The line size will be stored here.
    byte_t *q = q0 + 1;         // The line will be stored here.
    byte_t b;
    while (*p != '\0') {
        p += strspn(p, " \t");  // Skip blanks.
        if (*p < 32) ++ p;      // Skip non printable
        else if (*p > 127) {
            printf("SKIP INVALID ASCII CODE %i\n", *p);
            ++ p;
        } else if (isdigit(*p) || *p == '.' && isdigit(p[1])) {
            char *p1, *p2;
            int i = strtol(p, &p1, 10);
            num_t n = strtod(p, &p2);
            if (p1 == p2 && i >= -32768 && i < 32768) {
                *q = CODE_INTLIT;
                poke(q + 1, i);
                q += 3;
                p = p1;
            } else {
                *q = CODE_NUMLIT;
                poke_num(q + 1, n);
                q += 1 + sizeof(num_t);
                p = p2;
        }} else if (isalpha(*p)) {
            // Scan the string and create a new constant.
            int code = CODE_IDN;    // A priori numerical identifier.
            byte_t *p0 = p;         // First character of parsed identifier.
            byte_t *q1 = q;         // Where to copy the uppercase identifier.
            // Scan the identifier and copy in uppercase it at q.
            do *q1++ = toupper(*p++); while (isalnum(*p));
            if (*p == '$') {
                code = CODE_IDNS;   // String identifier.
                *q1 = '$';
                ++ p;
            }
            len = p - p0;
            // Check against operator (such as ABS, STR$ etc.).
            if ((k = token_operator(q, len)) > 0) *q++ = k;
            // Check against keyword (such as PRINT, THEN, etc.).
            else if ((k = token_keyword(q, len)) > 0) {
                // Deal with keywords, some prevent line from being tokenized.
                if ((*q++ = k) == CODE_DATA || k == CODE_REM) {
                    len = strlen(p) + 1;
                    memcpy(q, p, len);
                    q += len - 1;
                    p += len;
            }} else {
                // Identifier: parse it and, if it is not yet in the string
                // constants list, add it.
                if ((k = cstr_find(q, len)) < 0
                && (k = cstr_add(q, len)) < 0) {
                    puts(Errors[ERROR_OUT_OF_STRINGS]);
                    return 0;
                }
                // An identifier is stored as 3 bytes: code, address in constant
                // list.
                *q = code;
                poke(q + 1, k);
                q += 1 + sizeof(str_t);
        }} else if (*p == '\'') {    // Comment.
            /*  A comment is compiled as '\'' followed by the string
                with the comment text. */
            len = strlen(p) + 1;
            strcpy(q, p);
            q += len - 1;
            p += len;
        } else if (*p == '"') {
            len = strcspn(++p, "\"");
            if (p[len] != '"') {
                puts(Errors[ERROR_EOL_INSIDE_STRING]);
                return 0;
            }
            if ((k = cstr_find(p, len)) < 0
            && (k = cstr_add(p, len)) < 0) {
                puts(Errors[ERROR_OUT_OF_STRINGS]);
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

/// If a points to a token, return the address of the token following it.
addr_t token_skip(addr_t a) {
    byte_t b = RAM[a];
    // Numbers and strings are encoded by several bytes.
    if (b == CODE_IDN || b == CODE_IDNS || b == CODE_INTLIT || b == CODE_STRLIT) {
        a += sizeof(addr_t) + 1;
    } else if (b == CODE_NUMLIT) {
        a += sizeof(num_t) + 1;
    } else if (b == '\'' || b == CODE_DATA || b == CODE_REM) {
        // These instructions span through the end of the line.
        // Thus, ta + strlen(RAM + ta) points to the final '\0'.
        a += strlen(RAM + a);   // skip to final 0.
    } else if (b != 0) {
        ++ a;
    }
    return a;
}

/// \}
/**
    \defgroup PROG Program Editing

    A program line is stored as a sequence [s,i1,i2,c1,...,cn] of bytes, where
    s = 3 + n, i1 + 256*i2 is the line number, c1,...,cn are the bytecodes of
    the line. */
/// \{

extern int instr_exec(void);

//  Some useful macros: p is the first byte of a program line.
//  A program line starts with s, CODE_INTLIT, n1, n2, ...
//  where byte s = line size in bytes, n1 + n2*256 = line number.
#define LINE_NUM(p) (p + 2)         ///< Address containing 16 bit line number.
#define LINE_NEXT(p) (p + RAM[p])   ///< Address of line following p.
#define LINE_SIZE(p) (RAM[p])       ///< Length of the line.
#define LINE_TEXT(p) (p + 2 + sizeof(addr_t))   ///< Address of first token.

/** Set IP to the first token of the current line, that starts at rt.ip0.
    If rt.ip0 points after the last line of the program, IP is set to NIL.
    In any case, IP is returned. */
#define LINE_START (IP = (rt.ip0 >= rt.pp) ? NIL : LINE_TEXT(rt.ip0))

/** If there are unsaved changes in the current program, ask the user if they
    could be discarded: return 1 if there are no changes or the user agree to
    discard them, else return 0. */
int prog_check(void) {
    int ok = 1;
    if (rt.prog_changed) {
        fputs("\nUNSAVED CHANGES IN CURRENT PROGRAM: DISCARD THEM (Y/N)? ",
            stderr);
        char c[2];
        fgets(c, sizeof(c), stdin);
        if (toupper(*c) != 'Y') ok = 0;
    }
    return ok;
}

/** Delete the line with number n: return 1 if the line doesn't exist, else 0. */
int prog_delete(int n) {
    for (addr_t a = rt.pp0; a < rt.pp; a += LINE_SIZE(a)) {
        addr_t n1 = PEEK(LINE_NUM(a));
        if (n1 > n) break;
        if (n1 == n) {
            // Overwrite the line by the following ones and the variables.
            int size = LINE_SIZE(a);
            memmove(RAM + a, RAM + a + size, rt.vp0 - (a + size));
            // Adjust pointers.
            rt.pp -= size;
            rt.prog_changed = 1;
            return 0;
    }}
    return 1;
}

/// Edit a line at ram[line] starting with a line number.
void prog_edit(addr_t line) {
    extern void prog_insert(int n, addr_t a);
    // line = size; line+1 = CODE_INTLIT; line+2/+3 = integer; line+4 = line contents.
    int line_no = PEEK(LINE_NUM(line));
    if (line_no < LINE_MIN || line_no > LINE_MAX) {
        rt.error = ERROR_ILLEGAL_LINE_NUMBER;
    } else {
        if (RAM[LINE_TEXT(line)] == 0) {
            if (prog_delete(line_no))
                printf("LINE %i DOES NOT EXIST!\n", line_no);
        } else {
            // Overwrite (= delete + insert) the line.
            prog_delete(line_no);
            prog_insert(line_no, line);
}}}

/// Execute the current program until an error occurs.
void prog_exec(void) {
    rt.ip0 = rt.pp0;
    LINE_START;
    srand(0);   // makes RND deterministic by default.
    //~ while (rt.ip0 < rt.pp && !instr_exec())
    while (IP != NIL && !instr_exec())
        ;
    if (IP != NIL) puts("instr_exec() FAILED!");
}

/** Looks for a line with line number n: if found then its address is returned,
    else an error is raised. */
addr_t prog_find(int n) {
    for (addr_t a = rt.pp0; a < rt.pp; a += LINE_SIZE(a)) {
        addr_t n1 = PEEK(LINE_NUM(a));
        if (n1 > n) break;
        if (n1 == n) return a;
    }
    ERROR(ILLEGAL_LINE_NUMBER);
}

/** Insert line a with line number n (assume no such line exists). */
void prog_insert(int n, addr_t a) {
    // Get in p the address of the first line with number > n.
    addr_t p = rt.pp0;
    while (p < rt.pp && PEEK(LINE_NUM(p)) < n)
        p += LINE_SIZE(p);
    assert(p == rt.pp || PEEK(LINE_NUM(p)) != n);
    
    int size_new = LINE_SIZE(a);
    if (rt.pp + size_new >= rt.vp0) rt.error = ERROR_PROGRAM_TOO_LONG;
    else {
        /* Makes room for the new line, shifting on the right both the program
            and the variables. Next, copy the new line from a. */
        memmove(RAM + p + size_new, RAM + p, rt.pp - p);
        memmove(RAM + p, RAM + a, size_new);
        // Adjust pointers.
        rt.pp += size_new;
        rt.prog_changed = 1;
}}

/// Load a program with the given file name and return the value of rt.error.
int prog_load(const char *name) {
    extern int prog_repl(FILE *f);
    FILE *f = fopen(name, "r");
    if (f == NULL) rt.error = ERROR_FILE;
    else {
        prog_repl(f);
        fclose(f);
        rt.prog_changed = 0;
    }
    return rt.error;
}

/** Print the program on a file: if the file is stdout, some frills are used to
    list the instruction, namely bold keywords, italic operators, etc. */
void prog_print(FILE *f) {
    addr_t p = rt.pp0;
    while (p < rt.pp) {
        fprintf(f, "%4i", PEEK(LINE_NUM(p)));
        p = LINE_TEXT(p);
        while (RAM[p] != 0)
            p = token_dump(p, f);
        fputc('\n', f);
        ++ p;
}}

/** Keep scanning lines from a file and interpreting them either as insertion
    commands (if they starts with a line number) or instructions to be executed
    immediately. The return value is the rt.error variable which, if not zero,
    means that en error occurred during the REPL. */
int prog_repl(FILE *f) {
    while (!feof(f)) {
        rt.ip0 = rt.obj;    // In case rt_ctrlbreak is called!
        if (f == stdin) putchar('>');
        if (fgets(RAM + rt.buf[0], BUF_SIZE, f) == NULL) break;
        // Drop the final '\n' from the string.
        char *p = strchr(RAM + rt.buf[0], '\n');
        if (p != NULL) *p = '\0';
        tokenize();
        // RAM[rt.obj] is reserved to contain the line size.
        // If the token at rt.obj + 1 is a line number, then edit the line.
        if (RAM[rt.obj + 1] == CODE_INTLIT) prog_edit(rt.obj);
        else {
            IP = rt.obj + 1;    // First token of the line after the size byte.
            while (IP != NIL && CODE != 0 && !instr_exec())
                ;
    }}
    return rt.error;
}

/** Saves the current program on a file whose name is provided: the name is
    case sensitive. */
void prog_save(char *name) {
    FILE *f = fopen(name, "w");
    if (f == NULL) rt.error = ERROR_FILE;
    else {
        prog_print(f);
        fclose(f);
        rt.prog_changed = 0;
}}

/// \}
/// \defgroup INSTR Instruction Implementation
/// \{

/** Parse a possible channel and return its number. FILE file0 is assigned
    to the 0 channel if #0 is parsed. The channel is checked to be busy, else
    an error is issued. */
int instr_channel(FILE *file0) {
    int ch = 0; // a priori buffer #0
    rt.channels[ch] = file0;
    if (CODE == '#') {
        ++ IP;
        ch = expr_num();     // Parse the channel number.
        if (rt.channels[ch] == NULL) ERROR(CHANNEL_CLOSED);
        EXPECT(',', COMMA);
    }
    return ch;
}

/** Advance IP to the first token of the next instruction: if needed, IP is
    advanced to the first instruction of the next line, if any. The updated
    value of IP is returned, unless no further instruction is available, in
    which case NIL is returned. */
addr_t instr_skip(void) {
    /*  If RAM[ip] == 0 then the current line is ended, so ip+1 points to
        the next one; if RAM[ip] == ':' then ip+1 points to the next
        instruction. */
    while (CODE != 0) {
        if (CODE == ':' || CODE == CODE_THEN) return ++IP;
        IP = token_skip(IP);
    }
    // Here we are at the end of the line.
    rt.ip0 = IP + 1;
    return LINE_START;
}

/// Advance IP to the first token of the next program's line.
addr_t instr_skip_line(void) {
    rt.ip0 += RAM[rt.ip0];    // Skip to the next line.
    return LINE_START;
}

/** Starting from the pointer IP, looks for the instruction whose CODE is
    code and returns the updated value of IP to the address of the token
    following it, or NIL if no such instruction exists. */
addr_t instr_lookfor(byte_t code) {
    do if (CODE == code) return ++IP; while (instr_skip() != NIL);
    return NIL;
}

// Used by GOTO and ON
void instr_goto(int line) {
    rt.ip0 = prog_find(line);
    LINE_START;
}

// Used by GOSUB and ON
void instr_gosub(int line) {
    rpush(rt.ip0, IP);
    instr_goto(line);
}

void INSTR_ATTR(void) {
    // ATTR property = value, ...
    // where property can be: BOLD, UNDER, BACK, FORE, BRIGHT, BLINK, REVERSE
    // values can be BOLD, UNDER, BRIGHT, BLINK = 0|1; BACK, FORE = 0,...,7
    for (;;) {
        EXPECT(CODE_IDN, ILLEGAL_ATTRIBUTE);
        char *property = RAM + PEEK(IP);
        IP += sizeof(str_t);
        unsigned value = 1;
        if (CODE == CODE_EQ) {
            ++ IP;
            value = expr_num();
        }
        // What follows depends on the compliancy of the terminal with ECMA-48.
        if (strcmp(property, "BACK") == 0)
            printf("\033[48;2;%i;%i;%im", 255*(value&4), 255*(value&2),
                255*(value&1));
        else if (strcmp(property, "BLINK") == 0)
            printf("\033[%im", 25 - 20*(value & 1));
        else if (strcmp(property, "BOLD") == 0)
            printf("\033[%im", 22 - 21*(value & 1));
        else if (strcmp(property, "BRIGHT") == 0)
            printf("\033[%im", 2 + 20*(value & 1));
        else if (strcmp(property, "FORE") == 0)
            printf("\033[38;2;%i;%i;%im", 255*(value&4), 255*(value&2),
                255*(value&1));
        else if (strcmp(property, "RESET") == 0)
            fputs("\033[0m\033[38;2;0;255;0m\033[48;2;0;0;0m", stdout);
        else if (strcmp(property, "REVERSE") == 0)
            printf("\033[%im", 27 - 20*(value & 1));
        else if (strcmp(property, "UNDER") == 0)
            printf("\033[%im", 24 - 20*(value & 1));
        else {
            ERROR(ILLEGAL_ATTRIBUTE);
        }
        fflush(stdout);
        if (CODE != ',') break;
        ++ IP;
}}

void INSTR_BYE(void) {
    if (prog_check()) {
        puts("BYE.");
        exit(EXIT_SUCCESS);
}}

void INSTR_CHAIN(void) {
    // CHAIN filename[,linenumber]
    str_t name = expr_str();
    int line = LINE_MIN - 1;
    if (CODE == ',') {
        ++ IP;
        line = expr_num();
    }
    rt_reset(RT_RESET_PROG);
    if (prog_load(RAM + name)) ERROR(FILE);
    // Get to the first line to execute.
    if (line >= LINE_MIN) instr_goto(line); else prog_exec();
}

void INSTR_CLEAR(void) {
    // CLEAR [[s][,p]]
    // s = space for strings, p = space for program (in byte).
    // Get current values both for s and p.
    int s0 = rt.pp0, s = s0;   // s0 - 0 = current space for strings.
    int p0 = rt.vp0 - rt.pp0, p = p0;
    // In any case, delete all variables.
    rt.vp = rt.vp0;
    if (CODE == CODE_INTLIT) {
        s = PEEK(IP + 1);
        IP += 1 + sizeof(addr_t);
    }
    if (CODE == ',') {
        ++ IP;
        EXPECT(CODE_INTLIT, VALUE);
        p = PEEK(IP);
        IP += sizeof(addr_t);
    }
    if (s != s0 || p != p0) {
        rt.pp0 = rt.pp = s;
        rt.vp0 = rt.vp = s + p;
        IP = NIL;        
}}

void INSTR_CLOSE(void) {
    // CLOSE channel
    int ch = expr_num();
    if (ch < 1 || ch > BUF_NUM) ERROR(ILLEGAL_CHANNEL);
    if (rt.channels[ch] == NULL) ERROR(CHANNEL_CLOSED);
    fclose(rt.channels[ch]);
    rt.channels[ch] = NULL;
}

void INSTR_CLS(void) { fputs("\033[2J\033[1;1f", stderr); }
void INSTR_DATA(void) { instr_skip_line(); }
void INSTR_DEF(void) { instr_skip_line(); }

void INSTR_DIM(void) {
    for (;;) {
        if (CODE != CODE_IDN && CODE != CODE_IDNS) ERROR(IDENTIFIER);
        str_t name = PEEK(IP + 1);
        if (var_find(name) != NIL) ERROR(VARIABLE_ALREADY_DEFINED);
        addr_t d1, d2;
        int type = var_array_parse(&name, &d1, &d2);
        var_create(name, type, d1, d2, 0, 0, 0);
        if (CODE != ',') break;
        ++ IP;
}}

void INSTR_DUMP(void) {
    fputs("KEYWORDS:\n   ", stdout);
    for (int i = 0; i < sizeof(Keywords)/sizeof(*Keywords); ++ i)
        printf(" %s", Keywords[i]);
    fputs("\nOPERATORS:\n   ", stdout);
    for (int i = 0; i < sizeof(Operators)/sizeof(*Operators); ++ i)
        printf(" %s", Operators[i].name);
    putchar('\n');
    dump_memory();
    dump_cstr();
    dump_variables();
    dump_channels();
}

void INSTR_END(void) { IP = NIL; }
void INSTR_ERROR(void) { longjmp(rt.err_buffer, rt.error = expr_num()); }

void INSTR_FOR(void) {
    // FOR creates its variable if not already defined by another FOR statement.
    EXPECT(CODE_IDN, NUMVAR);
    str_t name = PEEK(IP);
    addr_t v = var_find(name);
    if (v != NIL) {
        if (VAR_TYPE(v) != VAR_FOR) ERROR(FORVAR);
    } else {    // Create the variable if it doesn't exist.
        v = rt.vp;     // The variable will be inserted here.
        // Set value = bound = 0, step = 1, ip0 = ip = 0.
        var_create(name, VAR_FOR, 0, 0, 1, 0, 0);
    }
    IP += sizeof(str_t);    // Skip the variable's name.
    // Assign the initial value
    addr_t va = VAR_ADDR(v);
    EXPECT(CODE_EQ, ASSIGNMENT);
    expr();
    POKE_NUM(va, pop_num());
    // Parse the TO bound
    EXPECT(CODE_TO, TO_EXPECTED);
    POKE_NUM(va + sizeof(num_t), expr_num());
    // Possibly, parse the STEP
    if (CODE == CODE_STEP) {
        ++ IP;
        POKE_NUM(va + 2*sizeof(num_t), expr_num());
    } else {
        POKE_NUM(va + 2*sizeof(num_t), 1);
    }
    /* A VAR_FOR variable contains also the reference of the line and
        the instruction where NEXT should jump. */
    POKE(va + 3*sizeof(num_t), rt.ip0);
    POKE(va + 3*sizeof(num_t) + sizeof(addr_t), rt.ip);
    if (!var_for_check(v)) {
        /*  If the FOR condition is initially false so we skip the loop: looks
            for the matching "NEXT var" instruction and skip after it. If the
            FOR-NEXTs don't respect nestings, cross your fingers... */
        do {
            if (instr_lookfor(CODE_NEXT) == NIL) ERROR(FOR_WITHOUT_NEXT);
            EXPECT(CODE_IDN, NUMVAR);
        } while (PEEK(VAR_NAME(v)) != PEEK(IP));
        IP += sizeof(str_t);    // Skip the variable's name after NEXT.
}}

void INSTR_GOSUB(void) { instr_gosub(expr_num()); }
void INSTR_GOTO(void) { instr_goto(expr_num()); }

void INSTR_IF(void) {
    // IF expr: ...
    // IF expr THEN line
    // IF expr THEN ...
    if (expr_num() == 0) instr_skip_line();
    else if (CODE == CODE_THEN) {
        if (RAM[IP + 1] == CODE_INTLIT) {
            ++ IP;
            instr_goto(expr_num());
}}}

void INSTR_INPUT(void) {
    int ch = instr_channel(stdin);
    if (ch == 0) {
        // A constant string may be printed at this point.
        if (CODE == CODE_STRLIT) {
            fputs(RAM + PEEK(IP + 1), stderr);
            IP += 1 + sizeof(str_t);
            if (CODE != ',' && CODE != ';') ERROR(SYNTAX);
            ++ IP;
        }
        putchar('?');
    }
    addr_t b = rt.buf[ch];
    if (!fgets(RAM + b, BUF_SIZE, rt.channels[ch])) ERROR(ILLEGAL_INPUT);
    // Drop the ending '\n' if any.
    char *p = strchr(RAM + b, '\n');
    if (p != NULL) *p = '\0';
    // Parse a list of variables and assign values read from buffer b to them.
    for (;;) {
        b = assign_item(b);
        if (b == NIL) ERROR(ILLEGAL_INPUT);
        if (CODE != ',') break;
        ++ IP;
        if (RAM[b] != ',') ERROR(ILLEGAL_INPUT);
        ++ b;
}}

void INSTR_LET(void) {
    // LET v=e, ..., v=e
    for (;;) {
        if (CODE != CODE_IDN && CODE != CODE_IDNS) ERROR(IDENTIFIER);        
        // In case of a new definition, save the variable limit rt.vp.
        addr_t vp_saved = rt.vp;
        addr_t v = var_insert(IP + 1);
        rt.vp = vp_saved;   // Hide the new variable, if any!
        addr_t va;
        int type = var_address(v, &va);
        if (type == VAR_NONE) ERROR(UNDEFINED_VARIABLE);
        EXPECT(CODE_EQ, ASSIGNMENT);
        expr();
        // If v is a new variable, make it available before assigning it.
        if (rt.vp == v) rt.vp += VAR_SIZE(v);
        // Finally, perform the assignment.
        if (type & VAR_STR)
            assign_string(v, va, pop_str());
        else
            POKE_NUM(va, pop_num());   // VAR_NUM | VAR_FOR
        if (CODE != ',') break;
        ++ IP;
}}

void INSTR_LINPUT(void) {
    int ch = instr_channel(stdin);
    // A constant string may be printed at this point.
    if (ch == 0 && CODE == CODE_STRLIT) {
        fputs(RAM + PEEK(IP + 1), stderr);
        IP += 1 + sizeof(str_t);
        if (CODE != ',' && CODE != ';') ERROR(SYNTAX);
        ++ IP;
    }
    // Create or retrieve the string variable to be read.
    addr_t v = var_insert(IP + 1);
    addr_t va;
    int type = var_address(v, &va);
    if (!(type & VAR_STR)) ERROR(STRVAR);
    addr_t b = rt.buf[ch];
    RAM[b] = '\0';  // A priori empty.
    fgets(RAM + b, BUF_SIZE, rt.channels[ch]);
    // Drop the ending '\n' if any.
    char *p = strchr(RAM + b, '\n'); if (p != NULL) *p = '\0';
    // Assign to variable v the value of the string b.
    assign_string(v, va, b);
}

void INSTR_LIST(void) { prog_print(stderr); }

void INSTR_LOAD(void) {
    if (prog_check()) {
        extern void INSTR_MERGE(void);
        rt_reset(RT_RESET_ALL);
        INSTR_MERGE();
    } else {
        expr_str();  // skip the program name.
}}

void INSTR_MERGE(void) {
    if (prog_load(RAM + expr_str())) longjmp(rt.err_buffer, rt.error);
    rt.prog_changed = 0;
    IP = NIL;
}

void INSTR_NEW(void) {
    if (prog_check())
        rt_reset(RT_RESET_ALL);
}

void INSTR_NEXT(void) {
    EXPECT(CODE_IDN, NUMVAR);
    addr_t v = var_find(PEEK(IP));
    IP += sizeof(str_t);
    if (v == NIL) ERROR(UNDEFINED_VARIABLE);
    if (VAR_TYPE(v) != VAR_FOR) ERROR(FORVAR);
    /* Applies the step to the provided FOR variable and set rt.ip0 and IP to
        the beginning of the loop if the bound has not yet been surpassed. */
    addr_t va = VAR_ADDR(v);
    // Increase the value by the step
    POKE_NUM(va, PEEK_NUM(va) + PEEK_NUM(VAR_STEP(v)));
    // If the bound has not been surpassed, jump to ip.
    if (var_for_check(v)) {
        rt.ip0 = PEEK(va + 3*sizeof(num_t));
        IP = PEEK(va + 3*sizeof(num_t) + sizeof(addr_t));
}}

void INSTR_ON(void) {
    // ON ERROR i   ' GOSUB i will be issued when an error will occur.
    // ON expr GOSUB i1, ..., in
    // ON expr GOTO i1, ..., in
    if (CODE == CODE_ERROR) {
        ++ IP;
        EXPECT(CODE_INTLIT, ILLEGAL_LINE_NUMBER);
        rt.on_error = (PEEK(IP) == 0) ? NIL : prog_find(PEEK(IP));
        IP += sizeof(addr_t);
    } else {
        int n = expr_num();
        if (n < 1) ERROR(ON);
        int code = CODE;    // Saves the code for later.
        // Parse the integer list, storing in nth the n-th element.
        int i = 1, nth = -1;
        do {
            ++ IP;
            if (i == n) nth = PEEK(IP + 1);
            IP += 1 + sizeof(addr_t);
            ++ i;
        } while (CODE == ',');
        // The next integer is the line number where to jump.
        if (nth == -1) ERROR(ON);
        push_num(nth);
        if (code == CODE_GOTO) instr_goto(nth);
        else if (code == CODE_GOSUB) instr_gosub(nth);
        else ERROR(SYNTAX);
}}

void INSTR_OPEN(void) {
    // OPEN channel, name, mode
    // channel and mode are integers, name is a string.
    int ch = expr_num();
    if (ch < 1 || ch > BUF_NUM) ERROR(ILLEGAL_CHANNEL);
    if (rt.channels[ch] != NULL) ERROR(CHANNEL_BUSY);
    EXPECT(',', COMMA);
    str_t name = expr_str();
    EXPECT(',', COMMA);
    int mode = expr_num();
    if (mode < 0 || mode > 2) ERROR(ILLEGAL_MODE);
    rt.channels[ch] = fopen(RAM + name, mode == 0 ? "r" : mode == 1 ? "w" : "a");
    if (rt.channels[ch] == NULL) ERROR(FILE);
}

void INSTR_PRINT(void) {
    int ch = instr_channel(stdout);
    FILE *f = rt.channels[ch];
    int newline = 1;    // True if a newline has to be eventually printed.
    while (CODE != 0 && CODE != ':' && CODE != '\'') {
        // A print-list is a sequence of commas/semi-colons possibly repeated
        // and expressions: the latter should be separated by at least a comma
        // or by a semi-colon.
        if (CODE == ',') {
            if (ch == 0) {
                // Try to print a tab on a Linux terminal.
                struct termios prev, curr;
                tcgetattr(0, &prev);
                curr = prev;                        // Init new settings.
                curr.c_lflag &= ~(ICANON | ECHO);   // No Echo, no buffer.
                tcsetattr(0, TCSANOW, &curr);       // Set up non waiting mode.
                int i, j;
                // Get the cursor position.
                fputs("\033[6n", stdout);
                // The terminal should answer "\033[j;iR", being i the column.
                if (scanf("\033[%i;%iR", &j, &i) != 2) {
                    // Failed! Use a standard tabulation.
                    fputs("\t\t", stdout);
                } else {
                    i += 16 - i % 16;   // Tabulation interval = 16 positions.
                    char *w = getenv("COLUMNS");
                    const int WIDTH = w != NULL ? atoi(w) : 80;
                    if (i < WIDTH) printf("\033[%iG", i); else putchar('\n');
                }
                tcsetattr(0, TCSANOW, &prev);       // Restore waiting mode.
            } else {
                fputs("\t", f);
            }
            ++ IP;
            newline = 0;
        } else if (CODE == ';') {
            ++ IP;
            newline = 0;
        } else {
            num_t n;
            str_t s;
            expr();
            pop(&n, &s);
            if (s == NIL) fprintf(f, "%g", n); else fputs(RAM + s, f);
            newline = 1;
    }}
    if (newline) fputc('\n', f);
    fflush(f);
}

void INSTR_RANDOMIZE(void) { srand(time(NULL) % RAND_MAX); }

void INSTR_READ(void) {
    /*  rt.data_next points to an instruction keyword, and in this case we
        check against a DATA, else find the next DATA instruction if any, or
        points to ',' or '\0' inside a DATA line. */
    for (;;) {
        if (RAM[rt.data_next] == CODE_DATA || RAM[rt.data_next] == ',') {
            ++ rt.data_next;   // Skip DATA or ','.
        } else {
            // Save current line pointers, instr_lookfor changes them!
            addr_t ip0_saved = rt.ip0, ip_saved = IP;
            IP = rt.data_next;
            rt.data_next = instr_lookfor(CODE_DATA);
            // Restore line pointers.
            rt.ip0 = ip0_saved, IP = ip_saved;
            if (rt.data_next == NIL) ERROR(OUT_OF_DATA);
        }
        rt.data_next = assign_item(rt.data_next);
        if (rt.data_next == NIL) ERROR(DATA);
        if (CODE != ',') break;
        ++ IP;  // Skip ','.
}}

void INSTR_REM(void) { instr_skip_line(); }

void INSTR_REPEAT(void) {
    if (rt.ip0 >= rt.pp) ERROR(ILLEGAL_OUTSIDE_PROGRAM);
    LINE_START;
}

void INSTR_RESTORE(void) {
    if (CODE == 0 || CODE == ':' || CODE == '\'') {
        rt.data_next = LINE_TEXT(rt.pp0);
    } else {
        rt.data_next = LINE_TEXT(prog_find(expr_num()));
    }
}

void INSTR_RETURN(void) { rpop(&rt.ip0, &IP); }

void INSTR_RUN(void) {
    rt_reset(RT_RESET_VARS);
    prog_exec();
}

void INSTR_SAVE(void) { prog_save(RAM + expr_str()); }

void INSTR_SKIP(void) {
    if (rt.ip0 >= rt.pp) ERROR(ILLEGAL_OUTSIDE_PROGRAM);
    instr_skip_line();
    instr_skip_line();
}

void INSTR_STEP(void) { ERROR(ILLEGAL_INSTRUCTION); }
void INSTR_STOP(void) { ERROR(STOP); }
void INSTR_SYS(void) { system(RAM + expr_str()); }
void INSTR_THEN(void) { ERROR(ILLEGAL_INSTRUCTION); }
void INSTR_TO(void) { ERROR(ILLEGAL_INSTRUCTION); }
void INSTR_TRACE(void) { rt.trace = expr_num(); }

/** Execute the instruction at IP, advancing it to the first token of
    the next instruction, even in case of error. The value of rt.error is
    returned, in any case. */
int instr_exec(void) {
    /// Table of all routines implementing instructions.
    static void (*Instructions[])(void) = {
#       define I(label) &INSTR_##label,
#       include "straybasic.h"
    };
    // Save the current exception buffer: instr_exec may eventually recurse.
    jmp_buf error_saved;
    memcpy(error_saved, rt.err_buffer, sizeof(jmp_buf));
Again:
    rt_reset(0);    // Reset only volatile data (stacks, etc.).
    if (setjmp(rt.err_buffer) == 0) {
        volatile byte_t opcode;
        // Skip possible instruction separators.
        while ((opcode = CODE) == ':' || opcode == CODE_THEN) ++ IP;
        // Trace statement execution if required.
        if (rt.trace) {
            fprintf(stderr, "\nEXECUTE % 4i ", PEEK(LINE_NUM(rt.ip0)));
            for (addr_t p = IP; RAM[p] != 0; p = token_dump(p, stderr))
                ;
            fputc('\n', stderr);
        }
        if (opcode > CODE_STARTKEYWORD && opcode < CODE_ENDKEYWORD) {
            // Skip the keyword and execute the corresponding INSTR_ routine.
            ++ IP; (*Instructions[opcode - CODE_STARTKEYWORD - 1])();
        } else {
            // Instruction of the form "var = expr".
            if (opcode != CODE_IDN && opcode != CODE_IDNS)
                ERROR(ILLEGAL_INSTRUCTION);
            INSTR_LET();
        }
        // If END has been reached, IP == NIL.
        if (IP != NIL) {
            /*  Here IP points to the first byte after the instruction, so a
                comment or an instruction delimiter should be parsed here. */
            if (CODE == '\'') {
                // Skip comment
                rt.ip0 = IP + strlen(RAM + IP) + 1;
                instr_skip();
            }
            // IP should point to a delimiter between instructions or to the
            // first token of a line (in case a jump occurred).
            if (CODE == 0) instr_skip();
            else if (LINE_TEXT(rt.ip0) != IP && CODE != ':' && CODE != CODE_THEN)
                ERROR(SYNTAX);
    }} else {
        // Check against the last ON ERROR statement, if any.
        if (rt.on_error == NIL) {
            // Default error handling: print a message and stop.
            int line = PEEK(LINE_NUM(rt.ip0));
            if (line >= LINE_MIN && line <= LINE_MAX && rt.ip0 < rt.pp)
                fprintf(stderr, "LINE %i: ", line);
            if (rt.error > 0 && rt.error <= ERROR_ZERO) {
                puts(Errors[rt.error]);
            } else if (rt.error != 0)
                printf("ERROR #%i\n", rt.error);
            IP = NIL;   // Definitely stops program execution.
            rt_reset(RT_RESET_FILES);
        } else {
            // User defined error handling: jump to the ON ERROR line.
            rt.ip0 = rt.on_error;
            LINE_START;
            // Reset the error condition (to avoid infinite loop), but before
            // saves the error code into err.
            rt.err = rt.error;
            rt.error = 0;
    }}
    // Restore exception buffer.
    memcpy(rt.err_buffer, error_saved, sizeof(jmp_buf));
    return rt.error;
}

/// \}
/// \defgroup FN User Defined Function Evaluation
/// \{

/** Looks for a user defined function with the provided name: if not found, then
    return 0, else 1. IP is assumed to point to the CODE_STRLIT byte of the
    function's name, so that, if the function is found, the actual parameter list
    is parsed from IP + sizeof(str_t) + 1 and matched to the formal parameter
    list in the DEF instruction. If the match suceeds then the expression in
    the DEF definition is executed and the value pushed on the stack. */
int fn_eval(str_t name) {
    int ok = 0;     // a priori not found.
    // We'll use instr_lookfor that alter pointers to the line under execution.
    addr_t ip0_saved = rt.ip0, ip_saved = IP;
    // Start looking for DEF FN from the very first program line.
    rt.ip0 = rt.pp0;
    LINE_START;
    while ((instr_lookfor(CODE_DEF)) != NIL) {
        // Check the name of the function
        if (CODE != CODE_IDN && CODE != CODE_IDNS) ERROR(IDENTIFIER);
        if (STREQ(PEEK(IP + 1), name)) {
            // Found! We swap IP and ip_saved since expr needs to be applied
            // to the actual parameters, so IP should point to them.
            addr_t tmp = IP; IP = ip_saved; ip_saved = tmp;
            // Skip the function name both in the call and in the DEF.
            IP += 1 + sizeof(str_t);
            ip_saved += 1 + sizeof(str_t);
            if (CODE == '(') {
                if (RAM[ip_saved] != '(') ERROR(OPENEDPAR);
                ++ IP;          // Skip '(' in the actual parameter list.
                ++ ip_saved;    // Skip '(' in the formal parameter list.
                /* Parse actual and formal parameter lists and assign values
                    to the formal parameters. */
                for (;;) {
                    // Parse the type of the formal parameter.
                    if (RAM[ip_saved] == CODE_IDN) {  // Number.
                        addr_t v = var_insert(ip_saved + 1);
                        POKE_NUM(VAR_ADDR(v), expr_num());
                    } else
                    if (RAM[ip_saved] == CODE_IDNS) { // String.
                        addr_t v = var_insert(ip_saved + 1);
                        assign_string(v, VAR_ADDR(v), expr_str());
                    } else {
                        ERROR(IDENTIFIER);
                    }
                    ip_saved += 1 + sizeof(str_t);  // Skip the parsed name.
                    if (RAM[ip_saved] != ',') break;
                    ++ ip_saved;
                    // Also in the actual parameter list we expect a comma.
                    EXPECT(',', COMMA);
                }
                EXPECT(')', CLOSEDPAR);
                // We also expect ')' to close the actual parameter list.
                if (RAM[ip_saved++] != ')') ERROR(CLOSEDPAR);
            }
            // Swap again IP and ip_saved so that IP points again to the
            // formal parameters list.
            tmp = IP; IP = ip_saved; ip_saved = tmp;
            // Now evaluate the function's body.
            EXPECT(CODE_EQ, ASSIGNMENT);
            expr();
            // Final swap to restore IP after the actual parameters list.
            IP = ip_saved;
            ok = 1;
            break;
    }}
    // Done! We restore line pointers before return.
    rt.ip0 = ip0_saved;
    IP = ip_saved;
    return ok;
}

/// \}
/// \defgroup MAIN Main Program
/// \{

int main(int npar, char **pars) {
    rt_init();
    if (npar > 2) {
        puts("USAGE: straybasic [file.bas]");
        return EXIT_FAILURE;
    }
    // green foreground, black background.
    fputs("\033[38;2;0;255;0m\033[48;2;0;0;0m", stdout);
    fputs("\033[2J\033[1;1f", stdout);   // cls, home.
    if (npar == 1) {
        puts("//== ====== ||==\\    =  \\\\  // ||==\\    =    //== ||  //=\\");
        puts("\\\\     ||   ||__/   / \\  \\\\//  ||__/   / \\   \\\\   || ||");
        puts("  \\\\   ||   ||\\\\   //_\\\\  ||   ||  \\  //_\\\\    \\\\ || ||");
        puts("==//   ||   || \\\\ //   \\\\ ||   ||==/ //   \\\\ ==// ||  \\\\=/");
        puts("");
        puts("(c) 2024 by Paolo Caressa");
        puts("[Type BYE to quit]\n");
        fflush(stdout);
        if (prog_repl(stdin)) puts(Errors[rt.error]);
    } else {
        for (int i = 1; i < npar; ++ i) {
            if (prog_load(pars[i])) {
                puts(Errors[rt.error]);
            } else {
                rt.prog_changed = 0;
                INSTR_RUN();
    }}}
    return EXIT_SUCCESS;
}

/// \}