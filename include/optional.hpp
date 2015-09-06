#ifndef ENUM_OPTIONAL_HPP
#define ENUM_OPTIONAL_HPP

#include "enum.hpp"

#include <stdexcept>

struct None {};

namespace impl {
    template<typename T>
    using OptionalBase = Enum::Variant<::None>::Variant<T>;
}

template<typename T>
class Optional : public impl::OptionalBase<T> {
public:
    static Optional<T> Some(T t) {
        return Optional(t);
    }

    static Optional<T> None() {
        return Optional(::None{});
    }

    explicit operator bool() {
        return match(
            [](::None) { return false; },
            [](T) { return true; }
        );
    }

    T get() {
        return match(
            [](::None) -> T { throw std::runtime_error("Error: attempted get() on Optional::None"); },
            [](T t) { return t; }
        );
    }

    template<typename F>
    auto map(F f) {
        using U = decltype(f(get()));

        return match(
            [](::None) { return Optional<U>::None(); },
            [&f](T t) { return Optional<U>::Some(f(t)); }
        );
    }
private:
    template<typename... Args>
    Optional(Args... args) : impl::OptionalBase<T>(std::forward<Args>(args)...) {}
};

#endif