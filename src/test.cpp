#include "enum.hpp"
#include "optional.hpp"

#include <iostream>
#include <string>
#include <vector>

struct Thing {
    int i;
    char c;
};

int main(int argc, char* argv[]) {
    using Test = Enum
        ::Variant<std::string>
        ::Variant<int>
        ::Variant<Thing>;

    std::vector<Test> v;
    v.emplace_back(7);
    v.emplace_back("Hello");
    v.emplace_back(5);
    v.emplace_back(5, 'a');

    for(auto& t : v) {
        std::cout << t.tag << std::endl;

        t.match(
            [](std::string& s) { std::cout << "string: " << s << std::endl; },
            [](int& i) { std::cout << "int: " << i << std::endl; },
            [](Thing& t) { std::cout << "thing: " << t.i << ", " << t.c << std::endl; }
        );

        std::cout << std::endl;
    }

	// Optional Test
	auto a = Optional<int>::Some(6);
	if(a) {
		std::cout << a.get() << std::endl;
	}

	try {
		Optional<std::string>::None().get();
	} catch(std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}
