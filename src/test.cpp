#include "enum.hpp"

#include <iostream>
#include <vector>

struct Thing {
    int i;
    char c;
};

int main(int argc, char* argv[]) {
    using Test = Enum
        ::Variant<int>
        ::Variant<Thing>;

    std::vector<Test> v;
    v.emplace_back(7);
    v.emplace_back(5);
    v.emplace_back(Thing{5, 'a'});

    for(auto& t : v) {
        std::cout << t.tag << std::endl;

        t.match(
            [](int& i) { std::cout << "int: " << i << std::endl; },
            [](Thing& t) { std::cout << "thing: " << t.i << ", " << t.c << std::endl; }
        );

        std::cout << std::endl;
    }
}
