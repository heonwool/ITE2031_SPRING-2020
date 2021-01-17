// Assembler code for LC-2K 
// 2016024875 Jeongwoo Son (2020-Spring)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINELENGTH 1000

#define NUMREGS 8
#define MAXMEM 65536

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

int makeRTypeInst(char *opcode, char *arg0, char *arg1, char *destination);
int makeITypeInst(FILE *file, int PC, char *opcode, char *arg0, char *arg1, char *offset);
int makeJTypeInst(char *opcode, char *arg0, char *arg1);
int makeOTypeInst(char *opcode);

int findIndex(FILE *file, char *offset);

int makeRTypeInst(char *opcode, char *arg0, char *arg1, char *destination) {
	int num_op = 0;
	int num_arg0, num_arg1, num_dest;

	if(!isNumber(arg0) || !isNumber(arg1) || !isNumber(destination)) {
		printf("error: non numeric arguments in r-type instructions\n");
		exit(1);
	}

	num_arg0 = atoi(arg0);
	num_arg1 = atoi(arg1);
	num_dest = atoi(destination);

	if (!strcmp(opcode, "add"))
		num_op = 0; // 000		

	else if (!strcmp(opcode, "nor"))
		num_op = 1;	// 001

	return (num_op << 22) + (num_arg0 << 19) + (num_arg1 << 16) + num_dest;
}

int makeITypeInst(FILE *file, int PC, char *opcode, char *arg0, char *arg1, char *offset) {
	int num_op = 0;
	int num_arg0, num_arg1, num_offset;

	if(!isNumber(arg0) || !isNumber(arg1)) {
		printf("error: non numeric arguments in i-type instructions\n");
		exit(1);
	}

	if (!strcmp(opcode, "lw"))
		num_op = 2; // 010

	else if (!strcmp(opcode, "sw"))
		num_op = 3;	// 011

	else if (!strcmp(opcode, "beq"))
		num_op = 4;	// 100

	num_arg0 = atoi(arg0);
	num_arg1 = atoi(arg1);

	if(isNumber(offset)) {
		num_offset = atoi(offset);
		if(num_offset > 32767 || num_offset < -32768) {
			printf("error: offset out of boundary\n");
			exit(1);
		}
	}

	else {
		num_offset = findIndex(file, offset);
	}

	if(!strcmp(opcode, "beq") && !isNumber(offset)) {
		num_offset = num_offset - PC - 1;
		if(num_offset > 32767 || num_offset < -32768) {
			printf("error: offset out of 16-bit boundary\n");
			exit(1);
		}
	}

	if(num_offset < 0)
		num_offset += 65536;

	return (num_op << 22) + (num_arg0 << 19) + (num_arg1 << 16) + num_offset;
	
}

int makeJTypeInst(char *opcode, char *arg0, char *arg1) {
	int num_op = 0;
	int num_arg0, num_arg1, num_dest;

	if(!isNumber(arg0) || !isNumber(arg1)) {
		printf("error: non numeric arguments in j-type instructions\n");
		exit(1);
	}

	if (!strcmp(opcode, "jalr"))
		num_op = 5; // 101

	num_arg0 = atoi(arg0);
	num_arg1 = atoi(arg1);

	return (num_op << 22) + (num_arg0 << 19) + (num_arg1 << 16);
}

int makeOTypeInst(char *opcode) {
	int num_op;

	if (!strcmp(opcode, "halt"))
		num_op = 6; // 110
	else if (!strcmp(opcode, "noop"))
		num_op = 7; // 111

	return (num_op << 22);
} 

int main(int argc, char *argv[]) {
    char *inFileString, *outFileString;
    FILE *inFilePtr, *inFilePtrForIndex, *outFilePtr;

    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH];
    char arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc!= 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n", argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    inFilePtrForIndex = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
    }

    /*

    readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2);
    printf("label: %s\n", label);
    printf("opcode: %s\n", opcode);
    printf("arg0: %s\n", arg0);
    printf("arg1: %s\n", arg1);
    printf("arg2: %s\n", arg2);

	*/

	/* after doing a readAndParse, you may want to do the following to test the opcode */
	/* this is how to rewind the file ptr so that you start reading from the beginning of the file */
	// rewind(inFilePtr);

    int PC;

    for(PC = 0; ; PC++) {
	  	if (! readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2) ) {
	    	break;
	    }
	    
	    int mc_result;

	    // R-type instructions
	    if (!strcmp(opcode, "add") || !strcmp(opcode, "nor")) {
	    	mc_result = makeRTypeInst(opcode, arg0, arg1, arg2);
	    }

		// I-type instructions
	    else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw") || !strcmp(opcode, "beq")) {
	    	mc_result = makeITypeInst(inFilePtrForIndex, PC, opcode, arg0, arg1, arg2);
	    }

	    // J-type instructions
	    else if (!strcmp(opcode, "jalr")) {
	    	mc_result = makeJTypeInst(opcode, arg0, arg1);
	    }

	    // O-type instructions
	    else if (!strcmp(opcode, "halt") || !strcmp(opcode, "noop")) {
	    	mc_result = makeOTypeInst(opcode);
	    }

	    else if (!strcmp(opcode, ".fill")) {
	    	if(isNumber(arg0))
	    		mc_result = atoi(arg0);
	    	else {
	    		mc_result = findIndex(inFilePtrForIndex, arg0);
	    	}
	    }

	    else {
	    	printf("error: unrecognized opcode in line %d\n", PC);
	    	printf("%s\n", opcode);
	    	exit(1);
	    }

	    fprintf(outFilePtr, "%d\n", mc_result);
	    // write the result mc data into outFilePtr
	}


    fclose(inFilePtr);
    fclose(inFilePtrForIndex);
    fclose(outFilePtr);

    return 0;
}

int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0, char *arg1, char *arg2) {
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
        /* reached end of file */
        return 0;
    }

    /* check for line too long (by looking for a \n) */
    if (strchr(line, '\n') == NULL) {
        /* line too long */
        printf("error: line too long\n");
        exit(1);
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n\r ]", label)) {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
    * Parse the rest of the line. Would be nice to have real regular expressions, but scanf will suffice.
    */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r "
                  "]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]", opcode, arg0, arg1, arg2);
    
    return 1;
}

int isNumber(char *string) {
    /* return 1 if string is a number */
    int i;
    return ((sscanf(string, "%d", &i)) == 1);
}

int findIndex(FILE *file, char *offset) {
	int addr;
	int flag = 0;

	char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH];
    char arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

	rewind(file);

	for(int i = 0; readAndParse(file, label, opcode, arg0, arg1, arg2); i++) {
		if(!strcmp(label, offset)) {
			addr = i;
			flag++;
		}
	}

	if(flag == 1) 
		return addr;

	else if(flag == 0) {
		printf("error: undefined label\n");
		exit(1);
	}

	else if(flag > 1) {
		printf("error: duplicate labels\n");
		exit(1);
	}
		
	else {
		printf("error: unknown error in findIndex()\n");
		exit(1);
	}
}