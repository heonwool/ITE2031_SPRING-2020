	lw 0 1 data1
	lw 0 2 data2
	lw 0 3 data3
start	add 1 2 1
	beq 1 3 2
	beq 0 0 start
	noop
done	halt
data1	.fill 10
data2	.fill -2
data3	.fill 0
