# Linux-based parallelized grep runner
This is a Linux-based C program

Comes with a multi-threaded and single-threaded version:
 
 <code>multithreaded.c</code>
 - The main multi-threaded version of the program
 - Performs grep with 1-8 different workers
 - Number of workers deployed is specified by the user
 
 <code>single.c</code>
 - A single-threaded version of the program
 - Performs grep with only 1 worker
