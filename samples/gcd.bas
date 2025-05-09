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