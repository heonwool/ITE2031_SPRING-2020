#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for this project */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDStruct {
	int instr;
	int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
} IDEXType;

typedef struct EXMEMStruct {
	int instr;
	int branchTarget;
	int aluResult;
	int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
	int instr;
	int writeData;
} MEMWBType;

typedef struct WBENDStruct {
	int instr;
	int writeData;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* number of cycles run so far */
} stateType;

void printInitialInst(stateType *statePtr);
void printState(stateType *statePtr);
int convertNum(int num);

int field0(int instruction) {
	return( (instruction>>19) & 0x7);
}

int field1(int instruction) {
	return( (instruction>>16) & 0x7);
}

int field2(int instruction) {
	return(instruction & 0xFFFF);
}

int opcode(int instruction) {
	return(instruction>>22);
}

void printInstruction(int instr)
{
	char opcodeString[10];
	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	} else if (opcode(instr) == NOR) {
		strcpy(opcodeString, "nor");
	} else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	} else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	} else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	} else if (opcode(instr) == JALR) {
		strcpy(opcodeString, "jalr");
	} else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	} else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	} else {
		strcpy(opcodeString, "data");
	}
	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr), field2(instr));
}

void flushIFID(stateType *statePtr) {
	statePtr->IFID.instr=NOOPINSTRUCTION;
	statePtr->IFID.pcPlus1=0;
}

void flushIDEX(stateType *statePtr) {
	statePtr->IDEX.instr=NOOPINSTRUCTION;
	statePtr->IDEX.pcPlus1=0;
	statePtr->IDEX.readRegA=0;
	statePtr->IDEX.readRegB=0;
	statePtr->IDEX.offset=0;
}

void flushEXMEM(stateType *statePtr) {
	statePtr->EXMEM.instr=NOOPINSTRUCTION;
	statePtr->EXMEM.branchTarget=0;
	statePtr->EXMEM.aluResult=0;
	statePtr->EXMEM.readRegB=0;
}

void flushMEMWB(stateType *statePtr) {
	statePtr->MEMWB.instr=NOOPINSTRUCTION;
	statePtr->MEMWB.writeData=0;
}

void flushWBEND(stateType *statePtr) {
	statePtr->WBEND.instr=NOOPINSTRUCTION;
	statePtr->WBEND.writeData=0;
}

void init(stateType *statePtr) {
	// initialize register with 0
	statePtr->pc = 0;
	statePtr->cycles = 0;
	for (int i = 0; i < NUMREGS; i++) {
		statePtr->reg[i] = 0;
	}

	flushIFID(statePtr);
	flushIDEX(statePtr);
	flushEXMEM(statePtr);
	flushMEMWB(statePtr);
	flushWBEND(statePtr);
}

int dataHazard(stateType *statePtr, int inputField) {
	int result, regNum;
	int condition;

	switch(inputField){
	case 0:
		result = statePtr->IDEX.readRegA;
		regNum = field0(statePtr->IDEX.instr);
		break;
	case 1:
		result = statePtr->IDEX.readRegB;
		regNum = field1(statePtr->IDEX.instr);
		break;
	}

	condition = (opcode(statePtr->WBEND.instr) == ADD || opcode(statePtr->WBEND.instr) == NOR);
	if (condition && (field2(statePtr->WBEND.instr) == regNum))
		result = statePtr->WBEND.writeData;

	condition = (opcode(statePtr->MEMWB.instr) == ADD || opcode(statePtr->MEMWB.instr) == NOR);
	if (condition && (field2(statePtr->MEMWB.instr) == regNum))
		result = statePtr->MEMWB.writeData;
	
	condition = (opcode(statePtr->EXMEM.instr) == ADD || opcode(statePtr->EXMEM.instr) == NOR);
	if (condition && (field2(statePtr->EXMEM.instr) == regNum))
		result = statePtr->EXMEM.aluResult;

	if (opcode(statePtr->WBEND.instr) == LW && field1(statePtr->WBEND.instr) == regNum)
		result = statePtr->WBEND.writeData;

	if (opcode(statePtr->MEMWB.instr) == LW && field1(statePtr->MEMWB.instr) == regNum)
		result = statePtr->MEMWB.writeData;

	return result;
}

