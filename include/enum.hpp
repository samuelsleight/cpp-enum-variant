#ifndef ENUM_ENUM_HPP
#define ENUM_ENUM_HPP

#include <algorithm>
#include <type_traits>
#include <utility>

template<typename... Variants>
class EnumT;

namespace impl {
    // Variadic Max
    template<typename T>
    constexpr auto max(T a) {
        return a;
    }

    template<typename T, typename... Args>
    constexpr T max(T a, T b, Args... args) {
        if(a > b) {
            return max(a, args...);
        } else {
            return max(b, args...);
        }
    }

    // List 
    template<typename T, std::size_t n>
    struct NthImpl : public NthImpl<typename T::Tail, n - 1> {};

    template<typename T>
    struct NthImpl<T, 0> {
        using value = typename T::Head;
    };

    template<typename...>
    struct List {};

    template<typename H, typename... Ts>
    struct List<H, Ts...> {
        using Head = H;
        using Tail = List<Ts...>;

        template<std::size_t n>
        using Nth = typename NthImpl<List<H, Ts...>, n>::value;
    };

    // Constructor
    template<typename E, typename L, bool enable, std::size_t n, typename... Args>
    struct EnumConstructorImpl;

    template<typename E, typename L, std::size_t n, typename... Args>
    struct EnumConstructorImpl<E, L, false, n, Args...> : public EnumConstructorImpl<E, typename L::Tail, std::is_constructible<typename L::Tail::Head, Args...>::value, n + 1, Args...> {};

    template<typename E, typename L, std::size_t n, typename... Args>
    struct EnumConstructorImpl<E, L, true, n, Args...> {
        static void construct(E* e, Args&&... args) {
            ::new (&(e->storage)) typename L::Head(std::forward<Args>(args)...);
            e->tag = n;
        }
    };

    template<typename T, typename... Fs>
    struct EnumConstructor;

    template<typename Variant, typename... Variants, typename... Args>
    struct EnumConstructor<::EnumT<Variant, Variants...>, Args...> : public EnumConstructorImpl<::EnumT<Variant, Variants...>, List<Variant, Variants...>, std::is_constructible<Variant, Args...>::value, 0, Args...> {};

    // Copy Constructor
    template<typename E, typename L, std::size_t n, std::size_t m>
    struct EnumCopyImpl {
        static void copy(const E& from, E* to) {
            using T = typename L::template Nth<n>;

            if(from.tag == n) {
                to->tag = n;
                ::new (&(to->storage)) T(*reinterpret_cast<T*>(&(from.storage)));
            } else {
                EnumCopyImpl<E, L, n + 1, m>::copy(std::forward<E>(from), to);
            }
        }
    };

    template<typename E, typename L, std::size_t n>
    struct EnumCopyImpl<E, L, n, n> {
        static void copy(const E& from, E* to) {}
    };

    template<typename E>
    struct EnumCopy;

    template<typename... Variants>
    struct EnumCopy<::EnumT<Variants...>> : public EnumCopyImpl<::EnumT<Variants...>, List<Variants...>, 0, sizeof...(Variants)> {};

    // Move Constructor
    template<typename E, typename L, std::size_t n, std::size_t m>
    struct EnumMoveImpl {
        static void move(E&& from, E* to) {
            using T = typename L::template Nth<n>;

            if(from.tag == n) {
                to->tag = std::move(from.tag);
                ::new (&(to->storage)) T(std::move(*reinterpret_cast<T*>(&(from.storage))));
            } else {
                EnumMoveImpl<E, L, n + 1, m>::move(std::forward<E>(from), to);
            }
        }
    };

    template<typename E, typename L, std::size_t n>
    struct EnumMoveImpl<E, L, n, n> {
        static void move(E&& from, E* to) {}
    };

    template<typename E>
    struct EnumMove;

    template<typename... Variants>
    struct EnumMove<::EnumT<Variants...>> : public EnumMoveImpl<::EnumT<Variants...>, List<Variants...>, 0, sizeof...(Variants)> {};

    // Match
    template<typename E, typename L, std::size_t n, std::size_t m, typename... Fs>
    struct EnumMatchImpl;

    template<typename E, typename L, std::size_t n, std::size_t m, typename F, typename... Fs>
    struct EnumMatchImpl<E, L, n, m, F, Fs...> {
        static auto match(E* e, F f, Fs... fs) {
            using T = typename L::template Nth<n>;

            if(e->tag == n) {
                return f(*reinterpret_cast<T*>(&(e->storage)));
            } else {
                return EnumMatchImpl<E, L, n + 1, m, Fs...>::match(e, fs...);
            }
        }
    };

