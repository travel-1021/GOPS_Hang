#pragma once
#include <random>
#include <stdexcept>
#include <vector>
#include <array>
#include <limits>

#include "common_env.h"
#include "slxpy/env.h"
#include "slxpy/data.h"

namespace slxpy::env {
using spec::EnvSpec;
using spec::ActionRepeatMode;
class GymEnv {
    raw_T mc{};
    rng_T rng{};
    EnvSpec* env_spec;
    struct {
        size_t steps{ 0 };
        bool init{ false };
        bool truncated{ false };
    } status;
private:
    void step_impl(const act_T act[1], obs_T obs[3], rew_T rew[1], done_T terminated[1], done_T truncated[1]) {
        if (status.init) {
            if (env_spec->strict_reset && (mc.*out_ptr.*done_ptr || status.truncated)) {
                throw std::runtime_error("Calling step after done is illegal.");
            }
            mc.*in_ptr.*act_ptr = *act;
            if (env_spec->action_repeat == 0) {
                // Shortcut for non-repeated actions.
                (mc.*step_ptr)();
                *rew = mc.*out_ptr.*rew_ptr;
            } else {
                if (env_spec->action_repeat_mode != ActionRepeatMode::SUM_BREAK) {
                    throw std::runtime_error("Unsupported action repeat mode.");
                }
                rew_T local_rew{ 0 };
                for (size_t i = 0; i < env_spec->action_repeat; ++i) {
                    (mc.*step_ptr)();
                    local_rew += mc.*out_ptr.*rew_ptr;
                    if (mc.*out_ptr.*done_ptr) { break; }
                }
                *rew = local_rew;
            }
            copy_n_with_coercion(mc.*out_ptr.*obs_ptr, 3, obs);
            status.steps += 1;
            status.truncated = env_spec->max_episode_steps && status.steps >= *env_spec->max_episode_steps;
            *terminated = mc.*out_ptr.*done_ptr;
            *truncated = status.truncated;
        } else {
            throw std::runtime_error("Calling step before reset is illegal.");
        }
    }
    void reset_impl(obs_T obs[3], callback_T* preinit = nullptr, callback_T* postinit = nullptr) {
        if (status.init) {
            // Allowed by
            // https://github.com/cplusplus/draft/blob/7df2b916044b3b47cd708ed1488f1d2fd5f70886/source/basic.tex#L3457-L3513
            static_assert(std::is_trivially_destructible_v<raw_T>);
            new (&mc) raw_T{};
            status.truncated = false;
            status.steps = 0;
        } else {
            status.init = true;
        }
        param_init(mc.*par_ptr, rng);
        invoke_callback(preinit);
        (mc.*init_ptr)();
        if (invoke_callback(postinit)) {
            // If user provides postinit callback, the user is responsible for resetting the environment.
            if (mc.*out_ptr.*done_ptr) {
                throw std::runtime_error("Got done after postinit in reset.");
            }
            copy_n_with_coercion(mc.*out_ptr.*obs_ptr, 3, obs);
            return;
        }
        
        (mc.*step_ptr)();
        if (mc.*out_ptr.*done_ptr) {
            throw std::runtime_error("Got done at first step in reset.");
        }
        copy_n_with_coercion(mc.*out_ptr.*obs_ptr, 3, obs);
    }
public:
    GymEnv(EnvSpec* env_spec): env_spec(env_spec) { seed(); }
    GymEnv(EnvSpec env_spec): GymEnv(new EnvSpec(env_spec)) {}

    // Avoid unintended copy
    GymEnv(const GymEnv&) = delete;
    GymEnv& operator=(const GymEnv&) = delete;
    GymEnv(GymEnv&&) = delete;
    GymEnv& operator=(GymEnv&&) = delete;

