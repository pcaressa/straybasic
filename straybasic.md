# Programming with StrayBasic

### A street Basic imaginary dialect

#### (c) 2024 by Paolo Caressa

## Foreword

In their book *Back to Basic*, mathematicians and computer scientists John Kemeny and Thomas Kurtz describe the design and implementation of the Basic language at Dartmouth College in the early 60s. They also criticise the Basic dialects of the 70s, from the seminal version of Gates, Allen and Davidoff on, that sacrified the language cleanness on the altair of execution speed and, most of all, memory saving.

Basic was, at the time, the only high level language capable to be implemented on small computers, with slow 8-bit CPUs and no more than 64K of total memory, both for the ROM where the interpreter was stored and for the RAM where user programs and data were stored. Kemeny and Kurtz, that in the meanwhile erected Basic to the same level of the 70s languages such as Pascal, called *Street Basics* those implementations that worked with so limited resources. Indeed, the Basic of these micro-computers were more difficult to read, almost impossible to maintain and full of device-oriented tricks to achieve goals such as graphics, sounds and game programming (that eventually, due to the slowness of the interpreters, were programmed in assembly).

It is by means of these street Basics that people like me had their first contact with computers and programming, and became the generation that EW Dijkstra complained to be *mentally mutilated beyond hope of regeneration* due to their exposure to Basic. But everyone could program with those small computers, some million times less powerful that nowadays laptops.

StrayBasic is a hommage both to the visionary ideas of Kemeny and Kurtz, whose aim was to let everyone to program, and to the programmers and engineers who wrote dozens, hundreds of basic interpreters for the most diverse machines, among which also the quite evolute Basic interpreter of the Sinclair ZX Spectrum with which I started more than four decades ago.

Most commands of the Kemeny-Kurtz 60s Basic are part of StrayBasic, most resembles street Basic dialects, and any Basic program can be easily ported to StrayBasic: ùelementary programs from the classical version of the language work almost with no change. The most striking differences are listed in an appendix to this document. On the other hand, StrayBasic also allows to reproduce the crazy and joyful improvisation of street basic programs, and the need for cleverly assemble data and code due to lack of resources.

While classical Basic was compiled, street Basic was interpreted as StrayBasic is: moreover, the source code is stored in a compressed form which is directly interpreted, as for most interpreters of the 70s. Therefore, the language trades time for space and it is slow, according to modern standards, and yet incredibly fast according to 70s and 80s standards. The interpreter manage up to 64Kb where both programs and data are to be stored, reproducing the constrained environments of the 8/16 bit era. Also the techniques I used were the ones of those street Basic interpreters.

The original idea of this project was to have an interpreter versatile enough to show the different aspects of the language to the audience of my talk at Codemotion Milan 2024. The talk had some success, mainly due to the nostalgia effect, and I decided to craft the interpreter to a complete version of Basic, very similar to most dialects and yet different from anyone of these.

But to program in Basic was also to port from a dialect to another one: in the GitHub repository there are some examples of programs ported, with minor changes, to StrayBasic, such as a famous version of the Eliza bot.

This is an eminently useless project, the ones I love the most to deal with: hope someone else will enjoy it.

## Setting up and using the system

### Compiling and launching StrayBasic

StrayBasic is written in C for Linux: in particular, to perform some print trick needed by any Basic interpreter, it assumes to run inside a terminal and uses escape codes to perform some rough graphical effects. The code is contained in a single file(!) `straybasic.c` with some data inside the file `straybasic.h` and can be compiled, inside a folder containing both, with

    $ clang -lm straybasic.c -o straybasic

A lot of warnings will be issued, to avoid them compile adding the `-Wno-pointer-sign` switch. In any case, the result will be an executable `straybasic` with which one can execute a Basic source as in

    $ straybasic sample.bas

of just enter the REPL mode by launching the program:

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

Since Basic is a case-insensitive language, you can insert identifiers, such as `print`, with no care about letter cases, for example `Print`, `PRINT` or even `pRiNt` are all converted to `PRINT` by the interpreter, before being processed. Only text inside strings and comments is preserved as it is inserted, while other text is tokenized and converted to uppercase.

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

Notice that the interpreter has converted to uppercase each keyword and identifier and also that the listing is in increasing order w.r.t. line numbers, thus the program is shown with the lines in their natural ordering, the one in which, from the first to the latter, will be executed. Also, the interpreter saves the program in an inner form which is converted back to printable one when listing the program: in this back and forth transformation additional spaces are lost or inserted by the interpreter itself.

To execute the program just type `run`: after that, the `INPUT` instruction at line 20 will make the program to pause and prompt you to insert a text; type a word and then press ENTER (or RETURN or whatever on your keyboard), then the program will transform the word you typed, stored into variable `X$`, into the word whose letters have been reversed, stored into `Y$`:

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

Upon listing with `list` we get:

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

To discard a program, use the `new` command: however you'll be prompted to discard or save the changes in the current program (if it has not been saved yet): for example

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

This is a *subroutine* (more on that later), but executing it will raise errors: first of all, the `X$` variable is not defined before being used in instruction at line 9020:

    >run
    LINE 9020: SYNTAX ERROR
    >

However, also if we define `X$` by adding a line

    >9005 let x$ = "test"

we get an error:

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
      10 PRINT "Insert a word"
      20 INPUT X$
      30 GOSUB 9000
      40 PRINT Y$
    9000 REM Reverse X$ into Y$
    9010 LET Y$ = ""
    9020 FOR I = 1 TO LEN X$
    9030 LET Y$ = X$(I) + Y$
    9040 NEXT I
    9050 RETURN

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

### Numbers: expressions and variables

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

Numbers are stored in a fixed precision floating point representation, with 4 bytes, so the expected precision is up to 6 digits. For example

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

The `LET` instruction assigns a value to a variable, which may exists in advance or, if not, it is created by the `LET` statement itself. StrayBasic allows lists of comma separated such assignments.

Actually, as in most street Basics, the `LET` keyword can be omitted, even if classical Basic forbids that.

