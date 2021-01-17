	lw 0 1 data	# lw generates result in WB stage
	add 2 1 1	# but, add instr. use register 1 before WB stage 
	halt		# therefore, data hazard occurs
data	.fill 3