void forwardOnHazard(stateType *statePtr, int *inputReg1, int *inputReg2) {
	int curInst, prevInst, destReg, regA, regB;

	curInst = opcode(statePtr->IDEX.instr);
	prevInst = opcode(statePtr->WBEND.instr);

	if(curInst == JALR || curInst == HALT || curInst == NOOP)
		return;

	regA = field0(statePtr->IDEX.instr);
	regB = field1(statePtr->IDEX.instr);

	prevInst = opcode(statePtr->WBEND.instr);
	switch(prevInst) {
	case LW:
		destReg = field1(statePtr->WBEND.instr);
		if(regA == destReg)
			*inputReg1 = statePtr->WBEND.writeData;
		if(regB == destReg)
			*inputReg2 = statePtr->WBEND.writeData;
		break;

	case ADD:
	case NOR:
		destReg = field2(statePtr->WBEND.instr);
		if(regA == destReg)
			*inputReg1 = statePtr->WBEND.writeData;
		if(regB == destReg)
			*inputReg2 = statePtr->WBEND.writeData;
		break;
	}

	prevInst = opcode(statePtr->MEMWB.instr);
	switch(prevInst) {
	case LW:
		destReg = field1(statePtr->MEMWB.instr);
		if(regA == destReg)
			*inputReg1 = statePtr->MEMWB.writeData;
		if(regB == destReg)
			*inputReg2 = statePtr->MEMWB.writeData;
		break;

	case ADD:
	case NOR:
		destReg = field2(statePtr->MEMWB.instr);
		if(regA == destReg)
			*inputReg1 = statePtr->MEMWB.writeData;
		if(regB == destReg)
			*inputReg2 = statePtr->MEMWB.writeData;
		break;
	}

	prevInst = opcode(statePtr->EXMEM.instr);
	switch(prevInst) {
	case ADD:
	case NOR:
		destReg = field2(statePtr->EXMEM.instr);
		if(regA == destReg)
			*inputReg1 = statePtr->EXMEM.aluResult;
		if(regB == destReg)
			*inputReg2 = statePtr->EXMEM.aluResult;
		break;
	}


}

