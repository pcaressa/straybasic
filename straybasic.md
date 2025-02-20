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

    ABS ACS AND ASC ASN AT ATN ATTR BYE CHAIN CHR$ CLEAR CLOSE CLS COS DATA DEF DIM DUMP END ERROR EXP FOR GO GOSUB GOTO IF INKEY$ INPUT INT LEFT$ LEN LET LINPUT LIST LOAD LOG MERGE MID$ NEW NEXT NOT ON OPEN OR PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RIGHT$ RND RUN SAVE SGN SIN SKIP SQR STEP STOP STR$ SUB$ SYS TAB TAN THEN TO TRACE VAL

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

Some Basics, as the ZX Spectrum one, didn't provide the `ON` instruction but allowed for *any* expression as argument of a `GOTO`, so that one could use *computed goto*, a mainteinance nightmare. StrayBasic provides computed `GOTO`s, so we can write

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
- The form `GO TO` instead of `GOTO` is forbidden! This is because StrayBasic do not allows for spaces inside identifiers (classical Basic did so, since identifiers in it were a letter or a letter followed by a digit: StrayBasic lets the programmer to use longer alphanumerical sequences).

As it is said time and again in the 60s and 70s books on programming, misuse of `GOTO`s can lead to *spaghetti code*: for example, let us consider the task to check whether a number is divisible by 2, 3, 5, or 7 and, in this case, to divide it and do the same on the quotient, until no check gives a positive result. A simple minded approach would be


10 INPUT N
20 LET X = N / 3
30 IF X = N THEN N = X: GOTO 20
35 LET X = N / 5
40 IF X = N THEN N = X: GOTO 20
45 LET X = N / 7
50 IF X = N THEN N = X: GOTO 20

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

    10 REM Compute GCD
    20 DIM F(29)
    30 LET F(1) = 1
    40 LET F(2) = 1
    50 FOR I = 3 TO 29
    60 LET F(I) = F(I-2) + F(I-1)
    70 NEXT I
    80 PRINT F(29)

We can generalize this program to let it compute any Fibonacci number:

    10 REM Compute GCD
    15 INPUT "N="; N
    20 DIM F(N)
    30 LET F(1) = 1
    40 LET F(2) = 1
    50 FOR I = 3 TO N
    60 LET F(I) = F(I-2) + F(I-1)
    70 NEXT I
    80 PRINT F(N)

Indeed, a `DIM` statement accepts any expression as vector or matrix size (this was common for street Basic but not for classical Basic, being the latter compiled and not interpreted).

However, since StrayBasic works in a 64Kb virtual machine, it is easy to go out of memory: for example

    >dim a(200,100)
    NO MORE ROOM FOR VARIABLES
    >

Indeed, a number takes 4 bytes, so that data for a 200x100 array amounts to 80,000 bytes, that exceeed the total 65,536 bytes memory of the virtual machine.

To check available memory use the `DUMP` statement, that prints information on the state of the system: just after the interpreter has been launched this is its output:

    >dump
    KEYWORDS:
        ATTR BYE CHAIN CLEAR CLOSE CLS DATA DEF DIM DUMP END ERROR FOR GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
        # & * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EOF EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY:
       STRINGS = 0/4096 ( 0%); PROGRAM = 0/8192 ( 0%); VARIABLES = 0/51532 ( 0%)
    MEMORY MAP:
        | strings | program | variables | free space | stacks | buffers |
      0000      1000      3000        3000         F94C     FA00      FFFF
    STRINGS:
    VARIABLES:
    CHANNELS:
        #0 FREE. #1 FREE. #2 FREE. #3 FREE. #4 FREE.
    >

The `DUMP` instruction lists all statement keywords and all operators available in the Basic interpreter. Next, it prints how much memory is occupied / reserved for strings and identifiers, program and variables. Finally, the lists of string constants, variables and channels are printed.

Let us define a matrix and see what happens:

    >dim a(100,100)
    >dump
    KEYWORDS:
        ATTR BYE CHAIN CLEAR CLOSE CLS DATA DEF DIM DUMP END ERROR FOR GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
        # & * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EOF EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY:
       STRINGS = 2/4096 ( 0%); PROGRAM = 0/8192 ( 0%); VARIABLES = 40009/51532 (77%)
    MEMORY MAP:
        | strings | program | variables | free space | stacks | buffers |
      0000      1000      3000        CC49         F94C     FA00      FFFF
    STRINGS:
     "A"
    VARIABLES:
     A(100,100) = | 0 0 0 ...  0 ; 0 0 0 ...  0 ; 0 0 0 ...  0 ; ...  ; 0 0 0 ...  0 ;|
    CHANNELS:
        #0 FREE. #1 FREE. #2 FREE. #3 FREE. #4 FREE.    >

Our 100x100 matrix occupies the 77% of available space for variables.

One can set the space assigned to strings, program and variables by means of the `CLEAR` statement, which can be used as

    CLEAR s, p

to assign s bytes for strings, p bytes for the program (the space for variables is just the remaining one). If a number is missing, then the corresponding size is not altered, for example, type

    >CLEAR 1024, 16384
    >DUMP
    KEYWORDS:
        ATTR BYE CHAIN CLEAR CLOSE CLS DATA DEF DIM DUMP END ERROR FOR GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
        # & * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EOF EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY:
       STRINGS = 2/1024 ( 0%); PROGRAM = 0/16384 ( 0%); VARIABLES = 0/46412 ( 0%)
    MEMORY MAP:
        | strings | program | variables | free space | stacks | buffers |
      0000      0400      4400        4400         F94C     FA00      FFFF
    STRINGS:
     "A"
    VARIABLES:
    CHANNELS:
        #0 FREE. #1 FREE. #2 FREE. #3 FREE. #4 FREE.
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

Notice that concatenation is associative, so there's no need of parentheses when using it time and again. Moreover, `&` is slightly more fast than `+`.

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

does work, thus `x$(i)` is the string which has just one character, the `i`-th character of `x$`. One can also assign such a character, for example

    >let x$ = "123456789"
    >let x$(1) = "0"
    >print x$
    023456789
    >

