#pragma once
#include <random>
#include <stdexcept>
#include <vector>
#include <array>
#include <memory>

#include "common_env.h"

namespace slxpy::env {
class RawEnv {
    raw_T mc{};
    rng_T rng{};
    bool init{ false };
private:
    void step_impl(const input_T* input, output_T* output) {
        if (init) {
            mc.*in_ptr = *input;
            (mc.*step_ptr)();
            *output = mc.*out_ptr;
        } else {
            throw std::runtime_error("Calling step before reset is illegal.");
        }
    }
    void reset_impl(output_T* output) {
        if (init) {
            // Allowed by
            // https://github.com/cplusplus/draft/blob/7df2b916044b3b47cd708ed1488f1d2fd5f70886/source/basic.tex#L3457-L3513
            static_assert(std::is_trivially_destructible_v<raw_T>);
            new (&mc) raw_T{};
        } else {
            init = true;
        }
        param_init(mc.*par_ptr, rng);
        (mc.*init_ptr)();
        
        (mc.*step_ptr)();
        *output = mc.*out_ptr;
    }
public:
    RawEnv() { seed(); }
    std::unique_ptr<output_T> step(input_T* input) {
        auto output = std::make_unique<output_T>();
        step_impl(input, output.get());
        return output;
    }
    std::unique_ptr<output_T> reset() {
        auto output = std::make_unique<output_T>();
        reset_impl(output.get());
        return output;
    }
    std::vector<uint32_t> seed() {
        std::random_device rd;
        auto s = rd();
        return seed(s);
    }
    std::vector<uint32_t> seed(uint32_t s) {
        rng.seed(s);
        return { s };
    }
    raw_T& model_class() { return mc; }
    
};



void slxpy_bind_raw_env(::pybind11::module_& m) {
    pybind11::class_<RawEnv> RawEnv_PB(m, "RawEnv", pybind11::module_local());
    RawEnv_PB
        .def(pybind11::init())
        .def("step", &RawEnv::step, "", "action"_a.noconvert())
        .def("reset", &RawEnv::reset, "")
        .def("seed", pybind11::overload_cast<>(&RawEnv::seed), "")
        .def("seed", pybind11::overload_cast<uint32_t>(&RawEnv::seed), "", "seed"_a)
        .def_property_readonly("model_class", &RawEnv::model_class);
    
}
}

using slxpy::env::slxpy_bind_raw_env;
