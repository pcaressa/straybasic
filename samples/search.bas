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
