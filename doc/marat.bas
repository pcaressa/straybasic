1   REM MARs ATtacks

100     REM Program data
110 WIDTH = 70, HEIGHT = 20 ' Screen dimensions
115 ER = 8                  ' Earth radius
120 PI2 = 3.14159 / 2       ' PI/2
150 DIM S$(HEIGHT, WIDTH)   ' The screen
151 DIM C(HEIGHT, WIDTH)    ' Screen attribute: fore + 8*back
154 REM Initialize S$ to blanks
155 FOR I = 1 TO HEIGHT
156     FOR J = 1 TO WIDTH
157         S$(I,J) = " ", C(I,J) = 0
159     NEXT J
160 NEXT I
164 REM The Earth is a semicircle of radius ER: draw it
165 X = HEIGHT/2 - ER
167 R = 0                   ' When 0 the game is over
170 FOR I = -ER TO ER
172     FOR J = 0 TO 1.8*SQR(ER*ER - I*I)
174         C(X,J+1) = 3*8, R = R + 1
176     NEXT J
178     X = X + 1
180 NEXT I
200 REM Data about the starship
210 SX = 2*ER + 4, SY = HEIGHT/2  ' Starship coordinates
215 SE = 8  ' Initial energy
220 GOSUB 850   ' Draw the ship
225 SHOTX = 0, SHOTY = 0    ' No shot at the moment

250 REM Data about martians
255 DIM M(8, 3) ' Up to 8 simultaneous martians
260 NM = 0      ' Current number of martians

400 FOR I = 1 to 100000: NEXT I: GOSUB 950  ' Refresh the screen
405 K = INKEY   ' Key pressed?
410 IF K = 81 OR K = 113 THEN GOSUB 550
415 IF K = 90 OR K = 122 THEN GOSUB 570
420 IF K = 32 THEN GOSUB 590
425 IF K = 73 OR K = 95 THEN GOTO 
450 IF SHOTX > 0 THEN GOSUB 600
455 REM Update martians positions
460 FOR I = 1 TO NM
465     X = M(I,2), Y = M(I,1)
470     IF X = 0 THEN GOSUB create martian
475     S$(X,Y) = " ", C(X,Y) = 0
480     X = X - M(I,3), M(I,2) = X: IF X <= 0 THEN 505
485     S$(X,Y) = "*": C(X,Y) = 4   ' Martians are red
490     REM Check if i-th martian reached the ship
495     IF X >= SX AND X <= SX + 3 AND Y >= SY-1 AND Y <= SY+1 THEN GOTO 700
499     REM Check if i-th martian reached the Earth
500     IF C(X,Y) = 3*8 THEN C(X,Y) = 0, M(I,2) = 0 ' mark the martian as dead
505 NEXT I

510 REM Create new martians if there's space
515 IF NM = 8 THEN GOTO 400
520 NM = NM + 1
525 M(NM,1) = WIDTH
530 M(NM,2) = INT(RND*HEIGHT) + 1
535 M(NM,3) = 1 + INT(RND*3)
540 GOTO 400

549 REM Move the ship up
550 GOSUB 800
555 IF SY > 2 THEN SY = SY - 1
560 GOTO 850
569 REM Move the ship down
570 GOSUB 800
575 IF SY < HEIGHT - 2 THEN SY = SY + 1
580 GOTO 850
589 REM Shot if it can
590 IF SE > 0 AND SHOTX = 0 THEN SHOTX = SX + 3: SHOTY = SY: SE = SE - 1
595 RETURN
599 REM Update shot position and check martian hit
600 S(SHOTX, SHOTY) = " ", C(SHOTX, SHOTY) = 0
605 SHOTX = SHOTX + 1: IF SHOTX > WIDTH THEN SHOTX = 0: RETURN
610 FOR I = 1 TO NM
615     IF M(I,1) = SHOTX AND M(I,2) = SHOTY THEN 650
620 NEXT I
625 S(SHOTX, SHOTY) = "-", C(SHOTX, SHOTY) = 7
630 RETURN
649 ' Here Martian I-th was destroyed
650 SE = SE + M(I,3)
655 SHOTX = 0
659 REM Martian is dead: drop it.
660 M(I,1) = M(NM,1), M(I,2) = M(NM,2), M(I,3) = M(NM,3)
665 NM = NM - 1
670 RETURN

699     REM Game over with ship destroyed
700 PRINT AT(HEIGHT/2,3);"GAME OVER: YOU LOST"
705 ATTR FORE = 2, BACK = 0
715 END

729     REM Game stopped
730 PRINT AT(HEIGHT/2,3);"GAME STOPPED"
735 ATTR FORE = 2, BACK = 0
740 END


799     REM Delete the ship
800 S$(SY-1, SX) = " ", C(SY-1, SX) = 1
802 S$(SY, SX) = " ", C(SY, SX) = 1
804 S$(SY, SX+1) = " ", C(SY, SX+1) = 1
806 S$(SY, SX+2) = " ", C(SY, SX+2) = 1
808 S$(SY, SX+3) = " ", C(SY, SX+3) = 1
810 S$(SY+1, SX) = " ", C(SY+1, SX) = 1
815 RETURN
819     REM Draw the ship
850 S$(SY-1, SX) = "\", C(SY-1, SX) = 1
852 S$(SY, SX) = " ", C(SY, SX) = 1+8
854 S$(SY, SX+1) = " ", C(SY, SX+1) = 1+8
856 S$(SY, SX+2) = " ", C(SY, SX+2) = 1+8
858 S$(SY, SX+3) = ">", C(SY, SX+3) = 1
860 S$(SY+1, SX) = "/", C(SY+1, SX) = 1
865 RETURN

949     REM Refresh the screen
950 CLS
955 FOR I = 1 TO HEIGHT
960    FOR J = 1 TO WIDTH
965        B = INT(C(I,J)/8)
970        ATTR BACK = B, FORE = C(I,J) - B
975        PRINT S$(I,J);
980    NEXT J
985    PRINT
990 NEXT I
992 ATTR FORE = 2, BACK = 0
995 RETURN
