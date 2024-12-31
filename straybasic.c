/// \file straybasic.c
/// \author Paolo Caressa <github.com/pcaressa>
/// \date 20241231
/// \todo Multiple line DEF FNs.
/// \todo Calling external programs.
/// \todo MAT instructions
/// \todo More functions, such as SPACE$(n), TIME(n) etc.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/// \defgroup CONSTANTS Global Constants
/// \{

#define BUF_SIZE (256)
#define CSTR_SIZE (4096)
#define ESTACK_SIZE (20)
#define PROG_SIZE (8192)
#define RAM_SIZE (65536)
#define RSTACK_SIZE (60)
#define STACK_SIZE (120)

/** Token codes: keyword and operator codes are in the same ordering as the
    corresponding items in the OPERATORS and INSTRUCTION_CODE tables are. */
enum {
    CODE_INTLIT = 128, CODE_NUMLIT, CODE_STRLIT, CODE_IDN, CODE_IDNS,
    CODE_STARTKEYWORD,
        CODE_ATTR, CODE_BYE, CODE_CHAIN, CODE_CLOSE, CODE_CLS, CODE_CONTINUE,
        CODE_DATA, CODE_DEF,
        CODE_DIM, CODE_DUMPMEM, CODE_DUMPVAR, CODE_END, CODE_ERROR, CODE_FOR,
        CODE_GO, CODE_GOSUB, CODE_GOTO, CODE_IF, CODE_INPUT, CODE_LET,
        CODE_LINPUT, CODE_LIST, CODE_LOAD, CODE_MERGE, CODE_NEW, CODE_NEXT,
        CODE_ON, CODE_OPEN, CODE_PRINT, CODE_READ, CODE_REM, CODE_REPEAT,
        CODE_RESTORE, CODE_RETURN, CODE_RUN, CODE_SAVE, CODE_SKIP, CODE_STEP,
        CODE_STOP, CODE_SYS, CODE_THEN, CODE_TO, CODE_TRACE,
    CODE_ENDKEYWORD,
    CODE_STARTOPERATOR,
        CODE_MUL, CODE_PLUS, CODE_MINUS, CODE_NEG, CODE_DIV, CODE_LT,
        CODE_LEQ, CODE_NEQ, CODE_EQ, CODE_GT, CODE_GEQ, CODE_ABS, CODE_ACS,
        CODE_AND, CODE_ASC, CODE_ASN, CODE_AT, CODE_ATN, CODE_CHRS, CODE_COS,
        CODE_EXP, CODE_INKEYS, CODE_INT,
        CODE_LEFTS, CODE_LEN, CODE_LOG, CODE_MIDS, CODE_NOT, CODE_OR,
        CODE_RIGHTS, CODE_RND, CODE_SIN, CODE_SQR, CODE_STRS, CODE_SUBS,
        CODE_TAB, CODE_TAN, CODE_VAL, CODE_POW,
    CODE_ENDOPERATOR,
};

/** Keywords, in the same ordering as the constant between CODE_STARTKEYWORD and
    CODE_ENDKEYWORD and items in INSTRUCTION_CODE are. */
const char *KEYWORDS[] = {
    /*  Same ordering as the constant between CODE_STARTKEYWORD and
        CODE_ENDKEYWORD */
    "ATTR", "BYE", "CHAIN", "CLOSE", "CLS", "CONTINUE", "DATA", "DEF", "DIM",
    "DUMPMEM", "DUMPVAR",
    "END", "ERROR", "FOR", "GO", "GOSUB", "GOTO", "IF", "INPUT", "LET",
    "LINPUT", "LIST", "LOAD", "MERGE", "NEW", "NEXT", "ON", "OPEN",
    "PRINT", "READ", "REM", "REPEAT", "RESTORE", "RETURN", "RUN", "SAVE",
    "SKIP", "STEP", "STOP", "SYS", "THEN", "TO", "TRACE",
};

/// Error codes: same ordering as the ERROR costants.
enum {
    ERROR_NONE, ERROR_FILE, ERROR_CHANNEL_BUSY,
    ERROR_CHANNEL_CLOSED, ERROR_CLOSEDPAR,
    ERROR_CLOSEPAR_WITHOUT_OPENPAR, ERROR_COMMA,
    ERROR_DOMAIN, ERROR_EOL_INSIDE_STRING,
    ERROR_EQ_EXPECTED, ERROR_FORVAR, ERROR_FOR_WITHOUT_NEXT,
    ERROR_HASH_EXPECTED, ERROR_IDENTIFIER_EXPECTED, ERROR_ILLEGAL_ATTRIBUTE,
    ERROR_ILLEGAL_CHANNEL, ERROR_ILLEGAL_CONVERSION, ERROR_ILLEGAL_INPUT,
    ERROR_ILLEGAL_INSTRUCTION, ERROR_ILLEGAL_INSTRUCTION_OUTSIDE_PROGRAM,
    ERROR_ILLEGAL_LINE_NUMBER, ERROR_ILLEGAL_MODE, ERROR_NUMVAR, ERROR_ON,
    ERROR_OPENEDPAR, ERROR_OPENPAR_WITHOUT_CLOSEPAR, ERROR_OUT_OF_DATA,
    ERROR_OUT_OF_STRING_MEMORY, ERROR_OUT_OF_VARIABLE_MEMORY,
    ERROR_PROGRAM_STOPPED, ERROR_PROGRAM_TOO_LONG, ERROR_RETURN,
    ERROR_STACK_OVERFLOW, ERROR_STACK_UNDERFLOW, ERROR_STRING_VARIABLE_EXPECTED,
    ERROR_SUBSCRIPT_ERROR, ERROR_SUBSCRIPT_EXPECTED, ERROR_SYNTAX, ERROR_TO_EXPECTED,
    ERROR_TOO_MANY_GOSUB, ERROR_TYPE, ERROR_UNDEFINED_VARIABLE,
    ERROR_VARIABLE_ALREADY_DEFINED,
    ERROR_ZERO
};

