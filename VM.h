// This code has been written by Joackim Orciere.
// the goal is to simulate a 16 bit CPU through a virtual machine and a custom set of (32 bits) instructions
// certains instruction allow for dereferencing, such as ADD, SUB, CMP, COPY, MUL, DIV and MOD.
// certains others instructions have been factorized inside a single instruction code
// such as AND, OR, NOT and XOR, in order to make room for other instructions as only 4 bits are used for OP codes.

#include <vector>

#pragma once

//	+------------------------------------+
//	|    Enums used for code clarity     |
//	+------------------------------------+
//    -- Registers --
enum Reg
{
	ax = 0,
	bx,
	cx,
	dx,
	ex,
	fx,
	si,	
	di,
	sp,		// 8.
	ip,		// 9.
	R_COUNT
};

//    -- OP codes --
enum OP				
{
	IO = 0,		//	.0			// used to communicate with outside the VM, not implemented yet
	ADD,		// 	.1
	SUB,		// 	.2
	COPY,		// 	.3
	MUL,		// 	.4			// Multiplication		ex :  MUL 5, ax			C : ax *= 5;
	DIV,		// 	.5			// Division
	MOD,		// 	.6			// Modulus
	BIN,		// 	.7			// contains AND, OR, NOT and XOR
	PUSH,		// 	.8
	POP,		// 	.9			
	JUMP,		// 	.10			// contains CALL and RET 
	PROMPT,		// 	.11			// act as both a cout and a cin depending on the mode
	RAND,		//  .12
	WAIT		//  .13			// implementation must depend on the context
};

//    -- CPU Flags --
enum Flag
{
	EQU = 0,
	ZRO,
	POS,
	NEG,
	OVF,
	ODD,
	F_COUNT
};

//	+---------------------------+
//	|    Virtual Machine Class  |
//	+---------------------------+

class VM
{

//	+---------------------+
//	|    VM attributes    |
//	+---------------------+

private:
	// array of word addresses  (16 bits offset)
	// ~ amount to 130 ko of memory, no need for dynamic allocation
	uint16_t memory[UINT16_MAX];	
	// no program size limit, dinamic array for less memory impact on average
	std::vector<uint32_t> program;
	// amount of reserved space in memory ( maybe useful for later ? )
	const uint16_t RESERVED_SPACE = 2;
	// array containing the CPU flags, access like flags[ZRO]
	bool flags[ F_COUNT ];
	// used to generate random numbers
	uint16_t rnd_seed;
public:
	// array containing the registers access like reg[r2], public so you can do : vm.reg[ax] = 238; in main()
	uint16_t reg[ R_COUNT ];


//    All functions are public
//	+--------------------------+
//	|    VM basic functions    |
//	+--------------------------+

	// initialize registers to 0, initialize the seed for random numbers
	void initialize( void );

	// display the stack values
	void dispMemoryStack( bool showReserved = false );
	
	// check if the address is RESERVED
	void checkForSegfault( int16_t address );

	// generate a 16bit random number
	uint16_t xorshift16( void );

//	+-----------------------------------+
//	|    OP Interpretation Functions    |
//	+-----------------------------------+

	// add a value (from a register or immediate) to a destination register
	// modify flags ZRO, NEG and POS
	void executeAddBasedOP( uint32_t instruction, OP op );

	// push a value ( either immediate or from a register ) to the top of the stack
	// push and pop are factorized inside the same opcode to make room for DISP instruction
	void executePUSH( uint32_t instruction );

	// take the top value, and decrement rsp, while placing ( or discarding ) the value in a register
	void executePOP( uint32_t instruction );

	// Either act as a cout or a cin, either with a value or a string
	void executePROMPT( uint32_t instruction ); // TODO subject to change input -> sfml

	// Binary Operator : Either act as AND, OR, NOT or XOR, used to compress 4 instructions in 1 opcode
	// Because of the compression, they cannot be used with dereferenced operands
	void executeBinBasedOP( uint32_t instruction );

	// take a register and set its value to a (pseudo) random one
	void executeRAND( uint32_t instruction );

	// sleep for a certain amount of time before going to the next instruction
	void executeWAIT( uint32_t instruction );

	// contains jump, conditionnal jump, call and ret
	void executeJUMP( uint32_t instruction );

	// redirect to the correct function depending on the instruction code contained in the first 8 bits
	void processInstruction( uint32_t instruction );


//	+------------------------------+
//	|    Flags Update Functions    |
//	+------------------------------+
	
	// display the content of Flags byte, with the corresponding flags eg ZRO, EQU, ODD etc..
	void dispFlagsRegister( void );

	// update every flag except Overflow since it needs operands value. Special case for EQU and ZRO see below
	// sub_or_cmp : false for sub (ZRO flag) and true for cmp (EQU flag)
	void updateFlags( int16_t value, bool cmp = false );

	// check if you can get back to the operand from the result, if not, result has overflowed
	void updateAddOverflow(  int16_t dest, int16_t src );

	// check if you can get back to the operand from the result, if not, result has overflowed
	void updateSubOverflow(  int16_t dest, int16_t src );

	// check if you can get back to the operand from the result, if not, result has overflowed
	void updateMulOverflow(  int16_t dest, int16_t src );
};


