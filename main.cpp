#include <iostream>
#include <string>
#include "interpreter.hpp"
#include "debug.hpp"

int main(int argc, char *argv[]) {
    if (argc <= 1) std::cout << "Please provide an inpute file." << std::endl;
    else {
        std::string input_file = argv[1];
        Interpreter interpreter(input_file);
        interpreter.run();
    }
    return 0;
}
