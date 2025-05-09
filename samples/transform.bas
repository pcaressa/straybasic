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