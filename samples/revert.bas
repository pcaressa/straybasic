10 REM Revert the characters of a string
20 INPUT "String to revert"; X$
30 LET Y$ = ""
40 FOR I = 1 TO LEN X$
50 Y$ = X$(I) + Y$
60 NEXT I
70 PRINT Y$