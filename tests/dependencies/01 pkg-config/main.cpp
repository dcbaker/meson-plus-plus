#include <iostream>

#include "zlib.h"

int main(int argc, char * argv[]) {
    std::cout << "zlib version (major): " << ZLIB_VER_MAJOR << std::endl;
    return zlibVersion() == ZLIB_VERSION;
}