In any case, we can evaluate an expression containing variables only if all those variables have assigned values. A variable's name should start with a letter, followed by letters or digits: valid names are `a`, `a1`, `a11`, `a1a1`, `a1a` etc. Variable's names are case insensitive and always converted to uppercase. Moreover, *variable names cannot be equal to any language keyword*: the list of keywords, denoting instructions or functions, is the following one:

    ABS ACS AND ASC ASN AT ATN ATTR BYE CHAIN CHR$ CLEAR CLOSE CLS CONTINUE COS DATA DEF DIM DUMP END ERROR EXP FOR GO GOSUB GOTO IF INKEY$ INPUT INT LEFT$ LEN LET LINPUT LIST LOAD LOG MERGE MID$ NEW NEXT NOT ON OPEN OR PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RIGHT$ RND RUN SAVE SGN SIN SKIP SQR STEP STOP STR$ SUB$ SYS TAB TAN THEN TO TRACE VAL

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

Classical Basic and most street Basics require that the name of a user defined function is `FNx` where `x` is any letter, while StrayBasic allows any identifier.

### Conditions and Jumps

Algorithms require in general the possibility to change the execution flow: in Basic this is done by jumps (conditional and inconditional) and loops.

One can jump according to a condition: this is usual when expressing algorithms as lists of enumerated points, for example as in the following description of an algorithm to print the squares of numbers from 1 to 10:

1. Set the variable x = 1.
2. Print x and its square.
3. Increase x by 1.
4. If x <= 10 then repeat from 2

which in Basic is translated verbatim as

    >10 let x = 1
    >20 print x, x^2
    >30 let x = x + 1
    >40 if x <= 10 then 20
    >run
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

Notice instruction 30: therein, on writing `x = x + 1` the equality token doesn't denote the mathematical equality relation, but it denotes the assignment operator: "evaluate the expression on my right and store the result in the variable on my left, overwriting its old value". Any occurrence of `x` in the expression on the right of `=` denotes the current value of `x`, before the reassignment which is performed after the value of the expression is got and substituted to the current value.

Valid and obvious relations among numbers are:

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

Boolean operators in StrayBasic are not bitwise operators, since there are not integer numbers in the language: rather they always produce either 0 or 1 according to the previous rules. Moreover, it is important to stress that, unlike the current programming languages, `AND` and `OR` *are not short-circuit operators*, thus they *always* evaluate both operands. For example,

    >10 let x = 0
    >20 print x <> 0 and 1/x < 1
    >run
    LINE 20: DIVISION BY ZERO
    >

The operand `1/x < 1` is computed even if the first operand `x <> 0` is false: this was common in pre-C languages (actually in languages ignoring Algol-68).

To get the shortcircuit effect just nest `IF`s, for example

    >10 let x = 0
    >20 if x <> 0 then if 1/x < 1 then print "ok"
    >run
    >10 let x = 2
    >run
    ok
    >

Indeed, after a `THEN` not only may appear a line number were to jump, but, alternatively, an instruction to execute in case the `IF` condition is true. In this case, if the `x <>0` condition is false, the nested `if 1/x < 1` is not executed at all.

In addition to conditional jumps, Basic also provides unconditional jumps, which are mainly implemented via the infamous `GOTO` statement. For example:

    >10 let x = 1 
    >20 if x < 0 then 100 
    >30 if x = 0 then 200
    >40 goto 300
    >100 print "Negative"
    >110 goto 400
    >200 print "Zero"
    >210 goto 400
    >300 print "Positive"        
    >400 end

This is quite cumbersome, however it shows the combined use of conditional and unconditional jumps. Each jump after a `THEN` is to a line number which execute an action and then jump to a final line, in this case the last.

One could rewrite this snippet in a simpler way by using several Basic features. First of all, we can nest several Basic instruction on a same line by separating them with a `:` symbol; that can be done also after an `IF` instruction, so that we can rewrite the program as

    >10 let x = 1
    >20 if x < 0 then print "Negative": goto 99
    >30 if x = 0 then print "Zero": goto 99
    >40 if x > 0 then print "Positive"
    >99 end
    >run
    Positive
    >10 let x = 0
    >run
    Zero
    >10 let x = -1
    >run
    Negative

In this case, because of the *trichotomy* property of the number ordering, the `GOTO 99` are superfluous, since the conditions are mutually exclusive.

Suppose however the need to check against a discrete number of values, for example:

    >10 LET X = 3
    >20 IF X = 0 THEN PRINT "Zero": GOTO 100
    >30 IF X = 1 THEN PRINT "One": GOTO 100
    >40 IF X = 2 THEN PRINT "Two": GOTO 100
    >50 IF X = 3 THEN PRINT "Three": GOTO 100
    >60 PRINT "I cannot name it!"
    >100 END
    >run
    Three
    >

When such multiple `GOTO`s are on order (which occurs when in modern languages we would use a switch or match instruction) one can take advantage of the `ON` instruction, stemming from classical Basic: its general form is

    ON E GOTO L1, ..., Ln

and it is executed in this way:

- Expression E is evaluated to a number x
- The integer part i of the number x is considered
- If i >= 1 and i <= n then GOTO Li is executed, else an error occurs.

The previous snippet can be rewrited as

    >10 LET X = 3
    >20 IF X < 0 OR X > 3 THEN 150
    >30 ON X + 1 GOTO 100, 110, 120, 130
    >100 PRINT "Zero": GOTO 200
    >110 PRINT "One": GOTO 200
    >120 PRINT "Two": GOTO 200
    >130 PRINT "Three": GOTO 200
    >150 PRINT "I cannot name it!": GOTO >200
    >200 END
    >run 
    Three

The check at line 20 is needed to prevent an error; for example, if we delete line 20 we get

    >10 let x = 4
    >20
    >list
    >list
      10 LET X = 4
      30 ON X + 1 GOTO 100, 110, 120, 130
     100 PRINT "Zero": GOTO 200
     110 PRINT "One": GOTO 200
     120 PRINT "Two": GOTO 200
     130 PRINT "Three": GOTO 200
     150 PRINT "I cannot name it!": GOTO 200
     200 END
    >run 
    LINE 30: ON VALUE OUT OF LIST