However one cannot assign a part of a string longer than one character, as in the Sinclair Basic: thus, `let x$(1 to) = "0"` won't work (perhaps in future version it will).

Other functions dealing with strings are:

- `ASC(x$)` that returns the ASCII code of the first character of string `x$` (0 if the string is empty).
- `CHR$(x)` the string with one character corresponding to ASCII code `x` (an empty string if `x` is not in the range 0-127).
- `LEFT$(x$,i)` the string extracted from `x$` taking its first `i` characters (an error occurs if `i` is greater than the length of the string).
- `MID$(x$,i,j)` the string extracted from `x$` taking `j` characters starting from the `i`-th  (an error occurs if `i` or `i+j` are greater than the length of the string).
- `RIGHT$(x$,i)` the string extracted from `x$` taking its last `i` characters (an error occurs if `i` is greater than the length of the string).
- `SUB$(x$,i,j)` the string extracted from `x$` taking its characters from the `i`-th to the `j`-th included.
- `STR$(x)` the string representing the number `x`.
- `VAL(x$)` the number represented by string `x$` (an error occurs if `x$` cannot be converted to a number).

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

The `LEFT$/MID$/RIGHT$` functions are provided mostly for compatibility: anything equivalent could be done by `SUB$(x$,i,j)` and its subscript version `x$(i TO j)`.

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

Notice also that we use the sequence of `DATA` statements as a string list; this is possible since we don't need to change its elements, otherwise we'd should use a string array. Let us give a try to the program:

    >run
    Match?conscience
    Match at 5 in Thus conscience doth make cowards of us all,
    Match?to be
    Match at 0 in To be, or not to be, that is the question:
    Match at 14 in To be, or not to be, that is the question:
    Match at 9 in Devoutly to be wish'd. To die, to sleep;
    Match?death 
    Match at 21 in For in that sleep of death what dreams may come,
    Match at 38 in But that the dread of something after death,
    Match?love
    Match at 23 in The pangs of dispriz'd love, the law's delay,
    Match?
    LINE 40: PROGRAM STOPPED
    >

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

We can call this subroutine time and again in a same program; for example, the following one defines an orthogonal matrix A (thus A^TA=I) and compute A^5v on the vector v = (1,1,1,1).

    10 READ N
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
    500 DATA 4
    510 DATA 0, 0, 0, 1
    511 DATA 0, 0, -1, 0
    512 DATA 1, 0, 0, 0
    513 DATA 0, -1, 0, 0
    520 DATA 1, 1, 1, 1
    1000 REM SUBROUTINE product of a matrix times a vector
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

A typical program of the old days used a textual menu to provide a choice of possible actions to the user: for example, the following one, performs some operations on a text file, providing a rudimentary line editor.

    10 REM Simple Line Editor
    20 LET N = 1024: REM Maximum number of lines in a file
    30 DIM T$(N): REM The list of all file lines
    40 LET TOP = 1: REM First free line
    100 CLS: REM This command clears the screen
    120 PRINT "A simple line editor"
    124 PRINT ,"0) Exit"
    126 PRINT ,"1) List the current buffer"
    128 PRINT ,"2) Append a line"
    130 PRINT ,"3) Delete a line"
    132 PRINT ,"4) Insert a line"
    134 PRINT ,"5) Update a line"
    140 INPUT "Choose a task"; C
    150 IF C = 0 THEN STOP
    160 ON INT C GOSUB 1000, 1100, 1200, 1300, 1400
    170 GOTO 120
    1000 REM SUBROUTINE: List the current buffer
    1010 FOR I = 1 TO TOP - 1
    1020 PRINT "["I"] " T$(I)
    1030 NEXT I
    1040 RETURN
    1050 REM SUBROUTINE: Query the user for a line number and store it into L
    1051 REM If the inserted line is invalid, OK is set to 0 else to 1
    1055 LET OK = 0
    1060 INPUT "Line Number? "; L
    1065 LET L = INT L
    1070 IF L < 1 OR L >= TOP THEN PRINT "Invalid line number!": RETURN
    1080 LET OK = 1
    1090 RETURN
    1100 REM SUBROUTINE: Append a line to the current buffer
    1110 IF TOP > N THEN PRINT "No more space for lines!": RETURN
    1120 INPUT "?"; T$(TOP)
    1130 LET TOP = TOP + 1
    1190 RETURN
    1200 REM SUBROUTINE: Delete a line from the current buffer
    1210 GOSUB 1050: IF NOT OK THEN RETURN
    1230 REM Shifts leftward all lines from the L-th
    1240 LET TOP = TOP - 1
    1250 FOR I = L TO TOP - 1
    1260 LET T$(I) = T$(I + 1)
    1270 NEXT I
    1290 RETURN
    1300 REM SUBROUTINE: Insert a line before another one
    1305 IF TOP > N THEN PRINT "No more space for lines!": RETURN
    1310 GOSUB 1050: IF NOT OK THEN RETURN
    1330 REM Shift rightward
    1340 FOR I = TOP TO L + 1 STEP -1
    1350 LET T$(I) = T$(I - 1)
    1360 NEXT I
    1370 INPUT "Line to insert? "; T$(L)
    1380 TOP = TOP + 1
    1390 RETURN
    1400 REM SUBROUTINE: Insert a line before another one
    1405 IF TOP > N THEN PRINT "No more space for lines!": RETURN
    1410 GOSUB 1050: IF NOT OK THEN RETURN
    1430 INPUT "New line? "; T$(L)
    1440 RETURN

Notice that some subroutines call another subroutine, the one at line 1050 used to ask for a line and check it. There's a limit to the number of nested `GOSUB`s calls one can use in a program.

At line 160 we used the `INT` function with no parentheses: indeed, when the argument of a unary function is a constant, variable or the result of another function call, we can omit parentheses. Notice also, that we could use a computed `GOSUB` instead of the `ON` statement on line 160:

    160 GOSUB 900 + C * 100

