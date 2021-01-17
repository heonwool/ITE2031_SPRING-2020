	lw 0 1 one	# simple branch hazard
	lw 0 2 two
	noop		# inserted noop instructions
	noop		# in order to remove the possibility of data hazard
	noop
	beq 0 0 here	# branch-not-taken to resolve branch hazard
	add 1 1 2	  <- need to flush this instr.
	add 2 1 2	  <- need to flush this instr.
here	lw 0 3 three	
	halt
one	.fill 1
two	.fill 2
three	.fill 3
