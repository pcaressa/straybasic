10 REM Find a match
20 INPUT "Match"; X$
30 REM To terminate insert an empty string
40 IF X$ = "" THEN STOP
50 REM Convert X$ to lowercase
60 FOR J = 1 TO LEN X$
70 IF X$(J) >= "A" AND X$(J) <= "Z" THEN X$(J) = CHR$(ASC(X$(J)) + 32)
80 NEXT J
90 RESTORE
100 REM Read a verse and check it
110 READ V$
120 IF V$ = "#" THEN 20
130 FOR I = 0 TO LEN V$ - LEN X$ - 1
140 REM Compare X$ with V$(I+1 TO I+LEN X$)
150 FOR J = 1 TO LEN X$
160 LET C$ = V$(I + J)
170 REM Convert to lowercase if needed
180 IF C$ >= "A" AND C$ <= "Z" THEN C$ = CHR$(ASC(C$) + 32)
190 IF C$ <> X$(J) THEN 230: REM No match
200 NEXT J
210 REM Match found!
220 PRINT "Match at "i" in "V$
230 NEXT I
240 GOTO 100
1000 REM Hamlet speech
1001 DATA "To be, or not to be, that is the question:"
1002 DATA "Whether 'tis nobler in the mind to suffer"
1003 DATA "The slings and arrows of outrageous fortune,"
1004 DATA "Or to take arms against a sea of troubles"
1005 DATA "And by opposing end them. To die—to sleep,"
1006 DATA "No more; and by a sleep to say we end"
1007 DATA "The heart-ache and the thousand natural shocks"
1008 DATA "That flesh is heir to: 'tis a consummation"
1009 DATA "Devoutly to be wish'd. To die, to sleep;"
1010 DATA "To sleep, perchance to dream—ay, there's the rub:"
1011 DATA "For in that sleep of death what dreams may come,"
1012 DATA "When we have shuffled off this mortal coil,"
1013 DATA "Must give us pause—there's the respect"
1014 DATA "That makes calamity of so long life."
1015 DATA "For who would bear the whips and scorns of time,"
1016 DATA "Th'oppressor's wrong, the proud man's contumely,"
1017 DATA "The pangs of dispriz'd love, the law's delay,"
1018 DATA "The insolence of office, and the spurns"
1019 DATA "That patient merit of th'unworthy takes,"
1020 DATA "When he himself might his quietus make"
1021 DATA "With a bare bodkin? Who would fardels bear,"
1022 DATA "To grunt and sweat under a weary life,"
1023 DATA "But that the dread of something after death,"
1024 DATA "The undiscovere'd country, from whose bourn"
1025 DATA "No traveller returns, puzzles the will,"
1026 DATA "And makes us rather bear those ills we have"
1027 DATA "Than fly to others that we know not of?"
1028 DATA "Thus conscience doth make cowards of us all,"
1029 DATA "And thus the native hue of resolution"
1030 DATA "Is sicklied o'er with the pale cast of thought,"
1031 DATA "And enterprises of great pith and moment"
1032 DATA "With this regard their currents turn awry"
1033 DATA "And lose the name of action."
1035 DATA #