The wildest street Basic just passed execution on in these cases, I preferred a more rigorous approach.

Some Basics, as the ZX Spectrum one, didn't provide the `ON` instruction but allowed for *any* expression as argument of a `GOTO`, so that one could use *computed goto*, a mainteinance nightmare. StrayBasic provides them, so we can write

    >10 LET X = 3
    >20 IF X < 0 OR X > 3 THEN 150
    >30 GOTO 100 + X * 10
    >100 PRINT "Zero": GOTO 200
    >110 PRINT "One": GOTO 200
    >120 PRINT "Two": GOTO 200
    >130 PRINT "Three": GOTO 200
    >150 PRINT "I cannot name it!": GOTO 200
    >200 END
    >run
    Three
    >

Two final remarks on the `GOTO` instruction as implemented by the StrayBasic interpreter:

- The line figuring after the `GOTO` (or the result of the expression after it) should be a line existing in the program: if we write `GOTO 110` then line 110 has to exist, else an error occurs (some street Basics pointed to the first line with number greater or equal to the line number after `GOTO`).
- One can use the form `GO TO` instead of `GOTO`.

### For loops

A typical use for jumps is in creating loops, thus chunks of code that are repeated a certain number of times, or according to some condition.

StrayBasic provides a specific instruction for the first kind of loop, as classic and street Basics all did. In short, the sequence of instructions

    1000 FOR v = e1 TO e2 STEP e3
         ...
    1100 NEXT v
    1110 ...

when the value of e3 is positive is equivalent to

    1000 LET v = e1, to = e2, step = e3
    1001 IF v > to THEN 1110
         ...
    1100 LET v = v + step: GOTO 1001
    1110 ...

Instead, when the value of e3 is negative the `FOR` loop is equivalent to

    1000 LET v = e1, to = e2, step = e3
    1001 IF to < v THEN 1110
         ...
    1100 LET v = v + step: GOTO 1001
    1110 ...

The case when e3 evaluates to 0 gives rise to an infinite loop, since the `LET` instruction on line 1100 would not alter v, so that the condition on line 1001 would be always (or never) fulfilled.

Notice that the previous equivalent form for the `FOR` instruction are not Basic snippets, since the name of the variable assigned to the values of e2 and e3 are not legal Basic names, since they are keywords.

For example, the following program prints the odd numbers from 1 to 99:

    >10 for i = 1 to 99 step 2
    >20 print i,
    >30 next i
    >run

### Some examples

Until now, we have introduced a few instructions, but we can already set up non trivial tasks in Basic. I will show some simple program here, using the part of Basic we learned till now.

#### Fibonacci numbers

As a first example, let us compute the n-th Fibonacci number: recall that Fibonacci numbers are the sequence obtained by the following recursive relations (recall that a sequence is just a functions from the natural numbers to another set, hence we use the F(n) notation)

    F(0) = 1
    F(1) = 1
    F(n) = F(n-1) + F(n-2)

so the sequence is 1, 1, 2, 3, 5, 8, 13, 21, ... Let us write a program to compute and print F(n):

```
  10 REM Compute Fibonacci numbers
  20 INPUT "N =", N
  30 LET E = 1
  35 LET F = 1
  40 LET I = 2
  50 LET E1 = F
  60 LET F = E + F
  70 LET E = E1
  80 LET I = I + 1
  90 IF I < N THEN 50
 100 PRINT "F(";N;") = ";F
```
The statement `REM` is just a comment and it is ignored during program execution. At line 20, the program uses the `INPUT` statement to print a prompt and ask for the value of a numerical variable N. Next, the program keeps on computing elements of the sequence until the N-th has been computed; then, it prints this number. The variable E1 is used at line 50 to save the value of F which is overwritten at line 60 but needed as new value of E at line 70. So, E = F(I-1) and F = F(I) and F(I+1) will be computed as E + F.

This loop uses a conditional jump and can be conveniently substituted by a FOR-loop:

```
  10 REM Compute Fibonacci numbers
  20 INPUT "N =", N
  30 LET E = 1
  35 LET F = 1
  40 FOR I = 2 TO N
  50    LET E1 = F
  60    LET F = E + F
  70    LET E = E1
  80 NEXT I
 100 PRINT "F(";N;") = ";F
```

(the indentation will be lost when the lines will be typed in the interpreter). 

#### Euclid Algorithm

A classical algorithm is the Euclid one, used to compute the greatest common divisor gcd(n,m) of two positive integer numbers n and m.

We can set up a simple version of this algorithm by applying two properties of gcd(n,m):

1. gcd(n,n) = n
2. if n > m, then gcd(n,m) = gcd(n-m,m)

The idea is to apply 2 to subtract n from m if n >m (or m from n if m >n), until n = m, in which case we got the gcd as n = m.

```
  10 REM Compute GCD
  15 REM Input N, M and check them to be positive integers.
  20 INPUT "N =", N
  25 IF N <> INT(N) OR N < 1 THEN PRINT "Please insert a positive integer": GOTO 20
  30 INPUT "M =", M
  35 IF M <> INT(M) OR M < 1 THEN PRINT "Please insert a positive integer": GOTO 30
  40 PRINT "GCD("; N; ","; M; ") = ";
  50 IF N < M THEN LET M = M - N: GOTO 50
  55 IF N > M THEN LET N = N - M: GOTO 50
  60 PRINT N
```

Notice that on lines 25 and 35, we used a more complex form of the `IF` statements, that allows after the `THEN` not only a line number where to jump, but also a sequence of statement to execute if the `IF` condition is true, separated by colons. Indeed we want a message to be printed and then perform an unconditional jump to repeat the number input.

Notice also that, to check if a number is an integer, we use the formula `n = INT(n)`. Indeed, the `INT` function takes the integer part of a number, thus the least integer number not exceeding that number. For example, `INT(2.1) = INT(2.9) = 2`, but `INT(-2.1) = INT(-2.9) = -3`. However, since our numbers ought to be positive, they are integer exactly when they do coincide with their integer part.

