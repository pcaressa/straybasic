10 REM Simple Line Editor
20 LET N = 1024: REM Maximum number of lines in a file
30 DIM T$(N): REM The list of all file lines
40 LET TOP = 1: REM First free line
100 CLS: REM This command clears the screen
120 PRINT "A simple line editor"
124 PRINT ,"0) Exit"
126 PRINT ,"1) List the current buffer"
128 PRINT ,"2) Append a line"
130 PRINT ,"3) Delete a line"
132 PRINT ,"4) Insert a line"
134 PRINT ,"5) Update a line"
140 INPUT "Choose a task"; C
150 IF C = 0 THEN STOP
160 ON INT C GOSUB 1000, 1100, 1200, 1300, 1400
170 GOTO 120
1000 REM SUBROUTINE: List the current buffer
1010 FOR I = 1 TO TOP - 1
1020 PRINT "["I"] " T$(I)
1030 NEXT I
1040 RETURN
1050 REM SUBROUTINE: Query the user for a line number and store it into L
1051 REM If the inserted line is invalid, OK is set to 0 else to 1
1055 LET OK = 0
1060 INPUT "Line Number? "; L
1065 LET L = INT L
1070 IF L < 1 OR L >= TOP THEN PRINT "Invalid line number!": RETURN
1080 LET OK = 1
1090 RETURN
1100 REM SUBROUTINE: Append a line to the current buffer
1110 IF TOP > N THEN PRINT "No more space for lines!": RETURN
1120 INPUT "?"; T$(TOP)
1130 LET TOP = TOP + 1
1190 RETURN
1200 REM SUBROUTINE: Delete a line from the current buffer
1210 GOSUB 1050: IF NOT OK THEN RETURN
1230 REM Shifts leftward all lines from the L-th
1240 LET TOP = TOP - 1
1250 FOR I = L TO TOP - 1
1260 LET T$(I) = T$(I + 1)
1270 NEXT I
1290 RETURN
1300 REM SUBROUTINE: Insert a line before another one
1305 IF TOP > N THEN PRINT "No more space for lines!": RETURN
1310 GOSUB 1050: IF NOT OK THEN RETURN
1330 REM Shift rightward
1340 FOR I = TOP TO L + 1 STEP -1
1350 LET T$(I) = T$(I - 1)
1360 NEXT I
1370 INPUT "Line to insert? "; T$(L)
1380 TOP = TOP + 1
1390 RETURN
1400 REM SUBROUTINE: Insert a line before another one
1405 IF TOP > N THEN PRINT "No more space for lines!": RETURN
1410 GOSUB 1050: IF NOT OK THEN RETURN
1430 INPUT "New line? "; T$(L)
1440 RETURN