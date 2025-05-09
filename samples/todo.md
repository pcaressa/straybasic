### A neural network

When I was 15 I bought a book that teached how to do AI on a microcomputer: it was written by Chris Naylor and was incredibly worth to read. Therein, I've had my first contact with a neural network, at the time the *backpropagation paper* by Hinton, Rumelhart &co was not yet published!

So, some 40 years after, to write a neural network in Basic is too nice not to do it: hereinafter, you may find a (simple for modern standard) neural network running inside a 64K machine and written in Basic.

The network has just one inner layer, and it is applied to a problem...........


### A 2D game

Most microcomputers where street Basic was available were intended for videogames: to do that, they relied on the machine-dependent equipment, that street Basic could access via special instructions (as the infamous `PEEK` and `POKE` that could read and write any memory location). StrayBasic is a virtual Basic, living inside a software running inside an OS perhaps deployed inside a virtual machine etc. so it can't provide such instructions and, in particular, it can't provide graphics. However, with a bit of imagination, we can implement a classic *space invaders* game with it. The key point is the possibility to print a character at a specific row and column in the terminal, via the `AT` function.

You have to defend the planet against cold-war style alien invaders, by just shotting at them. On the left you'll see a cyan smudge which should represent the Earth; on its right a blue defender starship much like a sputnik and on the right of it, red aliens coming from right to left. Aliens are actually kamikazes: if they hit the blue defender the game is over and you lost.

Martians will try to land on Earth and, when a martian does it destroys the piece of Earth on which it landed: if all the (visible) Earth is destroyed the game is over and you lost. You can shot aliens, and when you do it you lost energy, unless you hit the alien, in which case your energy increases. If you run out of energy the game is over and you lost. If you hit all aliens then you win.

To move up the ship use the `q` key, to move it down use the `z` key, to shot press the space bar, to pause the game press `p`, to resume it press `o` and to stop it press `i`. The game is rough but it can be improved in several ways: you'll see on top of the screen your residual energy and some other information.

The program store all elements in the screen into a string array `S$(i,j)`, and the color information into a numerical array `C(i,j)`. Martian data are stored in the array `M(i,j)`, where each row represents a martian and: `M(i,1)` is the x-coordinate, `M(i,2)` the y-coordinate, `M(i,3)` the energy of the martian which can be 1,2,3 and determines its velocity.


### ABACO: A Basic compiler

The final example will be a very non trivial one: I'll show you a complete compiler for the first version of the Basic programming language, the one released at Dartmouth College on May 1st 1964.

