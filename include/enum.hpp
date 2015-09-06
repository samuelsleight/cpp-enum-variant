#ifndef ENUM_ENUM_HPP
#define ENUM_ENUM_HPP

#include <algorithm>
#include <type_traits>
#include <utility>

// Variadic Max
template<typename T>
constexpr auto const_max(T a) {
    return a;
}

template<typename T, typename... Args>
constexpr T const_max(T a, T b, Args... args) {
    return a > b ? const_max(a, args...) : const_max(b, args...);
}

// TypeList 
template<typename T, std::size_t n>
struct NthImpl : public NthImpl<typename T::Tail, n - 1> {};

template<typename T>
struct NthImpl<T, 0> {
    using value = typename T::Head;
};

template<typename...>
struct TypeList {};

template<typename H, typename... Ts>
struct TypeList<H, Ts...> {
    using Head = H;
    using Tail = TypeList<Ts...>;

    template<std::size_t n>
    using Nth = typename NthImpl<TypeList<H, Ts...>, n>::value;
};

template<typename VariantT, typename... Variants>
class EnumT {
public:
    static constexpr std::size_t storage_size = const_max(sizeof(VariantT), sizeof(Variants)...);
    static constexpr std::size_t storage_align = const_max(alignof(VariantT), alignof(Variants)...);

    static constexpr std::size_t variants = sizeof...(Variants) + 1;

private:
    using Self = EnumT<VariantT, Variants...>;
    using VariantList = TypeList<VariantT, Variants...>;

    using StorageT = typename std::aligned_storage<storage_size, storage_align>::type;

    // Implementation detail
    template<typename E>
    struct impl {
        // Constructor
        template<typename E, bool enable, std::size_t n, typename... Args>
        struct ConstructorT;

        template<typename E, std::size_t n, typename... Args>
        struct ConstructorT<E, false, n, Args...> 
            : public ConstructorT<E, std::is_constructible<typename E::VariantList::template Nth<n + 1>, Args...>::value, n + 1, Args...> {};

        template<typename E, std::size_t n, typename... Args>
        struct ConstructorT<E, true, n, Args...> {
            static void construct(E* e, Args&&... args) {
                using T = typename E::VariantList::template Nth<n>;

                ::new (&(e->storage)) T(std::forward<Args>(args)...);
                e->tag = n;
            }
        };

        template<typename... Args>
        using Constructor = ConstructorT<E, std::is_constructible<typename E::VariantList::Head, Args...>::value, 0, Args...>;

        // Copy Constructor
        template<typename E, std::size_t n, std::size_t m>
        struct CopyConstructorT {
            static void copy(const E& from, E* to) {
                using T = typename E::VariantList::template Nth<n>;

                if(from.tag == n) {
                    to->tag = n;
                    ::new (&(to->storage)) T(*reinterpret_cast<T*>(&(from.storage)));
                } else {
                    CopyConstructorT<E, n + 1, m>::copy(std::forward<E>(from), to);
                }
            }
        };

        template<typename E, std::size_t n>
        struct CopyConstructorT<E, n, n> {
            static void copy(const E& from, E* to) {
                using T = typename E::VariantList::template Nth<n>;

                if(from.tag == n) {
                    to->tag = n;
                    ::new (&(to->storage)) T(*reinterpret_cast<T*>(&(from.storage)));
                }
            }
        };

        using CopyConstructor = CopyConstructorT<E, 0, E::variants - 1>;

        // Move Constructor
        template<typename E, std::size_t n, std::size_t m>
        struct MoveConstructorT {
            static void move(E&& from, E* to) {
                using T = typename E::VariantList::template Nth<n>;

                if(from.tag == n) {
                    to->tag = std::move(n);
                    ::new (&(to->storage)) T(std::move(*reinterpret_cast<T*>(&(from.storage))));
                } else {
                    MoveConstructorT<E, n + 1, m>::move(std::forward<E>(from), to);
                }
            }
        };

        template<typename E, std::size_t n>
        struct MoveConstructorT<E, n, n> {
            static void move(E&& from, E* to) {
                using T = typename E::VariantList::template Nth<n>;

                if(from.tag == n) {
                    to->tag = std::move(n);
                    ::new (&(to->storage)) T(std::move(*reinterpret_cast<T*>(&(from.storage))));
                }
            }
        };

