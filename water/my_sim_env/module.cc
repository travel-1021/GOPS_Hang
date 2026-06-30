// Pybind11 headers
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
using namespace pybind11::literals;
// check and define macros
#ifndef SLXPY_EXTENSION_NAME
    #error Necessary macro not defined
#endif
#define SLXPY_STRINGIFY(...) #__VA_ARGS__
#define SLXPY_TOSTRING(...) SLXPY_STRINGIFY(__VA_ARGS__)
#ifndef PYBIND11_CPP17
    #error Require CPP 17 compiler
#endif
// User headers
#include "rl_water.h"
// Slxpy headers
#define USE_FMT
#include "slxpy/common.h"
#include "slxpy/bind.h"
#include "slxpy/complex.h"
#include "slxpy/data.h"
#include "slxpy/simulink_builtin.h"
#include "slxpy/env.h"
#include "raw_env.h"
#include "gym_env.h"


using SlxpyExtensionModelClass = rl_water;
using SlxpyPodType_0 = SlxpyExtensionModelClass::ExtU_rl_water_T;static_assert(std::is_trivial_v<SlxpyPodType_0> && std::is_standard_layout_v<SlxpyPodType_0>);
using SlxpyPodType_1 = SlxpyExtensionModelClass::ExtY_rl_water_T;static_assert(std::is_trivial_v<SlxpyPodType_1> && std::is_standard_layout_v<SlxpyPodType_1>);
using SlxpyPodType_2 = SlxpyExtensionModelClass::B_rl_water_T;static_assert(std::is_trivial_v<SlxpyPodType_2> && std::is_standard_layout_v<SlxpyPodType_2>);
using SlxpyPodType_3 = SlxpyExtensionModelClass::X_rl_water_T;static_assert(std::is_trivial_v<SlxpyPodType_3> && std::is_standard_layout_v<SlxpyPodType_3>);
using SlxpyPodType_4 = SlxpyExtensionModelClass::XDis_rl_water_T;static_assert(std::is_trivial_v<SlxpyPodType_4> && std::is_standard_layout_v<SlxpyPodType_4>);
using SlxpyPodType_5 = SlxpyExtensionModelClass::InstP_rl_water_T;static_assert(std::is_trivial_v<SlxpyPodType_5> && std::is_standard_layout_v<SlxpyPodType_5>);
#include <fmt/core.h>

