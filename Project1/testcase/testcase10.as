	lw 0 2 mcand	mcand
	lw 0 3 mplier	mplier
	lw 0 4 max	reg that stores max value; initialize with 2^16
	lw 0 5 cnt	reg for calculation index
	noop
loop	nor 3 3 3	2's complement of mplier
	nor 5 5 5	2's complement of cnt
	nor 3 5 6	calculate reg3 nor reg5 and store it at reg6
	nor 3 3 3	restore mplier
	nor 5 5 5	restore cnt
	beq 0 6 pass	if reg6 != 0: pass
	beq 0 0 addRes	else: addRes
addRes	add 2 1 1	add mcand to result
pass	add 5 5 5	cnt = cnt << 1
	add 2 2 2	mcand = mcand << 1
	beq 5 4 done	if index reached max: terminate
	beq 0 0 loop	else: resume the loop
	noop
done	halt
mcand	.fill 12345
mplier	.fill 23456
max	.fill 32768
cnt	.fill 1