The `PRINT` statement at line 40 is somewhat complex: it prints strings and numbers separater by semi-colon; when two data on a `PRINT`-list are separated by semi-colon, they are printed with no spaces in between. Moreover, there's a final semi-colon, that asks the interpreter not to issue a new line but let the next `PRINT` instruction to continue just after the last printed item. Try to drop this lasting semi-colon to see what happens in this case.

The Euclid algorithm consists just in the two lines 50 and 55: if both the `IF` conditions are false, then it means that n = m, so we have our gcd.

Notice that this loop cannot be written as a `FOR` loop, since we do not know in advance how many iterations the loop will require: it is an undefinite loop that in most languages could be expressed with a `while` statement, lacking in classical and in most street Basics, therefore also in StrayBasic.

**Exercise for the reader** Combine the programs for the Fibonacci numbers and the GCD to compute the gcd of two Fibonacci numbers: try to compute some gcds and formulate a conjecture about all that.

#### Monte Carlo computation of pair

The classic Monte Carlo method, invented more or less when computers were, can be used in a simple form to approximate the pi number. Recall that pi is the ratio between the circumference of a circle and its diameter. How can we approximate it?

Imagine to take a square sheet with side 2 meters and to inscribe a circle in it, thus a circle with diameters of 2 meters that intersect the square in four tangent points, in the middle of each square side.

Now take the sheet outside and wait for the rain. After leaving the square under the rain for a while, take it back inside and count the number S of raindrops that fell inside the square and those C that fell inside the circle: since the circle is included in the square, we have S > C.

Now, Monte Carlo methods tells us that, as the number of raindrops increases, the ratio S/C will approximate better and better the ratio A/B of the area of the square divided by the area of the circle. But the area of the square is L^2, being L its side, and the area of the circle is pi*R^2, being R its radius; since the diameter of the circle is L = 2, we have R = L/2 = 1, hence

    A/B = L^2/(pi*R^2) = 4/(pi*1^2) = 4/pi

This is approximate by S/C, so that S/C = 4/pi, therefore pi = 4*C/S.

Let us write a Basic program that simulate this experiment. We describe the circle in Cartesian coordinates (x,y) as the set of points inside the curve of equation x^2 + y^2. A point (x,y) is in the square centered in the origin and with side 2 if |x| <= 1 and |y| <= 1. The function `ABS(x)` computes the absolute value |x|.

    10 REM Monte Carlo method to compute pi
    20 INPUT "Number of raindrops"; N
    30 LET C = 0
    40 FOR S = 1 TO N
    45 REM Extract a pair of random number >= -1 and <= 1
    50 LET X = 1 - 2 * RND
    55 LET Y = 1 - 2 * RND
    60 IF X ^ 2 + Y ^ 2 <= 1 THEN C = C + 1
    70 NEXT S
    80 PRINT "pi = "; 4 * C / S; " (circa)"

Notice that the `RND` built-in functions does not take any parameter at all, but it returns a pseudo-random number between 0 and 1, which we transform in a number between -1 and 1.

This is quite a poor method to compute the pi number: these are some results from the previous program:

    >run
    Number of raindrops?10
    pi = 2.90909 (circa)
    >run
    Number of raindrops?100
    pi = 2.81188 (circa)
    >run
    Number of raindrops?1000
    pi = 3.13287 (circa)
    >run
    Number of raindrops?10000
    pi = 3.12849 (circa)
    >run 
    Number of raindrops?100000
    pi = 3.14245 (circa)
    >run
    Number of raindrops?1000000
    pi = 3.14307 (circa)
    >run
    Number of raindrops?10000000
    pi = 3.14219 (circa)
    >

We need to repeat 100,000 times the loop to get the correct first two decimal digits, and we could not fix the third even runnin ten millions of iterations.

Moreover, we can simplify the program by noticing that, on supposing the raindrops to be uniformly distributed, we can use just a slice of the circle to perform the computation. For example, let us concentrate on the upper right quarter, thus 0 <= x <= 1 and 0 <= y <= 1. Thus we count raindrops on a quarter of the square and a quarter of the circle, equation above becomes

    S/C = (A/4)/(B/4) = 1 / (pi/4)

so we still have pi = 4*C/S. Let us implement this:

    10 REM Monte Carlo method to compute pi
    20 INPUT "Number of raindrops"; N
    30 LET C = 0
    40 FOR S = 1 TO N
    45 REM Extract a pair of random number >= 0 and <= 1
    50 LET X = RND
    55 LET Y = RND
    60 IF X ^ 2 + Y ^ 2 <= 1 THEN C = C + 1
    70 NEXT S
    80 PRINT "pi = "; 4 * C / S; " (circa)"

To make the program faster, we could drop lines 50 and 55 and use directly `RND` instead of `X` and `Y` in the formula at line 60:

    60 IF RND ^ 2 + RND ^ 2 <= 1 THEN C = C + 1

Moreover, we could also avoid the `IF` using the value of the Boolean expression in it as the number to add to `C` in any case (if the expression is false, we are adding 0 to `C`):

    60 LET C = C + (RND ^ 2 + RND ^ 2 <= 1)

The final version of the program is

    10 REM Monte Carlo method to compute pi
    20 INPUT "Number of raindrops"; N
    30 LET C = 0
    40 FOR S = 1 TO N
    45 REM Extract a pair of random number >= 0 and <= 1
    60 LET C = C + (RND ^ 2 + RND ^ 2 <= 1)
    70 NEXT S
    80 PRINT "pi = "; 4 * C / S; " (circa)"

Let us try it:

    >run
    Number of raindrops?10
    pi = 2.90909 (circa)
    >run
    Number of raindrops?100
    pi = 3.08911 (circa)
    >run
    Number of raindrops?1000
    pi = 3.15285 (circa)
    >run
    Number of raindrops?10000
    pi = 3.16648 (circa)
    >run
    Number of raindrops?100000
    pi = 3.13885 (circa)
    >run
    Number of raindrops?1000000
    pi = 3.14192 (circa)
    >run
    Number of raindrops?10000000
    pi = 3.1409 (circa)
    >