    step_T step(npa_T<act_T> act) {
        auto input_ndim = act.ndim();
        auto input_shape = act.shape();
        constexpr std::array<pybind11::ssize_t, 1> act_shape{ 1 };
        if (size_not_equal(input_ndim, act_shape.size()) || !std::equal(input_shape, input_shape + input_ndim, act_shape.begin())) {
            throw std::runtime_error("Action array should have dimension (1).");
        }
        act_T* act_buf = static_cast<act_T*>(act.request(false).ptr);
        npa_T<obs_T> obs{ 3 };
        obs_T* obs_buf = static_cast<obs_T*>(obs.request(true).ptr);
        rew_T rew;
        done_T terminated, truncated;
        step_impl(act_buf, obs_buf, &rew, &terminated, &truncated);

        pybind11::dict info;
        return pybind11::make_tuple(obs, rew, terminated, truncated, info);
    }
    reset_T reset(std::optional<uint32_t> s, std::optional<pybind11::dict> options, callback_T* preinit = nullptr, callback_T* postinit = nullptr) {
        if (s) {
            seed(*s);
        }
        npa_T<obs_T> obs{ 3 };
        obs_T* obs_buf = static_cast<obs_T*>(obs.request(true).ptr);
        reset_impl(obs_buf, preinit, postinit);
        pybind11::dict info;
        return pybind11::make_tuple(obs, info);
    }
    void seed() {
        std::random_device rd;
        seed(rd());
    }
    void seed(uint32_t s) {
        rng.seed(s);
    }
    std::string repr() {
        return fmt::format("<GymEnv wrapping underlying rl_water <{}>>", env_spec->id);
    }
    void close() { /* Currently no-op */ }
    const raw_T& model_class() { return mc; }
    const EnvSpec& spec() { return *env_spec; }
    ~GymEnv() {
        delete env_spec;
    }
    
};



bool slxpy_bind_gym_env(::pybind11::module_& m) {
    try {
        pybind11::module::import("gymnasium");
    } catch(pybind11::error_already_set& e) {
        if (!e.matches(PyExc_ModuleNotFoundError)) { throw; }
        // 7 stacklevels to skip importlib, but currently not used
        PyErr_WarnEx(nullptr, "You must install gymnasium to use Slxpy GymEnv. For now, GymEnv is not initialized.", 1);
        // NOTE: Below rethrow code leads to intimidating error message, so currently not used.
        // pybind11::raise_from(e, PyExc_ModuleNotFoundError, "You must install gymnasium to use Slxpy GymEnv.");
        // throw pybind11::error_already_set();
        return false;
    }

    auto s = m.def_submodule("_env", "Define env-related bindings.");
    slxpy_bind_spec(s);

    auto action_space = slxpy::env::gym_spaces::make_box<double>(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), { 1 });
    auto observation_space = slxpy::env::gym_spaces::make_box<double>({ -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), 0.0 }, { std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() }, { 3 });
    auto reward_range = pybind11::make_tuple(-101.0, 10.0);

    // Placeholder for metadata field
    pybind11::dict metadata;
    metadata["render.modes"] = pybind11::list();

    pybind11::class_<GymEnv> GymEnv_PB(m, "GymEnv", pybind11::module_local());
    GymEnv_PB
        .def(pybind11::init([]() -> std::unique_ptr<GymEnv> {
            return std::make_unique<GymEnv>(new EnvSpec{ "Rl_water-v0" });
        }))
        .def(pybind11::init<EnvSpec>(), "spec"_a)
        .def("step", &GymEnv::step, "", "action"_a.noconvert())
        .def("reset", &GymEnv::reset, pybind11::kw_only(), "seed"_a=nullptr, "options"_a=nullptr, "preinit"_a=nullptr, "postinit"_a=nullptr)
        .def("render", [](GymEnv& self) { PyErr_SetNone(PyExc_NotImplementedError); throw pybind11::error_already_set(); }, "")
        .def("close", &GymEnv::close, "")
        .def("__repr__", &GymEnv::repr, "")
        .def("__enter__", [](pybind11::object& self) { return self; }, "")
        .def("__exit__", [](GymEnv& self, pybind11::object& exc_type, pybind11::object& exc_value, pybind11::object& traceback) { self.close(); return false; }, "")
        .def_property_readonly("model_class", &GymEnv::model_class)
        .def_property_readonly("spec", &GymEnv::spec)
        .def_property_readonly("unwrapped", [](pybind11::object& self) { return self; });
    GymEnv_PB.attr("metadata") = metadata;
    GymEnv_PB.attr("action_space") = action_space;
    GymEnv_PB.attr("observation_space") = observation_space;
    GymEnv_PB.attr("reward_range") = reward_range;
    

    return true;
}
}

using slxpy::env::slxpy_bind_gym_env;
