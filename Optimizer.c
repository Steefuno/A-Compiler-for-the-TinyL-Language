/*
 *********************************************
 *  314 Principles of Programming Languages  *
 *  Spring 2014                              *
 *  Authors: Ulrich Kremer                   *
 *           Hans Christian Woithe           *
 *********************************************
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "InstrUtils.h"
#include "Utils.h"

typedef struct ImportantItem {
	int value; //reg or id
	char type; //0 if reg, 1 if id
	struct ImportantItem* next;
	struct ImportantItem* prev;
} ImportantItem;
ImportantItem* firstImportant; // Head of linked list of registers and ids that may have dependants

/* Adds to firstImport */
void insertImportant(int value, char type)
{
	// Alloc
	ImportantItem* item = (ImportantItem*)malloc(sizeof(ImportantItem));
	(*item).value = value;
	(*item).type = type;
	(*item).next = NULL;
	(*item).prev = NULL;

	// Set as first
	if (firstImportant != NULL) {
		(*item).next = firstImportant;
		(*firstImportant).prev = item;
	}
	firstImportant = item;

	return;
}

/* Removes a dependee from list */
void removeImportant(ImportantItem* item)
{
	if (item == firstImportant) { // If dependee if first
		// Increment first
		firstImportant = (*firstImportant).next;
		if (firstImportant != NULL)
			(*firstImportant).prev = NULL;
	} else {
		// Pop depdnee from linked list
		ImportantItem* prev = (*item).prev;
		ImportantItem* next = (*item).next;
		(*prev).next = next;
		if (next != NULL)
			(*next).prev = prev;
	}
	return;
}

/* Check if there is an important item that uses the select register/id */
/* Note, only ones that USE the reg/id, not ones that set it */
/* type is 0 if register, 1 if id */
ImportantItem* findImportantItem(int value, char type)
{
	ImportantItem* item = firstImportant;

	/* Iterate through each important item */
	while (item != NULL) {
		if ((*item).type == type && (*item).value == value)
			return item;

		// Next
		item = (*item).next;
	}

	return NULL;
}

/* Makes ImportantItems out of fields used by instruction */
/* Does not add if already added */
void storeInstrAsImportant(Instruction* instr)
{
	switch ((*instr).opcode) {
		case ADD:
		case SUB:
		case MUL:
		case AND:
		case OR:
			if (findImportantItem((*instr).field3, (char)0) == NULL)
				insertImportant((*instr).field3, (char)0);
		case STORE:
			if (findImportantItem((*instr).field2, (char)0) == NULL)
				insertImportant((*instr).field2, (char)0);
			break;
		case LOAD:
			if (findImportantItem((*instr).field2, (char)1) == NULL)
				insertImportant((*instr).field2, (char)1);
			break;
		case WRITE:
			if (findImportantItem((*instr).field1, (char)1) == NULL)
				insertImportant((*instr).field1, (char)1);
			break;
		default:
		break;
	}
	return;
}

/* Either free or handle dependee of dependant */
void handleDependant(Instruction* instr, ImportantItem* dependee, char type, Instruction** headPtr)
{
	// If instr is not important
	if (dependee == NULL) {
		// If instr is head
		if (instr == *headPtr) {
			// Increment head
			*headPtr = (*instr).next;
		} else {
			// Pop instr from linked listed
			Instruction* prev = (*instr).prev;
			Instruction* next = (*instr).next;
			(*prev).next = next;
		}
		// Dealloc instr
		free(instr);
	} else { // If instr should be important
		// Remove dependee
		removeImportant(dependee);
		// Dealloc dependee
		free(dependee);

		// Add instr's use fields list of regs/ids that may have dependants
		storeInstrAsImportant(instr);
	}

	return;
}

/* Check if necessary, if not, remove */
/* Continue by checking previous instruction */
void criticalCheck(Instruction* instr, Instruction** headPtr)
{
	// If instr DNE
	if (instr == NULL) return;

	if ((*instr).opcode == READ) {
		// Keep
	} else if ((*instr).opcode == WRITE) {
		// Keep
		// Add to list of items that may have dependants
		insertImportant((*instr).field1, (char)1);
	} else { // Check if is a dependant
		// Check type
		// Check if to be used in anything important
		ImportantItem* dependee = NULL;
		char type = (char)0; // Find important item using instr's reg
		switch ((*instr).opcode) {
			case STORE:
				// Change to find dependee using instr's id
				type = (char)1;
			default:
			dependee = findImportantItem((*instr).field1, type);
			break;
		}

		// Remove either instr or dependee
		handleDependant(instr, dependee, type, headPtr);
	}

	// critical check on previous
	return criticalCheck((*instr).prev, headPtr);
}

int main()
{
	Instruction *head;
	Instruction *tail;

	head = ReadInstructionList(stdin);
	if (!head) {
		WARNING("No instructions\n");
		exit(EXIT_FAILURE);
	}


	// Iterate to get tail and mark initial critical states
	tail = head;
	while ((*tail).next != NULL) {
		tail = (*tail).next;
	}

	// Start critical check from tail
	criticalCheck(tail, &head);

	// Clear list of dependees
	while (firstImportant != NULL) {
		ImportantItem* next = (*firstImportant).next;
		free(firstImportant);
		firstImportant = next;
	}

	if (head) {
		PrintInstructionList(stdout, head);
		DestroyInstructionList(head);
	}
	return EXIT_SUCCESS;
}