## More Basic

### Arrays

As most street Basics, StrayBasic allows for vector and matrices, but not for higher order tensors.

Vector and matrices always need to be declared before being used by the `DIM` statement: for example,

    10 DIM V(10), A(10,20)

declares a vector `V` with 10 elements and a matrix `A` with 10 rows and 20 columns. Elements of a vector are labelled by an index running from 1 to the size of the vector, and similarly for matrices.

(If you don't know what a vector or matrix are, stop reading this stuff and turn to a linear algebra book: if you'll insist on reading me, imagine a vector as an ordered list of a fixed number of numbers and a matrix as a rectangular table of numbers).

Once declared, elements of a vector or of a matrix can be assigned, for example:

    10 DIM V(10), A(10,20)
    20 LET V(1) = 1
    30 LET A(2,3) = 2

In general, to assign a value to each element a `FOR` loop is used: for example, let us rewrite our Fibonacci program:

    10 DIM F(29)
    20 LET F(1) = 1
    30 LET F(2) = 1
    40 FOR I = 3 TO 29
    50 LET F(I) = F(I-2) + F(I-1)
    60 NEXT I
    70 PRINT F(29)

We can generalize this program to let it compute any Fibonacci number:

     5 INPUT "N="; N
    10 DIM F(N)
    20 LET F(1) = 1
    30 LET F(2) = 1
    40 FOR I = 3 TO N
    50 LET F(I) = F(I - 2) + F(I - 1)
    60 NEXT I
    70 PRINT F(N)

Indeed, a `DIM` statement accepts any expression as vector or matrix size (this was common for street Basic but not for classical Basic, being the latter compiled and not interpreted).

However, since StrayBasic works in a 64Kb virtual machine, it is easy to go out of memory: for example

    >dim a(200,100)
    NO MORE ROOM FOR VARIABLES
    >

Indeed, a number takes 4 bytes, so that data for a 200x100 array amounts to 80,000 bytes, that exceeed the total 65,536 bytes memory of the virtual machine.

To check available memory use the `DUMP` statement, that prints information on the state of the system: just after the interpreter has been launched this is its output:

    >dump
    KEYWORDS:
     ATTR BYE CHAIN CLEAR CLOSE CLS CONTINUE DATA DEF DIM DUMP END ERROR FOR GO GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
     * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY OCCUPATION:
     STRINGS   =     0 /  4096 ( 0%)
     PROGRAM   =     0 /  8192 ( 0%)
     VARIABLES =     0 / 51532 ( 0%)
    MEMORY MAP:
     | strings | program | variables | free space | stacks | buffers |
     0000      1000      3000        3000         F94C     FA00      FFFF
    STRING CONSTANTS

    VARIABLES:
    >

The `DUMP` instruction lists all statement keywords and all operators available in the Basic interpreter. Next, it prints how much memory is occupied / reserved for strings and identifiers, program and variables. Lastly, the list of all variables is printed.

Let us define a matrix and see what happens:

    >dim a(100,100)
    >dump
    KEYWORDS:
     ATTR BYE CHAIN CLEAR CLOSE CLS CONTINUE DATA DEF DIM DUMP END ERROR FOR GO GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
     * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY OCCUPATION:
     STRINGS   =     2 /  4096 ( 0%)
     PROGRAM   =     0 /  8192 ( 0%)
     VARIABLES = 40009 / 51532 (77%)
    MEMORY MAP:
     | strings | program | variables | free space | stacks | buffers |
     0000      1000      3000        CC49         F94C     FA00      FFFF
    STRING CONSTANTS
     "A"
    VARIABLES:
     A(100,100)
    >

Our 100x100 matrix occupies the 77% of available space for variables.

One can set the space assigned to strings, program and variables by means of the `CLEAR` statement, which can be used as

    CLEAR s, p

to assign s bytes for strings, p bytes for the program (the space for variables is just the remaining one). If a number is missing, then the corresponding size is not altered, for example, type

    >CLEAR 1024, 16384
    >DUMP
    KEYWORDS:
     ATTR BYE CHAIN CLEAR CLOSE CLS CONTINUE DATA DEF DIM DUMP END ERROR FOR GO GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
     * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY OCCUPATION:
     STRINGS   =     0 /  1024 ( 0%)
     PROGRAM   =     0 / 16384 ( 0%)
     VARIABLES =     0 / 46412 ( 0%)
    MEMORY MAP:
     | strings | program | variables | free space | stacks | buffers |
     0000      0400      4400        4400         F94C     FA00      FFFF
    STRING CONSTANTS

    VARIABLES:
    >

Keep in mind that, when resizing either the constant string area or the program area, all data is lost, so that you cannot do that inside a program.

If `CLEAR` has no parameter at all, then its effect is only to delete all variables.

To delete both strings, program and variables, use `NEW`: since also identifiers are stored as strings, one cannot drop them and preserve the program at once, that's why `CLEAR` leaves them untouched.

### Read data and restore them

Suppose you want to initialize all elements of a matrix: the standard way in Basic is to put the elements of the matrix, row-wise, in a sequence of `DATA` statements and then read those elements. For example:

      10 DIM A(10, 10)
      20 FOR I = 1 TO 10
      30 FOR J = 1 TO 10
      40 READ A(I, J)
      50 NEXT J
      60 NEXT I
     100 FOR I = 1 TO 10
     110 FOR J = 1 TO 10
     120 PRINT A(I, J); " ";
     130 NEXT J
     135 PRINT
     140 NEXT I
    1000 DATA 1,2,0,0,0,0,0,0,0,0
    1001 DATA -2,1,2,0,0,0,0,0,0,0
    1002 DATA 0,-2,1,2,0,0,0,0,0,0
    1003 DATA 0,0,-2,1,2,0,0,0,0,0
    1004 DATA 0,0,0,-2,1,2,0,0,0,0
    1005 DATA 0,0,0,0,-2,1,2,0,0,0
    1006 DATA 0,0,0,0,0,-2,1,2,0,0
    1007 DATA 0,0,0,0,0,0,-2,1,2,0
    1008 DATA 0,0,0,0,0,0,0,-2,1,2
    1009 DATA 0,0,0,0,0,0,0,0,-2,1