The idea is to simulate the experience of using the first Basic version on the Dartmouth Time-Sharing System, as described in the first issued manual of the language [https://www.dartmouth.edu/basicfifty/basicmanual_1964.pdf](https://www.dartmouth.edu/basicfifty/basicmanual_1964.pdf). The user will be prompted to insert a program line or a command, much like is done in StreetBasic, but commands are not Basic statements, but commands of the Time-Sharing system:

- `BYE`: end the session.
- `LIST`: list the current program.
- `NEW`: erase the current file.
- `OLD`: load an already existing program from the disk.
- `REPLACE`: save the current program on an already existing file, replacing it.
- `RUN`: run the program.
- `SAVE`: save the current program on a new file.
- `UNSAVE`: delete a file from the directory.

Some commands expect a parameter, typically a file name, which is prompted after the command is issued.

The Basic version implemented is the very first one, which consists in the following statements:

- `DATA` c,... Introduces internal data
- `DEF FN`l`(`x`) =` e Introduces programmer-defined functions
- `DIM` v,... Allows dimensioning arrays
- `END` Is required
- `GOSUB` i Does a GOTO to a subroutine
- `GOTO` i Transfers to another line-numbered statement
- `FOR` v `=` e1 `TO` e2 `STEP` e3 Introduces the looping construct
- `IF` e1 r e2 `THEN` i Gives a conditional `GOTO`
- `LET` v `=` e Introduces the assignment statement, and is required
- `NEXT` v Terminates the looping construct
- `PRINT` e1,... Provides free-form output
- `READ` v1,... Assigns values to variables from internal data
- `REM` ... Provides comments
- `RETURN` Returns from the end of the subroutine
- `STOP` Same as reaching the END statement

I will provide two implementations of the Dartmouth Basic system: the first one takes advantage of the `CHAIN` command, and compile the program into a StrayBasic program which is then runned. The second version, translate the program into a pseudo-machine code which is executed by a subroutine.

By taking advantage of the `CHAIN` technique we used in the `FUNPLOT` program, the first version of our compiler will result from the chaining of the following programs:

- ABACO that just initializes data, next it chains to
- ABACMD, which accepts commands and, when the `RUN` command is issued chains to
- ABACRUN, that compiles the program into a StrayBasic program, whose last instruction is `CHAIN "ABACMD"`, which is chained to be executed. Thus we use StrayBasic as "object code".





Now, let us provide another version of our compiler: in this case, the program will be translated into a sort of machine code, whose instructions are contained into a string array; a virtual machine to run this machine code will be provided in a subroutine.

Therefore we need to write:

- REPL, the command interpreter that is also needed to edit/save/load the current program.
- COMP, the actual compiler that translates from source code into an object code.
- RT, a runtime environment to execute object code.

The data needed by those three software modules are:

- The source program, stored in a string array PROG$()
- The object program, stored in a numerical array CODE()
- The symbol table, where metadata about code variables are stored.

The structure of our program will be as follows:

    REPL module: lines from 10 to 999
    COMP module: lines from 1000 to 5999
    RT module: lines from 6000 to 9999

The object code is stored into a string array: each item of the array can be addressed by a JP instruction.

Instructions:

There are the following data areas

- CON a 256 array where constants are stored
- VAR a 256 array where scalar variables are stored
- ARR a 4096 array where Basic arrays are stored

MOV n   store into variable n the number in the accumulator
LD n    load the number at location n into the accumulator
ADD n   add the content of variable




1 REM File DABACO.BAS, version 2, author Paolo Caressa
10 REM Dartmouth Basic Compiler.
15 REM (c) 2025 by Paolo Caressa

89 REM Useful function definitions
90 DEF ISDIG(X$) = X$ >= "0" AND X$ <= "9"
91 DEF ISLET(X$) = (X$ >= "A" AND X$ <= "Z") OR (X$ >= "a" AND X$ <= "z")
91 DEF ISALN(X$) = (X$ >= "0" AND X$ <= "9") OR (X$ >= "A" AND X$ <= "Z") OR (X$ >= "a" AND X$ <= "z")

100 REM Program data
110 LET PMAX = 1024 ' Max number of program lines
112 DIM P$(PMAX)    ' Program lines, including line numbers.
114 LET PNEXT = 1   ' First free item in P$()
120 LET CMAX = 2048 ' Max size of object code
122 DIM C(CMAX)     ' Object code
124 LET CNEXT = 1   ' First free item in C()
130 LET T$ = ""     ' Terminal input line

200 REM Looks for a line with number L: if found return in I0 its index in P$
201 REM else retun -1 in I0
205 LET X0$ = STR$(L): LET X0 = LEN X0$
210 FOR I0 = 1 to PNEXT - 1: IF X0$ = P$(I0)(TO X0) THEN RETURN
220 NEXT I0
230 LET I0 = -1
240 RETURN

500 REM REPL module
510 PRINT ">";: LINPUT T$: IF LEN T$ = 0 THEN REPEAT
520 IF NOT ISDIGIT(T$(1)) THEN  ' Command to execute
525 REM Line insertion/deletion/updating: get the line.
530 FOR I = 2 TO LEN T$: IF ISDIGIT(T$(I)) THEN NEXT I
535 LET L = VAL(T$(TO I-1))
540 IF L > LEN T$ THEN  ' Line deletion
545 GOSUB ???  ' Looks for line L in string T$
550 IF Y0 > 0 THEN      ' Line update
555 ' Line insertion
560 IF Y0 = PNEXT THEN  ' No need to shift lines after
565 REM Move lines 





