#include <iostream>
#include "include/buffer.hpp"

int main() {
    buffer s("test2.bin");

    s.push_back("I want to");
    s.push_back("sleep");
    s.push_back("now");

    s[0] = "I've wanted to";
    s[1] = "eat";
    s[2] = "yesterday";

    return 0;
}
