  10 DIM A(10, 10)
  20 FOR I = 1 TO 10
  30 FOR J = 1 TO 10
  40 READ A(I, J)
  50 NEXT J
  60 NEXT I
 100 FOR I = 1 TO 10
 110 FOR J = 1 TO 10
 120 PRINT A(I, J); " ";
 130 NEXT J
 135 PRINT
 140 NEXT I
1000 DATA 1,2,0,0,0,0,0,0,0,0
1001 DATA -2,1,2,0,0,0,0,0,0,0
1002 DATA 0,-2,1,2,0,0,0,0,0,0
1003 DATA 0,0,-2,1,2,0,0,0,0,0
1004 DATA 0,0,0,-2,1,2,0,0,0,0
1005 DATA 0,0,0,0,-2,1,2,0,0,0
1006 DATA 0,0,0,0,0,-2,1,2,0,0
1007 DATA 0,0,0,0,0,0,-2,1,2,0
1008 DATA 0,0,0,0,0,0,0,-2,1,2
1009 DATA 0,0,0,0,0,0,0,0,-2,1