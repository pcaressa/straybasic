# StrayBasic

### A street Basic imaginary dialect

#### (c) 2024 by Paolo Caressa

## Foreword

In their book *Back to Basic*, mathematicians and computer scientists John Kemeny and Thomas Kurtz describe the design and implementation of the Basic language at Dartmouth College in the early 60s. They also criticise the Basic dialects of the 70s, from the seminal version of Gates, Allen and Davidoff on, that sacrified the language cleanness on the altair of execution speed and, most of all, memory saving.

Basic was, at the time, the only high level language capable to be implemented on small computers, with slow 8-bit CPUs and no more than 64K of total memory, both for the ROM where the interpreter was stored and for the RAM where user programs and data were stored. Kemeny and Kurtz, that in the meanwhile erected Basic to the same level of the 70s languages such as Pascal, called *Street Basics* those implementations that worked with so limited resources. Indeed, the Basic of these micro-computers were more difficult to read, almost impossible to maintain and full of device-oriented tricks to achieve goals such as graphics, sounds and game programming (that eventually, due to the slowness of the interpreters, were programmed in assembly).

It is on these street Basics that people like me had their first contact with computers and programming, and became the generation that EW Dijkstra complained to be *mentally mutilated beyond hope of regeneration* due to their exposure to Basic. But everyone could program with those small computers, some million times less powerful that nowadays laptops.

StrayBasic is a hommage both to the visionary ideas of Kemeny and Kurtz, whose aim was to let everyone to program, and to the programmers and engineers who wrote dozens, hundreds of basic interpreters for the most diverse machines, among which also the quite evolute Basic interpreter of the Sinclair ZX Spectrum with which I started more than four decades ago.

Most commands of the Kemeny-Kurtz 60s Basic are part of StrayBasic, others can be easily replaced, and elementary programs from the classical version of the language works almost with no change. The most striking differences are listed in an appendix to this document. On the other hand, StrayBasic also allows to reproduce the crazy and joyful improvisation of street basic programs, and the need for cleverly assemble data and code due to lack of resources.

While classical Basic was compiled, street Basic was interpreted as StrayBasic is: moreover, the source code is stored in a compressed form which is directly interpreted, as for most interpreters of the 70s. The language is slow, according to modern standards, and yet incredibly fast according to 70s and 80s standards. The interpreter manage up to 64Kb where both programs and data are to be stored, reproducing the constrained environments of the 8/16 bit era. Also the techniques I used were the ones of those street Basic interpreters.

The original idea of this project was to have an interpreter versatile enough to show the different aspects of the language to the audience of my talk at Codemotion Milan 2024. The talk had some success, mainly due to the nostalgia effect, and I decided to craft the interpreter to a complete version of Basic, very similar to most dialects and yet different from anyone of these.

But to program in Basic was also to port from a dialect to another one: in the GitHub repository there are some examples of programs ported, with minor changes, to StrayBasic, such as a famous version of the Eliza bot.

This is an eminently useless project, the one I love the most to deal with: hope someone else will enjoy it.

## Setting up and using the system

### Compiling and launching StrayBasic

StrayBasic is written in C for Linux: in particular, to perform some print trick needed by any Basic interpreter, it assumes to run inside a terminal and uses escape codes to perform some rough graphical effects. The all code is contained in a single file(!) `straybasic.c` that can be compiled with

    $ clang -lm straybasic.c -o straybasic

A lot of warnings will be issued, to avoid them compile adding the `-Wno-pointer-sign` switch. In any case, the result will be an executable `straybasic` with which one can execute a Basic source as in

    $ straybasic sample.bas

of just enter the REPL mode by simply launching the program:

    $ straybasic

In the latter case, the following will appear on top of the screen, in green foreground on black background:

    //== ====== ||==\    =  \\  // ||==\    =    //== ||  //=\
    \\     ||   ||__/   / \  \\//  ||__/   / \   \\   || ||
      \\   ||   ||\\   //_\\  ||   ||  \  //_\\    \\ || ||
    ==//   ||   || \\ //   \\ ||   ||==/ //   \\ ==// ||  \\=/

    (c) 2024 by Paolo Caressa
    [Type BYE to quit]

    >