PYBIND11_MODULE(SLXPY_EXTENSION_NAME, m) {
    m.doc() = "Water-tank reinforcement-learning environment adapted from CreateSimulinkEnvironmentAndTrainAgentExample.mlx";
    #ifdef SLXPY_EXTENSION_VERSION
        m.attr("__version__") = pybind11::str(SLXPY_TOSTRING(SLXPY_EXTENSION_VERSION));
    #endif
    #ifdef SLXPY_EXTENSION_AUTHOR
        m.attr("__author__") = pybind11::str(SLXPY_TOSTRING(SLXPY_EXTENSION_AUTHOR));
    #endif
    #ifdef SLXPY_EXTENSION_LICENSE
        m.attr("__license__") = pybind11::str(SLXPY_TOSTRING(SLXPY_EXTENSION_LICENSE));
    #endif
    slxpy_init_simulink(m);
    slxpy_init_complex();

    pybind11::class_<SlxpyExtensionModelClass> rl_water_PB(m, "rl_water", pybind11::module_local());
    pybind11::class_<SlxpyPodType_0> ExtU_rl_water_T_PB(rl_water_PB, "ExtU_rl_water_T");
    pybind11::class_<SlxpyPodType_1> ExtY_rl_water_T_PB(rl_water_PB, "ExtY_rl_water_T");
    pybind11::class_<SlxpyPodType_2> B_rl_water_T_PB(rl_water_PB, "B_rl_water_T");
    pybind11::class_<SlxpyPodType_3> X_rl_water_T_PB(rl_water_PB, "X_rl_water_T");
    pybind11::class_<SlxpyPodType_4> XDis_rl_water_T_PB(rl_water_PB, "XDis_rl_water_T");
    pybind11::class_<SlxpyPodType_5> InstP_rl_water_T_PB(rl_water_PB, "InstP_rl_water_T");
    

    PYBIND11_NUMPY_DTYPE(SlxpyPodType_0, Action);
    PYBIND11_NUMPY_DTYPE(SlxpyPodType_1, State, Reward, Done);
    PYBIND11_NUMPY_DTYPE(SlxpyPodType_2, Sum1, Sum);
    PYBIND11_NUMPY_DTYPE(SlxpyPodType_3, Integrator_CSTATE, H_CSTATE);
    PYBIND11_NUMPY_DTYPE(SlxpyPodType_4, Integrator_CSTATE, H_CSTATE);
    PYBIND11_NUMPY_DTYPE(SlxpyPodType_5, h_goal, h_initial);
    
    ExtU_rl_water_T_PB.attr("dtype") = pybind11::dtype::of<SlxpyPodType_0>();
    ExtU_rl_water_T_PB
        .def(pybind11::init())
        .def("numpy", [](pybind11::object& obj) {
            return pybind11::array_t<SlxpyPodType_0>{ {1}, {}, &obj.cast<SlxpyPodType_0&>(), obj };
        })
        .def("__repr__", [](const SlxpyPodType_0 &self) {
            return fmt::format(R"--(ExtU_rl_water_T(
    Action={}
))--", self.Action);
        })
        .def("__copy__", [](const SlxpyPodType_0 &self) {
            return std::make_unique<SlxpyPodType_0>(self);
        })
        .def("__deepcopy__", [](const SlxpyPodType_0 &self, pybind11::dict) {
            return std::make_unique<SlxpyPodType_0>(self);
        }, "memo"_a);{ BIND_SCALAR_FIELD(ExtU_rl_water_T_PB, SlxpyPodType_0, Action, "Action", ""); }
        ExtY_rl_water_T_PB.attr("dtype") = pybind11::dtype::of<SlxpyPodType_1>();
    ExtY_rl_water_T_PB
        .def(pybind11::init())
        .def("numpy", [](pybind11::object& obj) {
            return pybind11::array_t<SlxpyPodType_1>{ {1}, {}, &obj.cast<SlxpyPodType_1&>(), obj };
        })
        .def("__repr__", [](const SlxpyPodType_1 &self) {
            return fmt::format(R"--(ExtY_rl_water_T(
    State=<Numeric array of shape (3)>,
    Reward={},
    Done={}
))--", self.Reward, self.Done);
        })
        .def("__copy__", [](const SlxpyPodType_1 &self) {
            return std::make_unique<SlxpyPodType_1>(self);
        })
        .def("__deepcopy__", [](const SlxpyPodType_1 &self, pybind11::dict) {
            return std::make_unique<SlxpyPodType_1>(self);
        }, "memo"_a);{ BIND_ARRAY_FIELD(ExtY_rl_water_T_PB, SlxpyPodType_1, State, "State", "", 3); }
        { BIND_SCALAR_FIELD(ExtY_rl_water_T_PB, SlxpyPodType_1, Reward, "Reward", ""); }
        { BIND_SCALAR_FIELD(ExtY_rl_water_T_PB, SlxpyPodType_1, Done, "Done", ""); }
        B_rl_water_T_PB.attr("dtype") = pybind11::dtype::of<SlxpyPodType_2>();
    B_rl_water_T_PB
        .def(pybind11::init())
        .def("numpy", [](pybind11::object& obj) {
            return pybind11::array_t<SlxpyPodType_2>{ {1}, {}, &obj.cast<SlxpyPodType_2&>(), obj };
        })
        .def("__repr__", [](const SlxpyPodType_2 &self) {
            return fmt::format(R"--(B_rl_water_T(
    Sum1={},
    Sum={}
))--", self.Sum1, self.Sum);
        })
        .def("__copy__", [](const SlxpyPodType_2 &self) {
            return std::make_unique<SlxpyPodType_2>(self);
        })
        .def("__deepcopy__", [](const SlxpyPodType_2 &self, pybind11::dict) {
            return std::make_unique<SlxpyPodType_2>(self);
        }, "memo"_a);{ BIND_SCALAR_FIELD(B_rl_water_T_PB, SlxpyPodType_2, Sum1, "Sum1", ""); }
        { BIND_SCALAR_FIELD(B_rl_water_T_PB, SlxpyPodType_2, Sum, "Sum", ""); }
        X_rl_water_T_PB.attr("dtype") = pybind11::dtype::of<SlxpyPodType_3>();
    X_rl_water_T_PB
        .def(pybind11::init())
        .def("numpy", [](pybind11::object& obj) {
            return pybind11::array_t<SlxpyPodType_3>{ {1}, {}, &obj.cast<SlxpyPodType_3&>(), obj };
        })
        .def("__repr__", [](const SlxpyPodType_3 &self) {
            return fmt::format(R"--(X_rl_water_T(
    Integrator_CSTATE={},
    H_CSTATE={}
))--", self.Integrator_CSTATE, self.H_CSTATE);
        })
        .def("__copy__", [](const SlxpyPodType_3 &self) {
            return std::make_unique<SlxpyPodType_3>(self);
        })
        .def("__deepcopy__", [](const SlxpyPodType_3 &self, pybind11::dict) {
            return std::make_unique<SlxpyPodType_3>(self);
        }, "memo"_a);{ BIND_SCALAR_FIELD(X_rl_water_T_PB, SlxpyPodType_3, Integrator_CSTATE, "Integrator_CSTATE", ""); }
        { BIND_SCALAR_FIELD(X_rl_water_T_PB, SlxpyPodType_3, H_CSTATE, "H_CSTATE", ""); }
        XDis_rl_water_T_PB.attr("dtype") = pybind11::dtype::of<SlxpyPodType_4>();
    XDis_rl_water_T_PB
        .def(pybind11::init())
        .def("numpy", [](pybind11::object& obj) {
            return pybind11::array_t<SlxpyPodType_4>{ {1}, {}, &obj.cast<SlxpyPodType_4&>(), obj };
        })
        .def("__repr__", [](const SlxpyPodType_4 &self) {
            return fmt::format(R"--(XDis_rl_water_T(
    Integrator_CSTATE={},
    H_CSTATE={}
))--", self.Integrator_CSTATE, self.H_CSTATE);
        })
        .def("__copy__", [](const SlxpyPodType_4 &self) {
            return std::make_unique<SlxpyPodType_4>(self);
        })
        .def("__deepcopy__", [](const SlxpyPodType_4 &self, pybind11::dict) {
            return std::make_unique<SlxpyPodType_4>(self);
        }, "memo"_a);{ BIND_SCALAR_FIELD(XDis_rl_water_T_PB, SlxpyPodType_4, Integrator_CSTATE, "Integrator_CSTATE", ""); }
        { BIND_SCALAR_FIELD(XDis_rl_water_T_PB, SlxpyPodType_4, H_CSTATE, "H_CSTATE", ""); }
        InstP_rl_water_T_PB.attr("dtype") = pybind11::dtype::of<SlxpyPodType_5>();
    InstP_rl_water_T_PB
        .def(pybind11::init())
        .def("numpy", [](pybind11::object& obj) {
            return pybind11::array_t<SlxpyPodType_5>{ {1}, {}, &obj.cast<SlxpyPodType_5&>(), obj };
        })
        .def("__repr__", [](const SlxpyPodType_5 &self) {
            return fmt::format(R"--(InstP_rl_water_T(
    h_goal={},
    h_initial={}
))--", self.h_goal, self.h_initial);
        })
        .def("__copy__", [](const SlxpyPodType_5 &self) {
            return std::make_unique<SlxpyPodType_5>(self);
        })
        .def("__deepcopy__", [](const SlxpyPodType_5 &self, pybind11::dict) {
            return std::make_unique<SlxpyPodType_5>(self);
        }, "memo"_a);{ BIND_SCALAR_FIELD(InstP_rl_water_T_PB, SlxpyPodType_5, h_goal, "h_goal", ""); }
        { BIND_SCALAR_FIELD(InstP_rl_water_T_PB, SlxpyPodType_5, h_initial, "h_initial", ""); }
        

    

    static_assert(std::is_member_pointer_v<decltype(&SlxpyExtensionModelClass::rl_water_U)>);
    static_assert(std::is_member_pointer_v<decltype(&SlxpyExtensionModelClass::rl_water_Y)>);
    static_assert(std::is_member_pointer_v<decltype(&SlxpyExtensionModelClass::rl_water_B)>);
    static_assert(std::is_member_pointer_v<decltype(&SlxpyExtensionModelClass::rl_water_X)>);
    static_assert(std::is_member_pointer_v<decltype(&SlxpyExtensionModelClass::rl_water_XDis)>);
    static_assert(std::is_member_pointer_v<decltype(&SlxpyExtensionModelClass::rl_water_InstP)>);
    
    rl_water_PB
        .def(pybind11::init())
        .def("__repr__", [](const SlxpyExtensionModelClass &self) {
            return fmt::format(R"--(rl_water(
    rl_water_U=<Model struct: Input data>,
    rl_water_Y=<Model struct: Output data>,
    rl_water_B=<Model struct: Block signals of the system>,
    rl_water_X=<Model struct: Continuous states>,
    rl_water_XDis=<Model struct: Status of an enabled subsystem>,
    rl_water_InstP=<Model struct: Parameter arguments for the system>
))--");
        })
        .def("skip", [](SlxpyExtensionModelClass &self, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                self.step();
            }
        }, "n"_a, "Skip n steps, using current model input")
        ;
        { BIND_METHOD(rl_water_PB, SlxpyExtensionModelClass, step, "step", ""); }
        { BIND_METHOD(rl_water_PB, SlxpyExtensionModelClass, initialize, "initialize", ""); }
        { BIND_METHOD(rl_water_PB, SlxpyExtensionModelClass, terminate, "terminate", ""); }
        { BIND_SCALAR_FIELD(rl_water_PB, SlxpyExtensionModelClass, rl_water_U, "rl_water_U", ""); }
        { BIND_SCALAR_FIELD(rl_water_PB, SlxpyExtensionModelClass, rl_water_Y, "rl_water_Y", ""); }
        { BIND_SCALAR_FIELD(rl_water_PB, SlxpyExtensionModelClass, rl_water_B, "rl_water_B", ""); }
        { BIND_SCALAR_FIELD(rl_water_PB, SlxpyExtensionModelClass, rl_water_X, "rl_water_X", ""); }
        { BIND_SCALAR_FIELD(rl_water_PB, SlxpyExtensionModelClass, rl_water_XDis, "rl_water_XDis", ""); }
        { BIND_SCALAR_FIELD(rl_water_PB, SlxpyExtensionModelClass, rl_water_InstP, "rl_water_InstP", ""); }
        
    rl_water_PB.attr("sample_time") = 1;
    slxpy_bind_raw_env(m);
    bool gym_env_inited = slxpy_bind_gym_env(m);

    if (gym_env_inited) {
        m.attr("__all__") = pybind11::make_tuple(
            "RawEnv","GymEnv","rl_water"
        );
    } else {
        m.attr("__all__") = pybind11::make_tuple(
            "RawEnv","rl_water"
        );
    }
}