    template<typename E, typename L, std::size_t n, typename... Fs>
    struct EnumMatchImpl<E, L, n, n, Fs...> {
        static auto match(E* e, Fs... fs) {}
    };

    template<typename T, typename... Fs>
    struct EnumMatch;

    template<typename... Variants, typename... Fs>
    struct EnumMatch<::EnumT<Variants...>, Fs...> : public EnumMatchImpl<::EnumT<Variants...>, List<Variants...>, 0, sizeof...(Variants), Fs...> {};

    // Apply 
    template<typename E, typename L, std::size_t n, std::size_t m, typename F>
    struct EnumApplyImpl {
        static auto apply(E* e, F f) {
            using T = typename L::template Nth<n>;

            if(e->tag == n) {
                return f(*reinterpret_cast<T*>(&(e->storage)));
            } else {
                return EnumApplyImpl<E, L, n + 1, m, F>::apply(e, f);
            }
        }
    };

    template<typename E, typename L, std::size_t n, typename F>
    struct EnumApplyImpl<E, L, n, n, F> {
        static auto apply(E* e, F f) {}
    };

    template<typename T, typename F>
    struct EnumApply;

    template<typename... Variants, typename F>
    struct EnumApply<::EnumT<Variants...>, F> : public EnumApplyImpl<::EnumT<Variants...>, List<Variants...>, 0, sizeof...(Variants), F> {};

    // Destructor
    template<typename E, typename L, std::size_t n, std::size_t m>
    struct EnumDestructorImpl {
        static void destruct(E* e) {
            using T = typename L::template Nth<n>;

            if(e->tag == n) {
                reinterpret_cast<T*>(&(e->storage))->~T();
            } else {
                EnumDestructorImpl<E, L, n + 1, m>::destruct(e);
            }
        }
    };

    template<typename E, typename L, std::size_t n>
    struct EnumDestructorImpl<E, L, n, n> {
        static void destruct(E* e) {}
    };

    template<typename T>
    struct EnumDestructor;

    template<typename... Variants>
    struct EnumDestructor<::EnumT<Variants...>> : public EnumDestructorImpl<::EnumT<Variants...>, List<Variants...>, 0, sizeof...(Variants)> {};
};

template<typename VariantT, typename... Variants>
class EnumT<VariantT, Variants...> {
public:
    template<typename T>
    using Variant = EnumT<VariantT, Variants..., T>;
    //using Variant = typename std::enable_if<std::is_trivially_copyable<T>::value, EnumT<VariantT, Variants..., T>>::type;

    using SelfType = EnumT<VariantT, Variants...>;

    using List = impl::List<VariantT, Variants...>;

    template<typename... Args>
    using ConstructorType = impl::EnumConstructor<SelfType, Args...>;

    using CopyType = impl::EnumCopy<SelfType>;
    using MoveType = impl::EnumMove<SelfType>;

    template<typename... Fs>
    using MatchType = impl::EnumMatch<SelfType, Fs...>;

    template<typename F>
    using ApplyType = impl::EnumApply<SelfType, F>;

    using DestructorType = impl::EnumDestructor<SelfType>;

    static constexpr std::size_t storage_size = impl::max(sizeof(VariantT), sizeof(Variants)...);
    static constexpr std::size_t storage_align = impl::max(alignof(VariantT), alignof(Variants)...);

    using StorageType = typename std::aligned_storage<storage_size, storage_align>::type;

    template<typename... Args>
    EnumT(Args&&... args) {
        ConstructorType<Args...>::construct(this, std::forward<Args>(args)...);
    }

    EnumT() = delete;

    EnumT(const SelfType& other) {
        CopyType::copy(std::forward<SelfType>(other), this);
    }

    EnumT(SelfType&& other) noexcept {
        MoveType::move(std::forward<SelfType>(other), this);
    }

    EnumT& operator=(const SelfType& other) {
        CopyType::copy(std::forward<SelfType>(other), this);
        return *this;
    }

    EnumT& operator=(SelfType&& other) noexcept {
        MoveType::move(std::forward<SelfType>(other), this);
        return *this;
    }

    template<typename F>
    auto apply(F f) {
        return ApplyType<F>::apply(this, std::forward<F>(f));
    }

    template<typename... Fs>
    auto match(Fs... fs) {
        return MatchType<Fs...>::match(this, std::forward<Fs>(fs)...);
    }

    ~EnumT() {
        DestructorType::destruct(this);
    }

    std::size_t tag;
    StorageType storage;
};


class Enum {
public:
    template<typename T>
    using Variant = EnumT<T>;
    //using Variant = typename std::enable_if<std::is_trivially_copyable<T>::value, EnumT<T>>::type;
};

#endif