`DATA` instructions can appear anywhere in the program, however the runtime keeps track of the last item read, so that the next `READ` instruction will read from the next `DATA` item, or from the first item of the following `DATA` if all items in the current `DATA` have been read.

Of course, if you keep on reading also when the last item of the last `DATA` has been read, an error will occurr.

    >10 READ A: PRINT A: GOTO 10
    >20 DATA 1, 2, 3, 4, 5
    >RUN
    1
    2
    3
    4
    5
    LINE 10: OUT OF DATA
    >

One can reset the pointer to the next `DATA` statement to read from, via the `RESTORE` statement, that resets the `DATA` pointer, or set it to a given line.

For example, the following is a genuine infinite loop:

    10 RESTORE: READ A: PRINT A;: GOTO 10
    20 DATA 1, 2, 3, 4, 5

It'll print an infinite sequence of 1.

To break the program, which can be useful in case of infinite loop, just press Ctrl-C.

Another, rather weird, example of use of `RESTORE`

    10 FOR I = 1 TO 100
    20 RESTORE 100 + INT( RND * 10)
    30 READ A
    40 PRINT A,
    50 NEXT I
    100 DATA 1
    101 DATA 2
    102 DATA 3
    103 DATA 4
    104 DATA 5
    105 DATA 6
    106 DATA 7
    107 DATA 8
    108 DATA 9
    109 DATA 10

This select a random row and restore to it, so that the `READ` statement will read the corresponding number, and repeat. Remember, indeed, that almost everywhere a number is expected, an expression can appear.

### Strings

Basic was perhaps the first language to allow for string manipulation much as like we intend it today.

A string is enclosed between double quotes: there are no escape characters and there's no possibility to insert a double quote inside a string (on the contrary, almost all Basics allowed a double double quote to count as a single quote inside a string).

Examples:

    >print "Alpha"
    Alpha
    >print "I forgot a double quote
    END OF LINE INSIDE STRING
    VALUE EXPECTED
    >print "This won't work: ""Cannot nest quotes"""
    This won't work: Cannot nest quotes
    >

Notice the last one: the sequence `"This won't work: ""Cannot nest quotes"""` is interpreted as three consecutive strings: `"This won't work: "`, `"Cannot nest quotes"` and the empty string `""`. Indeed, print concatenates the result of strings expressions.

A string variable terminates with a dollar sign, and can be assigned as any other variable:

let a$ = "Alpha"
let b$ = "Bravo"
let x$ = a$

One can combine strings, both constants and variables, by the *concatenation* operator, which is denoted by the plus sign `+`. As an aside, I find this notation misleading, since `+` suggests a commutative operation, while string concatenation is not: wisely, when Kemeny and Kurtz introduced string concatenation in classical Basic, they used the `&` sign to denote it: that's why StrayBasic allows also `&` for the concatenation:

    >let s$ = "Basic"
    >let v$ = "is"
    >let o$ = "a programming language"
    >print s$ + " " + v$ + " " + o$
    Basic is a programming language
    >print s$ & " " & v$ & " " & o$
    Basic is a programming language

