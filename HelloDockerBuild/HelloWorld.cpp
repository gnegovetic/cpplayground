#include <iostream>
using namespace std;

int main()
{
    auto msg = "Hello Docker build world";
    cout << msg << endl;

    //auto *p = new uint8_t[128]; // compile with  -fsanitize=leak

	return 0;
}

// cross-compile to ARM32: arm-linux-gnueabihf-g++ HelloWorld.cpp -o HelloARM32
