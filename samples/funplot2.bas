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