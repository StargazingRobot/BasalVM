#include <iostream>
#include <chrono> 
#include "VM.h"
#include "Assembler.h"

using std::cout; 
using std::cin ;
using std::cerr;
using std::endl;

const bool DISP_TIME = true;

int main( int argc, char *argv[] )
{



    if( argc < 2 )
    {
        cerr << "No target to assemble. Terminating program." << endl;
        exit( -1 );
    }

    string file = argv[1];

    // string file = "";
    // cout << "load: ";
    // cin >> file;

    // start chrono 
    auto start = std::chrono::high_resolution_clock::now();

    // Instanciate Assambler, assemble instructions
    basm::Assembler assembler;
    bool s = assembler.assemble( file );

    // end chrono
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    if( DISP_TIME ) 
        cout << "Assembled in " << elapsed.count() << " ms\n";

    // terminate program if assemble returned false
    if( s == false )
    {
        cerr << "A problem has occured while assembling file '" << file << "'." << endl;
        exit( -1 );
    }

    // For Debugging purposes

    // cout << "\nAssembled :" << endl;
    // for( unsigned i=0; i<assembler.program.size(); i++)
    // {
    //     cout << i<< "\t" << std::hex << std::uppercase << assembler.program[i] << std::dec << endl;
    // }
    // cout << endl;
    
    // start chrono 
    start = std::chrono::high_resolution_clock::now();

    // Instanciate Virtual Machine
    VM vm;
    vm.initialize();
    vm.load( assembler.program );
    vm.start();

    // vm.dispFlagsRegister();
    // vm.dispMemoryStack();

    // end chrono
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    if( DISP_TIME )
        cout << "\nExecuted in " << elapsed.count() << " ms\n";

    // vm.dispMemoryStack();


    return 0;
}
