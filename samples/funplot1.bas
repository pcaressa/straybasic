1 REM Program FUNPLOT, v.1, author Paolo Caressa
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
