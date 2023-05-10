#pragma once
#include <tuple>

#include "error.hpp"

#ifdef CUTIL_NS
namespace CUTIL_NS {
#endif

template <class T>
struct Tag {
    using Type = T;
};

template <class... Ts>
class Variant {
  private:
    template <size_t n>
    using E = std::tuple_element_t<n, std::tuple<Ts...>>;

    template <class T>
    static constexpr auto false_v = false;

    static constexpr auto invalid_index = size_t(-1);

    alignas(Ts...) std::byte data[std::max({sizeof(Ts)...})];
    size_t index = invalid_index;

    template <size_t n, class T>
    static consteval auto find_index_of() -> size_t {
        if constexpr(n < sizeof...(Ts)) {
            if constexpr(std::is_same_v<E<n>, T>) {
                return n;
            } else {
                return find_index_of<n + 1, T>();
            }
        } else {
            static_assert(false_v<T>, "no such type");
        }
    }

    template <size_t n>
    static auto apply_void(auto self, auto visitor) -> bool {
        if constexpr(n < sizeof...(Ts)) {
            if(n == self->index) {
                visitor(self->template as<E<n>>());
                return true;
            }
            return apply_void<n + 1>(self, visitor);
        }

        return false;
    }

    template <size_t n, class R>
    static auto apply_result(auto self, auto visitor) -> std::optional<R> {
        if constexpr(n < sizeof...(Ts)) {
            if(n == self->index) {
                return visitor(self->template as<E<n>>());
            }
            return apply_result<n + 1, R>(self, visitor);
        }

        return std::nullopt;
    }

    static auto apply(auto self, auto visitor) -> decltype(auto) {
        using R = decltype(visitor(self->template as<E<0>>()));
        if constexpr(std::is_same_v<R, void>) {
            if(!self->is_valid()) {
                return false;
            }
            return apply_void<0>(self, visitor);
        } else {
            if(!self->is_valid()) {
                return std::optional<R>(std::nullopt);
            }
            return apply_result<0, R>(self, visitor);
        }
    }

    // static auto copy_assign(Variant* self, Variant* other) -> void {
    template <bool move>
    static auto assign(Variant* const self, auto other) -> void {
        if(!other->is_valid()) {
            self->reset();
            return;
        }

        if(self->index == other->index) {
            self->apply([other](auto& v) {
                other->apply([other, &v](auto& u) {
                    using V = std::remove_cvref_t<decltype(v)>;
                    using U = std::remove_cvref_t<decltype(u)>;
                    if constexpr(std::same_as<V, U>) {
                        if constexpr(move) {
                            v = std::move(u);
                            other->reset();
                        } else {
                            v = u;
                            (void)other;
                        }
                    } else {
                        // should not reach here
                    }
                });
            });
            return;
        }

        self->reset();
        other->apply([self, other](auto& v) {
            using T = std::remove_cvref_t<decltype(v)>;
            if constexpr(move) {
                self->emplace<T>(std::move(v));
                other->reset();
            } else {
                self->emplace<T>(v);
                (void)other;
            }
        });
        return;
    }

  public:
    template <class T>
    static constexpr auto index_of = find_index_of<0, T>();

    auto get_index() const -> size_t {
        return index;
    }

    template <class T>
    auto get() -> T* {
        return const_cast<T*>(std::as_const(*this).template get<T>());
    }

    template <class T>
    auto get() const -> const T* {
        if(index_of<T> == index) {
            return std::bit_cast<const T*>(&data);
        } else {
            return nullptr;
        }
    }

    template <class T>
    auto as() -> T& {
        return as<index_of<T>>();
    }

    template <class T>
    auto as() const -> const T& {
        return as<index_of<T>>();
    }

    template <size_t n>
    auto as() -> E<n>& {
        return *std::bit_cast<E<n>*>(&data);
    }

    template <size_t n>
    auto as() const -> const E<n>& {
        return *std::bit_cast<const E<n>*>(&data);
    }

    auto apply(auto visitor) -> decltype(auto) {
        return apply(this, visitor);
    }

    auto apply(auto visitor) const -> decltype(auto) {
        return apply(this, visitor);
    }

    auto is_valid() const -> bool {
        return index != invalid_index;
    }

    template <class T, class... Args>
    auto emplace(Args&&... args) -> T& {
        reset();
        if constexpr(std::is_constructible_v<T, Args...>) {
            new(&data) T(std::forward<Args>(args)...);
        } else {
            new(&data) T{std::forward<Args>(args)...};
        }
        index = index_of<T>;
        return as<T>();
    }

    auto reset() -> void {
        if(!is_valid()) {
            return;
        }
        apply([](auto& v) { std::destroy_at(&v); });
        index = invalid_index;
    }

    auto operator=(Variant& o) -> Variant& {
        assign<false>(this, &o);
        return *this;
    }

    auto operator=(const Variant& o) -> Variant& {
        assign<false>(this, &o);
        return *this;
    }

    auto operator=(Variant&& o) -> Variant& {
        assign<true>(this, &o);
        return *this;
    }

    Variant() = default;

    Variant(Variant&& o) {
        assign<true>(this, &o);
    }

    template <class T, class... Args>
    Variant(Tag<T>, Args&&... args) {
        emplace<T>(std::forward<Args>(args)...);
    }

    ~Variant() {
        reset();
    }
};

inline auto variant_test() -> void {
    {
        using V = Variant<int, float>;

        dynamic_assert(!V().is_valid());

        auto v = V(Tag<int>(), 1);
        auto u = V(Tag<float>(), 1.0);
        dynamic_assert(*v.get<int>() == 1);
        dynamic_assert(*u.get<float>() == 1.0);

        auto e = V();
        v      = e;
        dynamic_assert(!v.is_valid());

        v = std::move(u);
        dynamic_assert(*v.get<float>() == 1.0);
        dynamic_assert(!u.is_valid());
    }
    {
        static auto constructor      = 0;
        static auto move_constructor = 0;
        static auto copy_assign      = 0;
        static auto move_assign      = 0;
        static auto copy_constructor = 0;
        static auto destructor       = 0;
        struct Noisy {
            auto operator=(Noisy&&) {
                move_assign += 1;
            }

            auto operator=(Noisy&) {
                copy_assign += 1;
            }

            Noisy() {
                constructor += 1;
            }

            Noisy(Noisy&&) {
                move_constructor += 1;
            }

            Noisy(Noisy&) {
                copy_constructor += 1;
            }

            ~Noisy() {
                destructor += 1;
            }
        };

        using V = Variant<Noisy>;

        auto v = V();
        dynamic_assert(constructor == 0);

        v.emplace<Noisy>();
        dynamic_assert(constructor == 1);

        v.reset();
        dynamic_assert(destructor == 1);

        auto u = V();
        v.emplace<Noisy>();

        u = v;
        dynamic_assert(copy_constructor == 1 && v.is_valid());

        u.reset();
        u = std::move(v);
        dynamic_assert(move_constructor == 1 && !v.is_valid());

        v.emplace<Noisy>();
        u.emplace<Noisy>();

        u = v;
        dynamic_assert(copy_assign == 1 && v.is_valid());

        u = std::move(v);
        dynamic_assert(move_assign == 1 && !v.is_valid());
    }
}

#ifdef CUTIL_NS
}
#endif
