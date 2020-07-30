# Basal Virtual Machine
basic 16bits VM, written in C++ capable of interpreting basal bytecode, using 32bits instructions.
the VM automatically allocate 2^16 words of 16bits. this represent ~130ko of memory.
the program is loaded by the compiler in a std::vector, containing instructions encoded in 32 bits.

# Dependencies
	- gcc
	- cppclean
	- cppcheck
	
you can also just remove the # Static Analysis in the makefile instead if you dont want to use cppclean and cppcheck.
The project is compiled with a very large set of warnings enabled, which you can see typing "make flags".
It should compile without any warning being prompt, except a few from Static Analysis.

# Usage

Currently the assembler and the VM are coupled together, meaning you can only use both at the same time, meaning there is only one way to run the project:

./main <basm_file>

	ex : 	./bin/main bin/examples/GameOfLife.basm
	or :	cd bin && ./main examples/GameOfLife.basm


# Basal Assembler
proto Assembler based on GNU assembly, but simplified.
can assemble 20k basal assembly lines to instruction code under 0.1s

It has severals features such as dereferencing registers, using immediate values or direct addresses.
Check Doc.md for more details.

#Examples :

Game of life running on basal VM (bin/examples/GameOfLife.basm) :

![Game of life running on basal VM](preview.gif?raw=true "Basal VM interpreting bin/examples/GameOfLife.basm")

Display the first elements of the Fibonacci sequence (bin/examples/fibonacci.basm) :

	#--------------------
	# Fibonacci Sequence
	#--------------------

	:BEGIN
		copy    0,  ax
		copy    1,  bx
		copy    15, cx  

		disp    '{', char
		disp     bx, mem

	:LOOP   # loop to calculate fibonacci sequence
		sub     1,  cx
		cmp     0,  cx
		jump END if EQU 

		copy    bx, dx
		add     ax, bx
		copy    dx, ax

		call Inter
		disp    bx, mem

		jump LOOP
		
	:Inter  # separate numbers in display
		disp    '\,',   char
		disp    '\s',   char
		ret

	:END    # end program
		disp    '}',    char
		disp    '\n',   char
		exit
