/* -------------------------------------------------

            CFG for tinyL LANGUAGE

     PROGRAM ::= STMTLIST !
     STMTLIST ::= STMT MORESTMTS
     MORESTMTS ::= ; STMTLIST | epsilon
     STMT ::= ASSIGN | READ | PRINT
     ASSIGN ::= VAR = EXPR
     READ ::= % VAR
     PRINT ::= $ VAR
     EXPR ::= ARITH_EXPR | 
     	      LOGICAL_EXPR|
     	      VAR | 
              DIGIT
     ARITH_EXPR ::= + EXPR EXPR |
              		- EXPR EXPR |
              		* EXPR EXPR 
     LOGICAL_EXPR ::= | EXPR EXPR |
     				  & EXPR EXPR
              
     VAR ::= a | b | c | d | e | f 
     DIGIT ::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

     NOTE: tokens are exactly a single character long

     Example expressions:

           +12!
           +1b!
           +*34&78!
           -*+1+2a58!

     Example programs;

         %a;%b;c=&3*ab;d=+c1;$d!
         %a;b=|*+1+2a58;$b!

 ---------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "Instr.h"
#include "InstrUtils.h"
#include "Utils.h"

#define MAX_BUFFER_SIZE 500
#define EMPTY_FIELD 0xFFFFF
#define token *buffer

/* GLOBALS */
static char *buffer = NULL;	/* read buffer */
static int regnum = 1;		/* for next free virtual register number */
static FILE *outfile = NULL;	/* output of code generation */

/* Utilities */
static void CodeGen(OpCode opcode, int field1, int field2, int field3);
static inline void next_token();
static inline int next_register();
static inline int is_digit(char c);
static inline int to_digit(char c);
static inline int is_identifier(char c);
static char *read_input(FILE * f);

/* Routines for recursive descending parser LL(1) */
static void program();
static void stmtlist();
static void morestmts();
static void stmt();
static void assign();
static void read();
static void print();
static int expr();
static int var();
static int digit();
static int arith_expr();
static int logical_expr();


/*************************************************************************/
/* Definitions for recursive descending parser LL(1)                     */
/*************************************************************************/
static int digit()
{
	int reg;

	if (!is_digit(token)) {
		ERROR("Expected digit\n");
		exit(EXIT_FAILURE);
	}
	reg = next_register();
	CodeGen(LOADI, reg, to_digit(token), EMPTY_FIELD);
	next_token();
	return reg;
}

static int var()
{
	if (!is_identifier(token)) {
		ERROR("Symbol %c unknown\n", token);
		exit(EXIT_FAILURE);
	}

	int id = (int)token;

	next_token();
	return id;
}

static int expr()
{
	OpCode op;
	int reg, id;

	switch (token) {
		case '+':
		case '-':
		case '*':
			return arith_expr();
		case '&':
		case '|':
			return logical_expr();
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			op = LOAD;
			reg = next_register();
			id = var();

			CodeGen(op, reg, id, 0);
			
			return reg;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return digit();
		default:
		ERROR("Symbol %c unknown\n", token);
		exit(EXIT_FAILURE);
	}
}

