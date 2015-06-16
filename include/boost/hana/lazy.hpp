/*!
@file
Defines `boost::hana::Lazy`.

@copyright Louis Dionne 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_LAZY_HPP
#define BOOST_HANA_LAZY_HPP

#include <boost/hana/fwd/lazy.hpp>

#include <boost/hana/concept/applicative.hpp>
#include <boost/hana/concept/comonad.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/datatype.hpp>
#include <boost/hana/core/default.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/detail/operators/monad.hpp>
#include <boost/hana/detail/wrong.hpp>
#include <boost/hana/detail/closure.hpp>
#include <boost/hana/functional/apply.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/functional/id.hpp>
#include <boost/hana/functional/on.hpp>
#include <boost/hana/concept/functor.hpp>
#include <boost/hana/concept/monad.hpp>

#include <boost/hana/eval.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


namespace boost { namespace hana {
    //////////////////////////////////////////////////////////////////////////
    // lazy
    //////////////////////////////////////////////////////////////////////////
    template <typename Indices, typename F, typename ...Args>
    struct _lazy_apply;

    template <std::size_t ...n, typename F, typename ...Args>
    struct _lazy_apply<std::index_sequence<n...>, F, Args...>
        : operators::adl, detail::closure<F, Args...>
    {
        _lazy_apply(_lazy_apply const&) = default;
        _lazy_apply(_lazy_apply&) = default;
        _lazy_apply(_lazy_apply&&) = default;
        using detail::closure<F, Args...>::closure;
        using hana = _lazy_apply;
        using datatype = Lazy;
    };

    template <typename X>
    struct _lazy_value
        : operators::adl, detail::closure<X>
    {
        _lazy_value(_lazy_value const&) = default;
        _lazy_value(_lazy_value&) = default;
        _lazy_value(_lazy_value&&) = default;
        using detail::closure<X>::closure;
        using hana = _lazy_value;
        using datatype = Lazy;

        // If this is called, we assume that `X` is in fact a function.
        template <typename ...Args>
        constexpr _lazy_apply<
            std::make_index_sequence<sizeof...(Args)>,
            X, typename std::decay<Args>::type...
        > operator()(Args&& ...args) const& {
            return {detail::get<0>(*this), static_cast<Args&&>(args)...};
        }

        template <typename ...Args>
        constexpr _lazy_apply<
            std::make_index_sequence<sizeof...(Args)>,
            X, typename std::decay<Args>::type...
        > operator()(Args&& ...args) && {
            return {
                static_cast<X&&>(detail::get<0>(*this)),
                static_cast<Args&&>(args)...
            };
        }
    };

    //! @cond
    template <typename X>
    constexpr auto _lazy::operator()(X&& x) const {
        return _lazy_value<typename std::decay<X>::type>{
            static_cast<X&&>(x)
        };
    }
    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <>
        struct monad_operators<Lazy> { static constexpr bool value = true; };
    }

    //////////////////////////////////////////////////////////////////////////
    // eval for Lazy
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct eval_impl<Lazy> {
        // _lazy_apply
        template <std::size_t ...n, typename F, typename ...Args>
        static constexpr decltype(auto)
        apply(_lazy_apply<std::index_sequence<n...>, F, Args...> const& expr) {
            return detail::get<0>(expr)(
                detail::get<n+1>(expr)...
            );
        }

        template <std::size_t ...n, typename F, typename ...Args>
        static constexpr decltype(auto)
        apply(_lazy_apply<std::index_sequence<n...>, F, Args...>& expr) {
            return detail::get<0>(expr)(
                detail::get<n+1>(expr)...
            );
        }

        template <std::size_t ...n, typename F, typename ...Args>
        static constexpr decltype(auto)
        apply(_lazy_apply<std::index_sequence<n...>, F, Args...>&& expr) {
            return static_cast<F&&>(detail::get<0>(expr))(
                static_cast<Args&&>(detail::get<n+1>(expr))...
            );
        }

        // _lazy_value
        template <typename X>
        static constexpr X const& apply(_lazy_value<X> const& expr)
        { return detail::get<0>(expr); }

        template <typename X>
        static constexpr X& apply(_lazy_value<X>& expr)
        { return detail::get<0>(expr); }

        template <typename X>
        static constexpr X apply(_lazy_value<X>&& expr)
        { return static_cast<X&&>(detail::get<0>(expr)); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Functor
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct transform_impl<Lazy> {
        template <typename Expr, typename F>
        static constexpr auto apply(Expr&& expr, F&& f) {
            return hana::lazy(hana::compose(static_cast<F&&>(f),
                                            hana::eval))
                                        (static_cast<Expr&&>(expr));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Applicative
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct lift_impl<Lazy> {
        template <typename X>
        static constexpr _lazy_value<typename std::decay<X>::type>
        apply(X&& x) {
            return {static_cast<X&&>(x)};
        }
    };

    template <>
    struct ap_impl<Lazy> {
        template <typename F, typename X>
        static constexpr decltype(auto) apply(F&& f, X&& x) {
            return hana::lazy(hana::on(hana::apply, hana::eval))(
                static_cast<F&&>(f), static_cast<X&&>(x)
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Monad
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct flatten_impl<Lazy> {
        template <typename Expr>
        static constexpr decltype(auto) apply(Expr&& expr) {
            return hana::lazy(hana::compose(eval, eval))(
                static_cast<Expr&&>(expr)
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Comonad
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct extract_impl<Lazy> {
        template <typename Expr>
        static constexpr decltype(auto) apply(Expr&& expr)
        { return hana::eval(static_cast<Expr&&>(expr)); }
    };

    template <>
    struct duplicate_impl<Lazy> {
        template <typename Expr>
        static constexpr decltype(auto) apply(Expr&& expr)
        { return hana::lazy(static_cast<Expr&&>(expr)); }
    };

    template <>
    struct extend_impl<Lazy> {
        template <typename Expr, typename F>
        static constexpr decltype(auto) apply(Expr&& expr, F&& f) {
            return hana::lazy(static_cast<F&&>(f))(
                                        static_cast<Expr&&>(expr));
        }
    };
}} // end namespace boost::hana

#endif // !BOOST_HANA_LAZY_HPP
