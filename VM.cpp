#include <iostream>
#include <string>
#include <time.h> // used for seeding PRNG with entropy
#include <chrono> // used for cross platform sleep call
#include <thread>

#include "misc.hpp"
#include "VM.h"

using std::string;
using std::cout;
using std::endl;
using std::cin;

// take the intruction code from the instruction
OP getInstruction( uint32_t instruction )
{
	// only keep the 4 left hand bits
	instruction = ( instruction & 0xF0000000 ) >> 28;
	
	// convert value as an OP 
	OP op = OP( instruction );
	return op;	
}


//	+--------------------------+
//	|    VM basic functions    |
//	+--------------------------+

// initialize registers to 0, initialize PRNG generator with entropy
void VM::initialize( void )
{
	for( int i = 0; i < R_COUNT; i++ )
	{
		reg[i] = 0;
	}
	// push will increment reg[sp] before assignement
	reg[sp] = RESERVED_SPACE-1; // save space for flags
	reg[ip] = 0;

	srand(time(NULL));
	rnd_seed = rand();	 // seed the xorshift PRNG
}

// display the stack values
void VM::dispMemoryStack( bool showReserved )
{
	cout << "┌────────────────────┐\n│ -- Memory Stack    │" << endl;

	for( int i = reg[sp]; i >= RESERVED_SPACE; i-- )
	{
		int16_t value = static_cast<int16_t>(memory[i]);
		int s = 11 - std::to_string( value ).length(); 
		cout << "│ " << i << "\t";
		for( int j=0; j<s; j++ ) // align numbers on the right
		{
			cout << " ";
		}
		cout << value << "  │" << endl;
	}

	cout << "│ -- Reserved Memory │" << endl;
	if( showReserved )
	{
			for( int i = RESERVED_SPACE-1; i >= 0; i-- )
			{
			int16_t value = static_cast<int16_t>(memory[i]);
			int s = 11 - std::to_string( value ).length(); 
			cout << "│ " << i << "\t";
			for( int j=0; j<s; j++ ) // align numbers on the right
			{
				cout << " ";
			}
			cout << value << "  │" << endl;
		}
	}
	cout << "└────────────────────┘" << endl;
}

// check if the address is RESERVED
void VM::checkForSegfault( int16_t address )
{
	if( address < RESERVED_SPACE )
	{
		Error( "Segmentation fault, cannot access to reserved memory addresses");
		exit( -1 );
	}
}

// generate a 16bit random number
uint16_t VM::xorshift16( void )
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint16_t x = rnd_seed;
	x ^= x << 7;
	x ^= x >> 9;
	x ^= x << 8;
	rnd_seed = x;
	return x;
}


//	+-----------------------------------+
//	|    OP Interpretation Functions    |
//	+-----------------------------------+

