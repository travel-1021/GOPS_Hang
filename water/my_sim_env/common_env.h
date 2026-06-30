#pragma once
#include <random>
#include <vector>
#include <array>
#include <optional>

namespace slxpy::env {
    using raw_T = rl_water;
    using input_T = raw_T::ExtU_rl_water_T;
    using output_T = raw_T::ExtY_rl_water_T;
    using par_T = raw_T::InstP_rl_water_T;
    using rng_T = std::mt19937;

    constexpr auto step_ptr = &raw_T::step;
    constexpr auto init_ptr = &raw_T::initialize;
    constexpr auto term_ptr = &raw_T::terminate;
    constexpr auto in_ptr = &raw_T::rl_water_U;
    constexpr auto out_ptr = &raw_T::rl_water_Y;
    constexpr auto par_ptr = &raw_T::rl_water_InstP;

    template <typename T>
    using npac_T = pybind11::array_t<T, pybind11::array::c_style | pybind11::array::forcecast>;
    template <typename T>
    using npaf_T = pybind11::array_t<T, pybind11::array::f_style | pybind11::array::forcecast>;
    template <typename T>
    using npa_T = npac_T<T>;

    bool size_not_equal(pybind11::ssize_t size_py, size_t size_cpp) {
        // Explicit cast to avoid GCC and CLANG warning
        return static_cast<size_t>(size_py) != size_cpp;
    }

    
    using step_T = pybind11::tuple;
    using reset_T = pybind11::tuple;

    template <typename T, auto M>
    using underlying_t = std::remove_all_extents_t<std::remove_reference_t<decltype(std::declval<T>().*M)>>;
    constexpr auto act_ptr = &input_T::Action;
    using act_mT = underlying_t<input_T, act_ptr>; static_assert(std::is_arithmetic_v<act_mT>, "Currently only arithmetic data types are allowed.");
    using act_gT = double;
    using act_T = act_gT;
    constexpr auto obs_ptr = &output_T::State;
    using obs_mT = underlying_t<output_T, obs_ptr>; static_assert(std::is_arithmetic_v<obs_mT>, "Currently only arithmetic data types are allowed.");
    using obs_gT = double;
    using obs_T = obs_gT;
    
    constexpr auto rew_ptr = &output_T::Reward;
    using rew_T = underlying_t<output_T, rew_ptr>; static_assert(std::is_floating_point_v<rew_T>, "Data type of 'rew' shall be a floating-point type.");
    constexpr auto done_ptr = &output_T::Done;
    using done_T = underlying_t<output_T, done_ptr>; static_assert(std::is_same_v<done_T, bool>, "Data type of 'done' shall be 'bool'.");
    

    using callback_T = std::function<void()>;
    bool invoke_callback(callback_T* callback = nullptr) {
        if (callback->operator bool()) {
            callback->operator()();
            return true;
        }
        return false;
    }

    void param_init(par_T& params, rng_T& rng) {
        
        {
            std::normal_distribution<double> water_level_dist{10.0, 3.0};

do {
    params.h_goal = water_level_dist(rng);
} while (params.h_goal <= 0.0 || params.h_goal >= 20.0);

do {
    params.h_initial = water_level_dist(rng);
} while (params.h_initial <= 0.0 || params.h_initial >= 20.0);

        }
        
    }
}