Finally, once we leave the program and it stops, the array `T$` contains the text we introduced, you can inspect it by `DUMP`:

    >dump
    KEYWORDS:
        ATTR BYE CHAIN CLEAR CLOSE CLS DATA DEF DIM DUMP END ERROR FOR GOSUB GOTO IF INPUT LET LINPUT LIST LOAD MERGE NEW NEXT ON OPEN PRINT RANDOMIZE READ REM REPEAT RESTORE RETURN RUN SAVE SKIP STEP STOP SYS THEN TO TRACE
    OPERATORS:
        # & * + - - / < <= <> = > >= ABS ACS AND ASC ASN AT ATN CHR$ COS DIV EOF EXP INKEY$ INT LEFT$ LEN LOG MID$ MOD NOT OR RIGHT$ RND SGN SIN SQR STR$ SUB$ TAB TAN VAL ^
    MEMORY:
       STRINGS = 251/4096 ( 6%); PROGRAM = 1284/8192 (15%); VARIABLES = 1085/51532 ( 2%)
    MEMORY MAP:
        | strings | program | variables | free space | stacks | buffers |
      0000      1000      3000        343D         F94C     FA00      FFFF
    STRINGS:
     "N" "T$" "TOP" "A simple line editor" "0) Exit" "1) List the current buffer" "2) Append a line" "3) Delete a line" "4) Insert a line" "5) Update a line" "Choose a task" "C" "I" "[" "] " "OK" "Line Number? " "L" "Invalid line number!" "No more space for lines!" "?" "Line to insert? " "New line? "
    VARIABLES:
     N = 1024
     T$(1024) = | "line 1" "" "" ...  ""|
     TOP = 2
     C = 0
     I = 2 TO 1 STEP 1
    CHANNELS:
        #0 BUSY. #1 FREE. #2 FREE. #3 FREE. #4 FREE.
    >

*En passant*, notice that loop variables are stored along with their start, end and step values. However, when speaking about files, we'll se below how to save the file we edited on the hard disk.

### Chaining programs

An unusual statement provided by StrayBasic is the `CHAIN` one: that was introduced in classic Basic to get rid of lack of space for long programs: therefore Kemeny and Kurtz thought to break such programs into chunks and execute the program a chunk at the time.

Actually, the `CHAIN` *filename* command is equivalent to the sequence `LOAD` *filename* followed by `RUN`. But, consider

    >10 REM Program Hello
    >20 PRINT "Hello!"
    >save "hello"
    >new
    >10 REM Program Foo
    >20 LOAD "hello"
    >30 RUN
    >save "foo"
    >list
      10 REM Program Foo
      20 LOAD "hello"
      30 RUN
    >run
    ILLEGAL INSTRUCTION

What went wrong? That `LOAD "hello"` statement at line 20 of program *foo*, when executed, drops the current program and loads the new one, *hello*. However, the intruction pointer of the interpreter is still pointing after line 20 of the *foo* program, where the line 30 was: but such a line does not exist anymore. Indeed, we ask to list the current program and we get lines 10 and 20 of the *hello* one. Even on writing

    >10 REM Program Foo
    >20 LOAD "hello": RUN
    >run
    ILLEGAL INSTRUCTION

we still get an error: indeed the instruction pointer points not to any legal instruction anymore. Consider instead

    >10 REM Program Chain
    >20 CHAIN "hello"
    >save "chain"
    >run
    >Hello!
    >

The `CHAIN` instruction erase the current program before loading the chained one, but does not delete variables: consider

    >10 REM A
    >20 LET A = 10
    >30 PRINT "This is A, now chain B"
    >40 CHAIN "b"
    >save "a"
    >10 rem B
    >20 LET B = 20
    >30 print "This is B, now chain C"
    >40 chain "c"
    >save "b"
    >10 REM C
    >20 PRINT A + B
    >30 PRINT "This is C, bye"
    >save "c"
    >load "a"
    >run
    This is A, now chain B
    This is B, now chain C
    30
    This is C, bye
    >

By means of che `CHAIN` instruction, a program can generate another program and execute it: this technique will be shown in one of the sample programs in the last part of this tutorial.

### Files

The line editor of the previous section is quite useless, since data is lost when we exit the program: to remedy, we can use files. StrayBasic only provides text files, while classical and most street Basics provides also binary files. As far as files are concerned, classical and street Basics provided different syntaxes, depending on the hardware equipments of the implementation. StrayBasic adopts an approach similar to most street Basics, via the `OPEN/CLOSE` statement used to open and associate to a *channel* a specific file on the disk. The file name should be the one used in the hosting operating system to refer to the file.

The `OPEN` statement takes three parameters: a channel number, thus an integer from 1 to 4 which is associated to a specific file, a string containing the exact name of the file to open and to associate to the channel and a "mode flag" which is 0 for a read only file, 1 for writing on a file erasing its previous contents and 2 for appending on an existing file. The `CLOSE` statement just needs the channel number to close the file and free the channel, so that another `OPEN` statement could use it. To read from and write to the file one uses the `INPUT` and `PRINT` statement in a particular form that specifies the channel to read from/write to.

For example, the following program copies a file into another one: if the second file does not exist, it is created, else it is erased before writing on it.

      10 REM Copy file SOURCE$ to TARGET$
      20 INPUT "File to copy"; SOURCE$
      30 INPUT "File to write"; TARGET$
      40 OPEN 1, SOURCE$, 0: REM OPEN FOR READING
      50 OPEN 2, TARGET$, 1: REM OPEN FOR WRITING
      60 IF EOF 1 THEN 100
      70 LINPUT#1, X$
      80 PRINT#2, X$
      90 GOTO 60
     100 CLOSE 2
     110 CLOSE 1

The `EOF(n)` function returns 1 if the file associated to channel `n` is ended, thus all its elements have been read. We use the `LINPUT` function so we read an entire line into `X$`: if the line is empty, no error occurs but `X$` is set to the empty string.