static int arith_expr()
{
	int reg, left_reg, right_reg;
	OpCode op;

	switch (token) {
		case '+':
			op = ADD;
			break;
		case '-':
			op = SUB;
			break;
		case '*':
			op = MUL;
			break;
		default:
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	next_token();

	left_reg = expr();

	right_reg = expr();

	reg = next_register();

	CodeGen(op, reg, left_reg, right_reg);

	return reg;
}

static int logical_expr()
{
	int reg, left_reg, right_reg;
	OpCode op;

	switch (token) {
		case '&':
			op = AND;
			break;
		case '|':
			op = OR;
			break;
		default:
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	next_token();

	left_reg = expr();

	right_reg = expr();

	reg = next_register();
	CodeGen(op, reg, left_reg, right_reg);

	return reg;
}

static void assign()
{
	OpCode op = STORE;

	int id = var();

	if (token != '=') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	next_token();

	int reg = expr();

	CodeGen(op, id, reg, 0);

	return;
}

static void read()
{
	OpCode op = READ;

	if (token != '%') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	next_token();

	int id = var();

	CodeGen(op, id, 0, 0);

	return;
}

static void print()
{
	OpCode op = WRITE;

	if (token != '$') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	next_token();

	int id = var();

	CodeGen(op, id, 0, 0);

	return;
}

static void stmt()
{
	switch (token) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			assign();
			break;
		case '%':
			read();
			break;
		case '$':
			print();
			break;
		default:
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}

	return;
}

static void morestmts()
{
	switch (token) {
		case ';':
			next_token();

			stmtlist();
			break;
		default:
		break;
	}

	return;
}

static void stmtlist()
{
	stmt(); 

	morestmts();

	return;
}

static void program()
{
	stmtlist();

	if (token != '!') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	// Can't check if EOF?

	

	return;
}

/*************************************************************************/
/* Utility definitions                                                   */
/*************************************************************************/
static void CodeGen(OpCode opcode, int field1, int field2, int field3)
{
	Instruction instr;

	if (!outfile) {
		ERROR("File error\n");
		exit(EXIT_FAILURE);
	}
	instr.opcode = opcode;
	instr.field1 = field1;
	instr.field2 = field2;
	instr.field3 = field3;
	PrintInstruction(outfile, &instr);
}

static inline void next_token()
{
	if (*buffer == '\0') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	printf("%c ", *buffer);
	if (*buffer == ';')
		printf("\n");
	buffer++;
	if (*buffer == '\0') {
		ERROR("End of program input\n");
		exit(EXIT_FAILURE);
	}
	if (*buffer == '!')
		printf("!\n");
}

static inline int next_register()
{
	return regnum++;
}

static inline int is_digit(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

static inline int to_digit(char c)
{
	if (is_digit(c))
		return c - '0';
	WARNING("Non-digit passed to %s, returning zero\n", __func__);
	return 0;
}

static inline int is_identifier(char c)
{
	if (c >= 'a' && c <= 'f')
		return 1;
	return 0;
}

static char *read_input(FILE * f)
{
	size_t size, i;
	char *b;
	int c;

	for (b = NULL, size = 0, i = 0;;) {
		if (i >= size) {
			size = (size == 0) ? MAX_BUFFER_SIZE : size * 2;
			b = (char *)realloc(b, size * sizeof(char));
			if (!b) {
				ERROR("Realloc failed\n");
				exit(EXIT_FAILURE);
			}
		}
		c = fgetc(f);
		if (EOF == c) {
			b[i] = '\0';
			break;
		}
		if (isspace(c))
			continue;
		b[i] = c;
		i++;
	}
	return b;
}

/*************************************************************************/
/* Main function                                                         */
/*************************************************************************/

int main(int argc, char *argv[])
{
	const char *outfilename = "tinyL.out";
	char *input;
	FILE *infile;

	printf("------------------------------------------------\n");
	printf("CS314 compiler for tinyL\n");
	printf("------------------------------------------------\n");
	if (argc != 2) {
		ERROR("Use of command:\n  compile <tinyL file>\n");
		exit(EXIT_FAILURE);
	}
	infile = fopen(argv[1], "r");
	if (!infile) {
		ERROR("Cannot open input file \"%s\"\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	outfile = fopen(outfilename, "w");
	if (!outfile) {
		ERROR("Cannot open output file \"%s\"\n", outfilename);
		exit(EXIT_FAILURE);
	}
	input = read_input(infile);
	buffer = input;
	program();
	printf("\nCode written to file \"%s\".\n\n", outfilename);
	free(input);
	fclose(infile);
	fclose(outfile);
	return EXIT_SUCCESS;
}