int main(int argc, char *argv[]) {
	char line[MAXLINELENGTH];
	stateType state, newState;
	FILE *filePtr;

	int regA, regB, muxRegA, muxRegB;

	init(&state);
	init(&newState);

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
		if (sscanf(line, "%d", state.instrMem + state.numMemory) != 1) {
			printf("error in reading address %d\n", state.numMemory);
			exit(1);
		}
		printf("memory[%d]=%d\n", state.numMemory, state.instrMem[state.numMemory]);
	}

	state.IFID.instr = NOOPINSTRUCTION;
	state.IDEX.instr = NOOPINSTRUCTION;
	state.EXMEM.instr = NOOPINSTRUCTION;
	state.MEMWB.instr = NOOPINSTRUCTION;
	state.WBEND.instr = NOOPINSTRUCTION;

	printInitialInst(&state);

	while(1) {
		printState(&state);

		/* check for halt */
		if (opcode(state.MEMWB.instr) == HALT) {
			printf("machine halted\n");
			printf("total of %d cycles executed\n", state.cycles);
			exit(0);
		}

		newState = state;
		newState.cycles++;

		/* --------------------- IF stage --------------------- */
		newState.pc = state.pc + 1;
		newState.IFID.instr = state.instrMem[state.pc];
		newState.IFID.pcPlus1 = state.pc + 1;

		/* --------------------- ID stage --------------------- */
		newState.IDEX.instr = state.IFID.instr;
		newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
		newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
		newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
		newState.IDEX.offset = convertNum(field2(state.IFID.instr));

		if(opcode(state.IDEX.instr) == LW) {
			int dependReg;
			dependReg = field1(state.IDEX.instr);

			switch(opcode(state.IFID.instr)) {
			case ADD:
			case NOR:
			case BEQ:
			case SW:
				if(field0(state.IFID.instr) == dependReg || field1(state.IFID.instr) == dependReg) {
					newState.pc = state.pc;

					newState.IFID.instr = state.IFID.instr;
					newState.IFID.pcPlus1 = state.pc;

					newState.IDEX.instr = NOOPINSTRUCTION;
					newState.IDEX.pcPlus1 = 0;
					newState.IDEX.offset = 0;
				}
				break;

			case LW:
				if(field0(state.IFID.instr) == dependReg){
					newState.pc = state.pc;

					newState.IFID.instr = state.IFID.instr;
					newState.IFID.pcPlus1 = state.pc;

					newState.IDEX.instr = NOOPINSTRUCTION;
					newState.IDEX.pcPlus1 = 0;
					newState.IDEX.offset = 0;
				}
				break;

			case JALR:
			case HALT:
			case NOOP:
			default:
				break;
			}
		}

		/* --------------------- EX stage --------------------- */
		newState.EXMEM.instr = state.IDEX.instr;
		newState.EXMEM.branchTarget = state.IDEX.offset + state.IDEX.pcPlus1;
		newState.EXMEM.readRegB = state.IDEX.readRegB;

		regA = dataHazard(&state, 0);
		regB = dataHazard(&state, 1);
		muxRegA = state.IDEX.readRegA;
		muxRegB = state.IDEX.readRegB;

		forwardOnHazard(&state, &muxRegA, &muxRegB);
		regA = muxRegA;
		regB = muxRegB;

		switch(opcode(state.IDEX.instr)){
		case ADD:
			newState.EXMEM.aluResult = regA + regB;
			break;

		case NOR:
			newState.EXMEM.aluResult = ~(regA | regB);
			break;

		case LW:
		case SW:
			newState.EXMEM.aluResult = regA + state.IDEX.offset;
			break;

		case BEQ:
			newState.EXMEM.aluResult = regA - regB;
			break;

		case JALR:
		case HALT:
		case NOOP:
		default:
			break;
		}

		if(!state.EXMEM.aluResult && opcode(state.EXMEM.instr) == BEQ) {
			flushIFID(&newState);
			flushIDEX(&newState);
			flushEXMEM(&newState);

			newState.pc = state.EXMEM.branchTarget;
		}

		/* --------------------- MEM stage --------------------- */
		newState.MEMWB.instr = state.EXMEM.instr;

		switch(opcode(state.EXMEM.instr)) {
		case ADD:
		case NOR:
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			break;

		case LW:
			newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
			break;

		case SW:
			newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;

		case BEQ:
		case JALR:
		case HALT:
		case NOOP:
		default:
			break;
		}

		/* --------------------- WB stage --------------------- */
		newState.WBEND.instr = state.MEMWB.instr;
		newState.WBEND.writeData = state.MEMWB.writeData;

		switch(opcode(state.MEMWB.instr)) {
		case ADD:
		case NOR:
			newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
			break;

		case LW:
			newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
			break;

		case SW:
		case BEQ:
		case JALR:
		case HALT:
		case NOOP:
		default:
			break;
		}

		state = newState;
	}
}

void printInitialInst(stateType *statePtr) {
	int i;
	printf("%d memory words\n\tinstruction memory:\n", statePtr->numMemory);
	for(i=0; i<statePtr->numMemory; i++) {
		statePtr->dataMem[i] = statePtr->instrMem[i];
		printf("\t\tinstrmem[ %d ] ", i);
		printInstruction(statePtr->instrMem[i]);
	}
}

void printState(stateType *statePtr) {
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
	printf("\tpc %d\n", statePtr->pc);

	printf("\tdata memory:\n");
	for (i=0; i<statePtr->numMemory; i++) {
		printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}

	printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}

	printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);

	printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX.offset);

	printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM.instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);

	printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);

	printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
}

int convertNum(int num) {
	// Converts 16-bit number into a 32-bit number
	if (num & (1 << 15)) num -= (1 << 16);
	return num;
}