Now we can modify the previous line editor program to support file handling.

    10 REM Simple Line Editor
    20 LET N = 1024: REM Maximum number of lines in a file
    30 DIM T$(N): REM The list of all file lines
    40 LET TOP = 1: REM First free line
    50 LET CHANGED = 0: REM 1 if there are unsaved changes
    100 CLS: REM This command clears the screen
    120 PRINT "A simple line editor"
    124 PRINT ,"0) Exit"
    126 PRINT ,"1) List the current buffer"
    128 PRINT ,"2) Append a line"
    130 PRINT ,"3) Delete a line"
    132 PRINT ,"4) Insert a line"
    134 PRINT ,"5) Update a line"
    135 PRINT ,"6) Save to file"
    136 PRINT ,"7) Load from file"
    140 INPUT "Choose a task"; C
    160 ON C + 1 GOSUB 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600
    170 GOTO 120
    900 REM SUBROUTINE: Check against unsaved changes and stop
    910 IF CHANGED = 0 THEN 950
    920 INPUT "Save changes (y/n)"; X$
    930 IF X$ = "y" OR X$ = "Y" THEN GOSUB 1500
    950 STOP
    1000 REM SUBROUTINE: List the current buffer
    1010 FOR I = 1 TO TOP - 1
    1020 PRINT "["I"] " T$(I)
    1030 NEXT I
    1040 RETURN
    1050 REM SUBROUTINE: Query the user for a line number and store it into L
    1051 REM If the inserted line is invalid, OK is set to 0 else to 1
    1055 LET OK = 0
    1060 INPUT "Line Number? "; L
    1065 LET L = INT L
    1070 IF L < 1 OR L >= TOP THEN PRINT "Invalid line number!": RETURN
    1080 LET OK = 1
    1090 RETURN
    1100 REM SUBROUTINE: Append a line to the current buffer
    1110 IF TOP > N THEN PRINT "No more space for lines!": RETURN
    1120 INPUT "?"; T$(TOP)
    1130 LET TOP = TOP + 1
    1140 LET CHANGED = 1
    1190 RETURN
    1200 REM SUBROUTINE: Delete a line from the current buffer
    1210 GOSUB 1050: IF NOT OK THEN RETURN
    1230 REM Shifts leftward all lines from the L-th
    1240 LET TOP = TOP - 1
    1250 FOR I = L TO TOP - 1
    1260 LET T$(I) = T$(I + 1)
    1270 NEXT I
    1280 LET CHANGED = 1
    1290 RETURN
    1300 REM SUBROUTINE: Insert a line before another one
    1305 IF TOP > N THEN PRINT "No more space for lines!": RETURN
    1310 GOSUB 1050: IF NOT OK THEN RETURN
    1330 REM Shift rightward
    1340 FOR I = TOP TO L + 1 STEP -1
    1350 LET T$(I) = T$(I - 1)
    1360 NEXT I
    1370 INPUT "Line to insert? "; T$(L)
    1380 TOP = TOP + 1
    1385 LET CHANGED = 1
    1390 RETURN
    1400 REM SUBROUTINE: Insert a line before another one
    1405 IF TOP > N THEN PRINT "No more space for lines!": RETURN
    1410 GOSUB 1050: IF NOT OK THEN RETURN
    1430 INPUT "New line? "; T$(L)
    1435 LET CHANGED = 1
    1440 RETURN
    1500 REM SUBROUTINE: Save the file
    1510 INPUT "Save with name:"; N$
    1520 OPEN 1, N$, 1
    1530 FOR I = 1 TO TOP - 1
    1540 PRINT #1, T$(I)
    1550 NEXT I
    1560 CLOSE 1
    1570 LET CHANGED = 0
    1580 RETURN
    1600 REM SUBROUTINE: Load a file
    1610 INPUT "Load file:"; N$
    1620 OPEN 1, N$, 0
    1630 LET TOP = 1
    1640 IF EOF(1) THEN 1680
    1650 LINPUT #1, T$(TOP)
    1660 LET TOP = TOP + 1
    1670 GOTO 1640
    1680 CLOSE 1
    1685 LET CHANGED = 0
    1690 RETURN

We changed the `ON` instruction at line 160 to include a new subroutine which deals with the case `C = 0`: before exiting we check against unsaved changes and possibly save the program by calling the new subroutine at line 1500. We also defined a new variable `CHANGED` which keeps track of changes in the program, so to catch unsaved changes when exiting.

### Error handling

It it easy to crash the file version of the line editor program: that occurs usually when data are inserted either from the terminal or from a file. For example, at line 140 we ask a number `C` to choose the task to do: but, if the user inserts a number out of range, or neither a number at all, an error will occur.

It is safer to prompt for a string usually, and then convert to a number: in this case, the following changes may fix this possible error:

    140 INPUT "Choose a task"; C$
    150 IF LEN C$ <> 1 THEN 140
    155 IF C$ < "0" OR C$ > "7" THEN 140
    160 ON VAL C$ + 1 GOSUB 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600

We use `VAL C$` to convert the digit to a number, then add 1 since the `ON` list assumes that the first label to jump to correspond to the value 1 of the numerical expression following `ON`.

However, input errors from files are not so easy to deal with: consider the subroutine at line 1600:

    1600 REM SUBROUTINE: Load a file
    1610 INPUT "Load file:"; N$
    1620 OPEN 1, N$, 0
    1630 LET TOP = 1
    1640 IF EOF(1) THEN 1680
    1650 LINPUT#1, T$(TOP)
    1660 LET TOP = TOP + 1
    1670 GOTO 1640
    1680 CLOSE 1
    1685 LET CHANGED = 0
    1690 RETURN

The program prompts the user for a file name: but what if this name does not correspond to any available file in the OS? The `OPEN` statement fails in this case, with a `CANNOT OPEN FILE` error. The program would break.

StrayBasic provides a device to prevent these errors, which is the device of many street Basics of the 70s and 80s, and which is a simple exception handling mechanism: the `ON ERROR` statement.

