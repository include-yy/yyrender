#pragma once

// https://mc-deltat.github.io/articles/stateful-metaprogramming-cpp20
// funny stateful metaprogramming counter
namespace Resource
{
    template<unsigned N>
    struct reader
    {
        friend auto counted_flag(reader<N>);
    };

    template<unsigned N>
    struct setter
    {
        friend auto counted_flag(reader<N>) {};
        static constexpr unsigned n = N;
    };

    template<auto Tag, unsigned NextVal = 0>
    [[nodiscard]] consteval auto counter_impl()
    {
        constexpr bool counted_past_value = requires(reader<NextVal> r)
        {
            counted_flag(r);
        };
        if constexpr (counted_past_value) {
            return counter_impl<Tag, NextVal + 1>();
        }
        else
        {
            setter<NextVal> s;
            return s.n;
        }
    }

    template<auto Tag = [] {}, auto Val = counter_impl<Tag>() >
    constexpr auto counter = Val;
}

/* Open new obj file */
constexpr auto YYMID_OPEN = Resource::counter<>;
/* Select different Shaders */
constexpr auto YYMID_SHADERS = Resource::counter<>;
constexpr auto YYMID_SHADER_NORMAL_COLOR = Resource::counter<>;
constexpr auto YYMID_SHADER_MAPKD_COLOR = Resource::counter<>;
constexpr auto YYMID_SHADER_KE_COLOR = Resource::counter<>;
constexpr auto YYMID_SHADER_KAKDKS = Resource::counter<>;
constexpr auto YYMID_SHADER_PHONG_SHADOWMAP = Resource::counter<>;
/* Some Rendering Options */
constexpr auto YYMID_OPTIONS = Resource::counter<>;
// constexpr auto YYMID_OPTION_USE_LIGHT = Resource::counter<>;
constexpr auto YYMID_OPTION_USE_DEFAULT_TEXTURE = Resource::counter<>;
/* Author information */
constexpr auto YYMID_ABOUT = Resource::counter<>;

//static_assert(YYMID_OPEN == 0);
//static_assert(YYMID_SHADERS == 1);
