# README

I wrote an interpreter in C++ for a simple, made-up programming language. 

See `language-specification.md` to view the syntax rules of the programming language. 

The interpreter parses the input file and then builds an abstract syntax tree in order to execute the program.

To try it out, run:

    make
    ./main ./samples/primes.txt