// TODO changed instruction encoding, CF basm parseAddBasedInstr() function
// add a value (from a register or immediate) to a destination register
// modify flags ZRO, NEG and POS
void VM::executeAddBasedOP( uint32_t instruction, OP op )
{
	// 4 bits for sign and mod : 1bit of sign, 3bits of mode value
	short mode		= ( instruction & 0x0F000000 ) >> 24;	// mode of the instruction
	short dest		= ( instruction & 0x00F00000 ) >> 20;	// destination register
	short dest_off	= ( instruction & 0x00070000 ) >> 16;	// destination offset
	short src		= ( instruction & 0x0000F000 ) >> 12;	// source register
	short src_off	= ( instruction & 0x00000700 ) >>  8;	// source offset
	int16_t value	= ( instruction & 0x0000FFFF );			// immediate value

	// multiple dereferencement not possible eg: add 4(rsp), 5(rsp)
	// like a real assembly
	
	uint16_t* dest_p = NULL;
	uint16_t* src_p  = NULL; // if still null after the switch, use the imediate value instead
	uint16_t address = 0;

	switch( mode ) // only check for segfault when dereferencing 
	{			// example with a few values and registers
		case 0:	// add  3, rsp
			dest_p = &reg[dest]; break;
			
		case 1:	// add	3, (r7)
			address = reg[dest];
			dest_p	= &memory[ address ];
			checkForSegfault( address ); break;

		case 2:	// add	3, 4(r7)
			address = reg[dest] + coef(sign) * dest_off;
			dest_p	= &memory[ address ] ; 
			checkForSegfault( address ); break;
			
		case 3: // add	r1, r7
			dest_p = &reg[dest];
			src_p  = &reg[src]; break;

		case 4:	// add	r1, (r7)
			address = reg[dest];
			dest_p	= &memory[ address ];
			src_p	= &reg[src];
			checkForSegfault( address ); break;

		case 5:	// add	r1, 4(r7)
			address = reg[dest] + coef(sign) * dest_off;
			dest_p	= &memory[ address ] ; 
			src_p	= &reg[src]; 
			checkForSegfault( address ); break;

		case 6: // add	(r1), r7
			dest_p	= &reg[dest];
			address = reg[src];
			src_p	= &memory[ address ]; 
			checkForSegfault( address ); break;

		case 7:	// add	4(r1), r7
			dest_p	= &reg[dest]; 
			address = reg[src] + coef(sign) * src_off;
			src_p	= &memory[ address ]; 
			checkForSegfault( address ); break;
	}

	if( src_p != NULL )
		value = *src_p;
	
	switch( op )
	{
		case ADD :
			updateAddOverflow( *dest_p, value );
			*dest_p += value;
			updateFlags( *dest_p ); break;

		case SUB:
			updateSubOverflow( *dest_p, value );
			*dest_p -= value;
			updateFlags( *dest_p ); break;

		case COPY:
			*dest_p = value;
			updateFlags( *dest_p ); break;

		case CMP:
			if( static_cast<int16_t>(*dest_p) < 0 and value > 0 ) // avoid overflow
			{
				flags[POS] = 0;
				flags[NEG] = 1;
			}
			else if( static_cast<int16_t>(*dest_p) > 0 and value < 0 )
			{
				flags[POS] = 1;
				flags[NEG] = 0;	
			}
			else // no overflow possible left, substract normally
			{	// process result in a local variable, in order to preserve destination 
				uint16_t result = *dest_p - value;
				updateFlags( result );
			}
			break;

		case MUL:
			updateMulOverflow( *dest_p, value );
			*dest_p *= value;
			updateFlags( *dest_p ); break;

		case DIV:
			*dest_p /= value;
			updateFlags( *dest_p ); break;

		case MOD:
			*dest_p %= value;
			updateFlags( *dest_p ); break;

		default:
			Error("Unexpected OP code");

	}
}


// push a value ( either immediate or from a register ) to the top of the stack
// push and pop are factorized inside the same opcode to make room for DISP instruction
void VM::executePUSH( uint32_t instruction )
{
	uint16_t mode	= ( instruction & 0x0F000000 ) >> 24;	// mode of the instruction
	uint16_t src 	= ( instruction & 0x0000F000 ) >> 12;	// source register

	if( reg[sp] < UINT16_MAX ) // check for room in VM memory
	{
		if( mode == 0 )	// push source register
		{
			memory[++reg[sp]] = reg[src];
		}
		else if( mode == 1 ) // push immediate value
		{
			int16_t value = ( instruction & 0x0000FFFF );
			memory[++reg[sp]] = value;
		}
	}
	else // not enough memory left to push
		Error("Out of memory");
}

// take the top value, and decrement rsp, while placing ( or discarding ) the value in a register
void VM::executePOP( uint32_t instruction ) 
{
	if( reg[sp] >= RESERVED_SPACE ) // check if there is something on the stack
	{
		uint16_t mode	= ( instruction & 0x0F000000 ) >> 24;	// mode of the instruction
		uint16_t dest 	= ( instruction & 0x00F00000 ) >> 20;	// dest register
		if( mode == 0 ) // if mode is at 0, save top value to a dest reg, otherwise just discard it
		{
			reg[dest] = memory[reg[sp]]; // mov top of stack in destination register
		}
		reg[sp] -= 1; // decrease rsp
	}
	else 
		Error("Stack is empty");
}

