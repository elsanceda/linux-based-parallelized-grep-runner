# Linux-based parallelized grep runner
This is a Linux-based C program that divides the workload of searching through all file contents in a directory tree among parallel threads performing grep. This project was submitted as an academic requirement for the CS 140 (Operating Systems) course of the Department of Computer Science in the University of the Philippines - Diliman during the first semester of AY 2022-2023.

Comes with a multi-threaded and single-threaded version:
 
 <code>multithreaded.c</code>
 - The main multi-threaded version of the program
 - Performs grep with 1-8 different "workers"
 - Number of "workers" deployed is specified by the user
 
 <code>single.c</code>
 - A single-threaded version of the program
 - Performs grep with only 1 "worker"