Consider inserting the following lines in the subroutine at 1600:

    1600 REM SUBROUTINE: Load a file
    1610 INPUT "Load file:"; N$
    1615 ON ERROR 1695
    1620 OPEN 1, N$, 0
    1625 ON ERROR 1680
    1630 LET TOP = 1
    1640 IF EOF(1) THEN 1680
    1650 LINPUT#1, T$(TOP)
    1660 LET TOP = TOP + 1
    1670 GOTO 1640
    1680 CLOSE 1
    1685 LET CHANGED = 0
    1686 ON ERROR 0
    1690 RETURN
    1695 PRINT "CANNOT OPEN FILE "N$"!"
    1696 GOTO 1686
    
When executed, the `ON ERROR n` statement sets to the line number `n` the line where to jump in case of error: if `n = 0` then usual error discipline is restored, thus in case of error the execution is stopped with a message, else `n` should be the number of an actual program line, so that, when an error occurs, a `GOTO n` is performed. For example, at line 1615 we set the error line to be line 1695: that's because, if the `OPEN` statement fails, the subroutine prints the message at line 1695, next the line 1686 is executed, that reset the error catching to the default one. At line 1625, the error line is set to 1680, instead, so that on an input error, the routine will terminate as if the end of the file has been reached.

The `ERR` built-in function, which takes no parameters, returns the last error inside an `ON ERROR` routine. One can use it to check the nature of the error, so the previous code could be rewritten as

    1600 REM SUBROUTINE: Load a file
    1610 INPUT "Load file:"; N$
    1615 ON ERROR 1675
    1620 OPEN 1, N$, 0
    1630 LET TOP = 1
    1640 IF EOF(1) THEN 1680
    1650 LINPUT#1, T$(TOP)
    1660 LET TOP = TOP + 1
    1670 GOTO 1640
    1675 IF ERR = 11 THEN PRINT "CANNOT OPEN FILE "N$"!": SKIP
    1680 CLOSE 1
    1685 LET CHANGED = 0
    1686 ON ERROR 0
    1690 RETURN

We moved the error line at 1675 and let it check whether `ERR = 11`, since 11 is the error code raised by the `OPEN` statement if it fails. Notice the `SKIP` statement ending line 1675: its effect is to skip the following line, the 1680, indeed if we could not open the file then to try to close it would give rise to an error, too.

One error can be explicitly raised via the `ERROR` command:

    ERROR n

where `n` is the code of the error to raise. One can use any integer number, so that user defined error codes could be defined, and then `ON ERROR` and `ERR` could be used to deal with them.

Files can also be used to store data, not just texts. The following program reads a list of records set up by a code, a string and a numerical value, for example they could represent order code, product name and price of products to sell or buy. Items are stored one per line and their attributes are comma separated. An example of such a file can be:

    1, "Knuth, The Art of Computer Programming, vol.1", 80
    2, "Kernighan-Ritchie, The C Programming Language", 35
    3, "Abelson-Sussman, Structure and Interpretation of Computer Programs", 60
    4, "Brooks, The Mythical Man-Month", 30
    5, "Kemeny-Kurtz, Programming in Basic", 40

Notice that we put strings between quotes since they contain commas. After loading them, the user can look for the name of a product and recover its code and price.

      10 REM Product search
      20 LET INFILE$ = "items.csv"
      30 LET N = 1024: REM Max number of items in a file
      40 DIM ID(N), NAME$(N), PRICE(N)
      50 OPEN 1, INFILE$, 0
      60 LET NI = 0
      70 IF EOF(1) THEN 110
      80 LET NI = NI + 1
      85 IF NI > N THEN PRINT "TOO MANY ITEMS IN FILE!": STOP
      90 INPUT#1, ID(NI), NAME$(NI), PRICE(NI)
     100 GOTO 70
     110 CLOSE 1
     120 INPUT "Text to look for (empty to stop)", T$
     130 IF T$ = "" THEN STOP
     140 FOR I = 1 TO NI
     150 REM Looks for T$ as a substring of NAME$(NI)
     160 IF LEN T$ > LEN(NAME$(I)) THEN 220
     170 FOR J = 1 TO LEN(NAME$(I)) - LEN T$
     180 IF MID$(NAME$(I), J, LEN T$) <> T$ THEN 210
     190 PRINT NAME$(I)" COSTS "PRICE(I)"€"
     200 GOTO 220
     210 NEXT J
     220 NEXT I

### Remaining stuff

If you get here, you learned StrayBasic! Actually, there are some more statement and features not yet described, they are explained in this section: as each Basic dialect, even StrayBasic has some specific features.

First of all, let us complete our knowledge of the `PRINT` statement: generally speaking, after the `PRINT` keyword it may appear:

- nothing, which causes a new line to be printed;
- a series of expressions separated by commas or semicolons; such expressions are evaluated (either to numbers or to strings) and the result printed; when separated by commas, elements are printed at the next tabulation position, when separated by semicolons they are printed with no spaces in between;
- comma and semicolons may be repeated at will, and they may also be the last item in the list, in which case no newline is issued after printing the list;
- semicolons may usually be omitted.

Examples:

    >PRINT

    >PRINT 1,2,3
    1              2               3
    >PRINT 1;"2";1+1+1
    123
    >PRINT 1,
    1              >PRINT 1,,,2
    1                                              2
    >PRINT 1 2 3
    123
    >PRINT 2-1 3-1 4-1
    123
    >PRINT 1 -2 -3
    -4
    >

Notice the last one: the interpreter tries to extend an expression as far as possible, so the sequence `1 -2 -3` is interpreted as `1-2-3` which is an expression resulting in `-4`: in this case, we need to use the semicolon.

We have already encountered the `CLS` command that clear the screen and set the cursor position to the leftmost-topmost character on the terminal. If the terminal inside which StrayBasic runs is compliant to the standard Unix terminals, then some commands will work properly. The first one is `CLS` itself, which works by issuing special control characters to the terminal.