The `>` sign is the prompt which asks the user for a Basic command to be executed or for a program line to be edited. To quit the interpreter, type the `bye` command that ends the interpreter execution.

Try to type a simple Basic command, such as 

    >print "Hello World!"

and it should work.

Since Basic is a case-insensitive language, you can insert identifiers, such as `print`, with no care about letters, for example `Print`, `PRINT` or even `pRiNt` are all converted to `PRINT` by the interpreter, before being processed. Only text inside strings and comments is preserved as it is inserted, while other text is tokenized and converted to uppercase.

### Editing and running the program

Of course, typing commands in direct mode is not the only feature of the interpreter: one should write programs and run them, instead. To do that, the program is assembled as a set of lines, where each line is identified by a unique positive integer number: you need to explicitly insert the number, as first item of the line. For example, type in sequence the following:

    >10 let x$ = ""
    >30 for i = len x$ to 1 step -1
    >50 next i
    >40 let y$ = y$ + x$(i)
    >60 print y$
    >20 input x$
    >9999 end

You have inserted the lines of a simple program, even if the order of insertion is not the order of execution: the latter is dictated by the line numbering. To peruse the program, type the `LIST` command:

    >LIST
      10 LET X$ = ""
      20 INPUT X$
      30 FOR I = LEN X$ TO 1 STEP -1
      40 LET Y$ = Y$ + X$(I)
      50 NEXT I
      60 PRINT Y$
    9999 END

Notice that the interpreter has converted to uppercase each keyword and identifier and also that the listing is in increasing order w.r.t. line numbers, thus the program is shown with the lines in their natural ordering, the one in which, from the first to the latter, will be executed. To execute the program just type `run`: after that, the `INPUT` instruction at line 20 will make the program to pause and prompt you to insert a text; type a word and then press ENTER (or RETURN or whatever on your keyboard), then the program will transform the word you typed, stored into variable `X$`, into the word whose letters have been reversed, stored into `Y$`:

    >run
    ? Alpha
    ahplA
    >

To edit a program, one just inserts new lines or modify or delete existing lines: to insert a new line, just decide where to insert it and invent an appropriate line number: for example, suppose you want to print a message for the user before inputing the number; this should be done between lines 10 and 15, so we type

    >15 print "Insert a word"

so that the program becomes:

      10 LET X$ = ""
      15 PRINT "Insert a word"
      20 INPUT X$
      30 FOR I = LEN X$ TO 1 STEP -1
      40 LET Y$ = Y$ + X$(I)
      50 NEXT I
      60 PRINT Y$
    9999 END

Notice that the letters inside the string to be printed at line 15 are not converted to uppercase. Now, let us make a change: for people used to any programming language whatsoever, it is clear that our program scans, by means of a for-loop, the characters of the string `X$` and append them to the string `Y$`, resulting, when the all all `X$` has been scanned, in `Y$` containing the same characters as `X$` in reverse order.

