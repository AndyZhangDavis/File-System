# Phase 1
## Overview
#### Struct
We used a packed struct called superBlock to store all the data from the 
first block. Each of its members are ints of specific number of bytes so 
that calling block_read() will result in the correct bytes put into the 
correct data members. Another packed struct with members of fixed byte 
numbers is the implementation of a directory.
#### Globals
For this phase, we had a global superBlock pointer, a global Directory 
pointer, and a global pointer to unsigned integers of 16 bytes each, for 
the Fat array. These integers are specifically 16 bytes each so that 
reading from a block puts the right indices in the right places in the Fat.
### API Functions
`fs_mount()` first allocates space for each global variable necessary. For 
phase 1, this was the super block, the root directory, and the Fat array, 
but later we also included the open files array here. With error checking 
to guarantee each command works, the specified diskname is opened, and the 
super block and root directory are read from said block. Then, the number 
of Fat blocks (specified by the super block) are read to the Fat array.
`fs_umount()` writes everything back to the disk, and then frees the 
globals and closes the disk.
`fs_info()` determines the number of free fat blocks and files in the root 
directory, and prints all the info about the current block. We used this 
function to test our mount.
## Testing

To test our mount and umount, we used the simple script given in Discussion,
comparing the output of info from the reference program and our testing 
program when calling info. We also used the test_fs_student given to make 
sure that our mount could handle disks that were not a perfect multiple of 
4096 bytes. 
# Phase 2
## Overview
### API Functions
`fs_create()` checks to make sure the input filename is valid and not 
already in the directory. Then, it loops through the root directory to 
attempt to find a new location to create that file. If it finds an open 
spot, it initializes that index of the root directory.
`fs_delete()` checks to ensure the input filename is valid, in the root 
directory, and not currently open. If it passes all these tests, the index 
of this file is reset, and the Fat indexes taken up by this file are all 
looped through and reset.

`fs_ls()` prints each file in the root directory, and info about them. We 
used this function to test preliminary create() and delete().
## Testing
To test these functions, we used the reference function provided. We 
manually added different empty files to a disk with the reference function 
or our test function, and then compared the outputs of ls after different 
additions and removals. We also wrote simple scripts to do these 
comparisons.
# Phase 3
## Overview
#### Struct/Globals
We added a new struct "fd", which is not packed and does not contain length 
restricted variables, because it is not reading directly from the disk. 
This struct contains a file pointer and an offset, which is the place in 
the file to be written to/read from. 
We also added two new global variables for this phase. One is an array of 
pointers to file descriptors called open_files, and the other is an int 
that corresponds to the total number of open files. This number is 
incremented and decremented whenever open() or close() respectively are 
successfully called.
#### Helper Function
`get_valid_fd()` For every function that takes a file descriptor as an 
input, we wrote a helper function to ensure this fd is valid. The function 
checks that the fd is within the bounds possible, and that there is in fact 
a file in the open_files array at this index. This function is used in 
phases 3 and 4.
### API Functions
`fs_open()` first finds the filename in the root directory. If it is not 
found, or there are already the max number of files open, it returns. 
Otherwise, it looks through the open_files array and copies the data from 
said file in the root directory to the first empty spot in this array.
`fs_close()` simply erases the file from its index in the open array.
`fs_stat()` returns the size of the file pointed to by "fd"
`fs_lseek()` sets the offset of the file pointed to by "fd" to the input 
size_t.

# Phase 4

#### Helper Functions
`find_empty_fat()` This function is used to determine where the next 
available fat block is located. This is essential to fs_write() and is 
helpful for the updated version of fs_create() as well. It simply loops 
through the Fat array until it finds an empty index, and then returns that 
index.
`add_fat_blocks()` This function is only used in fs_write(). It is called 
when more blocks need to be added to write the entirety of a file. It 
simply calls find_empty_fat() enough times to initialize the correct number 
of extra indices in Fat to the next available block.
### API Functions
`fs_read()` The strategy we employed for read was to create a char* buffer 
called "bounce" which we allocated enough space for to hold the entirety of 
all of the blocks that the file to be read spans. Then, we check to see if 
the offset of said file is enough to cause the read to change which block 
it should actually start reading from. Finally, to perform the actual read, 
we used math and the Fat index to read each block to this bounce buffer, 
and then copied the part of this buffer desired to the output buffer.
`fs_write()` We used a similar strategy as read for write. First, we 
allocated extra Fat indices with the helper function, and moved through 
these if the offset required it. Then we read the first block pointed to by 
the Fat into a bounce buffer, copied the part of the input that fit in that 
block to the buffer, and then wrote the buffer back. For middle blocks, we 
wrote entire blocks from the input buffer. For the last block, we did 
essentially the same thing as the first block, except we only copied the 
remaining bytes to the bounce buffer before writing it back.

# Testing 
**//We included all of the three newtest files that we tested on in order f
or the shell script to effectively run on a file larger than 1 block.//**

In order to test the entire program in segments, we implemented a shell 
script that manipulates the disk and adds 4 different files to the file 
system and tests read, write, removal, and statistics of files along with 
the info of the file system.

In order to test the Phases, the script creates a disk and adds all of the 
files to the disk. Each of the files has varying byte sizes. The ls of the 
reference program and our own program is fed into stdout and stderr files 
and compared for differences. The tester tells the user if the outputs 
match and if not, where they differ. 

The script tests removal and checks the ls output of the reference program 
and our own to see if they match. For the remaining ones, it feeds the 
output of stat, info, and cat to see if the commands are being correctly 
implemented on our program's disk. 

The program tests the read and write functions by running cat on the 
largest file of 8305 bytes. This allows the script to see if the 
implementation properly writes the appropriate amount of blocks to the file 
system. The diff is once again called and the outputs compared. 

At the end of the shell script, the disk and the stdout/stderr files are 
removed so the script can start fresh.