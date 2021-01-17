	lw 0 1 value
	lw 0 2 count
	lw 0 3 one
	lw 0 4 sixty
	noop
start	beq 2 4 done
	add 2 3 2
	add 1 2 1
	beq 0 0 start
done	halt
	noop
value	.fill 5
count	.fill 0
one	.fill 1
sixty	.fill 60
