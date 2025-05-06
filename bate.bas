1 REM   BAsic Text Editor
100     REM Program data
110 WIDTH = 70, HEIGHT = 20 ' Screen dimensions
115 MAXBUF = 4096       ' Max number of buffer lines
120 DIM BUF$(MAXBUF)    ' Buffer lines
121 BUF$(1) = CHR$(10)
125 BUFL = 1            ' Index of last used item in BUF
130 BUFR = 1, BUFC = 1  ' Current row and column in BUF
135 SAVED = 1           ' 0 if there are unsaved changes
140 WINR = 1, WINC = 1  ' First row and column to print
145 NAME$ = ""          ' File name
150 TABN = 4            ' # of spaces written by a tabulation
155 REM Footer strings
170 CP$ = ""            ' Last copied line
175 FS$ = ""            ' String to look for when pressing ESC f
180 RS$ = ""            ' String with which to replace FS$
199     REM Main loop
200 GOSUB 2000          ' Refresh the screen
210 C$ = INKEY$: C = ASC(C$): IF C = 0 THEN REPEAT
215 IF C = 27 THEN GOSUB 350: GOTO 200  ' Escape
220 SAVED = 0
225 IF C = 9 THEN 280   ' Tabulation
230 IF C = 10 THEN 300  ' Newline
235 IF C = 127 THEN 320 ' Delete
240 IF C < 32 THEN 200  ' Ignore other control characters
245     REM Insert C$ after the cursor
250 IF BUFC = 240 THEN 200  ' Line too long
255 L = LEN(BUF$(BUFR))
260 S1$ = "": IF BUFC > 1 THEN S1$ = BUF$(BUFR)(TO BUFC - 1)
265 S2$ = "": IF BUFC <= L THEN S2$ = BUF$(BUFR)(BUFC TO)
270 BUF$(BUFR) = S1$ + C$ + S2$, BUFC = BUFC + 1
275 GOTO 200
279     REM Tabulation character
280 IF BUFC + TABN > WIDTH THEN 200 ' Ignore if there's no room for it
285 C$ = " "
290 FOR I = 1 TO TABN: GOSUB 300: NEXT I
295 GOTO 200
299     REM Newline
300 IF BUFC = LEN(BUF$(BUFR)) THEN X0$ = BUF$(BUFR), X1$ = CHR$(10): SKIP
301     X0$ = BUF$(BUFR)(TO BUFC-1) + CHR$(10), X1$ = BUF$(BUFR)(BUFC TO)
302 X1 = BUFR + 1: GOSUB 1000 ' Insert X1$ after BUFR
305 IF Y1 = 0 THEN BUF$(BUFR) = X0$, BUFR = BUFR + 1, BUFC = 1
310 GOTO 200
319     REM Delete
320 IF BUFC = 1 THEN 330
322 BUFC = BUFC - 1, BUF$(BUFR) = BUF$(BUFR)(TO BUFC-1) + BUF$(BUFR)(BUFC+1 TO)
324 GOTO 200
325 REM Check if the current line can be joined with the previous one.
330 IF BUFR = 1 THEN SAVED = 1: GOTO 200
331 IF LEN(BUF$(BUFR-1)) + LEN(BUF$(BUFR)) > 240 THEN SAVED = 1: GOTO 200
332 BUFC = LEN(BUF$(BUFR-1))
334 REM JOIN BUF$(BUFR-1) TO BUF$(BUFR) dropping the newline
335 BUF$(BUFR-1) = BUF$(BUFR-1)(TO BUFC-1) + BUF$(BUFR)
340 X1 = BUFR: GOSUB 1100: BUFR = X1    ' Delete line BUFR
345 GOTO 200
349     REM Escape character
350 C = ASC(INKEY$): IF C = 0 THEN REPEAT
351 IF C >= 97 AND C <= 122 THEN C = C - 32
352 IF C = 91 THEN 370
353 IF C = 67 THEN CP$ = BUF$(BUFR): RETURN ' C: copy current line
354 IF C = 68 THEN 600  ' D: duplicate current line
355 IF C = 70 THEN 680  ' F: Find a string
356 IF C = 72 THEN 2200 ' H: Print a help message
357 IF C = 73 THEN 610  ' I: insert copied line
358 IF C = 75 THEN 640  ' K: cut current line
359 IF C = 77 THEN 670  ' M: rename current buffer
360 IF C = 78 THEN 800  ' N: create a new buffer
361 IF C = 79 THEN 830  ' O: open buffer from disk
362 IF C = 81 THEN 930  ' Q: quit
363 IF C = 82 THEN 740  ' R: replace a string
364 IF C = 83 THEN 880  ' S: save current buffer
365 RETURN
369 REM More escape characters
370 C1 = ASC(INKEY$): IF C1 >= 97 AND C1 <= 122 THEN C1 = C1 - 32
371 IF C1 = 51 THEN 500 ' Canc (126 follows)
373 IF C1 = 53 THEN 540 ' PgUp (126 follows)
374 IF C1 = 54 THEN 550 ' PgDown (126 follows)
375 IF C1 = 65 THEN 560 ' Up arrow
376 IF C1 = 66 THEN 570 ' Down arrow
377 IF C1 = 67 THEN 580 ' Right arrow
378 IF C1 = 68 THEN 590 ' Left arrow
380 IF C1 = 70 THEN BUFC = LEN(BUF$(BUFR)): RETURN ' End of line
382 IF C1 = 72 THEN BUFC = 1: RETURN ' Start of line
390 RETURN
499     REM Canc
500 C$ = INKEY$ ' Skip the 126 that ends the sequence.
505 IF BUFC = LEN(BUF$(BUFR)) THEN 520
509 REM Delete the next character on the same line.
510 BUF$(BUFR) = BUF$(BUFR)(TO BUFC - 1) + BUF$(BUFR)(BUFC + 1 TO)
515 RETURN
519 REM Join the current line with the next one, if any
520 IF BUFR = BUFL THEN RETURN
525 BUF$(BUFR) = BUF$(BUFR) + BUF$(BUFR + 1)
530 X1 = BUFR + 1: GOSUB 1100   ' Delete line BUFR + 1
535 RETURN
539     REM Pg Up
540 C$ = INKEY$ ' Skip the 126 that ends the sequence.
542 IF BUFR > HEIGHT THEN BUFR = BUFR - HEIGHT: RETURN
545 BUFR = 1: RETURN
549     REM Pg Down
550 C$ = INKEY$ ' Skip the 126 that ends the sequence.
552 IF BUFR + HEIGHT <= BUFL THEN BUFR = BUFR + HEIGHT: RETURN
555 BUFR = BUFL: RETURN
559     REM Up arrow
560 IF BUFR > 1 THEN BUFR = BUFR - 1
565 RETURN
569     REM Down arrow
570 IF BUFR < BUFL THEN BUFR = BUFR + 1
575 RETURN
579     REM Right arrow
580 IF BUFC < LEN(BUF$(BUFR)) THEN BUFC = BUFC + 1: RETURN
582 IF BUFR < BUFL THEN BUFR = BUFR + 1: BUFC = 1
585 RETURN
589     REM Left arrow
590 IF BUFC > 1 THEN BUFC = BUFC - 1: RETURN
592 IF BUFR > 1 THEN BUFR = BUFR - 1: BUFC = LEN(BUF$(BUFR))
595 RETURN
599     REM Duplicate current line
600 CP$ = BUF$(BUFR)    ' Fall through
605     REM Insert the copied line
610 X1$ = CP$: X1 = BUFR + 1
615 GOTO 1000   ' Insert CP$ at BUFR
639     REM Cut the current line
640 CP$ = BUF$(BUFR)
645 IF BUFL = 1 THEN BUF$(1) = CHR$(10): RETURN
650 BUFL = BUFL - 1
655 FOR I = BUFR TO BUFL: BUF$(I) = BUF$(I+1): NEXT I
660 IF BUFR > BUFL THEN BUFR = BUFL
665 RETURN
669     REM Rename the current buffer in X2$
670 X1$ = "File name:": GOSUB 960: IF X2$ = "" THEN REPEAT
675 RETURN
679     REM Find a string
680 X1$ = "Find:": GOSUB 960: IF X2$ <> "" THEN FS$ = X2$
690 L = LEN FS$
695 FOR I = BUFR TO BUFL
700     FOR J = 1 TO LEN(BUF$(I)) - L
705         IF BUF$(I)(J TO J + L - 1) = FS$ THEN BUFR = I, BUFC = J: RETURN
710     NEXT J
715 NEXT I
720 RETURN
739     REM Replace a string
740 X1$ = "Find:": GOSUB 960: IF X2$ <> "" THEN FS$ = X2$
745 X1$ = "Replace:": GOSUB 960: IF X2$ <> "" THEN RS$ = X2$
750 L = LEN FS$, C = BUFC
755 FOR I = BUFR TO BUFL
760     FOR J = C TO LEN(BUF$(I)) - L
765         IF BUF$(I)(J TO J + L - 1) = FS$ THEN 785
770     NEXT J
772     C = 1
775 NEXT I
780 RETURN
784     REM String found: position the cursor there
785 BUFR = I, BUFC = J
790 BUF$(I) = BUF$(I)(TO J -1) + RS$ + BUF$(I)(J + L TO)
795 RETURN
799     REM Create a new buffer
800 GOSUB 940   ' Check against unsaved changes
805 BUF$(1) = CHR$(10): FOR I = 2 TO BUFL: BUF$(I) = "": NEXT I
810 BUFL = 1, BUFR = 1, BUFC = 1, SAVED = 1
815 WINR = 1, WINC = 1, NAME$ = ""
820 RETURN
829     REM Load a buffer from the disk
830 GOSUB 940       ' Check against unsaved changes
835 GOSUB 670       ' Get buffer's name into X2$
840 ON ERROR 910    ' Print file error and return
845 OPEN 1, X2$, 0
850 NAME$ = X2$, SAVED = 1, BUFR = 1
855 IF NOT EOF(1) THEN LINPUT #1, BUF$(BUFR): BUFR = BUFR + 1: REPEAT
860 CLOSE 1
865 FOR I = BUFR TO BUFL: BUF$(I) = "": NEXT I
870 BUFL = BUFR, BUFR = 1
875 GOTO 2000       ' Refresh the screen and return
879     REM Save current buffer
880 IF NAME$ = "" THEN GOSUB 670: NAME$ = X2$   ' Get buffer's name
885 ON ERROR 910    ' Print file error and return
890 OPEN 1, NAME$, 1
895 FOR I = 1 TO BUFL: PRINT #1, BUF$(I): NEXT I
900 CLOSE 1
903 SAVED = 1
905 GOTO 2000       ' Refresh the screen and return
909     REM Arrive here in case of file error
910 X1$ = "FILE ERROR! Press ENTER": GOSUB 960
915 ON ERROR 0
920 GOTO 2000
929     REM Quit the editor
930 GOSUB 940
935 PRINT AT(HEIGHT,1)
936 END
939     REM Check against unsaved changes
940 IF SAVED THEN RETURN
945 X1$ = "Unsaved changes: save?": GOSUB 960
950 IF LEN X2$ > 0 THEN IF X2$(1) = "N" OR X2$(1) = "n" THEN SAVED = 1: RETURN
955 GOTO 880    ' Save the file
959     REM Prompt the user printing X1$ and returns the answer in X2$
960 PRINT AT(HEIGHT,1) X1$;: LINPUT X2$: RETURN
995    REM Insert line X1$ at X1: return Y1 <> 0 if there's no more room.
1000 Y1 = (BUFL = MAXBUF): IF Y1 THEN X1$ = "BUFFER FULL! Press ENTER": GOTO 960
1010 FOR I = BUFL TO X1 STEP -1: BUF$(I+1) = BUF$(I): NEXT I
1020 BUFL = BUFL + 1
1030 BUF$(X1) = X1$
1040 RETURN
1095    REM Delete line X1.
1100 IF X1 < BUFL THEN 1115
1105 BUFL = BUFL - 1: IF X1 > BUFL THEN X1 = BUFL
1110 RETURN
1115 FOR I = X1 TO BUFL - 1: BUF$(I) = BUF$(I+1): NEXT I
1120 IF BUFL > 1 THEN BUFL = BUFL - 1
1125 X1 = X1 - 1
1130 RETURN
2000    REM Refresh the screen
2005 REM Compute the size of the file
2010 L = 0: FOR I = 1 TO BUFL: L = L + LEN(BUF$(I)): NEXT I
2015 REM Print header and footer
2020 CLS: ATTR REVERSE = 1
2025 FOR I = 1 TO WIDTH: PRINT " ";: NEXT I
2030 PRINT AT(1,1) CHR$(42 - SAVED * 10) "FILE: " NAME$;
2035 PRINT TAB(WIDTH/2) "ROW: " BUFR "; COL: " BUFC "; SIZE: " L;
2045 FOR I = 1 TO WIDTH: PRINT AT(HEIGHT,I) " ": NEXT I
2050 PRINT AT(HEIGHT,1) "BATE (c) 2025 by Paolo Caressa [Type ESC H ESC for help]";
2055 ATTR REVERSE = 0
2060 REM Print the text
2065 WINC = 1
2070 REM If (BUFR,BUFC) is outside the window to print, adjust the window
2075 IF BUFC >= WINC + WIDTH THEN WINC = WINC + 1: REPEAT
2085 IF BUFR > WINR + HEIGHT - 3 THEN WINR = WINR + 1: REPEAT
2090 IF BUFR < WINR THEN WINR = WINR - 1: REPEAT
2095 PRINT AT(2,1);
2100 FOR I = WINR TO WINR + HEIGHT - 3
2105    IF I > BUFL THEN RETURN
2110    PRINT AT(I - WINR + 2, 1);
2115    L = LEN(BUF$(I))
2120    FOR J = WINC TO WINC + WIDTH
2125        IF J > L THEN 2155
2130        IF ASC(BUF$(I)(J)) = 10 THEN 2155
2135        ATTR REVERSE = (I = BUFR AND J = BUFC)
2140        PRINT BUF$(I)(J);
2145        ATTR REVERSE = 0
2150    NEXT J
2155    IF I = BUFR AND J = BUFC THEN ATTR REVERSE = 1: PRINT " ";: ATTR REVERSE = 0
2160 NEXT I
2190 RETURN
2199    REM Print the help message
2200 PRINT AT(3,1);: ATTR REVERSE = 1
2205 PRINT "HELP                                        "
2210 PRINT "Control codes:                              "
2215 PRINT "    ESC C   Copy the current line           "
2220 PRINT "    ESC D   Duplicate the current line      "
2225 PRINT "    ESC F   Find a string                   "
2230 PRINT "    ESC H   Print this Help message         "
2235 PRINT "    ESC I   Insert the copied line          "
2240 PRINT "    ESC K   Cut the current line            "
2245 PRINT "    ESC M   Rename the current buffer       "
2250 PRINT "    ESC N   Erase the current buffer        "
2255 PRINT "    ESC O   Load a buffer from disk         "
2260 PRINT "    ESC Q   Quit the editor                 "
2265 PRINT "    ESC R   Replace a string                "
2270 PRINT "    ESC S   Save the current buffer         "
2280 PRINT "Copied/cutted lines are remembered          "
2285 PRINT "To reset search/replace strings press ENTER "
2290 ATTR REVERSE = 0
2292 X1$ = "Press ENTER to return to the buffer"
2295 GOTO 960