// Either act as a cout or a cin, either with a value or a string 
// TODO : proper source dereferencement ! like addBasedOP
void VM::executePROMPT( uint32_t instruction ) 
{
	uint16_t mode	= ( instruction & 0x0F000000 ) >> 24;	// mode of the instruction
	uint16_t src 	= ( instruction & 0x00F00000 ) >> 20;	// source register ( int16, or char ) or register containing address of string
	bool newline	= ( instruction & 0x00010000 ) >> 16;


	switch( mode )
	{
		// DISPLAY MODES
		case 0: // display as INT16 value
			cout << static_cast<int16_t>(reg[src]) ; break;
		case 1: // display as UINT16 value
			cout << reg[src] ; break;
		case 2: // display as char
			cout << static_cast<char>(reg[src]) ; break;

				// in this mode src register is dereferenced, containing the address of the string in memory
		case 3: // display a string : [ char16 ]* [ NULL ], 
		{
			string message = "";
			for( int i=0; i < 65536; i++ ) // max size of string
			{
				if( memory[ reg[src] + i ] == 0 ) // NULL terminated earlier
					break;
				char c = static_cast<char>( memory[ reg[src] + i ] );
				message += c;
			}
			cout << message; break;
		}
		case 4: // in this mode src register is dereferenced, containing the address of the value in memory
			cout << memory[ reg[src]]; break;
			
		case 6: // only print a newline
			cout << endl; break;

		// INPUT MODES
		case 7: // input a value in a register
			int16_t value; cin >> value;
			reg[src] = static_cast<uint16_t>(value); break;

		case 8: // input a string, to be pushed on the stack
			char str[256]; cin.get(str, 256, '\n'); // null terminated by default
			if( reg[sp] + 1 < UINT16_MAX - 256 ) // check if there is enough room
			{
				for( int i=0; i<256; i++)
				{
					memory[ ++reg[sp] ] = static_cast<uint16_t>( str[i] );
					if( str[i] == 0 ) break; // Null Terminated before end of buffer
				}
			}
			break;
	}
	// print a new line if requested
	if( newline ) cout << endl;
}

// take a register and set its value to a (pseudo) random one
void VM::executeRAND( uint32_t instruction )
{
	uint16_t mode		= ( instruction & 0x0F000000 ) >> 24;	// chose range 
	uint16_t dest		= ( instruction & 0x00F00000 ) >> 20;	// destination register	
	uint16_t max_value	= ( instruction & 0x0000FFFF );			// choose max value

	if( mode == 0 )		  // no max value
		reg[dest] = static_cast<int16_t>(xorshift16());
	else if( mode == 1 )  // -max_value < x < max_value
		reg[dest] =  static_cast<int16_t>(xorshift16()) % max_value;
	else if( mode == 2 )  // 0 <= x < max_value 	
		reg[dest] = ( static_cast<int16_t>(xorshift16()) % max_value / 2 ) + ( max_value / 2 );
}

// Binary Operator : Either act as AND, OR, NOT or XOR, used to compress 4 instructions in 1 opcode
void VM::executeBinBasedOP( uint32_t instruction ) 
{
	short mode		= ( instruction & 0x07000000 ) >> 24;	// mode of the instruction either AND, OR, NOT or XOR
	short op		= ( instruction & 0x00F00000 ) >> 24;	// choose between immediate value or source register
	short dest		= ( instruction & 0x000F0000 ) >> 20;	// destination register
	short src		= ( instruction & 0x0000F000 ) >> 12;	// source register
	uint16_t value	= ( instruction & 0x0000FFFF );			// immediate value

	uint16_t* dest_p = &reg[dest];

	if( op == 1 ) // source register
	{
		value = reg[src];
	}

	switch( mode ) // none of those operations can overflow
	{
		case 1:
			*dest_p &= value; 
			updateFlags( *dest_p ); break;

		case 2:
			*dest_p |= value;
			updateFlags( *dest_p ); break;

		case 3:
			*dest_p = ~(value);
			updateFlags( *dest_p ); break;

		case 4:
			*dest_p ^= (value);
			updateFlags( *dest_p ); break;
	}
}

