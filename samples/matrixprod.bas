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