/// Error messages corresponding to error codes.
const char *ERRORS[] = {
    "READY", "CANNOT OPEN FILE", "CHANNEL BUSY", "CHANNEL CLOSED",
    "\")\" EXPECTED", "\")\" WITH    \"(\"", "\",\" EXPECTED",
    "DOMAIN ERROR", "END OF LINE INSIDE STRING",
    "\"=\" EXPECTED", "FOR INDEX EXPECTED", "FOR WITHOUT NEXT",
    "\"#\" EXPECTED", "IDENTIFIER EXPECTED", "ILLEGAL ATTRIBUTE",
    "ILLEGAL CHANNEL", "ILLEGAL CONVERSION", "ILLEGAL INPUT",
    "ILLEGAL INSTRUCTION", "ILLEGAL INSTRUCTION OUTSIDE PROGRAM",
    "ILLEGAL LINE NUMBER", "ILLEGAL MODE", "NUMERICAL VARIABLE EXPECTED",
    "ON VALUE OUT OF LIST",
    "\"(\" EXPECTED", "\"(\" WITHOUT \")\"", "OUT OF DATA",
    "NO MORE ROOM FOR STRINGS", "NO MORE ROOM FOR VARIABLES", "PROGRAM STOPPED",
    "PROGRAM TOO LONG", "RETURN WITHOUT GOSUB", "EXPRESSION TOO LONG",
    "MISSING VALUE", "STRING VARIABLE EXPECTED", "SUBSCRIPT OUT OF RANGE",
    "MISSING SUBSCRIPT", "SYNTAX ERROR", "\"TO\" EXPECTED",
    "TOO MANY NESTED GOSUBS", "TYPE MISMATCH", "UNDEFINED VARIABLE",
    "VARIABLE ALREADY DEFINED",
    "DIVISION BY ZERO"
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

/** Class whose objects contain all data needed to store an instance of a
    virtual machine where Straybasic programs can be edited and run. */
typedef struct runtime_s {
    byte_t ram[RAM_SIZE];
    /*  RAM[csp0:csp] contains string constants;
        RAM[csp:tsp] contains temporary string constants;
        RAM[pp0:pp] contains the program;
        RAM[vp0:vp] contains the variables list;
        RAM[sp0:sp] contains the parameters stack;
        RAM[rsp0:rsp] contains the return stack;    */
    addr_t csp0, csp, tsp, pp0, pp, vp0, vp, sp0, sp, rsp0, rsp;
    
    //  Object code and file buffers: buf0 is reserved to the terminal.
    addr_t obj, buf0, buf1, buf2, buf3, buf4;

    addr_t ip0;         ///< First byte of the current line (its "size byte).
    addr_t ip;          ///< First byte of next token in the current line.
    addr_t ipn;         ///< First byte of the next line (needed by GOTO etc.).
    addr_t data_next;   ///< Address of item in a DATA statement to READ from.

    jmp_buf err_buffer; ///< Exception handler.

    int error;          ///< Current error code: 0 means "no error".
    addr_t on_error;    ///< If != NIL, instruction where to jump on error.
    int prog_changed;   ///< 1 if the program was not saved since last changes.
    int trace;          ///< 1 if runnig the program lines are printed.

    /** Operator stack: contains operator with priorities lower than the one
        under execution, inside an expression. */
    struct {
        void (*routine)(struct runtime_s*);
        int priority;
    } estack[ESTACK_SIZE];
    int estack_next;    ///< 0 it means "stack empty".

    /** I/O files: channels[0] is always stdin or stdout, while channels[i]
        from i = 1 to 4 are NULL or handles to opened text files. */
    FILE *channels[5];

} *runtime_t;

//  Common shortcuts: assume the runtime_t variable rt to be defined.
#define IP (rt->ip)
#define RAM (rt->ram)
#define CODE (RAM[IP])
#define ERROR(e) longjmp(rt->err_buffer, rt->error = (ERROR_##e))
#define EXPECT(tok, msg) if (RAM[IP++] != tok) ERROR(msg)

/** Pop an operator from the estack and return its routine. */
void *runtime_epop(runtime_t rt) {
    if (rt->estack_next == 0) ERROR(SYNTAX);
    -- rt->estack_next;
    return rt->estack[rt->estack_next].routine;
}

/** Push the routine address r and relative priority p on the estack: while
    on top of it there are operators with higher priorities, execute them. */
void runtime_epush(runtime_t rt, void (*r)(runtime_t), int p) {
    if (rt->estack_next >= ESTACK_SIZE) ERROR(STACK_OVERFLOW);
    rt->estack[rt->estack_next].routine = r;
    rt->estack[rt->estack_next].priority = p;
    ++ rt->estack_next;
}

/** Pop and execute all routines on the rt->estack with priority > p. */
void runtime_eunroll(runtime_t rt, int p) {
    while (rt->estack_next > 0 && rt->estack[rt->estack_next - 1].priority > p)
        (*rt->estack[--rt->estack_next].routine)(rt);
}

/** Initialize a virtual ram. */
void runtime_init(runtime_t rt) {
    // Memory area limits, always set
    rt->csp0 = 0;
    rt->pp0 = rt->csp0 + CSTR_SIZE;
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
    rt->on_error = NIL;
    rt->trace = 0;

    // Reset data pointer: points to the first token of the first line.
    rt->data_next = rt->pp0 + 2 + sizeof(addr_t);

    // Drop variables.
    rt->vp = rt->vp0;
    
    // Drop data and program.
    rt->tsp = rt->csp = rt->csp0;
    rt->pp = rt->pp0;
    rt->prog_changed = 0;
    for (int i = 0; i < 5; ++ i)
        rt->channels[i] = NULL;
}

/** Reset the virtual machine: if restart == 1 restart the machine, else
    preserves current program and data. Return the runtime. */
runtime_t runtime_reset(runtime_t rt, int restart) {
    // Drop stacks.
    rt->rsp = rt->rsp0;
    rt->sp = rt->sp0;
    rt->estack_next = 0;
    rt->error = 0;
    rt->on_error = NIL;
    //~ rt->trace = 0;

    // Reset data pointer: points to the first token of the first line.
    rt->data_next = rt->pp0 + 2 + sizeof(addr_t);

    // Drop variables.
    rt->vp = rt->vp0;
    
    if (restart) {
        // Also drop data and program.
        rt->tsp = rt->csp = rt->csp0;
        rt->pp = rt->pp0;
        rt->prog_changed = 0;
        for (int i = 1; i < 5; ++ i) {
            if (rt->channels[i] != NULL) {
                fclose(rt->channels[i]);
                rt->channels[i] = NULL;
    }}}
    return rt;
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

void dump_data(runtime_t rt) {
    puts("DATA");
    for (addr_t p = rt->csp0; p < rt->csp; p += strlen(rt->ram + p) + 1)
        printf(" \"%s\"", rt->ram + p);
    putchar('\n');
    puts("TEMP DATA");
    for (addr_t p = rt->csp; p < rt->tsp; p += strlen(rt->ram + p) + 1)
        printf(" \"%s\"", rt->ram + p);
    putchar('\n');
}

void dump_estack(runtime_t rt) {
    fputs("ESTACK: ", stderr);
    for (int i = 0; i< rt->estack_next; ++ i)
        printf(" [%p,%i]", rt->estack[i].routine, rt->estack[i].priority);
    putchar('\n');
}

addr_t dump_line(runtime_t rt, addr_t a) {
    addr_t a1 = a + RAM[a];
    printf("%04x:", a);
    for (; a < a1; ++ a)
        printf(" %i", RAM[a]);
    putchar('\n');
    return a1;
}

/// Dump the memory status.
void dump_memory(runtime_t rt) {
    puts("MEMORY OCCUPATION STATUS:");
    printf("\tSTRINGS   = % 5i (% 2i%%)\n", rt->csp - rt->csp0, (rt->csp - rt->csp0) / (rt->pp0 - rt->csp0));
    printf("\tPROGRAM   = % 5i (% 2i%%)\n", rt->pp - rt->pp0, (rt->pp - rt->pp0) / (rt->vp0 - rt->pp0));
    printf("\tVARIABLES = % 5i (% 2i%%)\n", rt->vp - rt->vp0, (rt->vp - rt->vp0) / (rt->sp - rt->vp0));
    printf("\tSTACK     = % 5i (% 2i%%)\n", rt->sp - rt->sp0, (rt->sp - rt->sp0) / STACK_SIZE);
    printf("\tRSTACK    = % 5i (% 2i%%)\n", rt->rsp - rt->rsp0, (rt->rsp - rt->rsp0) / RSTACK_SIZE);
}

void dump_program(runtime_t rt) {
    printf("PROGRAM: PP0 = %p, PP = %p\n", RAM + rt->pp0, RAM + rt->pp);
    for (addr_t a = rt->pp0; a < rt->pp; a = dump_line(rt, a))
        ;
}

void dump_variables(runtime_t rt) {
    puts("VARIABLES:");
    for (addr_t p = rt->vp0; p < rt->vp; p += PEEK(p)) {
        fputs(RAM + PEEK(p + sizeof(addr_t)), stderr);  // Name.
        addr_t p1 = p + 2*sizeof(addr_t);
        int type = RAM[p1++];
        if (type & VAR_VEC) {
            printf("(%i)\n", PEEK(p1));
        } else if (type & VAR_MAT) {
            printf("(%i,%i)\n", PEEK(p1), PEEK(p1 + sizeof(addr_t)));
        } else if (type == VAR_NUM) {
            printf(" = %g\n", PEEK_NUM(p1));
        } else if (type == VAR_FOR) {
            printf(" = %g TO %g STEP %g\n", PEEK_NUM(p1),
                PEEK_NUM(p1 + sizeof(num_t)), PEEK_NUM(p1 + 2*sizeof(num_t)));
        } else if (type == VAR_STR) {
            printf(" = \"%s\"\n", RAM + p1);
        } else {
            puts(" UNKNOWN!!!");
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

void pop(runtime_t rt, num_t *n, str_t *s) {
    if (rt->sp == rt->sp0) ERROR(STACK_UNDERFLOW);
    rt->sp -= sizeof(num_t);
    *n = PEEK_NUM(rt->sp);
    rt->sp -= sizeof(str_t);
    *s = PEEK(rt->sp);
}

num_t pop_num(runtime_t rt) {
    num_t n;
    str_t s;
    pop(rt, &n, &s);
    if (s != NIL) ERROR(TYPE);
    return n;
}

str_t pop_str(runtime_t rt) {
    num_t n;
    str_t s;
    pop(rt, &n, &s);
    if (s == NIL) ERROR(TYPE);
    return s;
}

void push(runtime_t rt, num_t n, str_t s) {
    if (rt->sp - rt->sp0 >= STACK_SIZE - sizeof(num_t) - sizeof(str_t))
        ERROR(STACK_OVERFLOW);
    POKE(rt->sp, s);
    rt->sp += sizeof(str_t);
    POKE_NUM(rt->sp, n);
    rt->sp += sizeof(num_t);
}

#define push_num(rt, n) push(rt, n, NIL)
#define push_str(rt, s) { assert(s != NIL); push(rt, 0, s); }

/// Address of the number on top of the stack.
addr_t tos_num(runtime_t rt) {
    if (rt->sp == rt->sp0) ERROR(STACK_UNDERFLOW);
    return rt->sp - sizeof(num_t);
}

/// Address of the string on top of the stack.
addr_t tos_str(runtime_t rt) {
    if (rt->sp == rt->sp0) ERROR(STACK_UNDERFLOW);
    return rt->sp - sizeof(num_t) - sizeof(str_t);
}

/** Pop from the return stack the pair (ip0, ip) denoting a position inside
    a program line. */
void rpop(runtime_t rt, addr_t *ip0, addr_t *ip) {
    if (rt->rsp == rt->rsp0) ERROR(RETURN);
    rt->rsp -= 2*sizeof(addr_t);
    *ip0 = PEEK(rt->rsp);
    *ip = PEEK(rt->rsp + sizeof(addr_t));
}

/** Push on the return stack the pair (ip0, ip) denoting a position inside a
    program line. Offset is stored as a single byte_t*/
void rpush(runtime_t rt, addr_t ip0, addr_t ip) {
    // An item in the rstack takes 4 (2*sizeof addr) bytes.
    if (rt->rsp - rt->rsp0 >= RSTACK_SIZE - 2*sizeof(addr_t))
        ERROR(TOO_MANY_GOSUB);
    POKE(rt->rsp, ip0);
    POKE(rt->rsp + sizeof(addr_t), ip);
    rt->rsp += 2*sizeof(addr_t);
}

/// \}
/** \defgroup CSTR String Constants Area

    The constant string area, starting at rt->csp0 and ending at rt->pp0 - 1,
    contains all string constants parsed in a program, along with all
    identifiers. It is maintained as a linked list, so to look for something in
    it, it requires linear time :-(

    The first free byte in the string area is pointed by rt->csp, which is also
    the pointer to the first temporary string: temporary strings may be built
    during expression evaluations, while rt->tsp points to the first byte after
    all temporary strings. Since rt->tsp is reset to rt->csp whenever an
    instruction starts to executing, all temporary strings are discarded when
    another expression will be evaluated. */
/// \{

#define STRCMP(s1, s2) (strcmp(RAM + (s1), RAM + (s2)))
#define STREQ(s1, s2) (strcmp(RAM + (s1), RAM + (s2)) == 0)

/** Add a new string to the data area and return its address: if there's no
    more space then return a negative number. */
int cstr_add(runtime_t rt, char *p0, int len) {
    int k = -1;
    if (rt->csp + len + 1 < rt->pp0) {
        k = rt->csp;
        memcpy(RAM + k, p0, len);
        RAM[k + len] = '\0';
        rt->csp += len + 1;
    }
    return k;
}

/** Add a temporary string to the data area and return its address: if there's
    no more space then return a negative number. */
int cstr_add_temp(runtime_t rt, char *p, int len) {
    int k = -1;
    if (rt->tsp + len + 1 < rt->pp0) {
        k = rt->tsp;
        memcpy(RAM + k, p, len);
        RAM[k + len] = '\0';
        rt->tsp += len + 1;
    }
    return k;
}

/** Look for a string in the data area: if found then return its address,
    else a negative number. */
int cstr_find(runtime_t rt, char *p0, int len) {
    str_t s = rt->csp0;
    while (s < rt->csp) {
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
    is done by maintaining two stack, the stack at rt->sp0 for data (each item
    being a pair (number, string), that is a number if string is NIL else it is
    a string, thus a pointer to a string in the constant string area), and the
    stack rt->estack for operators. Operators are executed or pushed on the
    stack according to their priorities, expressed in the OPERATORS[] table
    hereinafter. Constants and variables are immediately evaluated and pushed
    on the stack, instead. */
/// \{

//  Auxiliary functions.

/** Pop two values and compare them returning the result as an integer, the same
    as if strcmp or memcmp has been executed. */
static int oper_cmp(runtime_t rt) {
    num_t n1, n2;
    str_t s1, s2;
    pop(rt, &n2, &s2);
    pop(rt, &n1, &s1);
//~ printf("CMP %g,%i; %g,%i\n", n1, s1, n2, s2);
    if ((s1 == NIL) ^ (s2 == NIL)) ERROR(TYPE);
    return (s1 == NIL) ? ((n1 > n2) ? 1 : (n1 == n2) ? 0 : -1) : STRCMP(s1, s2);
}

/// Press the empty string on the stack.
static void oper_empty_string(runtime_t rt)
{
    // As empty string, use the ending '\0' of the last string in data area.
    // If the string list is empty (almost impossible), push an empty string.
    if (rt->csp == rt->csp0) { RAM[rt->csp++] = '\0'; }
    push_str(rt, rt->csp - 1);
}

/// Create a temporary string concatenating s1 and s2 and return it.
static str_t oper_concat(runtime_t rt, str_t s1, str_t s2) {
    // Create the concatenation as temporary string.
    char *p1 = RAM + s1;
    unsigned l1 = strlen(p1);
    char *p2 = RAM + s2;
    unsigned l2 = strlen(p2);
    int addr = cstr_add_temp(rt, p1, l1 + l2);
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    strcpy(RAM + addr + l1, p2);
    return addr;
}

#include <termios.h>
///  Get pressed key: Linux specific!
char oper_inkey(void) {
    int c, c1 = -1, c2 = -1;
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

void OPER_ABS(runtime_t rt) { push_num(rt, fabs(pop_num(rt))); }

void OPER_ACS(runtime_t rt) {
    num_t n = pop_num(rt);
    if (n < -1 || n > 1) ERROR(DOMAIN);
    push_num(rt, acos(n));
}

void OPER_AND(runtime_t rt) {
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 && n2);
}

void OPER_ASC(runtime_t rt) { push_num(rt, RAM[pop_str(rt)]); }

void OPER_ASN(runtime_t rt) {
    num_t n = pop_num(rt);
    if (n < -1 || n > 1) ERROR(DOMAIN);
    push_num(rt, asin(n));
}

void OPER_AT(runtime_t rt) {
    // AT(i,j) is used for its side effect to position the cursor at row i and
    // column j; it returns the empty string.
    char *w = getenv("COLUMNS"), *h = getenv("LINES");
    const int WIDTH = w != NULL ? atoi(w) : 80;
    const int HEIGHT = h != NULL ? atoi(h) : 24;
    int col = (int)pop_num(rt) % WIDTH;
    int row = (int)pop_num(rt) % HEIGHT;
    printf("\033[%i;%if", row, col);
    oper_empty_string(rt);
}

void OPER_ATN(runtime_t rt) { push_num(rt, atan(pop_num(rt))); }

void OPER_CHRS(runtime_t rt) {
    char c[2];
    c[0] = pop_num(rt);
    c[1] = '\0';
    int addr = cstr_add_temp(rt, c, 1);
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_COS(runtime_t rt) { push_num(rt, cos(pop_num(rt))); }

void OPER_DIV(runtime_t rt) {
    num_t n2 = pop_num(rt);
    if (n2 == 0) ERROR(ZERO);
    push_num(rt, pop_num(rt) / n2);
}

void OPER_EQ(runtime_t rt) { push_num(rt, oper_cmp(rt) == 0); }
void OPER_EXP(runtime_t rt) { push_num(rt, exp(pop_num(rt))); }
void OPER_GEQ(runtime_t rt) { push_num(rt, oper_cmp(rt) >= 0); }
void OPER_GT(runtime_t rt) { push_num(rt, oper_cmp(rt) > 0); }

void OPER_INKEYS(runtime_t rt) {
    push_num(rt, oper_inkey());
    OPER_CHRS(rt);
}

void OPER_INT(runtime_t rt) { push_num(rt, floor(pop_num(rt))); }

void OPER_LEFTS(runtime_t rt) {
    int n2 = pop_num(rt);
    char *s1 = RAM + pop_str(rt);
    if (n2 < 0 || n2 > strlen(s1)) ERROR(SUBSCRIPT_ERROR);
    // Create the substring as temporary string.
    int addr = cstr_add_temp(rt, s1, n2);
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_LEN(runtime_t rt) { push_num(rt, strlen(RAM + pop_str(rt))); }
void OPER_LEQ(runtime_t rt)  { push_num(rt, oper_cmp(rt) <= 0); }

void OPER_LOG(runtime_t rt) {
    num_t n = pop_num(rt);
    if (n <= 0) ERROR(DOMAIN);
    push_num(rt, log(n));
}

void OPER_LT(runtime_t rt) { push_num(rt, oper_cmp(rt) < 0); }

void OPER_MIDS(runtime_t rt) {
    int n2 = pop_num(rt);
    int n1 = pop_num(rt) - 1;
    char *s1 = RAM + pop_str(rt);
    if(n2 < 0 || n1 + n2 > strlen(s1) || n1 < 0 || n1 >= strlen(s1))
        ERROR(SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = cstr_add_temp(rt, s1 + n1, n2);
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_MINUS(runtime_t rt) {
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 - n2);
}

void OPER_MUL(runtime_t rt) {
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 * n2);
}

void OPER_NEG(runtime_t rt) { push_num(rt, -pop_num(rt)); }
void OPER_NEQ(runtime_t rt) { push_num(rt, oper_cmp(rt) != 0); }
void OPER_NOT(runtime_t rt) { push_num(rt, !pop_num(rt)); }

void OPER_OR(runtime_t rt) {
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    push_num(rt, n1 || n2);
}

void OPER_PLUS(runtime_t rt) {
    num_t n1, n2;
    str_t s1, s2;
    pop(rt, &n2, &s2);
    pop(rt, &n1, &s1);
    if (s1 == NIL) {
        if (s2 != NIL) ERROR(TYPE);
        push_num(rt, n1 + n2);
    } else {
        if (s2 == NIL) ERROR(TYPE);
        push_str(rt, oper_concat(rt, s1, s2));
}}

void OPER_POW(runtime_t rt) {
    num_t n2 = pop_num(rt), n1 = pop_num(rt);
    if (n1 == 0 && n2 <= 0) ERROR(DOMAIN);
    errno = 0;
    n1 = pow(n1, n2);
    if (errno) ERROR(DOMAIN);
    push_num(rt, n1);
}

void OPER_RIGHTS(runtime_t rt) {
    int n2 = pop_num(rt);
    char *s1 = RAM + pop_str(rt);
    if (n2 < 0 || n2 > strlen(s1)) ERROR(SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = cstr_add_temp(rt, s1 + strlen(s1) - n2, n2);
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_RND(runtime_t rt) { push_num(rt, rand()); }
void OPER_SIN(runtime_t rt) { push_num(rt, sin(pop_num(rt))); }

void OPER_SQR(runtime_t rt) {
    num_t n = pop_num(rt);
    if (n < 0) ERROR(DOMAIN);
    push_num(rt, sqrt(n));
}

void OPER_STRS(runtime_t rt) {
    char s[32];
    sprintf(s, "%g", pop_num(rt));
    int addr = cstr_add_temp(rt, s, strlen(s));
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_SUBS(runtime_t rt) {
    // SUB$(x$, i, j) = substring of x$ from i-th to j-th character (included).
    int j = pop_num(rt) - 1;
    int i = pop_num(rt) - 1;
    char *s = RAM + pop_str(rt);
    int len = strlen(s);
    if (j < i) j = i - 1;
    if (j < 0 || j >= len || i < 0 || i >= len) ERROR(SUBSCRIPT_ERROR);
    // Create the substring as temporaty string.
    int addr = cstr_add_temp(rt, s + i, j - i + 1);
    if (addr < 0) ERROR(OUT_OF_STRING_MEMORY);
    push_str(rt, addr);
}

void OPER_TAB(runtime_t rt) {
    // TAB(i) is used for its side effect to position the cursor at column j;
    // it returns the empty string.
    char *w = getenv("COLUMNS");
    const int WIDTH = w != NULL ? atoi(w) : 80;
    printf("\033[%iG", (int)pop_num(rt) % WIDTH);
    oper_empty_string(rt);
}

void OPER_TAN(runtime_t rt) { push_num(rt, tan(pop_num(rt))); }

void OPER_VAL(runtime_t rt) {
    char *p = RAM + pop_str(rt);
    errno = 0;
    num_t n = strtod(p, NULL);
    if (errno) ERROR(ILLEGAL_CONVERSION);
    push_num(rt, n);
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
    void (*routine)(runtime_t);
    byte_t arity;       // # of operator parameters.
    byte_t infix;       // 1 if the operator is binary infix, else 0.
    byte_t priority;    // operator priority.
} OPERATORS[] = {
    // Ordered according to the name field.
    {"*", OPER_MUL, 2, 1, 60}, {"+", OPER_PLUS, 2, 1, 50},
    {"-", OPER_MINUS, 2, 1, 50}, {"-", OPER_NEG, 1, 0, 70},
    {"/", OPER_DIV, 2, 1, 60}, {"<", OPER_LT, 2, 1, 30},
    {"<=", OPER_LEQ, 2, 1, 30}, {"<>", OPER_NEQ, 2, 1, 30},
    {"=", OPER_EQ, 2, 1, 30}, {">", OPER_GT, 2, 1, 30},
    {">=", OPER_GEQ, 2, 1, 30}, {"ABS", OPER_ABS, 1, 0, 100},
    {"ACS", OPER_ACS, 1, 0, 100}, {"AND", OPER_AND, 2, 1, 10},
    {"ASC", OPER_ASC, 1, 0, 100}, {"ASN", OPER_ASN, 1, 0, 100},
    {"AT", OPER_AT, 2, 0, 100},
    {"ATN", OPER_ATN, 1, 0, 100}, {"CHR$", OPER_CHRS, 1, 0, 100},
    {"COS", OPER_COS, 1, 0, 100}, {"EXP", OPER_EXP, 1, 0, 100},
    {"INKEY$", OPER_INKEYS, 0, 0, 100}, {"INT", OPER_INT, 1, 0, 100},
    {"LEFT$", OPER_LEFTS, 2, 0, 100}, {"LEN", OPER_LEN, 1, 0, 100},
    {"LOG", OPER_LOG, 1, 0, 100}, {"MID$", OPER_MIDS, 3, 0, 100},
    {"NOT", OPER_NOT, 1, 0, 20}, {"OR", OPER_OR, 2, 1, 10},
    {"RIGHT$", OPER_RIGHTS, 2, 0, 100}, {"RND", OPER_RND, 0, 0, 100},
    {"SIN", OPER_SIN, 1, 0, 100}, {"SQR", OPER_SQR, 1, 0, 100},
    {"STR$", OPER_STRS, 1, 0, 100}, {"SUB$", OPER_SUBS, 3, 0, 100},
    {"TAB", OPER_TAB, 1, 0, 100}, {"TAN", OPER_TAN, 1, 0, 100},
    {"VAL", OPER_VAL, 1, 0, 100}, {"^", OPER_POW, 2, 1, 80},
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

extern runtime_t expr(runtime_t);

// Some shortcuts: they retrieve the address/value of a variable's field.
#define VAR_SIZE(v) (PEEK((v)))
#define VAR_NAME(v) ((v) + sizeof(addr_t))
#define VAR_TYPE(v) (RAM[(v) + 2*sizeof(addr_t)])
#define VAR_ADDR(v) ((v) + 2*sizeof(addr_t) + 1)
#define VAR_TO(v) ((v) + 2*sizeof(addr_t) + 1 + sizeof(num_t))
#define VAR_STEP(v) ((v) + 2*sizeof(addr_t) + 1 + 2*sizeof(num_t))

/** Create a new variable at the address rt->vp. According to the provided type,
    which is also returned as value, inserts the data from d1 on, while name and
    type, of course, are always inserted. The pointer rt->csp is updated to the
    first byte following the created variable. */
int var_create(runtime_t rt, addr_t name, int type, num_t d1, num_t d2,
               num_t step, addr_t ip0, addr_t ip) {
    // Insert the variable: size, name, type, dim1 and possibly dim2 fields.
    addr_t v = rt->vp;
    POKE(VAR_NAME(v), name);
    VAR_TYPE(v) = type;
    rt->vp = VAR_ADDR(v);
    if (type == VAR_NUM) { POKE_NUM(rt->vp, 0); rt->vp += sizeof(num_t); }
    else if (type == VAR_STR) RAM[rt->vp++] = '\0';
    else if (type == VAR_FOR) {
        // 5 consecutive values to store: value, to, step, ip0, ip.
        POKE_NUM(rt->vp, d1); rt->vp += sizeof(num_t);
        POKE_NUM(rt->vp, d2); rt->vp += sizeof(num_t);
        POKE_NUM(rt->vp, step); rt->vp += sizeof(num_t);
        POKE(rt->vp, ip0); rt->vp += sizeof(addr_t);
        POKE(rt->vp, ip); rt->vp += sizeof(addr_t);
    } else {
        // array
        int numerical = type & VAR_NUM;
        // A string array is initialized by empty strings that take one byte.
        int size = d1 * d2 * (numerical ? sizeof(num_t) : 1);
        if (size >= rt->sp0 - rt->vp - 16) ERROR(OUT_OF_VARIABLE_MEMORY);
        POKE(rt->vp, d1); rt->vp += sizeof(addr_t);
        if (type & VAR_MAT) { POKE(rt->vp, d2); rt->vp += sizeof(addr_t); }
        // Allocates the actual array items.
        if (numerical) {
            for (int i = 0; i < d1*d2; ++ i) {
                POKE_NUM(rt->vp, 0);
                rt->vp += sizeof(num_t);
            }
        } else {
            memset(RAM + rt->vp, 0, size);
            rt->vp += size;
    }}
    // Finally writes the size field of the variable.
    POKE(v, rt->vp - v);
    return type;
}

/// Looks for a variable with name s and return its address, or NIL.
str_t var_find(runtime_t rt, str_t s) {
    for (addr_t v = rt->vp0; v < rt->vp; v += VAR_SIZE(v)) {
        if (STREQ(PEEK(VAR_NAME(v)), s)) {
           return v;
    }}
    return NIL;
}

/// Return 1 if the termination condition on the FOR variable is false.
int var_for_check(runtime_t rt, addr_t v) {
    num_t value = PEEK_NUM(VAR_ADDR(v));
    num_t to = PEEK_NUM(VAR_TO(v));
    return (PEEK_NUM(VAR_STEP(v)) > 0) ? value <= to : to <= value;
}

/** Applies the step to the provided FOR variable and set rt->ip0 and IP
    to the beginning of the loop if the bound has not yet been surpassed. */
void var_for_next(runtime_t rt, addr_t v) {
    addr_t va = VAR_ADDR(v);
    // Increase the value by the step
    POKE_NUM(va, PEEK_NUM(va) + PEEK_NUM(VAR_STEP(v)));
    // If the bound has not been surpassed, jump to ip.
    if (var_for_check(rt, v)) {
        rt->ip0 = PEEK(va + 3*sizeof(num_t));
        rt->ipn = rt->ip0 + RAM[rt->ip0];
        IP = PEEK(va + 3*sizeof(num_t) + sizeof(addr_t));
}}

/** If a scalar variable name (that should be the address of the address
    of the actual string) exists then its value is returned, else a new variable
    is created, of the type inferred from the name and its value is returned.
    An error, if there's no more space for the variable, can be raised. */
addr_t var_insert(runtime_t rt, addr_t name) {
    int type = RAM[name - 1] == CODE_IDN ? VAR_NUM : VAR_STR;
    str_t s = PEEK(name);
    addr_t v = var_find(rt, s);
    if (v == NIL) {
        v = rt->vp; // The next instruction will create a variable here.
        var_create(rt, s, type, 0, 0, 0, 0, 0);
    }
    return v;
}

/** Parse an array variable, thus "identifier(dim1, dim2)" and return all those
    data into: *name (address into the constant string area), *d1, *d2, while
    the type of the variable is returned as function value. Parse from IP which
    is updated to the token following che closed parenthesis. */
int var_array_parse(runtime_t rt, addr_t *name, addr_t *d1, addr_t *d2) {
    int type = (CODE == CODE_IDN ? VAR_NUM : VAR_STR);
    *name = PEEK(IP + 1);       // Store the name address.
    IP += 1 + sizeof(addr_t);   // Skip the name address.
    // Parse the subscript(s).
    EXPECT('(', SUBSCRIPT_EXPECTED);
    *d1 = pop_num(expr(rt));
    if (*d1 == 0) ERROR(SUBSCRIPT_ERROR);
    if (CODE == ',') {
        ++ IP;
        *d2 = pop_num(expr(rt));
        if (*d1 == 0) ERROR(SUBSCRIPT_ERROR);
        type |= VAR_MAT;
    } else {
        *d2 = 1;
        type |= VAR_VEC;
    }
    EXPECT(')', OPENPAR_WITHOUT_CLOSEPAR);
    return type;
}

/** Parse the subscripts of a vector or matrix variable: type is the actual type
    of the variable, p is assumed to point to the variable's i (first dimension)
    field, the resulting address of the variable's item will be stored into a1
    and its scalar type returned. */
static int var_array_address(runtime_t rt, int type, addr_t p, addr_t *a1) {
    int d1, d2, i, j = 1;
    int is_mat = type & VAR_MAT;
    d1 = PEEK(p); p += sizeof(addr_t);
    if (is_mat) { d2 = PEEK(p); p += sizeof(addr_t); }
    // Parse the subscript(s).
    EXPECT('(', SUBSCRIPT_EXPECTED);
    i = pop_num(expr(rt));
    if (i < 1 || i > d1) ERROR(SUBSCRIPT_ERROR);
    if (is_mat) {
        EXPECT(',', SUBSCRIPT_EXPECTED);
        j = pop_num(expr(rt));
        if (j < 1 || j > d2) ERROR(SUBSCRIPT_ERROR);
    }
    EXPECT(')', OPENPAR_WITHOUT_CLOSEPAR);
    // According to the type, compute the address *a1 of the item.
    if (type & VAR_NUM) {
        *a1 = p + (i-1)*sizeof(num_t) + (j-1)*d1*sizeof(num_t);
        type = VAR_NUM;
    } else {
        *a1 = p + (i-1)*sizeof(str_t) + (j-1)*d1*sizeof(str_t);
        type = VAR_STR;
    }
    return type;
}

/** Parse a variable, whose name (the CODE_IDN(S)) is pointed by IP and whose
    address in the variable's list is v, returning in *va the address of the
    value pointed by the variable expression (e.g. a$(1) points to the
    first string of the a$ array). On returning, IP points to the next token and
    the return value is the type of the variable. If a syntax error occurs, then
    an exception is raised. */
int var_address(runtime_t rt, addr_t v, addr_t *va) {
    IP += 1 + sizeof(addr_t);           // Skip the variable's name
    int type = VAR_TYPE(v);
    if (type & (VAR_VEC|VAR_MAT)) {
        type = var_array_address(rt, type, VAR_ADDR(v), va);
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

/// Compile a sequence of prefix (unary) operators, if any.
static void expr_prefix_operators(runtime_t rt) {
    // Prefix operators are unary ones (or '-' used as prefix).
    while (CODE == CODE_MINUS
    || CODE > CODE_STARTOPERATOR && CODE < CODE_ENDOPERATOR
    && OPERATORS[CODE - CODE_STARTOPERATOR - 1].arity == 1) {
        // Convert CODE_MINUS to CODE_NEG when considered as unary.
        int code = (CODE == CODE_MINUS ? CODE_NEG : CODE) - CODE_STARTOPERATOR - 1;
        // Execute operators on the stack with higher priority
        runtime_eunroll(rt, OPERATORS[code].priority);
        // Push the operator on the stack.
        runtime_epush(rt, OPERATORS[code].routine, OPERATORS[code].priority);
        ++ IP;
}}

/** Try to parse an operand from IP: if it fails an error is raised. */
static void expr_operand(runtime_t rt) {
    addr_t v, va;
    int code = CODE - CODE_STARTOPERATOR - 1;
    // Check against a built-in function or non infix operator.
    if (code >= 0 && CODE < CODE_ENDOPERATOR && !OPERATORS[code].infix) {
        // Execute operators on the stack with higher priority
        runtime_eunroll(rt, OPERATORS[code].priority);
        // Push the operator on the stack.
        runtime_epush(rt, OPERATORS[code].routine, OPERATORS[code].priority);
        // Parse and push on the stack the function parameters.
        int arity = OPERATORS[code].arity;
        ++IP;
        if (arity == 1) expr(rt);
        else if (arity > 1) {
            EXPECT('(', OPENEDPAR);
            for (int i = 1; i < arity; ++ i) {
                expr(rt);
                EXPECT(',', COMMA);
            }
            expr(rt);
            EXPECT(')', CLOSEDPAR);
    }} else
    switch (CODE) {
    case '(': {         // Subexpression.
        ++ IP;
        expr(rt);
        EXPECT(')', CLOSEDPAR);
        break;
    } case CODE_IDN: case CODE_IDNS: {  // Variable or function.
        extern int fn_eval(runtime_t, str_t);
        int code = CODE;
        str_t name = PEEK(IP + 1);
        if ((v = var_find(rt, name)) == NIL) {
            if (!fn_eval(rt, name)) ERROR(UNDEFINED_VARIABLE);
        } else {
            var_address(rt, v, &va);
//~ printf("VAR TYPE: %i\n", VAR_TYPE(v));
//~ printf("CODE: %i\n", code);
            if (code == CODE_IDN) {
//~ printf("VAR %s = %g\n", RAM + PEEK(VAR_NAME(v)), PEEK_NUM(va));
                push_num(rt, PEEK_NUM(va));
            } else {
                // Make a temporary copy of string va.
//~ printf("VAR$ %s = \"%s\"\n", RAM + PEEK(VAR_NAME(v)), RAM + PEEK(va));
                int s = cstr_add_temp(rt, RAM + va, strlen(RAM + va));
                if (s < 0) ERROR(OUT_OF_STRING_MEMORY);
                push_str(rt, s);
        }}
        break;
    } case CODE_INTLIT: {
        push_num(rt, PEEK(IP + 1));
        IP += 1 + sizeof(addr_t);
        break;
    } case CODE_NUMLIT: {
        push_num(rt, PEEK_NUM(IP + 1));
        IP += 1 + sizeof(num_t);
        break;
    } case CODE_STRLIT: {
        push_str(rt, PEEK(IP + 1));
        IP += 1 + sizeof(str_t);
        break;
    } default:
        ERROR(STACK_UNDERFLOW);
    }
}

/// Used after an operand to deal with a possible string subscript.
static void expr_str_subscript(runtime_t rt) {
    if (CODE == '(' && PEEK(tos_str(rt)) != NIL) {
        // x$(i TO j) is the same as SUB$(x$, i, j)
        // x$(i TO) is the same as x$(i TO LEN x$)
        // x$(TO j) is the same as x$(1 TO j)
        // x$(i) is the same as x$(i TO i)
        // Compute or use the default for both i and j
        ++ IP;
        if (CODE == CODE_TO) {  // x$(TO j).
            push_num(rt, 1);    // Missing i set to i by default.
            ++ IP;              // Skip "TO".
            expr(rt);           // Push j.
        } else {
            int len = strlen(RAM + PEEK(tos_str(rt)));
            expr(rt);           // Push i
            if (CODE == ')') {  // x$(i)
                // x$(i) = x$(i TO i), so duplicate number on top of stack.
                push_num(rt, PEEK_NUM(tos_num(rt)));
            } else {
                EXPECT(CODE_TO, TO_EXPECTED);
                // x$(i TO j) or x$(i TO)
                if (CODE != ')') expr(rt); else push_num(rt, len);
        }}
        EXPECT(')', OPENPAR_WITHOUT_CLOSEPAR);
        runtime_epush(rt, OPER_SUBS,
            OPERATORS[CODE_SUBS - CODE_STARTOPERATOR - 1].priority);
}}

/** Evaluate an expression at rt->ip and leave on the stack the value: return
    the runtime, so expr may be used in C expressions as pop_num(expr(rt)). */
runtime_t expr(runtime_t rt) {
    /*  Push a fake 0-priority operator on the operators stack to avoid
        to accidentally use operators already on the stack before executing
        the current expression (since expressions can be nested). */
    runtime_epush(rt, NULL, 0);
Again:
    /*  Recall than an expression is an operand, possibly preceded by a prefix
        operator (such as "-") and possibly followed by a string index "(i1 TO
        i2)", which can be followed by other expressions joined by a binary
        infix operator. */
    expr_prefix_operators(rt);
    expr_operand(rt);
    expr_str_subscript(rt);
    // Check against an infix operator followed by another operand etc.
    int code = CODE - CODE_STARTOPERATOR - 1;
    if (code >= 0 && CODE < CODE_ENDOPERATOR && OPERATORS[code].infix) {
        runtime_eunroll(rt, OPERATORS[code].priority);
        runtime_epush(rt, OPERATORS[code].routine, OPERATORS[code].priority);
        ++ IP;          // Skip the operator.
        goto Again;     // Repeat to evaluate the second operand.
    }
    // Parsed operators may be on the rt->estack: unroll them.
    runtime_eunroll(rt, 0);
    runtime_epop(rt);   // Drop the fake NULL operator.
    return rt;
}

/// \}
/** \defgroup ASSIGN Assignments Implementation

    To assign a number value use poke_num!!! */
/// \{

/** Assign to the string variable v the string s: the address of the string to
    overwrite is expected in va, while v contains the first byte of the variable
    in the variable's list (its size field). */
void assign_string(runtime_t rt, addr_t v, str_t va, str_t s) {
    // va points to the first character of the string to overwrite with s.
    unsigned len_v = strlen(RAM + va);
    unsigned len_s = strlen(RAM + s);
    int delta = len_s - len_v;
    if (delta >= rt->sp0 - rt->vp) ERROR(OUT_OF_VARIABLE_MEMORY);
    if (delta != 0) {
        // Reduce/Augment the space for the string.
        memmove(RAM + va + len_s, RAM + va + len_v, rt->vp - (va + len_v));
        // Adjust the pointer to the first free byte in the variable area.
        rt->vp += delta;
        // Adjust the size of the variable containing the string.
        POKE(v, PEEK(v) + delta);
    }
    // Finally copy string s on string a1.
    strcpy(RAM + va, RAM + s);
}

/** Parse "= expr" and assign the value to the variable of given type, at
    address v and whose value is at address va. */
void assign_expr(runtime_t rt, int type, addr_t v, addr_t va) {
    if (type == VAR_NONE) ERROR(UNDEFINED_VARIABLE);
    EXPECT(CODE_EQ, EQ_EXPECTED);
    expr(rt);
    if (type & (VAR_NUM|VAR_FOR)) {
        POKE_NUM(va, pop_num(rt));
    } else {
        assert(type & VAR_STR);
        assign_string(rt, v, va, pop_str(rt));
}}

/** Scan buffer b matching a constant (number, comma ending string or string
    delimited by double quotes) to the variable at IP: if no error occurs,
    assigns to the parsed variable the a value scanned from b. The updated
    pointer to the buffer is returned, pointing to ',' or '\0': if neither ','
    nor '\0' is found after the constant, NIL is returned. */
addr_t assign_item(runtime_t rt, addr_t b) {
    // We'll use C string standard functions.
    byte_t *p = RAM + b;
    byte_t *p1;
    addr_t v = var_insert(rt, IP + 1);
    addr_t va;
    int type = var_address(rt, v, &va);
    p += strspn(p, " \t\r\f\n");    // skip blanks
    if (type & (VAR_NUM|VAR_FOR)) {
        byte_t *p0 = p;
        num_t n = strtod(p0, &p);
        if (p0 == p) ERROR(ILLEGAL_INPUT);
        POKE_NUM(va, n);
    } else if (*p == '"') {
        // String between double quotes.
        ++ p;
        if ((p1 = strchr(p, '"')) == NULL) ERROR(EOL_INSIDE_STRING);
        // assign_string expects a C-string.
        *p1 = '\0'; assign_string(rt, v, va, p - RAM); *p1 = '"';
        p = p1 + 1;
    } else {
        // String ending with the line or the next comma.
        if ((p1 = strchr(p, ',')) == NULL) {
            assign_string(rt, v, va, p - RAM);
            p += strlen(p);
        } else {
        // assign_string expects a C-string.
            *p1 = '\0'; assign_string(rt, v, va, p - RAM); *p1 = ',';
            p = p1;
    }}
    p += strspn(p, " \t\r\f\n");    // skip blanks
    return (*p == ',' || *p == '\0') ? p - RAM : NIL;
}

/// \}
/// \defgroup TOKEN Lexical Analyzer
/// \{

/// Print a token at address a on file f: return the address of next token.
addr_t token_dump(runtime_t rt, addr_t a, FILE *f) {
    static int space = 0;   // 1 if a space should be printed in advance.
    byte_t b = RAM[a++];
    if (b == CODE_IDN || b == CODE_IDNS) {
        if (space) fputc(' ', f);
        fprintf(f, "%s", RAM + PEEK(a));
        a += 2;
    int    space = 0; //~ space = 1;
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
        //if (space) fputc(' ', f);
        fprintf(f, " %s", OPERATORS[b - CODE_STARTOPERATOR - 1].name);
        space = 1;
    } else if (b > CODE_STARTKEYWORD && b < CODE_ENDKEYWORD) {
        if (space) fputc(' ', f);
        if (b == CODE_DATA || b == CODE_REM) {
            fprintf(f, "%s%s", KEYWORDS[b - CODE_STARTKEYWORD - 1], RAM + a);
            a += strlen(RAM + a);   // points to the ending '\0'
            space = 0;
        } else {
            if (!space && (b == CODE_TO || b == CODE_STEP || b == CODE_THEN)) {
                fputc(' ', f);
            }
            fprintf(f, "%s", KEYWORDS[b - CODE_STARTKEYWORD - 1]);
            space = 1;
    }} else if (b != 0) {
        if (b == '(' || b == ')') {
            fputc(b, f);
            space = 0;
        } else if (b == ',' || b == ';' || b == ':') {
            fputc(b, f);
            space = 1;
        } else {
            if (space) fputc('\b', f);
            fputc(b, f);
            space = 0;
    }} else {
        space = 0;
    }
    return a;
}

/** Try to match a keyword with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding keyword code is returned, else 0. */
static int token_keyword(char *p0, unsigned len) {
    for (int i = 0; i < CODE_ENDKEYWORD - CODE_STARTKEYWORD - 1; ++ i) {
        int cmp = memcmp(KEYWORDS[i], p0, len);
        if (cmp == 0 && len == strlen(KEYWORDS[i])) {
            return (CODE_STARTKEYWORD + 1) + i;
        }
        if (cmp > 0) break;
    }
    return 0;
}

/** Try to match an operator with the text at p0, p0[1], ... p0[len-1]: if a
    match is found then the corresponding operator code is returned, else 0. */
static int token_operator(char *p0, unsigned len) {
    // In the following loop, j is an alias for i - (CODE_STARTOPERATOR + 1).
    for (int i = CODE_STARTOPERATOR + 1, j = 0; i < CODE_ENDOPERATOR; ++ i, ++ j) {
        int cmp = memcmp(OPERATORS[j].name, p0, len);
        if (cmp == 0 && OPERATORS[j].name[len] == '\0') {
            return i;
        }
        if (cmp > 0) break;
    }
    return 0;
}

///  Tokenize the contents of buf0 and store it at obj: return 0 on error.
int tokenize(runtime_t rt) {
    int k, len;
    // Encode the buf0 string at obj.
    byte_t *p = RAM + rt->buf0;
    byte_t *q0 = RAM + rt->obj; // The line size will be stored here.
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
                if ((k = cstr_find(rt, q, len)) < 0
                && (k = cstr_add(rt, q, len)) < 0) {
                    puts(ERRORS[ERROR_OUT_OF_STRING_MEMORY]);
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
                puts(ERRORS[ERROR_EOL_INSIDE_STRING]);
                return 0;
            }
            if ((k = cstr_find(rt, p, len)) < 0
            && (k = cstr_add(rt, p, len)) < 0) {
                puts(ERRORS[ERROR_OUT_OF_STRING_MEMORY]);
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
addr_t token_skip(runtime_t rt, addr_t a) {
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

extern int instr_exec(runtime_t);

//  Some useful macros: p is the first byte of a program line.
#define LINE_NUM(p) (p + 2)         ///< Address containing 16 bit line number.
#define LINE_NEXT(p) (p + RAM[p])   ///< Address of line following p.
#define LINE_SIZE(p) (RAM[p])       ///< Length of the line.
#define LINE_TEXT(p) (p + 2 + sizeof(addr_t))   ///< Address of first token.

/** If there are unsaved changes in the current program, ask the user if they
    could be discarded: return 1 if there are no changes or the user agree to
    discard them, else return 0. */
int prog_check(runtime_t rt) {
    int ok = 1;
    if (rt->prog_changed) {
        fputs("\nUNSAVED CHANGES IN CURRENT PROGRAM: DISCARD THEM (Y/N)? ",
            stderr);
        char c[2];
        fgets(c, sizeof(c), stdin);
        if (toupper(*c) != 'Y') ok = 0;
    }
    return ok;
}

/** Delete the line with number n: return 1 if the line doesn't exist, else 0. */
int prog_delete(runtime_t rt, int n) {
    for (addr_t a = rt->pp0; a < rt->pp; a += LINE_SIZE(a)) {
        addr_t n1 = PEEK(LINE_NUM(a));
        if (n1 > n) break;
        if (n1 == n) {
            // Overwrite the line by the following ones.
            int size = LINE_SIZE(a);
            memmove(RAM + a, RAM + a + size, rt->pp - (a + size));
            rt->pp -= size;
            rt->prog_changed = 1;
            return 0;
    }}
    return 1;
}

/// Edit a line at ram[line] starting with a line number.
void prog_edit(runtime_t rt, addr_t line) {
    extern void prog_insert(runtime_t rt, int n, addr_t a);
    // line = size; line+1 = CODE_INTLIT; line+2/+3 = integer; line+4 = line contents.
    int line_no = PEEK(LINE_NUM(line));
    if (RAM[LINE_TEXT(line)] == 0) {
        if (prog_delete(rt, line_no))
            printf("LINE %i DOES NOT EXIST!\n", line_no);
    } else {
        // Overwrite (= delete + insert) the line.
        prog_delete(rt, line_no);
        prog_insert(rt, line_no, line);
}}

/** Looks for a line with line number n: if found then its address is returned,
    else an error is raised. */
addr_t prog_find(runtime_t rt, int n) {
    for (addr_t a = rt->pp0; a < rt->pp; a += LINE_SIZE(a)) {
        addr_t n1 = PEEK(LINE_NUM(a));
        if (n1 > n) break;
        if (n1 == n) return a;
    }
    ERROR(ILLEGAL_LINE_NUMBER);
    //~ return NIL;
}

/** Insert line a with line number n (assume no such line exists). */
void prog_insert(runtime_t rt, int n, addr_t a) {
    // Get in p the address of the first line with number > n.
    addr_t p = rt->pp0;
    while (p < rt->pp && PEEK(LINE_NUM(p)) < n)
        p += LINE_SIZE(p);
    assert(p == rt->pp || PEEK(LINE_NUM(p)) != n);
    
    int size_new = LINE_SIZE(a);
    if (rt->pp + size_new >= rt->vp0) rt->error = ERROR_PROGRAM_TOO_LONG;
    else {
        // If the new is not the last, makes room for it, shifting on the right.
        if (p != rt->pp) memmove(RAM + p + size_new, RAM + p, rt->pp - p);
        // Copy the line.
        memmove(RAM + p, RAM + a, size_new);
        rt->pp += size_new;
        rt->prog_changed = 1;
}}

/** Print the program on a file: if the file is stdout, some frills are used to
    list the instruction, namely bold keywords, italic operators, etc. */
void prog_print(runtime_t rt, FILE *f) {
    addr_t p = rt->pp0;
    while (p < rt->pp) {
        fprintf(f, "%4i ", PEEK(LINE_NUM(p)));
        p = LINE_TEXT(p);
        while (RAM[p] != 0)
            p = token_dump(rt, p, f);
        fputc('\n', f);
        ++ p;
}}

/** Keep scanning lines from a file and interpreting them either as insertion
    commands (if they starts with a line number) or instructions to be executed
    immediately. */
void prog_repl(runtime_t rt, FILE *f) {
    while (!feof(f)) {
        if (f == stdin) putchar('>');
        if (fgets(RAM + rt->buf0, BUF_SIZE, f) == NULL) break;
        // Drop the final '\n' from the string.
        char *p = strchr(RAM + rt->buf0, '\n');
        if (p != NULL) *p = '\0';
        tokenize(rt);
        // RAM[rt->obj] = line size.
        if (RAM[rt->obj + 1] == CODE_INTLIT) prog_edit(rt, rt->obj);
        else {
            rt->ip0 = rt->obj;
            IP = rt->obj + 1;
            rt->ipn = NIL;
            while (IP != NIL && CODE != 0 && !instr_exec(rt))
                ;
}}}

/** Saves the current program on a file whose name is provided: the name is
    case sensitive. */
void prog_save(runtime_t rt, char *name) {
    FILE *f = fopen(name, "w");
    if (f == NULL) rt->error = ERROR_FILE;
    else {
        prog_print(rt, f);
        fclose(f);
        rt->prog_changed = 0;
}}

/// \}
/// \defgroup INSTR Instruction Implementation
/// \{

/** Parse a possible channel and return the corresponding file and buffer into
    *file and *buffer. */
static void instr_channel(runtime_t rt, FILE **file, addr_t *buffer) {
    *buffer = rt->buf0; // Terminal buffer
    if (CODE == '#') {
        ++ IP;
        int ch = pop_num(expr(rt));
        if (ch < 0 || ch > 4) ERROR(ILLEGAL_CHANNEL);
        if (rt->channels[ch] == NULL) ERROR(CHANNEL_CLOSED);
        *file = rt->channels[ch];
        *buffer += ch * BUF_SIZE;
        EXPECT(',', COMMA);
}}

/** Set rt->ip and rt->ipn before executing the line at rt->ip0. If rt->ip0
    is the next line of the program, IP is set to NIL. */
static inline void instr_line(runtime_t rt) {
    IP = LINE_TEXT(rt->ip0);
    if (IP >= rt->pp) IP = NIL;
    else rt->ipn = LINE_NEXT(rt->ip0);
}

/** Advance IP to the first token of the next instruction: if needed, IP is
    advanced to the first instruction of the next line, if any. The updated
    value of IP is returned, unless no further instruction is available, in
    which case NIL is returned. */
addr_t instr_skip(runtime_t rt) {
    /*  If RAM[ip] == 0 then the current line is ended, so ip+1 points to
        the next one; if RAM[ip] == ':' then ip+1 points to the next
        instruction. */
    while (CODE != 0) {
        if (CODE == ':' || CODE == CODE_THEN) return ++IP;
        IP = token_skip(rt, IP);
    }
    // Here we passed to the next line, adjust line pointers.
    if (rt->ipn >= rt->pp) return NIL;
    rt->ip0 = rt->ipn;
    rt->ipn = rt->ip0 + RAM[rt->ip0];
    return IP = LINE_TEXT(rt->ip0);
}

/** Starting from the pointer rt->ip, looks for the instruction whose CODE is
    code and returns the updated value of IP to the address of the token
    following it, or NIL if no such instruction exists. */
addr_t instr_lookfor(runtime_t rt, byte_t code) {
    do if (CODE == code) return ++IP; while (instr_skip(rt) != NIL);
    return NIL;
}

// Used by GOTO and ON
void instr_goto(runtime_t rt, int line) {
    rt->ip0 = prog_find(rt, line);
    instr_line(rt);
}

// Used by GOSUB and ON
void instr_gosub(runtime_t rt, int line) {
    rpush(rt, rt->ip0, IP);
    instr_goto(rt, line);
}

void INSTR_ATTR(runtime_t rt) {
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
            value = pop_num(expr(rt));
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

void INSTR_BYE(runtime_t rt) {
    if (prog_check(rt)) {
        puts("BYE.");
        exit(EXIT_SUCCESS);
}}

void INSTR_CHAIN(runtime_t rt) {
    // CHAIN filename[,linenumber]
    extern void INSTR_CONTINUE(runtime_t);
    extern void INSTR_END(runtime_t);
    str_t name = pop_str(expr(rt));
    int line = 0;
    if (CODE == ',') {
        ++ IP;
        line = pop_num(expr(rt));
    }
    // Delete the current file and load the chained one.
    rt->pp = rt->pp0;
    FILE *f = fopen(RAM + name, "r");
    if (f == NULL) ERROR(FILE);
    prog_repl(rt, f);
    fclose(f);
    // Get to the first line to execute.
    if (line == 0) rt->ip0 = rt->pp0;
    else instr_goto(rt, line);
    INSTR_CONTINUE(rt);
    INSTR_END(rt);
}

void INSTR_CLOSE(runtime_t rt) {
    // CLOSE #channel
    EXPECT('#', HASH_EXPECTED);
    int ch = pop_num(expr(rt));
    if (ch < 1 || ch > 4) ERROR(ILLEGAL_CHANNEL);
    if (rt->channels[ch] == NULL) ERROR(CHANNEL_CLOSED);
    fclose(rt->channels[ch]);
    rt->channels[ch] = NULL;
}

void INSTR_CLS(runtime_t rt) { fputs("\033[2J\033[1;1f", stderr); }

void INSTR_CONTINUE(runtime_t rt) {
    /*  Execute the program from instruction rt->ip on: both rt->ip0, rt->ip
        and rt->ipn should be properly settled. */
    while (IP >= rt->pp && !instr_exec(rt))
        ;
}

void INSTR_DIM(runtime_t rt) {
    for (;;) {
        if (CODE != CODE_IDN && CODE != CODE_IDNS)
            ERROR(IDENTIFIER_EXPECTED);
        str_t name = PEEK(IP + 1);
        if (var_find(rt, name) != NIL) ERROR(VARIABLE_ALREADY_DEFINED);
        addr_t d1, d2;
        int type = var_array_parse(rt, &name, &d1, &d2);
        var_create(rt, name, type, d1, d2, 0, 0, 0);
        if (CODE != ',') break;
        ++ IP;
}}

void INSTR_END(runtime_t rt) { IP = NIL; }
void INSTR_ERROR(runtime_t rt) { longjmp(rt->err_buffer, rt->error = pop_num(expr(rt))); }

void INSTR_FOR(runtime_t rt) {
    // FOR creates its variable if not already defined by another FOR statement.
    EXPECT(CODE_IDN, NUMVAR);
    str_t name = PEEK(IP);
    addr_t v = var_find(rt, name);
    if (v != NIL) {
        if (VAR_TYPE(v) != VAR_FOR) ERROR(FORVAR);
    } else {    // Create the variable if it doesn't exist.
        v = rt->vp;     // The variable will be inserted here.
        // Set value = bound = 0, step = 1, ip0 = ip = 0.
        var_create(rt, name, VAR_FOR, 0, 0, 1, 0, 0);
    }
    IP += sizeof(str_t);    // Skip the variable's name.
    addr_t va = VAR_ADDR(v);
    assign_expr(rt, VAR_FOR, v, va);
    EXPECT(CODE_TO, TO_EXPECTED);
    POKE_NUM(va + sizeof(num_t), pop_num(expr(rt)));
    if (CODE == CODE_STEP) {
        ++ IP;
        POKE_NUM(va + 2*sizeof(num_t), pop_num(expr(rt)));
    } else {
        POKE_NUM(va + 2*sizeof(num_t), 1);
    }
    POKE(va + 3*sizeof(num_t), rt->ip0);
    POKE(va + 3*sizeof(num_t) + sizeof(addr_t), rt->ip);
    if (!var_for_check(rt, v)) {
        /*  The FOR condition is initially false so we skip the loop, thus looks
            for the matching "NEXT var" instruction and skip after it. If the
            FOR-NEXTs don't respect nestings, cross your fingers... */
        do {
            if (instr_lookfor(rt, CODE_NEXT) == NIL) ERROR(FOR_WITHOUT_NEXT);
            EXPECT(CODE_IDN, NUMVAR);
        } while (PEEK(VAR_NAME(v)) != PEEK(IP));
        IP += sizeof(str_t);    // Skip the variable's name after NEXT.
}}

void INSTR_GOSUB(runtime_t rt) { instr_gosub(rt, pop_num(expr(rt))); }
void INSTR_GOTO(runtime_t rt) { instr_goto(rt, pop_num(expr(rt))); }

//  GO TO or GO SUB.
void INSTR_GO(runtime_t rt) {
    int code = RAM[IP++];
    if (code == CODE_TO) return INSTR_GOTO(rt);
    if (code != CODE_IDN || strcmp(RAM + PEEK(IP), "SUB") != 0)
        ERROR(ILLEGAL_INSTRUCTION);
    IP += sizeof(str_t);
    return INSTR_GOSUB(rt);
}

void INSTR_IF(runtime_t rt) {
    extern void INSTR_SKIP(runtime_t);
//~ printf("IF "); for (int i = IP; RAM[i] != 0; i = token_skip(rt, i)) token_dump(rt, i, stdout);
    if (pop_num(expr(rt)) == 0) INSTR_SKIP(rt);
    else if (CODE == CODE_THEN) {
        if (RAM[IP + 1] == CODE_INTLIT) {
            ++ IP;
            instr_goto(rt, pop_num(expr(rt)));
}}}

void INSTR_INPUT(runtime_t rt) {
    // A constant string may be printed at this point.
    if (CODE == CODE_STRLIT) {
        fputs(RAM + PEEK(IP + 1), stderr);
        IP += 1 + sizeof(str_t);
        if (CODE != ',' && CODE != ';') ERROR(SYNTAX);
        ++ IP;
    }
    FILE *f = stdin;
    addr_t b = rt->buf0;    // Terminal buffer
    instr_channel(rt, &f, &b);
    if (f == stdin) putchar('?');
    if (fgets(RAM + b, BUF_SIZE, f) == NULL) ERROR(ILLEGAL_INPUT);
    // Drop the ending '\n' if any.
    char *p = strchr(RAM + b, '\n'); if (p != NULL) *p = '\0';
    // Parse a list of variables and assign values read from buffer b to them.
    for (;;) {
        b = assign_item(rt, b);
        if (b == NIL) ERROR(ILLEGAL_INPUT);
        if (CODE != ',') break;
        ++ IP;
        if (RAM[b] != ',') ERROR(ILLEGAL_INPUT);
        ++ b;
}}

void INSTR_LET(runtime_t rt) {
    // LET v=e, ..., v=e
    addr_t v, va;
    for (;;) {
        if (CODE != CODE_IDN && CODE != CODE_IDNS)
            ERROR(IDENTIFIER_EXPECTED);
        v = var_insert(rt, IP + 1);
        int type = var_address(rt, v, &va);
        assign_expr(rt, type, v, va);
        if (CODE != ',') break;
        ++ IP;
}}

void INSTR_LINPUT(runtime_t rt) {
    // A constant string may be printed at this point.
    if (CODE == CODE_STRLIT) {
        fputs(RAM + PEEK(IP + 1), stderr);
        IP += 1 + sizeof(str_t);
        if (CODE != ',' && CODE != ';') ERROR(SYNTAX);
        ++ IP;
    }
    FILE *f = stdin;
    addr_t b = rt->buf0;    // Terminal buffer
    instr_channel(rt, &f, &b);
    // Create or retrieve the string variable to be read.
    addr_t v = var_insert(rt, IP + 1);
    addr_t va;
    int type = var_address(rt, v, &va);
    if (!(type & VAR_STR)) ERROR(STRING_VARIABLE_EXPECTED);
    if (fgets(RAM + b, BUF_SIZE, f) == NULL) ERROR(ILLEGAL_INPUT);
    // Drop the ending '\n' if any.
    char *p = strchr(RAM + b, '\n'); if (p != NULL) *p = '\0';
    // Assign to variable v the value of the string k.
    assign_string(rt, v, va, b);
}

void INSTR_LIST(runtime_t rt) { prog_print(rt, stderr); }

void INSTR_LOAD(runtime_t rt) {
    extern void INSTR_MERGE(runtime_t);
    if (prog_check(rt)) INSTR_MERGE(runtime_reset(rt, 1));
    else (void) pop_str(expr(rt));  // skip the program name.
}

void INSTR_MERGE(runtime_t rt) {
    FILE *f = fopen(RAM + pop_str(expr(rt)), "r");
    if (f == NULL) ERROR(FILE);
    prog_repl(rt, f);
    fclose(f);
    rt->prog_changed = 0;
    // The rt->obj buffer has been corrupted loading the file.
    longjmp(rt->err_buffer, 0);
}

void INSTR_NEW(runtime_t rt) { if (prog_check(rt)) runtime_reset(rt, 1); }

void INSTR_NEXT(runtime_t rt) {
    EXPECT(CODE_IDN, NUMVAR);
    addr_t v = var_find(rt, PEEK(IP));
    IP += sizeof(str_t);
    if (v == NIL) ERROR(UNDEFINED_VARIABLE);
    if (VAR_TYPE(v) != VAR_FOR) ERROR(FORVAR);
    /* Applies the step to the provided FOR variable and set rt->ip0 and IP to
        the beginning of the loop if the bound has not yet been surpassed. */
    addr_t va = VAR_ADDR(v);
    // Increase the value by the step
    POKE_NUM(va, PEEK_NUM(va) + PEEK_NUM(VAR_STEP(v)));
    // If the bound has not been surpassed, jump to ip.
    if (var_for_check(rt, v)) {
        rt->ip0 = PEEK(va + 3*sizeof(num_t));
        rt->ipn = rt->ip0 + RAM[rt->ip0];
        IP = PEEK(va + 3*sizeof(num_t) + sizeof(addr_t));
}}

void INSTR_ON(runtime_t rt) {
    // ON ERROR i   ' GOSUB i will be issued when an error will occur.
    // ON expr GOSUB i1, ..., in
    // ON expr GOTO i1, ..., in
    if (CODE == CODE_ERROR) {
        ++ IP;
        EXPECT(CODE_INTLIT, ILLEGAL_LINE_NUMBER);
        rt->on_error = prog_find(rt, PEEK(IP));
        IP += sizeof(addr_t);
    } else {
        int n = pop_num(expr(rt));
        if (n < 1) ERROR(ON);
        int code = CODE;
        // Parse the integer list, storing in nth the n-th element.
        int i = 1, nth = -1;
        do {
            ++ IP;
            if (CODE != CODE_INTLIT) ERROR(ILLEGAL_LINE_NUMBER);
            if (i == n) nth = PEEK(IP + 1);
            IP += 1 + sizeof(addr_t);
            ++ i;
        } while (CODE == ',');
        // The next integer is the line number where to jump.
        if (nth == -1) ERROR(ON);
        push_num(rt, nth);
        if (code == CODE_GOTO) instr_goto(rt, nth);
        else if (code == CODE_GOSUB) instr_gosub(rt, nth);
        else ERROR(SYNTAX);
}}

void INSTR_OPEN(runtime_t rt) {
    // OPEN #channel, name, mode
    // channel and mode are integers, name is a string.
    EXPECT('#', HASH_EXPECTED);
    int ch = pop_num(expr(rt));
    if (ch < 1 || ch > 4) ERROR(ILLEGAL_CHANNEL);
    if (rt->channels[ch] != NULL) ERROR(CHANNEL_BUSY);
    EXPECT(',', COMMA);
    str_t name = pop_str(expr(rt));
    EXPECT(',', COMMA);
    int mode = pop_num(expr(rt));
    if (mode < 0 || mode > 2) ERROR(ILLEGAL_MODE);
    rt->channels[ch] = fopen(RAM + name, mode == 0 ? "r" : mode == 1 ? "w" : "a");
    if (rt->channels[ch] == NULL) ERROR(FILE);
}

void INSTR_PRINT(runtime_t rt) {
    FILE *f = stdout;
    addr_t b = rt->buf0;    // Terminal buffer
    instr_channel(rt, &f, &b);
    int newline = 1;        // True if a newline has to be eventually printed.
    while (CODE != 0 && CODE != ':' && CODE != '\'') {
        // A print-list is a sequence of commas/semi-colons possibly repeated
        // and expressions: the latter should be separated by at least a comma
        // or by a semi-colon.
        if (CODE == ',') {
            if (f == stdout) {
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
                    fputs("\t\t", f);
                } else {
                    i += 16 - i % 16;   // Tabulation interval = 16 positions.
                    printf("\033[%iG", i);
                }
                tcsetattr(0, TCSANOW, &prev);       // Restore waiting mode.
            } else {
                fputs("\t\t", f);
            }
            ++ IP;
            newline = 0;
        } else if (CODE == ';') {
            ++ IP;
            newline = 0;
        } else {
            num_t n;
            str_t s;
            pop(expr(rt), &n, &s);
            if (s == NIL) fprintf(f, "%g", n);
            else fputs(RAM + s, f);
            newline = 1;
    }}
    if (newline) fputc('\n', f);
    fflush(f);
}

void INSTR_READ(runtime_t rt) {
    /*  rt->data_next points to an instruction keyword, and in this case we
        check against a DATA, else find the next DATA instruction if any, or
        points to ',' or '\0' inside a DATA line. */
    for (;;) {
        if (RAM[rt->data_next] == CODE_DATA || RAM[rt->data_next] == ',') {
            ++ rt->data_next;   // Skip DATA or ','.
        } else {
            // Save current line pointers, instr_lookfor changes them!
            addr_t ip0_saved = rt->ip0, ip_saved = IP, ipn_saved = rt->ipn;
            IP = rt->data_next;
            rt->data_next = instr_lookfor(rt, CODE_DATA);
            if (rt->data_next == NIL) ERROR(OUT_OF_DATA);
            // Restore line pointers.
            rt->ip0 = ip0_saved, IP = ip_saved, rt->ipn = ipn_saved;
        }
        rt->data_next = assign_item(rt, rt->data_next);
        if (rt->data_next == NIL) ERROR(SYNTAX);
        if (CODE != ',') break;
        ++ IP;  // Skip ','.
}}

void INSTR_REPEAT(runtime_t rt) {
    if (rt->ipn == NIL) ERROR(ILLEGAL_INSTRUCTION_OUTSIDE_PROGRAM);
    IP = rt->ip0 + 4;
}

void INSTR_RESTORE(runtime_t rt) {
    // RESTORE [line-number]
    if (CODE == CODE_INTLIT) {
        rt->data_next += LINE_TEXT(prog_find(rt, PEEK(IP + 1)));
        IP += 1 + sizeof(addr_t);
    } else {
        rt->data_next = LINE_TEXT(rt->pp0);
        ++ IP;
}}

void INSTR_RETURN(runtime_t rt) {
    rpop(rt, &rt->ip0, &IP);
    rt->ipn = LINE_NEXT(rt->ip0);
}

void INSTR_RUN(runtime_t rt) {
    runtime_reset(rt, 0);
    rt->ip0 = rt->pp0;
    instr_line(rt);
    INSTR_CONTINUE(rt);
}

void INSTR_SAVE(runtime_t rt) { prog_save(rt, RAM + pop_str(expr(rt))); }
void INSTR_SKIP(runtime_t rt) { IP = rt->ipn == NIL ? NIL : rt->ipn - 1; }
void INSTR_STEP(runtime_t rt) { ERROR(ILLEGAL_INSTRUCTION); }
void INSTR_STOP(runtime_t rt) { ERROR(PROGRAM_STOPPED); }
void INSTR_SYS(runtime_t rt) { system(RAM + pop_str(expr(rt))); }
void INSTR_THEN(runtime_t rt) { ERROR(ILLEGAL_INSTRUCTION); }
void INSTR_TO(runtime_t rt) { ERROR(ILLEGAL_INSTRUCTION); }
void INSTR_TRACE(runtime_t rt) { rt->trace = pop_num(expr(rt)); }

/// Table of all routines implementing instructions.
void (*INSTRUCTION_CODE[])(runtime_t) = {
    /*  Same ordering as the constant between CODE_STARTKEYWORD and
        CODE_ENDKEYWORD */
    INSTR_ATTR, INSTR_BYE, INSTR_CHAIN, INSTR_CLOSE, INSTR_CLS, INSTR_CONTINUE,
    INSTR_SKIP /* DATA */, INSTR_SKIP /* DEF */, INSTR_DIM,
    dump_memory, dump_variables, INSTR_END,
    INSTR_ERROR, INSTR_FOR, INSTR_GO, INSTR_GOSUB, INSTR_GOTO, INSTR_IF,
    INSTR_INPUT, INSTR_LET, INSTR_LINPUT, INSTR_LIST, INSTR_LOAD, INSTR_MERGE,
    INSTR_NEW, INSTR_NEXT, INSTR_ON, INSTR_OPEN, INSTR_PRINT,
    INSTR_READ,
    INSTR_SKIP /* REM */, INSTR_REPEAT, INSTR_RESTORE, INSTR_RETURN, INSTR_RUN,
    INSTR_SAVE, INSTR_SKIP, INSTR_STEP, INSTR_STOP, INSTR_SYS, INSTR_THEN,
    INSTR_TO, INSTR_TRACE
};

/** Execute the instruction at rt->ip, advancing rt->ip to the first token of
    the next instruction, evern in case of error. The value of rt->error is
    returned, in any case. */
int instr_exec(runtime_t rt) {
    // Save exception buffer
    jmp_buf error_saved;
    memcpy(error_saved, rt->err_buffer, sizeof(error_saved));
Again:
    rt->error = ERROR_NONE; // Reset error condition.
    rt->estack_next = 0;    // Reset operator stack.
    rt->sp = rt->sp0;       // Reset operand stack.
    rt->tsp = rt->csp;      // Reset temporary string area.
    if (setjmp(rt->err_buffer) == 0) {
        volatile byte_t opcode;
        // Skip possible instruction separators.
        while ((opcode = CODE) == ':' || opcode == CODE_THEN) ++ IP;
        if (rt->trace) {
            fprintf(stderr, "\nEXECUTE % 4i ", PEEK(LINE_NUM(rt->ip0)));
            for (addr_t p = IP; RAM[p] != 0; p = token_dump(rt, p, stderr))
                ;
            fputc('\n', stderr);
        }
        if (opcode > CODE_STARTKEYWORD && opcode < CODE_ENDKEYWORD) {
            // Skip the keyword and execute the corresponding INSTR_ routine.
            ++ IP; (*INSTRUCTION_CODE[opcode - CODE_STARTKEYWORD - 1])(rt);
        } else {
            // Instruction of the form "var = expr".
            if (opcode != CODE_IDN && opcode != CODE_IDNS)
                ERROR(ILLEGAL_INSTRUCTION);
            INSTR_LET(rt);
        }
        // If END has been reached, IP == NIL.
        if (IP != NIL) {
            /*  Here IP points to the first byte after the instruction, so a
                comment or an instruction delimiter should be parsed here. */
            if (CODE == '\'') {
                // Skip comment
                rt->ip0 = IP + strlen(RAM + IP) + 1;
                instr_skip(rt);
            }
            // IP should point to a delimiter between instructions or to the
            // first token of a line (in case a jump occurred).
            if (CODE == 0) instr_skip(rt);
            else if (LINE_TEXT(rt->ip0) != IP && CODE != ':' && CODE != CODE_THEN)
                ERROR(SYNTAX);
    }} else {
        // If some ON ERROR instruction has been issued, execute it.
        if (rt->on_error == NIL) {
            if (rt->ipn != NIL)
                fprintf(stderr, "LINE %i: ", PEEK(LINE_NUM(rt->ip0)));
            if (rt->error > 0 && rt->error <= ERROR_ZERO) {
                puts(ERRORS[rt->error]);
            } else {
                printf("ERROR #%i\n", rt->error);
        }} else {
            rpush(rt, rt->ip0, IP);
            rt->ip0 = rt->on_error;
            instr_line(rt);
    }}
    // Restore exception buffer.
    memcpy(rt->err_buffer, error_saved, sizeof(error_saved));
    return rt->error;
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
int fn_eval(runtime_t rt, str_t name) {
    int ok = 0;     // a priori not found.
    // We'll use instr_lookfor that alter pointers to the line under execution.
    addr_t ip0_saved = rt->ip0, ip_saved = IP, ipn_saved = rt->ipn;
    // Start looking for DEF FN from the very first program line.
    rt->ip0 = rt->pp0;
    instr_line(rt);
    while ((instr_lookfor(rt, CODE_DEF)) != NIL) {
        // Check the name of the function
        if (CODE != CODE_IDN && CODE != CODE_IDNS)
            ERROR(IDENTIFIER_EXPECTED);
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
                        addr_t v = var_insert(rt, ip_saved + 1);
                        POKE_NUM(VAR_ADDR(v), pop_num(expr(rt)));
                    } else
                    if (RAM[ip_saved] == CODE_IDNS) { // String.
                        addr_t v = var_insert(rt, ip_saved + 1);
                        assign_string(rt, v, VAR_ADDR(v), pop_str(expr(rt)));
                    } else {
                        ERROR(IDENTIFIER_EXPECTED);
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
            EXPECT(CODE_EQ, EQ_EXPECTED);
            expr(rt);
            // Final swap to restore IP after the actual parameters list.
            IP = ip_saved;
            ok = 1;
            break;
    }}
    // Done! We restore line pointers before return.
    rt->ip0 = ip0_saved;
    IP = ip_saved;
    rt->ipn = ipn_saved;
    return ok;
}

/// \}
/// \defgroup MAIN Main Program
/// \{

int main(int npar, char **pars) {
    struct runtime_s rt;
    runtime_init(&rt);
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
        prog_repl(&rt, stdin);
    } else {
        for (int i = 1; i < npar; ++ i) {
            FILE *f = fopen(pars[1], "r");
            if (f == NULL) puts(ERRORS[ERROR_FILE]);
            else {
                prog_repl(&rt, f);
                fclose(f);
                rt.prog_changed = 0;
                INSTR_RUN(&rt);
    }}}
    return EXIT_SUCCESS;
}

/// \}