// sleep for a certain amount of time before going to the next instruction
void VM::executeWAIT( uint32_t instruction )
{
	uint16_t mode		= ( instruction & 0x0F000000 ) >> 24;	// chose range 
	uint16_t value		= ( instruction & 0x0000FFFF );			// choose max value
	// uncomment below to display precisely the time slept.
    using namespace std::chrono_literals;
    // auto start = std::chrono::high_resolution_clock::now();
	if( mode == 0 )
		std::this_thread::sleep_for(std::chrono::milliseconds( value ));
	if( mode == 1 )
		std::this_thread::sleep_for(std::chrono::seconds( value ));
    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double, std::milli> elapsed = end-start;
    // cout << "Waited " << elapsed.count() << " ms\n";
}

// redirect to the correct function depending on the instruction code contained in the first 8 bits
void VM::processInstruction( uint32_t instruction )
{
	// get the current opcode
	OP op = getInstruction( instruction );
	
	switch( op )
	{
		case PUSH:
			executePUSH( instruction ); break;
		case POP:
			executePOP( instruction ); break;
		case BIN:
			executeBinBasedOP( instruction ); break;
		case RAND:
			executeRAND( instruction ); break;
		case WAIT:
			executeWAIT( instruction ); break;

		case JUMP:
			//TODO
			break;
		case PROMPT:
			executePROMPT( instruction ); break;
		default :
			executeAddBasedOP( instruction, op ); break;
	}
}


//	+------------------------------+
//	|    Flags Update Functions    |
//	+------------------------------+

void VM::dispFlagsRegister( void )
{
	cout << "┌─────┬─────┬─────┬─────┬─────┬─────┐" << endl;
	cout << "│ EQU │ ZRO │ POS │ NEG │ OVF │ ODD │" << endl;
	for( int i=0; i<F_COUNT; i++ )
	{
		cout << "│  " << flags[i] <<  "  ";
	}
	cout << "│ \n" << "└─────┴─────┴─────┴─────┴─────┴─────┘" << endl;
}

// update every flag except Overflow since it needs operands value. Special case for EQU and ZRO see below
// sub_or_cmp : false for sub (ZRO flag) and true for cmp (EQU flag)
void VM::updateFlags( int16_t value, bool cmp )
{
	flags[NEG] = 0;
	flags[POS] = 0;
	if( value == 0 )
	{
		flags[ZRO] = 1;
		if( cmp == true )
			flags[EQU] = 1;
	}
	else if( value < 0 )
		flags[NEG] = 1;
	else
		flags[POS] = 1;

	flags[ODD] = value % 2;
}

// check if you can get back to the operand from the result, if not, result has overflowed
void VM::updateAddOverflow(  int16_t dest, int16_t src )
{
	int16_t result = dest + src;
	if( dest != result - src )	flags[OVF] = 1; 
	else flags[OVF] = 0;
}

// check if you can get back to the operand from the result, if not, result has overflowed
void VM::updateSubOverflow(  int16_t dest, int16_t src )
{
	int16_t result = dest - src;
	if( dest != result + src )	flags[OVF] = 1; 
	else flags[OVF] = 0;
}

// check if you can get back to the operand from the result, if not, result has overflowed
void VM::updateMulOverflow(  int16_t dest, int16_t src )
{
	int16_t result = dest * src;
	if( dest != result / src )	flags[OVF] = 1; 
	else flags[OVF] = 0;
}






