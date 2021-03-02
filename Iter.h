#ifndef __ITER_H__
#define __ITER_H__

#include <tuple>
#include <iterator>

namespace zip {

    //! \brief An exception class for initializing Iters with containers of different lengths.
    class LengthEqualityException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Lengths of underlying containers do not match.";
        }
    };

    //! \brief A class that allow groups of vectors or arrays to be sorted together. The classes
    //! must have random access iterators (so, e.g. lists will not work). This class can be used
    //! to create ad-hoc "iterators" for collections of collections
    template<typename ...Args>
    class Iter {
    public:
        // ============================================
        //  Iterator pack structure.
        // ============================================

        struct IterPack;

        using Iterators = std::tuple<typename Args::iterator...>;

        // ============================================
        //  Pointer traits types.
        // ============================================
        using difference_type = std::ptrdiff_t;
        using value_type = IterPack;
        using pointer = IterPack *;
        using reference = IterPack &;
        using iterator_category = std::random_access_iterator_tag;

        // ============================================
        //  Member functions
        // ============================================

        //! \brief Construct an Iter from individual iterators.
        explicit Iter(typename Args::iterator &&... iters) : pack(iters...) {}

        //! \brief Dereference operator. Return the iterator pack.
        IterPack &operator*() {
            return pack;
        }

        //! \brief Increment the Iter by incrementing all the underlying iters.
        Iter &operator++() {
            Iter::apply_all([](auto &&x) { ++x; }, pack.iters_);
            return *this;
        }

        //! \brief Decrement the Iter by decrementing all the underlying iters.
        Iter &operator--() {
            Iter::apply_all([](auto &&x) { --x; }, pack.iters_);
            return *this;
        }

        //! \brief Get the difference between two iterators.
        //!
        //! This operator is needed for std::sort.
        difference_type operator-(const Iter<Args...> &rhs) const {
            return std::get<0>(pack.iters_) - std::get<0>(rhs.pack.iters_);
        }

        //! \brief Add to an Iter (advance the Iter) by adding to all the underlying iterators.
        Iter<Args...> operator+(int shift) const {
            Iter<Args...> it = *this;
            it += shift;
            return it;
        }

        //! \brief In place addition operator.
        Iter<Args...> &operator+=(int shift) {
            apply_all([=](auto &&x) { std::advance(x, shift); }, pack.iters_);
            return *this;
        }

        //! \brief Equality checks that all the underlying iterators are equal.
        bool operator==(Iter<Args...> &rhs) const {
            return pack.iters_ == rhs.pack.iters_;
        }

        //! \brief Inequality checks that all the underlying iterators are not equal.
        bool operator!=(const Iter<Args...> &rhs) const {
            return pack.iters_ != rhs.pack.iters_;
        }

        bool operator<=(const Iter<Args...> &rhs) const {
            return pack.iters_ <= rhs.pack.iters_;
        }

        bool operator>=(const Iter<Args...> &rhs) const {
            return pack.iters_ >= rhs.pack.iters_;
        }

        bool operator>(const Iter<Args...> &rhs) const {
            return pack.iters_ > rhs.pack.iters_;
        }

        bool operator<(const Iter<Args...> &rhs) const {
            return pack.iters_ < rhs.pack.iters_;
        }

        // ============================================
        //  Static creation functions.
        // ============================================

        //! \brief The function to create the begin iterator
        friend Iter<Args...> Begin(Args &...containers);

        friend Iter<Args...> End(Args &...containers);

    private:
        // ============================================
        //  Private helper functions.
        // ============================================

        //! \brief The template wrapper function. This allows us to expand parameter pack
        //! expressions in a comma separated list. Note that these expressions must not be "void"
        //! so that the variadic template type is valid.
        template<typename ...Any>
        static void null_wrap(Any...) {};

        //! \brief Apply a function to all the iterators in the tuple.
        template<typename F>
        static void apply_all(F &&func, Iterators &iters) {
            auto func_r = [&](auto &&x) {
                func(x);
                return 0;
            };
            apply_all_(std::make_index_sequence<sizeof...(Args)>{}, func_r, iters);
        }

        //! \brief The apply_all helper function.
        template<std::size_t ...Seq, typename F>
        static void apply_all_(std::index_sequence<Seq...>, F &&func, Iterators &iters) {
            Iter::null_wrap(func(std::get<Seq>(iters))...);
        }

        //! \brief Helper function to apply a binary operation to every pair of elements in
        //! the the two iterator tuples. Since both are of the same Iterators type, they will
        //! have the same number of elements.
        template<typename Binop>
        static void apply_binary(Binop &&binop, Iterators &lhs, Iterators &rhs) {
            auto binop_r = [&](auto &x, auto &y) {
                binop(x, y);
                return 0;
            };
            apply_binary_(std::make_index_sequence<sizeof...(Args)>{}, binop_r, lhs, rhs);
        }

        //! \brief The apply binary helper function.
        template<std::size_t ...Seq, typename Binop>
        static void apply_binary_(std::index_sequence<Seq...>, Binop &&binop, Iterators &lhs, Iterators &rhs) {
            null_wrap(binop(std::get<Seq>(lhs), std::get<Seq>(rhs))...);
        }

        // ============================================
        //  Private member data.
        // ============================================

        //! \brief The iterator pack that underlies the Iter.
        IterPack pack;
    };

    template<typename ...Args>
    class Iter<Args...>::IterPack {
    public:
        explicit IterPack(typename Args::iterator... iters) : iters_(std::make_tuple(iters...)) {}

        //! \brief Checks if the tuples of values pointed at by the current iterators are "less than."
        bool operator<(const IterPack &rhs) const {
            return deref() < rhs.deref();
        }

        //! \brief Checks if the tuples of values pointed at by the current iterators are equal.
        bool operator==(const IterPack &rhs) const {
            return deref() == rhs.deref();
        }

        //! \brief Allow swap to find this function via ADL instead of std::swap.
        friend void swap(IterPack &lhs, IterPack &rhs) {
            using std::swap;
            apply_binary([](auto &p, auto &q) {
                std::iter_swap(p, q);
                return 0;
            }, lhs.iters_, rhs.iters_);
        }

        //! \brief Dereferencing yields the a tuple of the values pointed at by the iterators.
        std::tuple<typename Args::value_type &...> operator*() {
            return deref();
        }

        //! \brief Iter<Args...> is a friend class, for convenience.
        friend class Iter<Args...>;

    private:
        // ============================================
        //  Private helper functions.
        // ============================================

        //! \brief Helper function for mutable dereference.
        auto deref() {
            return deref_(std::make_index_sequence<sizeof...(Args)>{});
        }

        //! \brief Helper function for const dereference.
        auto deref() const {
            return deref_(std::make_index_sequence<sizeof...(Args)>{});
        }

        //! \brief Helper function for mutable dereference.
        template<std::size_t ...Seq>
        auto deref_(std::index_sequence<Seq...>) {
            return std::forward<>(*std::get<Seq>(iters_)...);
        }

        //! \brief Helper function for const dereference.
        template<std::size_t ...Seq>
        auto deref_(std::index_sequence<Seq...>) const {
            return std::forward_as_tuple(*std::get<Seq>(iters_)...);
        }

        // ============================================
        //  Private member data.
        // ============================================

        //! \brief A tuple of iterators.
        Iterators iters_;
    };

    // ============================================
    //  Helper functions
    // ============================================

    template<typename T>
    inline bool all_equal(T&& x) {
        return true;
    }

    template<typename T>
    inline bool all_equal(T&& x, T&& y) {
        return x == y;
    }

    template<typename T, typename ...Args>
    inline bool all_equal(T&& x, T&& y, Args&&... args) {
        return x == y && all_equal(y, args...);
    }


    // ============================================
    //  Definitions of static creation functions.
    // ============================================

    template<typename ...Args>
    Iter<Args...> Begin(Args &...containers) {
        if (!all_equal(containers.size()...)) {
            throw LengthEqualityException();
        }
        return Iter<Args...>(containers.begin()...);
    }

    template<typename ...Args>
    Iter<Args...> End(Args &...containers) {
        return Iter<Args...>(containers.end()...);
    }

}
#endif // __ITER_H__