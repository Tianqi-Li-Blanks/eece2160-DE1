#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <bitset>
#include <string>
#include <cerrno>

using namespace std;

// Physical base address of FPGA Devices
const unsigned int LW_BRIDGE_BASE = 0xFF200000; // Base offset

// Length of memory-mapped IO window
const unsigned int LW_BRIDGE_SPAN = 0x00005000; // Address map size

// Cyclone V FPGA device addresses
const unsigned int LEDR_BASE = 0x00000000; // Leds offset
const unsigned int SW_BASE = 0x00000040; // Switches offset
const unsigned int KEY_BASE = 0x00000050; // Push buttons offset


/**
* Write a 4-byte value at the specified general-purpose I/O location.
*
* @param pBase  Base address returned by 'mmap'.
* @parem offset  Offset where device is mapped.
* @param value  Value to be written.
*/
void RegisterWrite(char *pBase, unsigned int reg_offset, int value)
{
  * (volatile unsigned int *)(pBase + reg_offset) = value;
}

/**
* Read a 4-byte value from the specified general-purpose I/O location.
*
* @param pBase  Base address returned by 'mmap'.
* @param offset  Offset where device is mapped.
* @return  Value read.
*/
int RegisterRead(char *pBase, unsigned int reg_offset)
{
  return * (volatile unsigned int *)(pBase + reg_offset);
}

/**
* Initialize general-purpose I/O
* - Opens access to physical memory /dev/mem
* - Maps memory into virtual address space
*
* @param fd File descriptor passed by reference, where the result
*  of function 'open' will be stored.
* @return  Address to virtual memory which is mapped to physical,
* or MAP_FAILED on error.
*/
char *Initialize(int *fd)
{
  // Open /dev/mem to give access to physical addresses
  *fd = open( "/dev/mem", (O_RDWR | O_SYNC));
  if (*fd == -1) // check for errors in openning /dev/mem
  {
    cout << "ERROR: could not open /dev/mem..." << endl; //<< strerror(errno)  << endl;
    exit(1);
    
  }
  // Get a mapping from physical addresses to virtual addresses
  char *virtual_base = (char *)mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, *fd, LW_BRIDGE_BASE);
  if (virtual_base == MAP_FAILED) // check for errors
  {
    cout << "ERROR: mmap() failed..." << endl;
    close (*fd); // close memory before exiting
    exit(1); // Returns 1 to the operating system;
  }
  return virtual_base;
}

/**
* Close general-purpose I/O.
*
* @param pBase  Virtual address where I/O was mapped.
* @param fd File descriptor previously returned by 'open'.
*/
void Finalize(char *pBase, int fd)
{
  if (munmap (pBase, LW_BRIDGE_SPAN) != 0){
    cout << "ERROR: munmap() failed..." << endl;
    exit(1);
  }
  close (fd); // close memory
}

/** Changes the state of an LED (ON or OFF)
* @param pBase  Base address returned by 'mmap'
* @param ledNum  LED number (0 to 9)
* @param state  State to change to (ON or OFF)
*/

void Write1Led(char *pBase, int ledNum, int state) {
   if(0 <= ledNum <= 9) {
     int value = 1;
     int count = ledNum;
     if(state == 1) {
       while(count > 0) {
         value = 2 * value;
         count --;
       }
       RegisterWrite(pBase, LEDR_BASE, value);
     }
     else if(state == 0){
       RegisterWrite(pBase, LEDR_BASE, 0);
     }
     else{
       cout << "ERROR: state should only be 0 and 1 means off and on" << endl;
     }
  }
  else {
    cout << "ERROR: ledNum should be in 0-9" << endl;
  }

}

/** Reads the value of a switch
* - Uses base address of I/O
* @param pBase  Base address returned by 'mmap'
* @param switchNum Switch number (0 to 9)
* @return  Switch value read
*/

int Read1Switch(char *pBase, int switchNum) {
  if(0 <= switchNum <= 9) {
    int value = RegisterRead(pBase, SW_BASE);
    int state;
    while (switchNum >= 0){
      state = value % 2;
      value = (value - state) / 2;
      switchNum--;
    }
    return state;
  }
  else{
    cout << "ERROR: switchNum should be in 0-9" << endl;
  }

}


/** Set the state of the LEDs with the given value.
 * *
 * * @param pBase Base address for general-purpose I/O
 * * @param value Value between 0 and 1023 written to the LEDs
 * */
void WriteAllLeds(char *pBase, int value){
//  1111111111 is 1023 means all turn on
  if(0 <= value <= 1023){
    RegisterWrite(pBase, LEDR_BASE, value);
  }
  else{
    cout << "out rage" << endl;
  }
}


/** Reads all the switches and returns their value in a single integer.
 * *
 * * @param pBase Base address for general-purpose I/O
 * * @return A value that represents the value of the switches
 * */
int ReadAllSwitches(char *pBase){
  //bitset<10> bits;
  //for (int i = 0; i < 10; i++) {
  //  bits.set(i, Read1Switch(pBase, i));
  //}  
  //int result = (int)(bits.to_ulong());
  //return result;
  return RegisterRead(pBase, SW_BASE);

}

int main()
{
// Initialize
  int fd;
  char *pBase = Initialize(&fd);

// ************** Put your code here **********************
  int ledNum;
  cout << "Type a ledNum between 0 to 9: ";
  cin >> ledNum;
  Write1Led(pBase, ledNum, 1);
  
  int close;
  cout << "Type 0 to close the led : ";
  cin >> close;
  if(close == 0){
    Write1Led(pBase, ledNum, 0);
  } 

  int value;
  cout << "Type a value between 0 to 1023: ";
  cin >> value;

  WriteAllLeds(pBase, value);
  int option;
  cout << "Type 1 to continue once you move well with the switch: ";
  cin >> option;
  if(option == 1){
    cout << ReadAllSwitches(pBase) << endl;
    value = ReadAllSwitches(pBase);
    WriteAllLeds(pBase, value);
   
    int switch1;
    cout << "Type a switchNum to read between 0 to 9 (1 is on, 0 is off): ";
    cin >> switch1;
    cout << Read1Switch(pBase, switch1) << endl;
  } 
  else{
    cout << "fail to continue" << endl; 
  }
   
// Done
  Finalize(pBase, fd);
}

