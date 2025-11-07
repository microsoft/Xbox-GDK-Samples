#include <iostream>
#include <gsdk.h>
#include <httplib.h>

#include "EchoCommon.h"

int main(int argc, char* argv[]) {
    // Print each argument
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
    }
    std::cout << "Beginning" << std::endl;
    int result = echoBegin(argc, argv);
    std::cout << "Exited MsQuic" << std::endl;

    return result;
}
