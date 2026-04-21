#include <iostream>
#include <gsdk.h>
#include <codeanalysis\warnings.h>
#pragma warning(push)
#pragma warning(disable: 4242 4244) // std::transform in httplib uses int(int) tolower on char
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <httplib.h>
#pragma warning(pop)

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