        using MoveConstructor = MoveConstructorT<E, 0, E::variants - 1>;

        // Destructor
        template<typename E, std::size_t n, std::size_t m>
        struct DestructorT {
            static void destruct(E* e) {
                using T = typename E::VariantList::template Nth<n>;

                if(e->tag == n) {
                    reinterpret_cast<T*>(&(e->storage))->~T();
                } else {
                    DestructorT<E, n + 1, m>::destruct(e);
                }
            }
        };

        template<typename E, std::size_t n>
        struct DestructorT<E, n, n> {
            static void destruct(E* e) {
                using T = typename E::VariantList::template Nth<n>;

                if(e->tag == n) {
                    reinterpret_cast<T*>(&(e->storage))->~T();
                }
            }
        };

        using Destructor = DestructorT<E, 0, E::variants - 1>;

        // Match
        template<typename E, std::size_t n, std::size_t m, typename... Fs>
        struct MatchT;

        template<typename E, std::size_t n, std::size_t m, typename F, typename... Fs>
        struct MatchT<E, n, m, F, Fs...> {
            static auto match(E* e, F f, Fs... fs) {
                using T = typename E::VariantList::template Nth<n>;

                if(e->tag == n) {
                    return f(*reinterpret_cast<T*>(&(e->storage)));
                } else {
                    return MatchT<E, n + 1, m, Fs...>::match(e, fs...);
                }
            }
        };

        template<typename E, std::size_t n, typename F>
        struct MatchT<E, n, n, F> {
            static auto match(E* e, F f) {
                using T = typename E::VariantList::template Nth<n>;

                if(e->tag == n) {
                    return f(*reinterpret_cast<T*>(&(e->storage)));
                }
            }
        };

        template<typename... Fs>
        using Match = MatchT<E, 0, E::variants - 1, Fs...>;

        // Apply
        template<typename E, std::size_t n, std::size_t m, typename F>
        struct ApplyT {
            static auto apply(E* e, F f) {
                using T = typename E::VariantList::template Nth<n>;

                if(e->tag == n) {
                    return f(*reinterpret_cast<T*>(&(e->storage)));
                } else {
                    return ApplyT<E, n + 1, m, F>::apply(e, f);
                }
            }
        };

        template<typename E, std::size_t n, typename F>
        struct ApplyT<E, n, n, F> {
            static auto apply(E* e, F f) {
                using T = typename E::VariantList::template Nth<n>;

                if(e->tag == n) {
                    return f(*reinterpret_cast<T*>(&(e->storage)));
                }
            }
        };

        template<typename F>
        using Apply = ApplyT<E, 0, E::variants, F>;
    };

    std::size_t tag;
    StorageT storage;

public:
    template<typename T>
    using Variant = EnumT<VariantT, Variants..., T>;

    EnumT() = delete;

    template<typename... Args>
    EnumT(Args&&... args) {
        impl<Self>::Constructor<Args...>::construct(this, std::forward<Args>(args)...);
    }

    EnumT(const Self& other) {
        impl<Self>::CopyConstructor::copy(std::forward<Self>(other), this);
    }

    EnumT(Self&& other) noexcept {
        impl<Self>::MoveConstructor::move(std::forward<Self>(other), this);
    }

    EnumT& operator=(const Self& other) {
        impl<Self>::Destructor::destruct(this);
        impl<Self>::CopyConstructor::copy(std::forward<Self>(other), this);
        return *this;
    }

    EnumT& operator=(Self&& other) noexcept {
        impl<Self>::Destructor::destruct(this);
        impl<Self>::MoveConstructor::move(std::forward<Self>(other), this);
        return *this;
    }

    template<typename F>
    auto apply(F f) {
        return impl<Self>::Apply<F>::apply(this, std::forward<F>(f));
    }

    template<typename... Fs>
    auto match(Fs... fs) {
        return impl<Self>::Match<Fs...>::match(this, std::forward<Fs>(fs)...);
    }

    ~EnumT() {
        impl<Self>::Destructor::destruct(this);
    }
};


class Enum {
public:
    template<typename T>
    using Variant = EnumT<T>;
};

#endif
