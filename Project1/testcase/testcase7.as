	lw 0 0 base
	lw 0 1 init
	lw 0 2 count
	lw 0 3 one
	lw 0 4 ten
	noop
start	beq 2 4 done
	add 3 2 2
	add 1 1 1
	beq 0 0 start
done	halt
	noop
base	.fill 0
init	.fill 111111
one	.fill 1
ten	.fill 10
count 	.fill 0