(Notice that concatenation is associative, so there's no need of parentheses when using it time and again.)

After a string one can add a *subscript* to select part of it, as in the Sinclair Basic and ECMA Basic: such a subscript is a given by a pair of indexes pointing to the string characters, separated by the `TO` keyword (neither comma nor colon). For example

    >print "123456789"(3 to 4)
    34
    >print "123456789"(3,4)
    "TO" EXPECTED
    >print "123456"(3:4)
    "TO" EXPECTED
    >let x$ = "123456789"
    >let i1 = 3
    >let i2 = 4
    >let y$ = x$(i1 to i2)
    >print y$
    34
    >

String characters are indexed from 1 to the length of the string, that can be retrieved by the `LEN` built-in function:

    >let x$ = "123456789"
    >print x$(5 to len x$)   
    56789
    >

If in a subscript `(i1 TO i2)` i1 is 1 it can be omitted and if i2 is the length of the string it can be omitted, but both cannot be omitted:

    >print x$(to 4)
    1234
    >print x$(5 to)
    56789
    >print x$(to)
    VALUE EXPECTED
    >

Notice however that

    print x$(4)
    4
    >

does work thus `x$(i)` is the string which has just one character, the `i`-th character of `x$`. One can also assign such a character, for example

    >let x$ = "123456789"
    >let x$(1) = "0"
    >print x$
    023456789
    >

However one cannot assign a part of a string longer than one character, as in the Sinclair Basic: thus, `let x$(1 to) = "0"` won't work (perhaps in future version it will).

Other functions dealing with strings are:

- `ASC(x$)` that returns the ASCII code of the first character of string `x$`.
- `CHR$(x)` the string with one character corresponding to ASCII code `x`.
- `LEFT$(x$,i)` the string extracted from `x$` taking its first `i` characters.
- `MID$(x$,i,j)` the string extracted from `x$` taking `j` characters starting from the `i`-th.
- `RIGHT$(x$,i)` the string extracted from `x$` taking its last `i` characters.
- `SUB$(x$,i,j)` the string extracted from `x$` taking its characters from the `i`-th to the `j`-th included.
- `STR$(x)` the string representing the number `x`.
- `VAL(x$)` the number represented by string `x$`.

For example:

    >let x$ = "123456789"
    >print asc(x$)
    49
    >print left$(x$,5)
    12345
    >print mid$(x$,5,3)
    567
    >print right$(x$,3)
    789
    >print sub$(x$,5,7)
    567
    >print val(x$)
    1.23457e+08
    >print str$(0.123456e6)
    123456
    >print str$(val(x$(to 5)) / 100000)
    0.12345
    >

The `LEFT$/MID$/RIGHT$` functions are provided mostly for compatibility: anything can be easily done by `SUB$(x$,i,j)` and its subscript version `x$(i TO j)`.

Let us make some simple examples of programs.

    10 REM Revert the characters of a string
    20 INPUT "String to revert"; X$
    30 LET Y$ = ""
    40 FOR I = 1 TO LEN X$
    50 Y$ = X$(I) + Y$
    60 NEXT I
    70 PRINT Y$

Notice that, as obvious, `INPUT` works also with strings: one can ask several strings at once, and they are still separated by commas, so that a string cannot contain a comma:

    >input a$,b$
    ?Alpha, Bravo
    >print a$, b$
    Alpha          Bravo
    >input a$,b$
    ?Wait, I said, what are you doing?
    >print a$,b$
    Wait           I said
    >

The text `Wait, I said, what are you doing?` is split in the commma separated strings: `Wait`, `I said`, `what are you doing?`. Initial spaces are dropped and the third string is ignored since in the `INPUT` statements just two variables are asked.

As in many Basics, StrayBasic provides a *line input* statement, with which a string is read containing all typed characters until the newline

    >linput a$
    Wait, I said, what are you doing?
    >print a$
    Wait, I said, what are you doing?
    >

(Notice that, in this case, the prompt `?` is omitted.)

This program looks for a substring in a text: we want the search to ignore the case, so "alpha" will match both "alpha" and "Alpha" etc.

    10 REM Find a match
    20 INPUT "Match"; X$
    30 REM To terminate insert an empty string
    40 IF X$ = "" THEN STOP
    50 REM Convert X$ to lowercase
    60 FOR J = 1 TO LEN X$
    70 IF X$(J) >= "A" AND X$(J) <= "Z" THEN X$(J) = CHR$(ASC(X$(J)) + 32)
    80 NEXT J
    90 RESTORE
    100 REM Read a verse and check it
    110 READ V$
    120 IF V$ = "#" THEN 20
    130 FOR I = 0 TO LEN V$ - LEN X$ - 1
    140 REM Compare X$ with V$(I+1 TO I+LEN X$)
    150 FOR J = 1 TO LEN X$
    160 LET C$ = V$(I + J)
    170 REM Convert to lowercase if needed
    180 IF C$ >= "A" AND C$ <= "Z" THEN C$ = CHR$(ASC(C$) + 32)
    190 IF C$ <> X$(J) THEN 230: REM No match
    200 NEXT J
    210 REM Match found!
    220 PRINT "Match at "i" in "V$
    230 NEXT I
    240 GOTO 100
    1000 REM Hamlet speech
    1001 DATA "To be, or not to be, that is the question:"
    1002 DATA "Whether 'tis nobler in the mind to suffer"
    1003 DATA "The slings and arrows of outrageous fortune,"
    1004 DATA "Or to take arms against a sea of troubles"
    1005 DATA "And by opposing end them. To die—to sleep,"
    1006 DATA "No more; and by a sleep to say we end"
    1007 DATA "The heart-ache and the thousand natural shocks"
    1008 DATA "That flesh is heir to: 'tis a consummation"
    1009 DATA "Devoutly to be wish'd. To die, to sleep;"
    1010 DATA "To sleep, perchance to dream—ay, there's the rub:"
    1011 DATA "For in that sleep of death what dreams may come,"
    1012 DATA "When we have shuffled off this mortal coil,"
    1013 DATA "Must give us pause—there's the respect"
    1014 DATA "That makes calamity of so long life."
    1015 DATA "For who would bear the whips and scorns of time,"
    1016 DATA "Th'oppressor's wrong, the proud man's contumely,"
    1017 DATA "The pangs of dispriz'd love, the law's delay,"
    1018 DATA "The insolence of office, and the spurns"
    1019 DATA "That patient merit of th'unworthy takes,"
    1020 DATA "When he himself might his quietus make"
    1021 DATA "With a bare bodkin? Who would fardels bear,"
    1022 DATA "To grunt and sweat under a weary life,"
    1023 DATA "But that the dread of something after death,"
    1024 DATA "The undiscovere'd country, from whose bourn"
    1025 DATA "No traveller returns, puzzles the will,"
    1026 DATA "And makes us rather bear those ills we have"
    1027 DATA "Than fly to others that we know not of?"
    1028 DATA "Thus conscience doth make cowards of us all,"
    1029 DATA "And thus the native hue of resolution"
    1030 DATA "Is sicklied o'er with the pale cast of thought,"
    1031 DATA "And enterprises of great pith and moment"
    1032 DATA "With this regard their currents turn awry"
    1033 DATA "And lose the name of action."
    1035 DATA #

Notice the last line: it is a DATA instruction with one string constant, "#" which is written without quotes since it contains no commas (we could do the same for several lines of the speech).

Notice also that we use the sequence of `DATA` statements as a string list; this is possible since we don't need to change its elements, otherwise we'd should use a string array.

### String Arrays

A string array is declared as a numerical array and can be a list of strings or a table of strings: this latter feature is scarcely found in classical and street Basics, I'll give an example later of its use.

For example, let us write a program that defines a list of strings, load their values from a `DATA` list and use a table of lists with two columns to translate some words of the list. Namely, the list `T$` will containt the text, while the table `R$` will contain a set of pairs (s1, s2) that we'll use on the `T$` list to replace each occurrence of s1 in it by s2.

      10 REM The list of strings to modify is from 1000 on and its first element
      20 REM    is the number of its elements.
      30 REM The table used to translate is from 2000 on and its first first element
      40 REM    is the number of its rows.
      45 PRINT "TEXTS TO TRANSFORM:"
      50 RESTORE 1000
      60 READ TSIZE
      70 DIM T$(TSIZE)
      80 FOR I = 1 TO TSIZE
      85 READ T$(I): PRINT "("I; ") "; T$(I)
      90 NEXT I
      95 PRINT "TRANSFORMATION RULES:"
     100 RESTORE 2000
     105 READ RSIZE
     110 DIM R$(RSIZE, 2)
     120 FOR J = 1 TO RSIZE
     125 READ R$(J, 1), R$(J, 2): PRINT "("; CHR$(64 + J); ") "; R$(J, 1)"=>"R$(J, 2)
     130 NEXT J
     200 REM For each string in T$ transform it according to R$
     210 FOR I = 1 TO TSIZE
     220 FOR H = 0 TO LEN T$(I) - 1: REM Index of a possible word to translate
     230 FOR J = 1 TO RSIZE
     240 REM Check whether the LHS of the rule is at T$(I)(H TO ...)
     245 IF H + LEN R$(J, 1) > LEN T$(I) THEN 310
     250 IF MID$(T$(I), H + 1, LEN(R$(J, 1))) <> R$(J, 1) THEN 310
     260 REM We have a match! Substitute the word
     270 PRINT "APPLY RULE ("; CHR$(64 + J); ") TO TEXT ("; I; ")"
     280 LET T$(I) = T$(I)( TO H) + R$(J, 2) + T$(I)(H + LEN R$(J, 1) + 1 TO)
     290 LET H = H + LEN R$(J, 2) + 1: REM Skip the substituted word
     300 GOTO 230
     310 NEXT J
     320 NEXT H
     330 NEXT I
     400 PRINT "TRANSFORMED TEXTS:"
     410 FOR I = 1 TO TSIZE
     420 PRINT T$(I)
     430 NEXT I
     440 STOP
    1000 REM Text to translate
    1005 DATA 3
    1010 DATA MY APPLES ARE RED
    1011 DATA YOUR BOOK IS VERY INTERESTING
    1012 DATA I GAVE HER MY BOOK
    2000 REM Translation table
    2005 DATA 4
    2010 DATA "I ", "YOU "
    2011 DATA "YOU ", "I "
    2012 DATA "MY ", "YOUR "
    2015 DATA "YOUR ", "MY "

Let us execute this program:

    >run
    TEXTS TO TRANSFORM:
    (1) MY APPLES ARE RED
    (2) YOUR BOOK IS VERY INTERESTING
    (3) I GAVE HER MY BOOK
    TRANSFORMATION RULES:
    (A) I =>YOU 
    (B) YOU =>I 
    (C) MY =>YOUR 
    (D) YOUR =>MY 
    APPLY RULE (C) TO TEXT (1)
    APPLY RULE (D) TO TEXT (2)
    APPLY RULE (A) TO TEXT (3)
    APPLY RULE (C) TO TEXT (3)
    TRANSFORMED TEXTS:
    YOUR APPLES ARE RED
    MY BOOK IS VERY INTERESTING
    YOU GAVE HER YOUR BOOK
    LINE 440: PROGRAM STOPPED

Remember to check against indexes before using them in an expression: that is what we did at line 245 in this case.

### Subroutines and chaining

Classic and street Basics programs are actually scripts: the all program is contained in a single file, and there's just one scope, thus each symbol, once introduced, is global to the all program, available everywhere.

However, often several tasks can be factorized to just one, at the cost of introducing one or more parameters: old fashioned languages as Basic used subroutine for this (the term appeared with the Fortran language). In Basic a subroutine is just a chink of consecutive lines which can be called by the `GOSUB` instruction: once the subroutine needs to return to the caller statement, a `RETURN` statement will do the job.

For example, consider a subroutine to multiply a matrix times a vector, leaving the result in the vector itself.

    1004 REM SUBROUTINE matrix times a vector
    1004 REM INPUT: Matrix A(N,N), vector X(N)
    1005 REM OUTPUT: Vector Y(N)
    1010 FOR I = 1 TO N
    1020 Y(I) = 0
    1030 FOR J = 1 TO N
    1040 Y(I) = Y(I) + A(I,J) * X(J)
    1050 NEXT J
    1060 NEXT I
    1070 RETURN

We can call this subroutine time and again in a same program, for example

    10 LET N = 4
    20 DIM A(N,N), X(N), Y(N)
    30 FOR I = 1 TO N: FOR J = 1 TO N: READ A(I,J): NEXT J: NEXT I
    40 FOR I = 1 TO N: READ X(I): NEXT I
    50 FOR K = 1 TO 5
    60 GOSUB 1000
    65 PRINT "Y = ";
    70 FOR I = 1 TO N: PRINT X(I),: NEXT I: PRINT
    75 REM COPY Y() TO X()
    80 FOR I = 1 TO N
    85 LET X(I) = Y(I)
    90 NEXT I
    100 NEXT K
    300 STOP
    500 DATA 0, 1, 1, 1
    501 DATA 0, 0, 2, 2
    502 DATA 0, 0, 0, 3
    503 DATA 0, 0, 0, 0
    510 DATA 4, 3, 2, 1
    1000 REM SUBROUTINE matrix times a vector
    1004 REM INPUT: Matrix A(N,N), vector X(N)
    1005 REM OUTPUT: Vector Y(N)
    1010 FOR I = 1 TO N
    1020 Y(I) = 0
    1030 FOR J = 1 TO N
    1040 Y(I) = Y(I) + A(I,J) * X(J)
    1050 NEXT J
    1060 NEXT I
    1070 RETURN

The main pitfall in Basic subroutines is due to the possible clash of variables used inside the subroutine: in this case, we perform the `GOSUB` inside a `FOR` loop controlled by the `K` variable; had we used `I` or `J`, a clash would be occurred.

Since StrayBasic allows for identifiers longer than a single letter, one can use variables inside the subroutine with special names, for example `I1000` and `J1000` for index variables, to remind the fact that they belong to a subroutine starting at line 1000. More tricks can be devised.



### Files

## Examples

### A linear algebra library

### A Markovian bullshit generator

### A Bayes text classifier

### A screen text editor

Let us program a simple screen editor to edit text files, for example, Basic programs.

Our editor will allow to deal with a single file at once, to load it, or define it from scratch, save it, and modify it.


### Function plotter

chiede y = f(x) e la disegna in modo rudimentale con asterischi, croci, etc.

### A 2D game

un pupazzetto che si muove su un piano e deve saltare da un piano all'altro evitando ostacoli.