There are a couple of StrayBasic functions that always return the empty string, but that have side effects on where `PRINT` will print the next character. The `TAB(n)` function sets the print position to column `n`, were columns are indexed from 1 to a constant `C` depending on the terminal; if `n < 1` then is it assumed that `n = 1`; if `n > C`, then `n` is divided by such number of column and `TAB` is executed on the rest of this division.

For example:

    >PRINT TAB(10) "*"
             *
    >PRINT TAB(50) "*"
                                                     *
    >PRINT TAB(79) "*"
                                                                                  *
    >PRINT TAB(80) "*"
    *

The last statement shows that the number of coumns is in this case 80, so after `TAB(80)` any character will be printed at the 81-th column, so at the first one, since 81 divided by 80 has rest 1.

To get the number of columns supported by the terminal, use the function `COL` which takes no parameter but returns the number of columns (or `-1` if the terminal cannot be queried for such an information). In the same way, the `ROW` parameterless function returns the number of rows of the terminal, or `-1` if the terminal cannot be queried for such an information. Row 1 is the topmost one, and column 1 is the leftmost one.

Each character on the terminal is addressed by a pair `(n,m)` in this way, and the `AT(n,m)` function, still returning the empty string, changes the print position to line `n` and column `m`.

For example, the following program first clears the screen with a `CLS` statement, next draw a diagonal line:

    10 CLS
    20 FOR I = 1 TO ROW
    30 PRINT AT(I,I) "*"
    40 NEXT I

Next we have a specific StrayBasic command, `ATTR`, which is used to set screen properties as follows: use it as

    ATTR key = value, ...

where keys can be one of: `BLINK BOLD BRIGHT REVERSE UNDER BACK FORE` and values can be numbers. One or more key may be preset at will in the attribute list.

- The `BACK` property decides the colour of characters background: its value should be an integer number between 0 and 7, which is considered as a three bits binaru number rgb.
- The `FORE` property decides the colour of characters foreground: its value should be an integer number between 0 and 7, which is considered as a three bits binaru number rgb.
- The `BOLD` property decides if the character is printed in bold face (value 1) or not (value 0).
- The `BLINK` property decides if the character is printed blinking (value 1) or not (value 0).
- The `BRIGHT` property decides if the character is printed brighter than usual (value 1) or not (value 0).
- The `REVERSE` property decides if the character is printed with foreground and background reversed (value 1) or not (value 0).
- The `UNDER` property decides if the character is printed as underlined (value 1) or not (value 0).

For example, we can print some colour names in the colours themselves:

    10 ATTR FORE = 0: PRINT "BLACK"
    20 ATTR FORE = 1: PRINT "BLUE"
    30 ATTR FORE = 2: PRINT "GREEN"
    40 ATTR FORE = 3: PRINT "CYAN = BLUE + GREEN"
    50 ATTR FORE = 4: PRINT "RED"
    60 ATTR FORE = 5: PRINT "VIOLET = RED + BLUE"
    70 ATTR FORE = 6: PRINT "YELLOW = RED + GREEN"
    80 ATTR FORE = 7: PRINT "WHITE = BLUE + RED + GREEN"
    90 ATTR FORE = 2

The statement on line 100 restores the default green colour of the interpreter. We could reverse, underline and bold those texts by adding two lines

    5 ATTR BOLD = 1, REVERSE = 1, UNDER = 1
    95 ATTR BOLD = 0, REVERSE = 0, UNDER = 0

Try to play with `ATTR`, it may happens that not all its property will properly(!) work on your terminal.

A popular function among street Basics was the `INKEY$` one: it returns the character corresponding to the pressed key. It is supported by StrayBasic, but via the GNU/Linux standard library: to make StrayBasic more portable, it should use a version of `ncurses` but unluckily laziness prevented this.

Namely, in StrayBasic, `INKEY$` expects the user to press any key before returning its value. I'll show an example of all these terminal-oriented stuff by programming a simple game in the next chapter.

What else? Yep! The `RANDOMIZE` function: this can be used to reset the pseudo-random number series to a certain point. The `RND` function cannot generate truly random numbers, instead it keeps on generating numbers from a sequence whose elements "seems like" random but are actually generated by an algorithm. When the program starts, the index of the sequence is reset, so that each time the program runs the `RND` assumes always the same values.

For example, try:

    >10 PRINT RND, RND, RND, RND
    >RUN
    0.840188       0.394383        0.783099        0.79844
    >RUN
    0.840188       0.394383        0.783099        0.79844
    >RUN
    0.840188       0.394383        0.783099        0.79844
    >

This behavior preserves the determinism of the program, for example we can test the program running it time and again and get the same sequence of values of `RND` calls. However, if we want to get a different sequence any time, we can use the `RANDOMIZE` statement, which scrumble the starting point of the pseudo-random number sequence somehow. For example:

    >10 RANDOMIZE
    >20 PRINT RND, RND, RND, RND
    >RUN
    0.377653       0.785811        0.515028        0.234914
    >RUN
    0.237492       0.702525        0.82163         0.0568017
    >RUN
    0.0930361      0.611893        0.120664        0.873546
    >

The `TIME` parameterless function returns the number of seconds elapsed from the launch of the interpreter: we can use it to measure time intervals, for example: how many times we can increase a variable and repeat a loop within one second? We measure it time and again and next we take the mean.

    10 LET ITER = 0
    15 LET N = 10
    20 DIM A(N)
    30 FOR I = 1 TO N
    40 LET T0 = TIME
    50 FOR DT = 1 TO 1e20
    55 LET T1 = TIME
    60 IF T0 < T1 THEN 80
    70 NEXT DT
    80 LET A(I) = DT
    90 NEXT I
    100 LET AVG = 0
    110 FOR I = 1 TO N
    120 LET AVG = AVG + A(I) / N
    130 NEXT I
    150 PRINT AVG " AVERAGE ITERATIONS IN " T1 - T0 " SECONDS."

    >run
    1.36414e+06 AVERAGE ITERATIONS IN 1 SECONDS.

