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
