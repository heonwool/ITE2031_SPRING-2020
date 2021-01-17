// LC-2K Instruction-level simulator
// 2016024875 Jeongwoo Son (2020-Spring)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

typedef struct stateStruct {
	int pc;
	int mem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
} stateType;

enum opCode { ADD = 0, NOR, LW, SW, BEQ, JALR, HALT, NOOP };

void printState(stateType *);
int convertNum(int num);

void parseAndDecode(stateType *statePtr, int *opcode, int *arg0, int *arg1, int *arg2);
int isValidReg(int arg);

void execRTypeInst(stateType *statePtr, int opcode, int arg0, int arg1, int destination);
void execITypeInst(stateType *statePtr, int opcode, int arg0, int arg1, int offset);
void execJTypeInst(stateType *statePtr, int opcode, int arg0, int arg1);

int isValidReg(int arg) {
	if(arg < 0 || arg >= NUMREGS)
		return 0;
	else
		return 1;
}

void parseAndDecode(stateType *statePtr, int *opcode, int *arg0, int *arg1, int *arg2) {
	int mc = statePtr->mem[statePtr->pc];

	*opcode = (mc >> 22) % 8;
	*arg0 = (mc >> 19) % 8;
	*arg1 = (mc >> 16) % 8;
	*arg2 = (mc & 0xFFFF);
}

void execRTypeInst(stateType *statePtr, int opcode, int arg0, int arg1, int destination) {
	if(!isValidReg(destination)) {
		printf("error: invalid register in execRTypeInst()\n");
		printf("raised state.pc %d\n", statePtr->pc);
		exit(1);
	} 

	switch (opcode) {
		case ADD:
			statePtr->reg[destination] = statePtr->reg[arg0] + statePtr->reg[arg1];
			break;
		case NOR:
			statePtr->reg[destination] = ~(statePtr->reg[arg0] | statePtr->reg[arg1]);
			break;
		default:
			printf("error: unknown error in execRTypeInst()\n");
			printf("raised state.pc %d\n", statePtr->pc);
			exit(1);
			break;
	}
}

void execITypeInst(stateType *statePtr, int opcode, int arg0, int arg1, int offset) {
	offset = convertNum(offset);
	if (offset > 32767 || offset < -32768) {
		printf("error: invalid offset (out of memory)\n");
		printf("raised state.pc %d\n", statePtr->pc);
		exit(1);
	}

	int newOffset = statePtr->reg[arg0] + offset;
	switch (opcode) {
		case LW:
			statePtr->reg[arg1] = statePtr->mem[newOffset];
			break;
		case SW:
			statePtr->mem[newOffset] = statePtr->reg[arg1]; 
			break;
		case BEQ:
			if (statePtr->reg[arg0] == statePtr->reg[arg1])
				statePtr->pc = statePtr->pc + offset;
			break; 
		default:
			printf("error: unknown error in execITypeInst()\n");
			printf("raised state.pc %d\n", statePtr->pc);
			exit(1);
			break;
	}
}

void execJTypeInst(stateType *statePtr, int opcode, int arg0, int arg1) {
	switch (opcode) {
		case JALR:
			statePtr->reg[arg1] = statePtr->pc;
			statePtr->pc = statePtr->reg[arg0];
			break;
		default:
			printf("error: unknown error in execJTypeInst()\n");
			printf("raised state.pc %d\n", statePtr->pc);
			exit(1);
			break;
	}
}

int main(int argc, char *argv[]) { 
	char line[MAXLINELENGTH];
	stateType state;
	FILE *filePtr;

	if (argc != 2) {
		printf("error: usage: %s <machine-code file>\n", argv[0]);
		exit(1);
	}

	filePtr = fopen(argv[1], "r");
	if (filePtr == NULL) {
		printf("error: can't open file %s", argv[1]);
		perror("fopen");
		exit(1);
	}

	/* read in the entire machine-code file into memory */
	for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
		if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
			printf("error in reading address %d\n", state.numMemory);
			exit(1);
		}
		printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
	}

	// initialize register with 0
	for (int i = 0; i < NUMREGS; i++) {
		state.reg[i] = 0;
	}

	// print initial state
	printState(&state);

	// count the total executed instructions
	int execInstIdx;

	for(execInstIdx = 1; ; execInstIdx++) {
		int isHalt = 0;
		int opcode, arg0, arg1, arg2;

		parseAndDecode(&state, &opcode, &arg0, &arg1, &arg2);

		state.pc++;

		if(state.pc < 0 || state.pc >= NUMMEMORY) {
			printf("error: invalid state.pc (out of memory)\n");
			exit(1);
		}

		if(!isValidReg(arg0) || !isValidReg(arg1)) {
			printf("error: invalid register\n");
			printf("raised state.pc %d\n", state.pc);
			exit(1);
		}

		switch(opcode) {
			case ADD:
			case NOR:
				execRTypeInst(&state, opcode, arg0, arg1, arg2);
				break;
			case LW:
			case SW:
			case BEQ:
				execITypeInst(&state, opcode, arg0, arg1, arg2);
				break;
			case JALR:
				execJTypeInst(&state, opcode, arg0, arg1);
				break;
			case HALT:
				isHalt = 1;
				break;
			case NOOP:
				break;
			default:
				printf("error: unsupported opcode\n");
				exit(1);
				break;
		}

		if(isHalt)
			break;

		printState(&state);

	}

	printf("machine halted\n");
	printf("total of %d instructions executed\n", execInstIdx);
	printf("final state of machine:\n");
	printState(&state);


	fclose(filePtr);

	return(0);
}

void printState(stateType *statePtr) { 
	int i;

	printf("@@@\nstate:\n");
	printf("\tpc %d\n", statePtr->pc);
	printf("\tmemory:\n");

	for (i=0; i<statePtr->numMemory; i++) {
		printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
	}
	printf("\tregisters:\n");

	for (i=0; i<NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}

	printf("end state\n");
}

int convertNum(int num) {
	/* convert a 16-bit number into a 32-bit Linux integer */
	if (num & (1<<15)) {
		num -= (1<<16);
	}
	return(num);
}