For debug purposes, one should inspect the variables, for example the `DUMP` statement prints them, or also activate the `TRACE` option: by the statement `TRACE 1` all subsequent statements will be printed on the screen, along with their line number, before being executed. To turn off tracing, type `TRACE 0`.

Last but not least we have the `SYS` command, which takes a string and send it to the hosting operating system to be executed. For example you can list the contents of the current folder with

    SYS "ls"

Not any shell command will be executed of course (typically you wouldn't be able to change the directory), but this can be useful to avoid exiting StrayBasic to check something.

## Examples

Now that we know *all* the features of StrayBasic, we can program with it: I'll show some example of non trivial programs that can be run within the 64Kb of the StrayBasic virtual machine.

### A magic squares generator



### Function plotter

With no graphic available, it may seems to be odd that one can plot function's graphs with Basic, but indeed this is a standard example, to be found even in classical Basic 60s books.

On a line-oriented system, we can plot the graph of a function clockwise rotated by 90 degrees, as follows:

    10 REM PLOT A FUNCTION
    20 DEF FNF(X) = COS(X)
    30 LET X0 = -3.15
    40 LET X1 = 3.15
    50 LET Y0 = -2
    60 LET Y1 = 2
    70 LET DX = 1/4
    80 LET DY = 1/10
    100 FOR X = X0 TO X1 STEP DX
    110 LET Y = FNF(X)
    120 IF Y0 <= Y AND Y <= Y1 THEN PRINT TAB((Y - Y0)/DY) "*"
    130 NEXT X

The plot is bounded along `X` variable between `X0` and `X1`, with step `DX`, and along `Y` between `Y0` and `Y1`. The step `DY` is used to rescale the `Y` values, so that 10 characters correspond to a length of 1.

Let's try it:

    >RUN
             *
             *
              *
               *
                 *
                   *
                      *
                        *
                           *
                             *
                              *
                                *
                                *
                                *
                                *
                               *
                             *
                           *
                         *
                      *
                    *
                 *
               *
              *
             *
             *
    >

We can also draw the x-axis:

    10 REM PLOT A FUNCTION
    20 DEF FNF(X) = COS(X)
    30 LET X0 = -3.15
    40 LET X1 = 3.15
    50 LET Y0 = -2
    60 LET Y1 = 2
    70 LET DX = 1/4
    80 LET DY = 1/10
    100 FOR X = X0 TO X1 STEP DX
    110 LET Y = FNF(X)
    115 REM PLOT Y AXIS
    120 IF X <= 0 AND X + DX > 0 THEN FOR I = 1 TO (Y1 - Y0)/DY: PRINT "-";:NEXT I
    125 REM PLOT X AXIS
    130 IF Y0 <= 0 AND 0 <= Y1 THEN PRINT TAB(-Y0/DY) "|";
    135 REM PLOT CURVE POINT
    140 IF Y0 <= Y AND Y <= Y1 THEN PRINT TAB((Y - Y0)/DY) "*";
    145 PRINT
    150 NEXT X

We plot the `y` axis by inserting a horizontal line in correspondence of the value of `X` closer to 0, while we plot the `x` axis just drawing a piece of it each time we draw a point of the function. Let us try it:

    >RUN
             *         |
             *         |
              *        |
               *       |
                 *     |
                   *   |
                      *|
                       |*
                       |   *
                       |     *
                       |      *
                       |        *
    -------------------|--------*-----------
                       |        *
                       |        *
                       |       *
                       |     *
                       |   *
                       | *
                      *|
                    *  |
                 *     |
               *       |
              *        |
             *         |
             *         |
    >

Nice, isn't it?

But, how can we make the function to be inserted by the user? A first rough solution is to change line 20 each time we want the graph of a new variable. However, by means of the `CHAIN` statement, we can create a program that plots a specific function and then execute it!

    10 REM Prompts for a function and create the appropriated plotting program.
    20 PRINT "Insert an expression depending ONLY on variable X"
    30 LINPUT E$
    40 INPUT "Insert X interval X0, X1", X0, X1
    50 INPUT "Insert Y interval Y0, Y1", Y0, Y1
    60 INPUT "Insert DX, DY", DX, DY
    70 LET Q$ = CHR$(34): REM Double quote
    100 OPEN 1, "templot", 1
    110 PRINT#1, "10 REM PLOT FUNCTION " + E$
    120 PRINT#1, "20 DEF FNF(X) = " + E$
    130 PRINT#1, "30 LET X0 = " + STR$(X0)
    140 PRINT#1, "40 LET X1 = " + STR$(X1)
    150 PRINT#1, "50 LET Y0 = " + STR$(Y0)
    160 PRINT#1, "60 LET Y1 = " + STR$(Y1)
    170 PRINT#1, "70 LET DX = " + STR$(DX)
    180 PRINT#1, "80 LET DY = " + STR$(DY)
    200 PRINT#1, "100 FOR X = X0 TO X1 STEP DX"
    210 PRINT#1, "110 LET Y = FNF(X)"
    220 PRINT#1, "120 IF X <= 0 AND X + DX > 0 THEN FOR I = 1 TO (Y1 - Y0)/DY: PRINT " Q$ "-" Q$ ";: NEXT I"
    230 PRINT#1, "130 IF Y0 <= 0 AND 0 <= Y1 THEN PRINT TAB(-Y0/DY) " Q$ "|" Q$ ";"
    240 PRINT#1, "140 IF Y0 <= Y AND Y <= Y1 THEN PRINT TAB((Y - Y0)/DY) " Q$ "*" Q$ ";"
    245 PRINT#1, "145 PRINT"
    250 PRINT#1, "150 NEXT X"
    290 CLOSE 1
    300 REM Now load and run the program just saved.
    310 CHAIN "templot"

For example:

    >RUN
    Insert an expression depending ONLY on variable X
    EXP(-X^2)
    Insert X interval X0, X1?-3,3
    Insert Y interval Y0, Y1?-1,2
    Insert DX, DY?0.25, 0.1
             *
             *
             *
             *
             *
             *
             |*
             | *
             |  *
             |    *
             |      *
             |        *
    ---------|---------*----------
             |        *
             |      *
             |    *
             |  *
             | *
             |*
             *
             *
             *
             *
             *
             *
    >

Notice that the current program now is "templot", just type `LIST` to see it.

By means of this `CHAIN` one can in general use a program to build another program and run it.

However, this plotting program still has a bitter taste, since we are drawing up to a 90° rotation. How can we get rid of that? I propose two ways: the first one is more portable, the second one relies on the `AT` function and to port it the program should be modified.

In the first place, instead of drawing by the `PRINT` function, we could store inside an array the graph and then print it the right way. Let us see how to do that on the plotting program, leave to the reader to adapt this special program to the general case by the `CHAIN` trick.

    10 REM PLOT A FUNCTION
    20 DEF FNF(X) = EXP(-X^2)
    30 LET X0 = -2
    35 LET X1 = 2
    40 LET Y0 = -0.2
    45 LET Y1 = 1.2
    50 LET DX = 1/8
    55 LET DY = 1/10
    60 LET N = 1 + (X1 - X0)/DX
    65 LET M = 1 + (Y1 - Y0)/DY
    70 DIM P(N,M)   ' P(N,M) = 0 (blank), 1 (*), 2 (-), 3(|) 
    100 FOR X = X0 TO X1 STEP DX
    105 LET IX = 1 + (X - X0)/DX
    110 LET Y = FNF(X)
    115 REM PLOT Y AXIS
    120 IF X <= 0 AND X + DX > 0 THEN FOR J = 1 TO M: LET P(IX,J) = 2: NEXT J
    125 REM PLOT X AXIS
    130 IF Y0 <= 0 AND 0 <= Y1 THEN LET P(IX,1 - Y0/DY) = 3
    135 REM PLOT CURVE POINT
    140 IF Y0 <= Y AND Y <= Y1 THEN LET P(IX,1 + (Y - Y0)/DY) = 1
    150 NEXT X
    200 REM Now plot the graph
    205 ATTR BACK = 7, FORE = 0
    210 FOR J = M TO 1 STEP -1
    220 FOR I = 1 TO N
    230 IF P(I,J) = 0 THEN PRINT " ";
    235 IF P(I,J) = 1 THEN PRINT "*";
    240 IF P(I,J) = 2 THEN PRINT "|";
    245 IF P(I,J) = 3 THEN PRINT "-";
    250 NEXT I
    260 PRINT
    270 NEXT J
    275 ATTR BACK = 0, FORE = 2

Notice that we swap `I` and `J` when printing and also that we run `J` from `M` downward to 1, since the `y` axis is upside down in the `AT` coordinates. We also printed in white background and black foreground the graph:

    >RUN
                    |                
                    |                
                    *                
                  **|**              
                 *  |  *             
                *   |   *            
               *    |    *           
              *     |     *          
             *      |      *         
            *       |       *        
          **        |        **      
        **          |          **    
    ****-------------------------****
                    |                
                    |                
    >

The result is uglier than before, since in general the height and width are different for characters printed on a terminal (indeed we changed `DX` and `DY` to get a better picture).

Notice that lines 230-245 may be replaced by a single but cumbersome instruction:

230 PRINT CHR$(32 + 10*(P(I,J) = 1) + 92*(P(I,J) = 2) + 13*(P(I,J) = 3));

Indeed, `CHR$(32) = " "`, `CHR$(32+10) = "*"`, `CHR$(32+92) = "|"` and `CHR$(32+13) = "-"` (recall that the result of a relation or of a Boolean operator is always either 0 or 1).

Another solution to print the graph in the correct orientation is to use the `AT` function instead of the `TAB` one:

    10 REM PLOT A FUNCTION
    20 DEF FNF(X) = EXP(-X^2)
    30 LET X0 = -2
    35 LET X1 = 2
    40 LET Y0 = -0.2
    45 LET Y1 = 1.2
    50 LET DX = 1/8
    55 LET DY = 1/10
    60 LET N = 1 + (X1 - X0)/DX
    65 LET M = 1 + (Y1 - Y0)/DY
    90 CLS
    100 REM PLOT X AXIS
    110 IF X0 > 0 OR X1 < 0 THEN 130
    115 LET K = 1 + Y1/DY
    120 FOR I = 1 TO N: PRINT AT(K, I) "-": NEXT I
    125 REM PLOT Y AXIS
    130 IF Y0 > 0 OR Y1 < 0 THEN 150
    135 LET K = 1 - X0/DX
    140 FOR J = 1 TO M: PRINT AT(J, K) "|": NEXT J
    145 REM PLOT THE FUNCTION GRAPH
    150 FOR X = X0 TO X1 STEP DX
    160 LET Y = FNF(X)
    170 IF Y0 <= Y AND Y <= Y1 THEN PRINT AT(1 + (Y1 - Y)/DY, 1 + (X - X0)/DX) "*"
    180 NEXT X

Notice that, at linee 170, we compute the `y` coordinate by using `(Y1 - Y)/DY`, since the `y` axis is upside down in the `AT` coordinates.

    >RUN
                    |
                   ***
                 ** | **
                *   |   *
               *    |    *
             **     |     **
            *       |       *
          **        |        **
        **          |          **
    ****------------|------------****
    >               |
                    |
                    |
                    |
                    |

Question: what happens if we enclose lines 90-180 between a pair of `ATTR BACK = 7, FORE = 0` and `ATTR BACK = 0, FORE = 2`?

OK, 'nuff said about function plots.

### A Bayes text classifier

### A Markovian bullshit generator

### A screen text editor

Let us program a simple screen editor to edit text files, for example, Basic programs.

Our editor will allow to deal with a single file at once, to load it, or define it from scratch, save it, and modify it.

### A 2D game

un pupazzetto che si muove su un piano e deve saltare da un piano all'altro evitando ostacoli.

### A Basic interpreter

The final example will be a very non trivial one: I'll show you a complete interpreter for the first version of the Basic programming language, the one released at Dartmouth College on May 1st 1964.

