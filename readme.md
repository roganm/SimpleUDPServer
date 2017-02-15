## Simple UDP Server
Author: Rogan McNeill

### Made For
CSC 361
Winter 2017

### Files

- sws.c
- sws.h
- makefile
- readme.txt
- testcases.sh
- p1.pdf



### Description

This is a simple UDP server which responds to GET requests.

### Repository

[GitHub](https://www.github.com/roganm/simpleudpserver)

### Build

To build, run the following command from the project directory:
~~~~
make
~~~~
To clean the directory of executables, run:
~~~~
make clean
~~~~
### Run

To run the simple web server, use type the following:

~~~~
./sws <port> <directory>
~~~~

where directory is the directory containing the files to serve.

### Test

Some test cases are provided in the testcases.sh script.
With the server running on port 8000, simply run the script.

### Credit


Some sample code was used from the CSC 361 course website.