The for-loop we wrote scans from the last character of the string to the first one, stepping by -1 at each loop iteration (if this is not clear don't mind, all that is explaned below). This is cumbersome, as we could scan `X$` from its first character on and append each character in front of `Y$` at each iteration.

So we change line 30, reinserting it with the same line number, which makes the interpreter to overwrite the previous line with the new one, and the same we do with line 40: type

    >30 for i = 1 to len x$
    >40 let y$ = x$(i) + y$

Upon listing wiht `list` we get:

      10 LET X$ = ""
      15 PRINT "Insert a word"
      20 INPUT X$
      30 FOR I = 1 TO LEN X$
      40 LET Y$ = X$(I) + Y$
      50 NEXT I
      60 PRINT Y$
    9999 END

Suppose now that we find too verbose the string printed at line 15: we could decide to drop the entire line, and to do that we type the line number and press `enter` (or `newline` or `return` or whatever way you call that key!)

    >15
    >list
      10 LET X$ = ""
      20 INPUT X$
      30 FOR I = 1 TO LEN X$
      40 LET Y$ = X$(I) + Y$
      50 NEXT I
      60 PRINT Y$
    9999 END

Of course, trying to delete a line that doesn't exist will generate a complain:

    >37
    LINE 37 DOES NOT EXIST!
    >

### Saving and loading the program

Once a program is edited, we can save it via the `save` instruction, that requires a string with the name of the file to save, for example

    >save "reverse.bas"

The file is saved in the folder where the interpreter has been launched: you can access commands on the shell of the operating system via the `sys` command which accepts a string that contains the command to be executed: for example we can create a directory and store the program therein by

    >sys "mkdir tmp"
    >save "tmp/reverse.bas"
    >sys "ls tmp"
    reverse.bas
    >

To discard a program, use the `new` command: however you'll be prompt to discard or save the changes in the current program (if it has not been saved yet): for example

    >new

    UNSAVED CHANGES IN CURRENT PROGRAM: DISCARD THEM (Y/N)? Y
    >list
    >

When `list` produces no output, the program is currently empty.

One can load a previously saved program by means of the `LOAD` command, which needs a string with the name (possibly with a path) of the file to load. For example, we could reload our `reverse.bas` program with

    >load "tmp/reverse.bas"

If the program name is not spelt correctly, an error shall occurs, as in

    >load "tmp/reverse"
    CANNOT OPEN FILE
    >

When a program is loaded, the current one is erased to avoid conflicts (and if it has not been saved, then StrayBasic prompts the user to be sure). However, one can also merge a program into the current one. This is useful if one has some subroutines (see below) in a single file and want to include them in a program.

For example, suppose we insert our previous program in the following modified version:

    >new
    >9000 rem Reverse X$ into Y$
    >9010 let y$ = ""
    >9020 for i = 1 to len x$
    >9030 let y$ = x$(i) + y$
    >9040 next i
    >9050 return

This is just a subroutine, executing it will raise errors: first of all, the `X$` variable is not defined before being used in instruction at line 9020:

    >run
    LINE 9020: SYNTAX ERROR
    >

However, also if we define `X$` by adding a line

    >9005 let x$ = "test"

we'd get an error:

    >run
    LINE 9050: RETURN WITHOUT GOSUB

This is due to the fact that the ending `RETURN` statement expect to be executed upon a *go to subroutine* instruction, more on that later. However, let us drop line 9005, save this snippet and start a new program

    >9005
    >save "tmp/reverse.lib"
    >new
    >10 print "Insert a word"
    >20 input x$
    >30 gosub 9000
    >40 print y$
    >

But, on executing it, we get an error

    >run
    Insert a word
    ?test
    LINE 30: ILLEGAL LINE NUMBER

Indeed line 9000 does not exists in our program. So, we merge the previous snippet with

    >merge "tmp/reverse.lib"
    >list
      10  PRINT "Insert a word"
      20 INPUT X$
      30  GOSUB 9000
      40 PRINT Y$
    9000  REM Reverse X$ into Y$
    9010 LET Y$ = ""
    9020 FOR I = 1 TO LEN X$
    9030  LET Y$ = X$(I) + Y$
    9040  NEXT I
    9050  RETURN

Now we can run the program, which prints the correct result but ends with an error:

    >run
    Insert a word
    ?test
    tset
    LINE 9050: RETURN WITHOUT GOSUB
    >

Indeed, we fall through the subroutine at line 9000 after line 40. To prevent them, we stop the program in a previous line, by

    >99 stop
    >run
    Insert a word
    ?test
    tset
    LINE 99: PROGRAM STOPPED
    >

To avoid any message at all, we can use `END` instead of `STOP` (this is a street Basic feature, in classical Basic `END` should be the last instruction of a program):

    >99 end
    >run
    Insert a word
    ?test
    tset
    >

One last remark: when `LOAD`ing a file, the interpreter reads a line at the time and parse it as if it were typed on the keyboard; this means that not only one can insert lines but also execute commands, as one could do in an interactive session.

For example, consider the following "hello" file

    10 print "Hello World!"
    run

Upon loading it, it will be automatically executed:

    >load "hello"
    Hello World!

However these scripts cannot be saved directly from Basic (they can be produced using the file facilities I'll explain later on).

## Basic Basic

In this chapter I'll introduce the basic features of the Basic language provided by StrayBasic.

### Working with numbers

An immediate use of Basic is as a calculator: numbers are native objects of the language which also understands the four arithmetical operations according to the standard algebraic notation. For example, try

    >print 1+2*3
    7
    >print 1*2+3
    5
    >print (1+2)*3 
    9
    >

Each `PRINT` command asks the interpreter to evaluate and print the following expression. Notice that products are executed before sums (the same is true for quotients w.r.t. differences). Invalid operations will cause an error, for example

    >print 1/0
    DIVISION BY ZERO
    >print log(-1) 
    DOMAIN ERROR
    >print sqr(-1) 
    DOMAIN ERROR
    >

Usual exponential, logarithmic and trigonometric functions are available: the list of numerical functions built in StrayBasic is the following one.

- `ABS(x)`  Absolute value of x: x if x>=0, else -x. The result is >= 0.
- `ACS(x)`  Arc whose cosinus is x: defined for -1 <= x <= 1 (else a DOMAIN ERROR occurs), the result is between 0 and 3.14159 (pi).
- `ASN(x)`  Arc whose sinus is x: defined for -1 <= x <= 1 (else a DOMAIN ERROR occurs), the result is >= -1.5708 and  <= 1.5708.
- `ATN(x)`  Arc whose tangent is x, the result is >= -1.5708 and  <= 1.5708.
- `COS(x)`  Cosinus of x, the result is >= -1 and <= 1.
- `EXP(x)`  Exponential of x, the result is > 0.
- `INT(x)`  Integer part of x: the maximum integer less or equal to x, for example `INT(2.9)` is 2 while `INT(-2.1)` is -3.
, the result is > 0.
- `LOG(x)`  The natural logarithm of x, thus the inverse of the exponential function, defined for x > 0 (else a DOMAIN ERROR occurs).
- `SIN(x)`  Sinus of x, the result is >= -1 and <= 1.
- `SQR(x)`  Squared root of x, defined for x >= 0 (else a DOMAIN ERROR occurs), the result is >= 0.
- `TAN(x)`  Tangent of x.

Notice that for trygonometrical functions the argument x has to be expressed in radians, as usual.

Powers are computed via the operator `x^y`, which handles integer exponents, for example:

    >print 2^3
    8
    >print (-2)^3
    -8
    >print 2^0.3
    1.23114
    >print (-2)^0.3
    DOMAIN ERROR
    >print 0^0.1
    0
    >print 0^0
    DOMAIN ERROR
    >

Numbers in Basic are decimal ones, which may also display an exponential part: for example

    >print .001, 0.001, 1e-3
    0.001          0.001           0.001
    >

Numbers are stored in a fixed precision floating point representation, with 4 bytes, so the expected precision is up to 6 or 7 digits. For example

    >print 1 + 1e-1 
    1.1
    >print 1 + 1e-2
    1.01
    >print 1 + 1e-3
    1.001
    >print 1 + 1e-4
    1.0001
    >print 1 + 1e-5
    1.00001
    >print 1 + 1e-6
    1
    >


Of course, an algebraic expression usually has variables as operands rather than numbers: however, Basic always tries to evaluate the expression, so that values should be assigned to variables. For example

    >print sqr(x^2 + y^2) 
    UNDEFINED VARIABLE
    >let x = 3, y = 4
    >print sqr(x^2 + y^2) 
    5
    >

The `LET` instruction assign a value to a variable, which may exists in advance or, if not, it is created by the `LET` statement itself. StrayBasic allows lists of comma separated such assignments.

Actually, as in most street Basics, the `LET` keyword can be omitted, even if classical Basic forbids that.

In any case, we can evaluate an expression containing variables only if all those variables have assigned values. A variable's name should start with a letter, followed by letters or digits: valid names are `a`, `a1`, `a11`, `a1a1`, `a1a` etc. Variable's names are case insensitive and always converted to uppercase.

One can overwrite the current value of a variable reassigning it:

    >let x = 3, y = 4
    >print sqr(x^2 + y^2) 
    5
    >let x = 5
    >print sqr(x^2 + y^2) 
    6.40312
    >let y = 6
    >print sqr(x^2 + y^2) 
    7.81025
    >

If an expression is used time and again in a program, one can define it as a function, by means of a definition that has to be inside a program. For example consider

    >10 def fnh(x,y) = sqr(x^2 + y^2)
    >20 let x = 1, y = 2
    >30 print fnh(x,y)
    >40 let x = x + 1, y = y + 2
    >50 print fnh(x,y)
    >run
    2.23607
    4.47214
    >

Classical Basic and most street Basics require that the name of a user defined function is `FNx` where `x` is any letter, while StrayBasic allows for any identifier.

### Jumps

Algorithms require in general the possibility to change the execution flow: in Basic this is done by jumps (conditional and inconditional) and loops.

One can jump according to a condition: for example consider

    >10 let x = 0
    >20 print x, x^2
    >30 let x = x + 1
    >40 if x <= 10 then 20
    >run
    0              0
    1              1
    2              4
    3              9
    4              16
    5              25
    6              36
    7              49
    8              64
    9              81
    10             100
    >

The first three instructions are executed in a sequence, while when we get at instruction on line 40, the condition `X <= 10` is tested: that means "is the current value of X less or equal o ten?". If the answer is "yes", thus the condition is true, the execution continues from the line quoted after the `THEN` keyword. Else, if the answer is "no", thus the condition is false, the jump is not performed and execution continues to the next line, after the one containing the `IF`. This is a *conditional jump*.

Notice instruction 30: therein, on writing `x = x + 1` the equality token doesn't denote the mathematical equality relation, but it denotes the assignment operator: "evaluate the expression on my right and store the result in the variable on my left, overwriting its old value".

Valid relations among numbers are:

- `x = y` true if x is equal to y, false otherwise.
- `x <> y` true if x is not equal to y, false otherwise.
- `x < y` true if x is less than y, false otherwise.
- `x >= y` true if x is not less than y, false otherwise.
- `x > y` true if x is greater than y, false otherwise.
- `x <= y` true if x is not greater than y, false otherwise.

The "true" and "false" values are in Basic 1 and 0, thus genuine numbers. Moreover, such relations may be used inside expressions, but they have less priority than any other operator.

For example:

    >print 1 < 1 + 2
    1
    >print (1 < 1) + 2
    2
    >print (1 <= 1) + 2
    3
    >

One can connect relation results (indeed any pair of numbers) via the classical Boolean connectives:

- `x AND y` which is 1 if both x and y are not zero, else it is zero.
- `x OR y` which is 1 if either x or y are not zero, else it is zero.
- `NOT x` which is 1 if x is zero, else it is 0.

In particular, `NOT x` and `x <> 0` have the same effect.

It is important to stress that, unlike the current programming languages, `AND` and `OR` are not shortcircuit operators, thus they *always* evaluate both operands. For example,

    >10 let x = 0
    >20 print x <> 0 and 1/x
    >run
    LINE 20: DIVISION BY ZERO
    >

You should use an `IF` instead, as in

    >10 let x = 0
    >20 if x <> 0 then print 1/x      
    >run
    >

Indeed, after a `THEN` not only may appear a line number were to jump, but, alternatively, an instruction to execute in case the `IF` condition is true. In this case, the condition is false so the `PRINT 1/X` is not executed at all.

One can also insert a list of instructions separated by colons: consider

