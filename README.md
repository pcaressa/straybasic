# Straybasic

## A street Basic imaginary dialect

This interpreter simulates an old fashioned BASIC system, a "street Basic"
as Kemeny and Kurtz called them, which runs on top of a 16-bit computer.

The techniques used are primitive and imitate the ones employed by 70s
interpreters, obsessed with space saving at the expense of time and
maintenability. (It may well be that that some unconscious adolescent
reminiscence of my past with the ZX Spectrum influenced the development
of this code.)

The project was originally designed and implemented for a speech at the
Codemotion Milan conference in 2024, commemorating the 60 years of the Basic
programming language: I wanted to show which was the developer experience
in the old days by an

### Paolo Caressa

#### Version 1.0, november 2024

## Foreword

%%%



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

%%%



## Editing, saving and loading programs

## Working with numbers

In Basic (as in most programming languages) computer memory is not directly accessible, but data can be stored in it and retrieved from it only when associated to symbolic names: a name can be associated to a numerical or string value. To associate a name to a number use the `LET` statement, as in

    LET x = 10, y = x + 20, z = 2 *(x + y)

To retrieve the values associated to a name, just mention the name when a number is expected, for example, once che previous line has been typed at the StrayBasic prompt, type also

    PRINT x, y, z

The result of these insertion is as follows

    >LET x = 10, y = x + 20, z = 2 *(x + y)
    >PRINT x, y, z
    10      30      80
    >

So, whenever `x` is parsed inside an expression, its numerical value is used. A noteworthy difference between StrayBasic and all other Basic dialects, is that by means of `LET` one can introduce a variable and give to it a value, but to modify the value one just use an equation-like notation, as in

    >x = x * 2
    >PRINT x
    20
    >

A statement of the form `variable = value` is called an *assignment*: on its left hand side, it should figure an already defined variable, on the right hand side an expression resulting in a value.

It would be an error to use `LET` to assign a new value to a variable that has been already defined, the interpreter would complain in that case. Notice that, while one can define multiple variables in a single `LET` instruction, the assignment instruction does not provide a multiple version, thus

    >x = x * 2, y = y + 1

would result in an error (however the first assignment changing the value of `x` will be performed).

A numerical value is produced by evaluating a numerical expression, which is composed by connecting operands with binary operators. An operand can be:

- a number;
- a numerical variable;
- a built-in function applied to an expression, such as `COS x` or also `-x` which changes the sign of the number `x`;
- .....................

## Working with strings

## Working with arrays

## Working with files

## Odds & Ends

## System limitations

## Formal description

## Sample programs
