#include "enum.hpp"
#include "optional.hpp"
#include "tree.hpp"

#include <iostream>
#include <string>
#include <vector>

struct Thing {
    int i;
    char c;
};

int main(int argc, char* argv[]) {
    using Test = venum::Enum
        ::Variant<std::string>
        ::Variant<int>
        ::Variant<Thing>;

    std::vector<Test> v;
    v.emplace_back(7);
    v.emplace_back("Hello");
    v.emplace_back(5);
	v.emplace_back(5, 'a');

    for(auto& t : v) {
        t.match(
            [](std::string& s) { std::cout << "string: " << s << std::endl; },
            [](int& i) { std::cout << "int: " << i << std::endl; },
            [](Thing& t) { std::cout << "thing: " << t.i << ", " << t.c << std::endl; }
        );

        std::cout << std::endl;
    }

    venum::Enum::Variant<int>::Variant<std::string>(7).apply([](auto a) { std::cout << a << std::endl; });

    using Test2 = venum::Enum::Variant<int*>::Variant<int>::Variant<char>;

    int z = 67;

    Test2 a(5);
    Test2 b('c');
    Test2 ptr(&z);

    a.match(
        [](int* i) { std::cout << "int* " << *i << std::endl; },
        [](int& i) { std::cout << "int: " << i << std::endl; },
        [](char& c) { std::cout << "char: " << c << std::endl; }
    );

    b.match(
        [](int* i) { std::cout << "int* " << *i << std::endl; },
        [](int& i) { std::cout << "int: " << i << std::endl; },
        [](char& c) { std::cout << "char: " << c << std::endl; }
    );

    ptr.match(
        [](int* i) { std::cout << "int* " << *i << std::endl; },
        [](int& i) { std::cout << "int: " << i << std::endl; },
        [](char& c) { std::cout << "char: " << c << std::endl; }
    );

    Test2::construct<char>(47).match(
        [](int* i) { std::cout << "int* " << *i << std::endl; },
        [](int& i) { std::cout << "int: " << i << std::endl; },
        [](char& c) { std::cout << "char: " << c << std::endl; }
    );

    // Optional Test
    auto o = Optional<int>::Some(6);
    if(o) {
        std::cout << o.get() << std::endl;
    }

    try {
        Optional<std::string>::None().get();
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    std::string s = o.map([](int i) { return std::to_string(i); }).match(
        [](None) { return std::string(""); },
        [](std::string s) { return s; }
    );
    std::cout << "result: " << s << std::endl;

    o.and_then([](int i) { return Optional<float>::None(); });

    std::cout << std::endl;

    // Tree Test
    Tree<int> tree;
    tree.insert(5);
    tree.insert(2);
    tree.insert(7);
    tree.insert(3);
    tree.insert(19);
    tree.apply([](int i) { std::cout << i << std::endl; });

    std::cout << tree.contains(19) << std::endl;
    std::cout << tree.contains(8) << std::endl;

    []() {}();
}
