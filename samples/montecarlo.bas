10 REM Monte Carlo method to compute pi
20 INPUT "Number of raindrops"; N
30 LET C = 0
40 FOR S = 1 TO N
45 REM Extract a pair of random number >= 0 and <= 1
60 LET C = C + (RND ^ 2 + RND ^ 2 <= 1)
70 NEXT S
80 PRINT "pi = "; 4 * C / S; " (circa)"