//////////////////////////////////////////////////////////////////////////////
//  File: cpp-enum-variant/test.cpp
//////////////////////////////////////////////////////////////////////////////
//  Copyright 2017 Samuel Sleight
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//////////////////////////////////////////////////////////////////////////////

#include "enum.hpp"
#include "optional.hpp"
#include "tree.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

struct Thing {
    int i;
    char c;
};

struct BadThing {
    std::string str;

    BadThing(std::string str) : str(str) {}

    BadThing(const BadThing& other) {
        throw std::runtime_error("lol");
    }
};

void exception_test() {
    using Test = venum::Enum
        ::Variant<BadThing>
        ::Variant<std::string>;

    Test test(std::string{"hello"});
    test = "u wot m8";

    test.match(
        [](BadThing& thing) { std::cout << "thing: " << thing.str << std::endl; },
        [](std::string& s) { std::cout << "s: " << s << std::endl; },
        [](const venum::InvalidVariantError& iv) { std::cout << "invalid: " << iv.what() << ": " << iv.reason() << std::endl; }
    );

    try {
        test.match(
            [](BadThing& thing) { std::cout << "thing: " << thing.str << std::endl; },
            [](std::string& s) { std::cout << "s: " << s << std::endl; }
        );
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    Test test2(std::string{"hello2"});

    Test test3 = std::move(test2);

    test2.match(
        [](BadThing& thing) { std::cout << "thing: " << thing.str << std::endl; },
        [](std::string& s) { std::cout << "s: " << s << std::endl; },
        [](const venum::InvalidVariantError& iv) { std::cout << "invalid: " << iv.what() << ": " << iv.reason() << std::endl; }
    );

    test3.match(
        [](BadThing& thing) { std::cout << "thing: " << thing.str << std::endl; },
        [](std::string& s) { std::cout << "s: " << s << std::endl; },
        [](const venum::InvalidVariantError& iv) { std::cout << "invalid: " << iv.what() << ": " << iv.reason() << std::endl; }
    );
}

int main(int argc, char* argv[]) {
    exception_test();

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
    auto c = Test2::construct<char>(47);

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

    std::string s2 = a.match(
        [](int* i) { std::ostringstream str; str << "int*: " << i; return str.str(); },
        [](int i) { std::ostringstream str; str << "int: " << i; return str.str(); },
        [](char c) { std::ostringstream str; str << "char: " << c; return str.str(); }
    );

    c.match(
        [](int* i) { std::cout << "int* " << *i << std::endl; },
        [](int& i) { std::cout << "int: " << i << std::endl; },
        [](char& c) { std::cout << "char: " << c << std::endl; }
    );

    std::cout << c.which() << " " << c.contains<char>() << " " << c.contains<int>() << std::endl;

    try {
        std::cout << "not a real int: " << c.get<int>() << std::endl;
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    std::cout << "not a real int: " << c.get_unchecked<int>() << std::endl;

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
