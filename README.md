# bigint
C++ arbitrary precision integer project (>64 bits per value)
This program is duplicated on my website http://www.intricate-simplicity.com.

I wanted to write routines that would handle arbitrarily-size integers. There are certainly enough of those floating around on the Internet but I wanted to solve the various problems involved independently.
 
So here it is.
 
It is not in a finished form for use by others, though. I wanted to write it as a stand-alone single file program that would compile without dependencies and demonstrate its capabilities. If anyone is interested, it would not be to difficult to turn it into a header file and a library for more general use. I invite anyone to do that if they would like, but take no responsibility for the result.
 
So how does it work.
 
The program defines a new object, a BigInteger. Unlike the built-in numeric types, the size of the value held by this object is limited only by the memory resources of the computer.
 
Each numeric value is held in an object with two parts, a magnitude and a sign. Unlike standard numeric values, the magnitude of the value is always directly represented; two's complement notation is not used for negative values as the sign is stored independently from the magnitude.
 
What puts the Big in BigInteger is the fact that the magnitude is held in an adjustable-size C++ vector of individual CHUNKs. A CHUNK is an arbitrary-sized atomic numeric value. Currently it represents an unsigned long long, the largest atomic numeric value possible. The advantage of this representation is that the C++ constructors for all atomic numeric values are simple as none will overflow a single vector element.
 
I've defined all the arithmetic operators to apply to BigIntegers (if I've forgotten any, please let me know).
 
I've added a non-standard constructor. A BigInteger can be initialized when declared by a numeric string. This was necessary so that a BigInteger could be initialized with a value too large for the atomic numeric types. As with most of C and C++, the numeric string can be preceded by 0x (or 0X) for hexadecimal values and by a 0 for octal values.
 
Note that although BigIntegers can be initialized by numeric strings, a numeric string can not be assigned (the = operator) to a BigInteger.
 
How the program works.
 
The bulk of the program is the declaration and definition of the BigInteger class. The declaration and definition are combined; if someone wants to use this code as a utility library they should be separated into header and implementation files.
 
The final section of the program is a test routine. I've tried to include test cases to show the capabilities of the BigInteger object. I've compiled and run this program on both Windows and Macintosh without problems.
 
The test program will take an optional command-line parameter. If the program is compiled as bigint (bigint.exe on Windows), specifying "hex" as the single command line parameter will cause all numbers to be input or output in hexadecimal. Similarly, specifying "octal" will use octal representation for all numbers.
 
Specifying nothing, or anything other than the two words above will lead to all numbers being displayed and⁄or input in decimal.
 
I truly hope someone discovers and tries to use this code and, if they do, inform me of any problems or errors. I did test it but, as an individual doing this work in spare time, I can't be as thorough a tester as I would like.
 
One more caveat: If you compile the program you'll need my debugging macro header file ydebug.hpp in order to compile and execute the program. ydebug.hpp can be found in this repository.
