#ifndef ENUM_OPTIONAL_HPP
#define ENUM_OPTIONAL_HPP

#include "enum.hpp"

#include <stdexcept>

struct None {};

template<typename T>
using OptionalBase = Enum::Variant<::None>::Variant<T>;

template<typename T>
class Optional : public OptionalBase<T> {
public:
    using ValueType = T;

    static Optional<T> Some(T t) {
        return Optional(t);
    }

    static Optional<T> None() {
        return Optional(::None{});
    }

    explicit operator bool() {
        return this->match(
            [](::None) { return false; },
            [](T) { return true; }
        );
    }

    T get() {
        return this->match(
            [](::None) -> T { throw std::runtime_error("Error: attempted get() on Optional::None"); },
            [](T t) { return t; }
        );
    }

    template<typename F>
    auto map(F f) {
        using U = decltype(f(get()));

        return this->match(
            [](::None) { return Optional<U>::None(); },
            [&f](T t) { return Optional<U>::Some(f(t)); }
        );
    }

    template<typename F>
    auto and_then(F f) {
        using U = decltype(f(get()))::ValueType;

        return this->match(
            [](::None) { return Optional<U>::None(); },
            [&f](T t) { return f(t); }
        );
    }

    template<typename... Args>
    Optional(Args... args) : OptionalBase<T>(std::forward<Args>(args)...) {}
};

#endif