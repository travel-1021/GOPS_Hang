# GOPS + Simulink 环境配置、训练与回灌顺序操作手册

> 本文先说明可复用于其他控制对象的通用流程，再在每个环节给出当前 DDPG 水箱实例。
> 当前工程：`E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang`
> 平台：Windows、PowerShell、Conda、MATLAB/Simulink
> 原则：严格按顺序执行；当前环节未验收，不进入下一环节。

## 0. 先理解完整流程

GOPS 与 Simulink 的完整闭环包含两条方向相反的转换链：

```text训练环境生成：
路线一、首先
Simulink 被控对象
→ Embedded Coder 生成 C++
→ slxpy 生成绑定并编译 Python 扩展
→ 注册为 GOPS 环境
→ 强化学习训练

路线二、反向加载
策略回灌：
GOPS 训练检查点
→ Py2slxRunner 导出 TorchScript
→ Level-2 MATLAB S-Function 加载策略
→ Simulink 闭环验证
```

从一台新电脑开始，按以下顺序执行：

```text
第 1 环节  拉取并安装 GOPS
第 2 环节  安装 MATLAB/Simulink，并按【通用接口要求设计】强化学习模型
第 3 环节  首次安装 slxpy Python，并创建、配置 slxpy 工程——（一些方便后续生成 C++ 代码的配置文件，在明确模型的基础上，可由 AI 完善）
第 4 环节  首次安装 slxpy MATLAB Toolbox 和 Coder 组件，并从 Simulink 生成 C++ 模型代码
第 5 环节  首次配置 C++ 编译器，生成绑定、编译并测试 Python 扩展——（生成 .pyd 文件，但最好由 AI 测试）
第 6 环节  将扩展注册成 GOPS 环境——（撰写 GOPS 和 .pyd 运行文件之间的接口代码等）
第 7 环节  对 GOPS 环境做 reset/step 验收——（测试效果，可由 AI 完成）
第 8 环节  配置算法，先冒烟训练，再正式训练——（可由 AI 完成，人工调参或者给定稳定要求由 AI 协助调参）
第 9 环节  选择检查点并导出 TorchScript 策略——（撰写导出脚本，可由 AI 完成）
第 10 环节  从正确的 Python 环境启动 MATLAB——（在对应且受支持的 Python 环境下启动）
第 11 环节  在 Simulink 中接入策略并分层验证——（选用模块加载训练好的策略）
```

文中使用以下占位符：

| 占位符              | 含义                                    | 当前水箱实例                                         |
| ------------------- | --------------------------------------- | ---------------------------------------------------- |
| `<GOPS_ROOT>`     | GOPS 仓库根目录                         | `E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang` |
| `<SLXPY_PROJECT>` | 单个 Simulink 模型的 ==slxpy== 工程目录 | `<GOPS_ROOT>\water\my_sim_env`                     |
| `<MODEL_NAME>`    | 不带`.slx` 的模型名                   | `rl_water`                                         |
| `<ENV_ID>`        | GOPS 创建环境时使用的 ID                | `simu_rl_water`                                    |
| `<RESULT_DIR>`    | 某次训练的完整结果目录                  | `results\simu_rl_water\DDPG_260630-063942`         |
| `<CHECKPOINT_ID>` | 不带前后缀的检查点标识                  | `30500_opt`                                        |

---

## 第 1 环节：拉取并安装 GOPS

### 1.1 通用目的和要求

本环节获得 GOPS 源代码，创建【独立 Conda 环境】，并以可编辑方式安装 GOPS。

官方基本要求：

- Windows 7 或更新版本，或 Ubuntu 18.04 及更新版本；
- GOPS 本身支持 Python 3.6 以上；如果要接入 MATLAB/Simulink，使用 Python 3.8；
- GOPS 安装路径使用英文，不包含空格、中文或其他特殊字符；
- 已安装 Git、Conda 或 Miniconda。

### 1.2 从零安装 GOPS

在计划存放项目的父目录打开 PowerShell：

```powershell
# 1. 拉取官方仓库；第二个参数决定本地目录名称
git clone https://github.com/Intelligent-Driving-Laboratory/GOPS.git GOPS_Hang

# 2. 进入仓库根目录
Set-Location '.\GOPS_Hang'

# 3. Windows 使用仓库中的 Windows 环境文件
# -n gops_env 将环境名称明确设为本项目当前使用的名称
conda env create -f gops_environment.win.yml -n gops_env
（创建环境）

# 4. 激活环境
conda activate gops_env

# 5. 以 editable 模式安装当前仓库
python -m pip install -e .
————
python -m pip：使用当前 Python 环境里的 pip。
install：安装包。
-e：editable，可编辑模式。
.：安装当前目录中由 setup.py 或 pyproject.toml 描述的项目
————
【（形成的是 GOPS Python 库的安装链接，不是创建新的 Python 环境。
安装后，在当前 Conda/虚拟环境中可以直接直接导入 GOPS 库。
   gops_env（Conda/Python 环境）
   └── 注册了 GOPS 包
      └── 指向当前 GOPS_Hang 仓库源码
）】

-执行后，你可以在仓库中的训练脚本里直接写：from gops.create_pkg.create_env import create_env；而不用手动配置 PYTHONPATH，也不必把脚本放进 gops 目录。这意味着：
   无论训练脚本放在：
      -GOPS_Hang/example_train/
      -D:/my_experiment/
      -其他任意目录/
   只要使用的是安装了 GOPS 的那个 Python 环境，通常都能执行：import gops
   如果没有安装，Python 通常只会在以下位置寻找：
      -当前运行目录
      -脚本所在目录
      -Python 环境的 site-packages
      -PYTHONPATH 指定的目录

gops_env 是环境，gops 是安装在其中的库
```


Linux 应将环境文件换成：

```bash
conda env create -f gops_environment.nix.yml
```

### 1.3 当前水箱工程怎么做

当前目录已经是拉取并修改过的 GOPS 工程：

```text
E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang
```

因此不要在该目录内部再次执行 `git clone`。只需确认 `gops_env` 且可以 `import` 即可，并重新执行可编辑安装，以保证 Python 导入当前代码：

```powershell
Set-Location 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang'
conda activate gops_env
python -m pip install -e .
```

### 1.4 验收

```powershell
& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -c "import sys, gops; 
print(sys.executable); 
print(gops.__file__)"
```

必须满足：

- 解释器指向 `gops_env`；
- `gops.__file__` 指向 `<GOPS_ROOT>\gops`，而不是电脑上的另一份 GOPS；
- 仓库根目录中存在 `gops`、`example_train`、`example_run` 和 `setup.py`。

参考：[GOPS 官方安装说明](https://gops.readthedocs.io/en/latest/introduction.html)。

---

## 第 2 环节：安装 MATLAB/Simulink，并按通用接口要求设计强化学习模型

### 2.1 首次使用：安装并检查 MATLAB/Simulink

本环节第一次实际使用 MATLAB 和 Simulink，因此在这里完成安装。建模至少需要 MATLAB 和 Simulink；后续代码生成和策略回灌所需组件将在第一次使用它们的环节再安装。

通用版本要求：

- 建模和从 Simulink 生成代码：MATLAB 最低 R2018a，推荐 R2021a 以上；
- 使用 `gops_validation_bridge.m` 的 `pyrun` 回灌策略：MATLAB 最低 R2021b；
- MATLAB 版本必须与后续使用的 Python 版本兼容；本流程统一使用 64 位 Python 3.8。

从 MathWorks 安装器中安装 MATLAB 和 Simulink。安装完成后启动 MATLAB，执行：

```matlab
ver
simulink
```

`ver` 应列出 MATLAB 和 Simulink，`simulink` 应能打开 Simulink Start Page。当前水箱模型位于：

```text
water\my_sim_env\rl_water.slx
```

### 2.2 通用的 Gym-like 模型接口

```powershell
---补充说明---
“Gym-like 模型接口”指的是：让 Simulink 模型表现得像一个强化学习环境。
模仿 OpenAI Gym 常见的交互方式
   -bservation = env.reset()
   -observation, reward, done, info = env.step(action)
对应到 Simulink 模型，就是：
   -智能体输出 action
   -Simulink 模型运行一步
   -返回 observation、reward、done、info
之所以叫 Gym-like 而不是严格的 Gym 接口，是因为 Simulink 本身不是 Python Gym 环境。它只是按照类似 Gym 的输入输出结构设计，之后由 slxpy 包装成真正可以 reset() 和 step() 的 Python 环境
```

任意 Simulink 控制对象接入 ==slxpy== 时，模型根级接口应满足下表。

| 信号        | 通用要求                                                    | 默认位置              |
| ----------- | ----------------------------------------------------------- | --------------------- |
| action      | 一个`double` 标量或定长数组输入；推荐根级只有一个输入端口 | 第 1 个 Inport        |
| observation | 一个`double` 标量或定长数组输出                           | 第 1 个 Outport       |
| reward      | 一个标量`double` 输出                                     | 第 2 个 Outport       |
| done        | 一个标量`logical` 输出                                    | 第 3 个 Outport       |
| info        | 可选的额外`double` 输出                                   | 第 4 个及后续 Outport |

重要说明：

1. `State`、`Reward`、`Done` 是当前水箱使用的端口名称，并不是所有模型必须使用的名字。
2. 如果 `env.toml` 不设置 key，==slxpy== 默认按端口位置取 action、observation、reward 和 done。
3. 如果在 `env.toml` 设置 `action_key`、`observation_key`、`reward_key`、`done_key`，则按指定端口名映射；即便如此，也建议保留默认顺序，便于检查和迁移。
4. reward 必须是标量 `double`，done 必须是标量 `logical`，不能把 done 直接设计成向量。
5. action 和 observation 必须是固定维度；Embedded Coder C++ 接口和 ==slxpy== 不适合可变维信号。
6. 推荐只有一个根级动作输入。额外输入在默认 Gym 包装中可能只会得到零输入，通常没有意义。
7. 如果需要额外诊断量，可作为 `double` Outport，并通过 `info` 返回。

### 2.3 通用的模型结构要求

- 如果原模型包含控制器，将被控对象整理为 `Plant` 子系统，并让根级 action 驱动 Plant。
- observation 的预处理、reward 计算和 done 判断可放在根级，形成完整环境。
- 使用【固定步长求解器】；变量步长不支持 Embedded Coder 目标代码生成。
- 【避免代数环】；必要时使用 Unit Delay、Memory 或显式迭代方法处理。
- 【避免可变维输入输出、字符串、固定点、事件系统和函数调用系统】。
- 非内联 S-Function 通常需要 `.tlc`；可以时优先使用支持代码生成的 MATLAB Function。
- 【固定步长模型生成代码后，必须对比普通 Simulink 仿真与生成代码的数值结果】。

### 2.4 通用的 reset 随机化要求

】=
程序本身是确定性的。若每回合需要改变初始状态、目标值或物理参数，【应把这些量设置为可调参数】：

1. 在 Model Workspace 中创建 `Simulink.Parameter`；
2. 勾选参数的 `Argument`；
3. 在 `env.toml` 的 `[parameter]` 中配置常数、均匀分布、种子或自定义 C++ 初始化代码。

只有出现在生成模型可调参数中的名称，才能在每次 reset 时赋新值。

### 2.5 当前水箱实例

当前模型：

```text
water\my_sim_env\rl_water.slx
```

水箱端口和语义：

| 通用角色    | 水箱端口名 | 类型和维度      | 水箱中的定义                       |
| ----------- | ---------- | --------------- | ---------------------------------- |
| action      | `Action` | 1 维`double`  | 物理流量命令                       |
| observation | `State`  | 3 维`double`  | `[积分误差, 跟踪误差, 实际水位]` |
| reward      | `Reward` | 标量`double`  | 达到误差带为 10，否则为 -1         |
| done        | `Done`   | 标量`logical` | 水位越界时为真                     |

`State` 三个元素的固定顺序为：

```text
State[0] = 积分跟踪误差，Simulink 中限幅为 [-10, 10]
State[1] = 目标水位 - 实际水位
State[2] = 实际水位
```

其他约定：

```text
动力学：dh/dt = 0.25 × Action - 0.1 × sqrt(h)
动作安全范围：[-100, 100]
abs(跟踪误差) < 0.1：Reward = 10
其他正常状态：Reward = -1
水位不在 (0,20)：额外 -100，并令 Done = true
每回合最多 200 步
```

`h_goal` 和 `h_initial` 是 Model Workspace 中可调的 `Simulink.Parameter`，每回合从 `N(10,3²)` 采样并截断到 `(0,20)`。

### 2.6 验收

在普通 Simulink 仿真中使用固定初值和固定动作，确认：

- 每个端口类型、维度和顺序正确；
- 状态转移方向符合物理规律；
- reward 与 done 在边界处正确切换；
- 无 NaN/Inf，`sqrt` 等函数的输入不会越过定义域；
- 固定步长模型能够正常更新和仿真。
- MATLAB 能正常打开并仿真当前 `.slx` 模型。

参考：[==slxpy== Gym-like 模型要求](https://gops.readthedocs.io/en/latest/slx2py.html#gymlike-environment)。

---

## 第 3 环节：首次安装 ==slxpy== Python，并创建、配置 ==slxpy== 工程

### 3.1 首次使用：创建 ==slxpy== 专用环境

```
（本质上和gops_env是一个东西，其实都是环境而已）
```

本环节第一次使用 `slxpy` 命令，因此在这里安装 ==slxpy== Python 包。它负责初始化工程，并在后续根据模型代码和 TOML 配置生成 Python 绑定资产。

从零配置可执行：

```powershell
conda create -n slxpy python=3.8 -y
使用 Conda 创建一个名为 slxpy 的独立虚拟 Python 环境，该环境绑定 Python3.8，安装过程全部自动确认无需人工交互

conda activate slxpy
激活 slxpy 环境，确保后续命令在该环境中执行。

python -m pip install "slxpy[gym]"
安装 slxpy Python 包，包含 Gym-like 模型接口。
```

当前工程分别使用：

```text
slxpy 构建环境：C:\Users\admin\miniconda3\envs\slxpy
GOPS 训练环境： C:\Users\admin\miniconda3\envs\gops_env
```

本环节只详细安装 ==slxpy== 环境；`gops_env` 已在第 1 环节安装。验证 ==slxpy== 环境：

```powershell
& 'C:\Users\admin\miniconda3\envs\slxpy\python.exe' --version
& 'C:\Users\admin\miniconda3\envs\slxpy\python.exe' -c "import slxpy, setuptools, pybind11, numpy; print('slxpy environment OK')"
& 'C:\Users\admin\miniconda3\envs\slxpy\Scripts\slxpy.exe' --help
```

### 3.2 通用流程

每个 Simulink 模型使用一个独立 ==slxpy== 工程目录。新模型可执行：【就是每个 Simulink 模型使用一个新的==slxpy== 工程目录（新的文件夹），主要是存放simulink模型转换过程中的文件】

```powershell
conda activate slxpy

New-Item -ItemType Directory '<SLXPY_PROJECT>'    【创建文件夹，<SLXPY_PROJECT> 是占位符，代表你的项目路径
Set-Location '<SLXPY_PROJECT>'    【等价于 CMD 的 cd

slxpy init  【一键生成项目模板、配置文件、目录结构、依赖文件等】完成下文的初始化时填写，就可得到model.toml、env.toml等文件，但这些文件需要根据模型后续重新进行配置（可由AI完成）
```

初始化时填写：

```text
Simulink model name：不带 .slx 的模型名
Code generation C++ class name：合法的 C++ 标识符
Code generation C++ namespace：简单项目可留空
```

然后把 `.slx` 放入工程目录（这个是 ‘初始化时填写’ 完成后后放进来的），并配置：

```text
<SLXPY_PROJECT>\
├─ <MODEL_NAME>.slx
├─ model.toml
└─ env.toml
```

已有工程不应再次运行 `slxpy init` 覆盖配置。

### 3.3 `model.toml` 的通用职责

`model.toml` 描述：

- Simulink 模型名；
- 固定步长求解器；
- 是否使用连续状态、绝对时间、非有限数和复数；
- 是否存在可变维信号或非内联 S-Function；
- 生成的 C++ 类名和命名空间。

这些值必须反映模型真实特性。例如模型有连续 Integrator 时，`continuous_time` 应为 `true`。

### 3.4 `env.toml` 的通用职责

`env.toml` 描述：

- 是否生成 Raw、Gym 和向量化环境；
- Simulink 端口到 action、observation、reward、done、info 的映射；
- action space、observation space 和 reward range；
- reset 时如何取得初始 observation；
- 每回合如何初始化可调参数。

通用映射示例：

```toml
[gym]
action_key = "模型中的动作端口名"
observation_key = "模型中的观测端口名"
reward_key = "模型中的奖励端口名"
done_key = "模型中的终止端口名"
info = false
type_coercion = false
reward_range = ["-inf", "inf"]

[gym.action_space]
type = "Box"
low = -1.0
high = 1.0
shape = [动作维数]
dtype = "float64"

[gym.observation_space]
type = "Box"
low = "-inf"
high = "inf"
shape = [观测维数]
dtype = "float64"
```

`type_coercion=true` 可以对 action 和 observation 做隐式类型转换，但模型和 Python 侧最好本身就使用一致 dtype。

### 3.5 当前水箱实例

水箱工程已经存在，不再执行 `slxpy init`：

```text
water\my_sim_env\
├─ rl_water.slx
├─ model.toml
└─ env.toml
```

`model.toml` 关键项：

```toml
model = "rl_water"

[simulink]
solver = "FixedStepAuto"
non_finite = true
continuous_time = true
variable_size_signal = false
non_inlined_sfcn = false

[cpp]
class_name = "rl_water"
namespace = ""
```

`env.toml` 关键项：

```toml
use_raw = true
use_gym = true
use_rng = true
use_vec = false
vec_parallel = false

[gym]
action_key = "Action"
observation_key = "State"
reward_key = "Reward"
done_key = "Done"
info = false
type_coercion = true
reward_range = [-101.0, 10.0]
```

水箱动作空间为 1 维 `float64`，观测空间为 3 维 `float64`；`[reset] first_step=true`；`[parameter.h_goal]` 的自定义代码同时初始化 `h_goal` 和 `h_initial`。

### 3.6 验收

- `.slx`、`model.toml`、`env.toml` 位于同一工程目录；
- 模型名、C++ 类名、端口名大小写一致；
- action/observation 的 shape 与模型真实端口一致；
- `[parameter]` 中的名称都是模型可调参数；
- reward range 覆盖模型所有可能奖励。
- ==slxpy== 专用环境能启动，`slxpy`、`setuptools`、`pybind11` 和 `numpy` 均可导入。

---


### 4.1 首次使用## 第 4 环节：首次安装 ==slxpy== MATLAB Toolbox 和 Coder 组件，并生成 C++ 模型代码

：安装 MATLAB 侧工具和代码生成组件

本环节第一次在 MATLAB 中调用 `slxpy.setup_config` 和 `slxpy.codegen`，因此在这里安装：

- ==slxpy== 提供的 MATLAB Toolbox；
- Simulink Coder；
- Embedded Coder；
- MATLAB Coder（按当前 ==slxpy== 工具链要求安装）。

```
【给 Simulink 装代码生成工具链、配置模型导出规则、把 slx 仿真模型编译成可被 Python 调用的 C++ 底层库。】

slxpy MATLAB Toolbox
slxpy 官方给 MATLAB 写的插件包，提供 slxpy.setup_config / slxpy.codegen 这两个核心.m 函数，没有这个工具箱后面两行命令直接报错找不到函数。

Simulink Coder
基础代码生成工具：把.slx 框图模型翻译成 C/C++ 源码，是导出代码的基础模块

Embedded Coder
嵌入式专用代码生成：生成轻量化、无动态内存、可外部调用的标准接口代码，Python 要调用仿真模型必须靠它输出规范 API。

MATLAB Coder
若模型里手写了 MATLAB Function 脚本，负责把.m 函数同步转成 C++，补齐完整代码生成能力
```

使用 MathWorks 安装器为当前 MATLAB 添加上述 Coder 产品，并使用 ==slxpy== 提供的 MATLAB Toolbox 安装包安装 MATLAB 侧工具。安装后在 MATLAB 中验证：

```matlab
ver
which slxpy.setup_config
which slxpy.codegen
```

`ver` 应列出所需 Coder 产品，两个 `which` 命令应返回实际 `.m` 文件路径。这里安装的是 MATLAB 侧代码生成工具；用于编译 Python `.pyd` 的 MSVC 将在第 5 环节第一次使用时配置。

### 4.2 通用要求

使用 MATLAB 和 Embedded Coder 完成本步。【以下情况必须重新 codegen：】

- `.slx` 动力学、端口、参数或状态变化；
- 求解器或采样周期变化；
- `model.toml` 的代码生成配置变化。

只改变显示布局通常不需要重新生成。

### 4.3 通用操作

在 MATLAB 命令窗口执行：

```matlab
workdir = '<SLXPY_PROJECT 的绝对路径>';   【<SLXPY_PROJECT> 是占位符，代表你的项目路径】
slxpy.setup_config(workdir);  % 首次运行或 model.toml 改变后执行
slxpy.codegen(workdir);       % 模型改变后执行
```

如果 MATLAB 提示模型配置为 C++ 但当前使用 C-only 编译器：

1. 在 Code Generation 中勾选 `Generate code only`；
2. 选择 C++ 工具链；
3. 重新执行 codegen。

### 4.4 当前水箱实例

```matlab
workdir = 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\water\my_sim_env';
slxpy.setup_config(workdir);
slxpy.codegen(workdir);
```

### 4.5 验收

- MATLAB 没有端口、可调参数或不支持模块错误；
- MATLAB 能找到 `slxpy.setup_config` 和 `slxpy.codegen`，并已安装所需 Coder 产品；
- `<SLXPY_PROJECT>\model\` 中的 C/C++ 文件已经刷新；
- 不允许在修改 `.slx` 后继续使用旧 `model\*.cpp`。

---

## 第 5 环节：首次配置 C++ 编译器，生成绑定、编译并测试 Python 扩展

### 5.1 首次使用：安装并检查 C++ 编译器

本环节第一次真正编译 Python 原生扩展，因此在这里安装和检查 C++ 编译器。Windows 使用支持 C++17（好像是C++14，看运行提示吧） 的 Visual Studio/MSVC；推荐 Visual Studio 2019 16.11 或更新版本，避免使用存在已知问题的 16.7。

通过 Visual Studio Installer 安装“使用 C++ 的桌面开发”工作负载，并确保包含 MSVC、Windows SDK 和 C++ 构建工具。随后在能够发现 MSVC 的终端中执行本环节的 `python setup.py build`。

这里编译的是 Python `.pyd`，主要由 Python/setuptools 寻找 MSVC；`mex -setup C++` 只配置 MATLAB 的 MEX 编译器，不能代替本步骤的实际 `.pyd` 编译验证。

为了避免 ABI 混乱，编译 `.pyd` 与运行 `.pyd` 的 Python 应使用相同主次版本和位数。例如 `cp38-win_amd64` 表示 64 位 CPython 3.8。当前水箱扩展为 `rl_water.cp38-win_amd64.pyd`，因此 `slxpy` 构建环境与 `gops_env` 都必须兼容 CPython 3.8/64 位。

检查两个解释器：

```powershell
& 'C:\Users\admin\miniconda3\envs\slxpy\python.exe' --version
& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' --version
```

### 5.2 通用顺序

这一步必须按以下次序执行：

```text
slxpy generate
→ python setup.py build
→ 从 build/lib<平台后缀> 导入扩展
→ 创建模型类、RawEnv 和 GymEnv
→ reset/step 测试
```

`slxpy generate` 根据模型代码和两个 TOML 生成 `module.cc`、`setup.py`、包装器头文件等资产。不能把新模型代码与旧绑定文件混合编译。

### 5.3 通用操作模板

```powershell
Set-Location '<SLXPY_PROJECT>' 【进入你的 slxpy 项目根目录】

conda activate slxpy   【激活 slxpy 环境】

slxpy generate    【生成绑定文件】
   -承接上一步 MATLAB 的slxpy.codegen生成的 C++ 源码，这条是Python 侧绑定代码自动生成，核心做三件事：
      -读取 MATLAB 导出的 Simulink C++ 仿真源码、接口头文件；
      -自动生成 Python-C++ 交互胶水代码（Cython/Pybind11 封装层）；
      -生成编译配置文件 setup.py、编译参数、头文件路径、库链接规则；
   -前提：必须先在 MATLAB 执行完 slxpy.setup_config + slxpy.codegen，有完整 C++ 源码才能正常 generate。

python setup.py build
   -调用 Python setuptools 编译工具，执行C++ 源码 + 胶水代码编译，生成 `.pyd` 扩展。
      -调用 MSVC 编译器（第 5 环节配置的 VS 编译工具链）；
      -把 Simulink 导出的动力学 C++ 代码 + slxpy 生成的绑定代码，一起编译成 Windows 下 Python 可直接导入的 .pyd 动态库；
      -输出编译产物到 build/ 文件夹；
      -编译完成后，你在 Python 代码里就能 import 这个 Simulink 仿真模型，作为强化学习环境和 GOPS 算法交互。
```

编译产物通常位于：

```text
<SLXPY_PROJECT>\build\lib.<平台-Python后缀>\<扩展名>.pyd
```

测试时应覆盖：

- 模型 C++ 类能构造、initialize 和 step；
- RawEnv 能 reset/step；   【是 ==slxpy== generate 这一步提前自动生成的 Python 封装源码文件，包含 reset 和 step 方法】
- GymEnv 能 reset/step；   【==slxpy== generate 这一步提前自动生成的 Python 封装源码文件，GymEnv 是 OpenAI Gym 环境的 Python 封装，包含 reset 和 step 方法】
- action 的 shape/dtype 正确；
- 返回的 observation、reward、done 和 info 类型正确；
- 多次 step、终止及终止后的 reset 正常；
- 无崩溃、NaN/Inf 和明显内存泄漏。

```
RawEnv
轻量化原生封装，直接对接 .pyd 里的 C++ 模型接口，只提供最基础的 initialize、step、模型参数读写，无 Gym 标准化接口。
GymEnv
在 RawEnv 上层再封装一层，对齐强化学习通用 Gym 规范：
自带 action_space / observation_space
标准化 reset()、step(action) 返回 (obs, reward, terminated, truncated, info)
你的 GOPS、Transformer 训练代码直接用 GymEnv，兼容主流 RL 算法库。



1. rl_water.cp38-win_amd64.pyd 是什么？
它只是最底层的 C++ 二进制仿真内核，由 module.cc + Simulink 生成的动力学代码编译而来：
只暴露极简的 C++ 原生接口：模型初始化、单步仿真、读写状态 / 参数；
没有强化学习标准接口，没有观测 / 动作空间定义，没有奖励、终止信号封装；
你如果直接 import rl_water，只能调用一堆底层裸函数，写 RL 训练会极度繁琐。

2. RawEnv 是 .pyd 的第一层 Python 封装（必须存在）
slxpy generate 自动生成的 raw_env.py 内部导入了你这个 rl_water.pyd，做了一层轻量化封装：
统一管理模型实例生命周期；
自动处理数组类型、维度转换、内存拷贝；
封装底层异常捕获（仿真 NaN、崩溃报错提示）；
提供批量读取内部 Simulink 状态、修改模型参数的接口。
GymEnv 的内部逻辑完全依赖 RawEnv，代码结构类似：

3. GymEnv 是给你 GOPS 训练用的标准 RL 外壳（你代码直接调用）
它在 RawEnv 之上，补齐 Gymnasium 标准强化学习接口：
定义 action_space / observation_space（算法采样、回放池依赖）；
标准化 step() 返回：(obs, reward, terminated, truncated, info)；
内置自动重置、边界截断、奖励计算逻辑；
兼容你现有的 Transformer、MPC、GOPS 训练框架。
```

### 5.4 当前水箱实例

生成并编译：

```powershell
Set-Location 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\water\my_sim_env'

& 'C:\Users\admin\miniconda3\envs\slxpy\Scripts\slxpy.exe' generate
& 'C:\Users\admin\miniconda3\envs\slxpy\python.exe' setup.py build
```

预期产物：

```text
water\my_sim_env\build\lib.win-amd64-cpython-38\rl_water.cp38-win_amd64.pyd
```

使用真正运行 GOPS 的 `gops_env` 测试 ABI 和 step：

```powershell
$env:PYTHONPATH = 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\water\my_sim_env\build\lib.win-amd64-cpython-38'

& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\water\my_sim_env\test_extension.py'

& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -c "import numpy as np, rl_water; e=rl_water.GymEnv(); print('reset=',e.reset(seed=0)); print('step=',e.step(np.array([0.0],dtype=np.float64))); del e"

Remove-Item Env:PYTHONPATH
```

当前 `test_extension.py` 中 step 仍被注释，所以必须执行后面的单步命令，不能只看测试脚本无报错。

### 5.5 验收

- `.pyd` 能被目标 Python 导入；
- reset 返回 3 维水箱 observation；
- step 返回 observation、reward、终止标志和 info；
- 结果中没有崩溃、NaN/Inf。
- 编译日志确认使用支持 C++17 的 MSVC，且生成扩展的 Python ABI 与 `gops_env` 一致。

---

## 第 6 环节：将扩展注册成 GOPS 环境  【可由AI辅助来写】

```
前面 4、5 环节只是单独生成了一套能跑的 Simulink 仿真.pyd+GymEnv，这套仿真还不能被 GOPS 算法直接识别调用；第 6 环节就是做一层适配、文件归档、框架注册，让 GOPS 训练代码能统一加载、调度你的 Simulink 动力学仿真环境。

1、适配器文件：simu_<对象名>.py（桥梁转换器）:GOPS 框架 ↔ slxpy 生成的 GymEnv 仿真环境中间适配层
2、资源文件夹：resources\simu_rl_water\（仿真静态资源仓库）
   -存放和当前仿真模型强绑定的 3 个配套文件，三者必须版本统一、同步更新：
      -rl_water.cp38-win_amd64.pyd：Simulink 动力学编译二进制内核；
      -model.toml：slxpy 生成的 Simulink 代码生成配置（端口、步长、C++ 编译参数）；
      -env.toml：仿真环境参数配置（观测维度、奖励规则、终止条件）。
```

### 6.1 通用结构

本环节使用第 1 环节的 `gops_env` 和第 5 环节生成、验证过的 `.pyd`，不新增软件安装。

建议每个 Simulink 环境包含两部分：

```text
gops\env\env_matlab\simu_<对象名>.py
gops\env\env_matlab\resources\simu_<对象名>\
├─ <编译扩展>.pyd
├─ env.toml
└─ model.toml
```

适配器 `simu_<对象名>.py` 的通用职责：

1. 导入 ==slxpy== 生成的扩展；
2. 创建底层 `GymEnv`；
3. 定义或继承 `observation_space`、`action_space` 和 `reward_range`；
4. 接收 `max_episode_steps` 等 GOPS 参数；
5. 把底层 API 转换为当前 GOPS 训练器期望的 reset/step 返回格式；
6. 必要时在 Python 层预处理 action 或后处理 observation；
7. 提供 `env_creator(**kwargs)`。

资源目录中的 `.pyd`、`env.toml` 和 `model.toml` 必须来自同一版模型。不要只替换 `.pyd` 而保留旧 TOML。

### 6.2 当前水箱实例

水箱适配器：

```text
gops\env\env_matlab\simu_rl_water.py
```

水箱资源目录：

```text
gops\env\env_matlab\resources\simu_rl_water\
```

同步经过第 5 环节验证的文件：

```powershell
Set-Location 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang'

Copy-Item 'water\my_sim_env\build\lib.win-amd64-cpython-38\rl_water.cp38-win_amd64.pyd' 'gops\env\env_matlab\resources\simu_rl_water\' -Force

Copy-Item 'water\my_sim_env\env.toml' 'gops\env\env_matlab\resources\simu_rl_water\env.toml' -Force

Copy-Item 'water\my_sim_env\model.toml' 'gops\env\env_matlab\resources\simu_rl_water\model.toml' -Force
```

当前 `simu_rl_water.py`：

- 用 `EnvSpec` 设置最大回合步数和严格 reset；
- 将动作空间定义为 1 维 `float64`、范围 `[-100,100]`；
- 将底层 terminated/truncated 兼容为 GOPS 使用的 done；
- 环境 ID 为 `simu_rl_water`。

### 6.3 验收

- 适配器文件可以导入；
- 资源目录中没有与当前 Python 不兼容的旧扩展；
- 三个资源文件来自同一版 `.slx`；
- `env_creator` 能创建环境实例。

---

## 第 7 环节：对 GOPS 环境做 reset/step 验收  【可由AI辅助来写】

### 7.1 通用测试要求

本环节继续使用第 1 环节的 `gops_env`，测试第 6 环节注册的 GOPS 环境，不新增软件安装。

训练前至少验证：

1. `create_env(env_id=...)` 能找到环境；
2. reset 的 observation shape、dtype、上下界正确；
3. action space 与物理动作定义一致；
4. step 返回 GOPS 训练器期望的格式；
5. 固定动作下状态变化方向正确；
6. 随机运行几十至几百步无 NaN/Inf；
7. 物理终止和时间截断都能触发；
8. 终止后可以再次 reset；
9. 不存在持续内存增长。

如果训练配置使用 `action_scale=False`，环境测试也必须使用相同设置，否则测试时看到的动作空间可能是归一化的 `[-1,1]`，与正式训练不一致。

### 7.2 当前水箱实例

```powershell
Set-Location 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang'

& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -c "import numpy as np; from gops.create_pkg.create_env import create_env; e=create_env(env_id='simu_rl_water',max_episode_steps=200,action_scale=False); o,i=e.reset(seed=0); print('reset',o,o.shape,i); print('spaces',e.observation_space,e.action_space); print('step',e.step(np.array([0.0],dtype=np.float64))); e.close()"
```

预期关键结果：

```text
observation shape = (3,)
action space      = Box(-100.0, 100.0, (1,), float64)
```

还应测试正动作、负动作、接近边界水位和连续 200 步。

### 7.3 验收

全部测试通过后才允许训练。环境中的 shape、符号或时序错误进入 replay buffer 后，往往会伪装成“DDPG 不收敛”。

---

## 第 8 环节：配置算法，先冒烟训练，再正式训练  【可由AI辅助来写】

### 8.1 GOPS 训练配置的通用组成

本环节使用第 1 环节已经安装 GOPS、PyTorch、NumPy 和 Ray 的 `gops_env`，不再重复安装依赖。

GOPS 训练脚本通过 `argparse` 收集参数，再交给 `init_args()` 组装各模块。配置通常包含：

| 参数组     | 典型内容                                                |
| ---------- | ------------------------------------------------------- |
| 用户和环境 | `env_id`、`algorithm`、`seed`、CUDA、最大回合步数 |
| 近似函数   | policy/value 类型、隐藏层、激活函数                     |
| 算法       | 学习率、`gamma`、`tau`、延迟更新、梯度裁剪          |
| trainer    | 串行/并行模式、最大迭代数                               |
| buffer     | buffer 类型、预热量、容量、batch size                   |
| sampler    | 采样 batch、采样间隔、探索噪声                          |
| evaluator  | 评估间隔、评估回合数、是否保存轨迹                      |
| 保存       | 结果目录、检查点间隔、日志间隔                          |

这些参数相互依赖。例如算法类型、policy 结构、动作类型、动作缩放和探索噪声必须一致，不能只改一个名字就直接训练。

通用顺序是：

```text
先创建并关闭一个环境完成参数初始化
→ 创建算法
→ 创建 sampler
→ 创建 replay buffer
→ 创建 evaluator
→ 创建 trainer
→ 冒烟训练
→ 正式训练
```

### 8.2 当前水箱 DDPG 配置

训练入口：

```text
example_train\ddpg\ddpg_mlp_rl_water_offserial_slx.py
```

主要配置：

| 参数                  |              当前值 |
| --------------------- | ------------------: |
| `env_id`            |   `simu_rl_water` |
| `algorithm`         |            `DDPG` |
| `max_episode_steps` |                 200 |
| `max_iteration`     |               40000 |
| policy/value 隐藏层   |         `[25,25]` |
| policy/value 学习率   | `1e-4` / `1e-3` |
| `gamma` / `tau`   |  `1.0` / `1e-3` |
| buffer 预热量         |                 400 |
| replay batch          |                 400 |
| sample batch          |                   8 |
| 探索噪声              |  OU，物理标准差 0.3 |
| 评估间隔              |                 500 |
| 每次评估回合          |                  20 |
| 检查点保存间隔        |                 300 |
| `action_scale`      |           `False` |

`action_scale=False` 表示 Actor 直接输出物理动作。Simulink 回灌时不能再把策略输出乘以动作范围。

### 8.3 先做冒烟训练

开始前确认：

- 第 7 环节全部通过；
- 没有其他训练进程写相同结果目录；
- TensorBoard 端口没有被错误进程占用；
- Ray 能启动，磁盘空间足够。

执行：

```powershell
Set-Location 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang'

& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -u example_train\ddpg\ddpg_mlp_rl_water_offserial_slx.py --max_iteration 1000 --eval_interval 500
```

冒烟训练只验收：环境采样、buffer 预热、Actor/Critic 更新、evaluator、日志和检查点是否正常，不用于判断最终收敛性。

### 8.4 正式训练

冒烟训练通过后：

```powershell
& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -u example_train\ddpg\ddpg_mlp_rl_water_offserial_slx.py
```

训练期间观察：

- Evaluation/TAR 是否总体改善；
- Actor/Critic loss 是否出现 NaN/Inf；
- 动作是否长期贴住上下限；
- 终止和完成回合数量是否合理；
- 采样时间、算法时间和 RAM 是否异常；
- 检查点和评估轨迹是否持续保存。

### 8.5 验收和产物

每次训练的真实配置以结果目录中的 `config.json` 为准。目录通常包含：

```text
results\<ENV_ID>\<算法_时间戳>\
├─ config.json
├─ apprfunc\
├─ evaluator\
├─ data\
└─ figure\
```

当前水箱已有结果：

```text
results\simu_rl_water\DDPG_260630-063942
```

这是历史实例，不是每次训练固定使用的目录。新训练必须使用终端最后打印的 `Results:` 路径。

参考：[GOPS Training Configuration](https://gops.readthedocs.io/en/latest/example_config.html)。

```
需要说明：
训练的效果可同步通过Tensorboard刷新实时查看。
Tensorboard展示的数据均可自行设置更改。
（一般平均奖励啥的平稳就可以，只展示这个效果的图，actor和critic等不用太关心，能稳定那肯定更好）

若效果不好，可调整参数，重新训练。
这个我觉得也可以交给AI来做，一直调整到一个合适的参数和效果
```

---

## 第 9 环节：选择检查点并导出 TorchScript 策略  【可由AI辅助来写Py2slxRunner脚本】

### 9.1 通用选择原则

本环节继续使用第 8 环节训练所用的 `gops_env`，并使用其中的 PyTorch 和 GOPS 导出工具，不新增软件安装。

不要只因为文件名带 `_opt` 就直接部署。应结合 evaluator：

1. 比较候选检查点的累计奖励；
2. 在相同初始状态和目标下比较状态、误差、动作和终止；
3. 排除偶然高分、频繁越界和长期动作饱和；
4. 保存对应 `config.json`，因为导出时需要重建相同网络。

### 9.2 通用导出配置

`Py2slxRunner` 需要四组参数：

```text
log_policy_dir_list：训练结果目录，目录内含 config.json
trained_policy_iteration_list：检查点标识，不含 apprfunc_ 和 .pkl
export_controller_name：输出控制器名称，不含 .pt
save_path：.pt 保存目录
```

导出过程会重建网络、加载检查点、取得示例 observation、执行 `torch.jit.trace` 并保存 `.pt`。自定义网络必须满足 PyTorch trace 要求。

### 9.3 当前水箱实例

当前选用：

```text
训练目录：results\simu_rl_water\DDPG_260630-063942
检查点：  apprfunc\apprfunc_30500_opt.pkl
标识：    30500_opt
输出名：  DDPG_rl_water_30500_opt.pt
```

导出脚本：

```text
gops\env\py2slx_tools\py2slx_ddpg_rl_water.py
```

执行：

```powershell
Set-Location 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang'

& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -u gops\env\py2slx_tools\py2slx_ddpg_rl_water.py
```

脚本可能在导出后尝试启动 MATLAB。MATLAB 启动失败或等待关闭，不一定表示 `.pt` 没有生成，应先验证文件：

```powershell
& 'C:\Users\admin\miniconda3\envs\gops_env\python.exe' -c "import torch; p=r'gops\env\py2slx_tools\DDPG_rl_water_30500_opt.pt'; m=torch.jit.load(p); m.eval(); y=m(torch.zeros(3)); print(y,tuple(y.shape))"
```

### 9.4 验收

- `.pt` 文件存在；
- `torch.jit.load` 能加载；
- 输入 shape 与 observation space 一致；
- 输出 shape 与 action space 一致；
- 当前水箱为 3 维输入、1 维输出。

---

## 第 10 环节：从正确的 Python 环境启动 MATLAB 【打开的matlab需要具备PyTorch、NumPy的环境(运行第九个环节生成的脚本)，因此默认用gops_env下打开matlab】

### 10.1 使用的环境和组件

本环节使用第 2 环节安装的 MATLAB，以及第 1 环节创建且含 PyTorch、NumPy 的 `gops_env`，不重新安装。策略桥使用 `pyrun` 调用该 Python，因此：

- MATLAB 至少为 R2021b；
- MATLAB 版本必须支持所选 Python 版本；
- 该 Python 中必须安装 PyTorch、NumPy，建议同时安装 GOPS；
- MATLAB 一旦加载 Python，通常不能在同一会话随意切换解释器；配错后关闭 MATLAB 再重启。

### 10.2 通用操作

从含 PyTorch 的 Conda 环境启动 MATLAB：

```powershell
conda activate <训练和导出策略的环境>
matlab
```

在 MATLAB 中检查：

```matlab
pyenv
pyrun("import sys, torch, numpy; print(sys.executable); print(torch.__version__); print(numpy.__version__)")
```

### 10.3 当前水箱实例

```powershell
conda activate gops_env
matlab
```

`pyenv` 的 `Executable` 应为：

```text
C:\Users\admin\miniconda3\envs\gops_env\python.exe
```

### 10.4 验收

- `pyenv` 指向目标环境；
- `pyrun` 能导入 torch 和 numpy；
- `.pt` 可从该 Python 加载。

参考：[GOPS to Simulink 前置条件](https://gops.readthedocs.io/en/latest/py2slx.html)。

---

## 第 11 环节：在 Simulink 中接入策略并分层验证

### 11.1 使用的环境、组件和接入方式

本环节继续使用第 10 环节已经验证的 MATLAB、`gops_env` 和 `pyrun`，并加载第 9 环节导出的 `.pt`，不新增软件安装。

准备：

```text
gops_validation_bridge.m
导出的策略模型.pt
```

把桥复制到目标 Simulink 模型目录，或把桥所在目录加入 MATLAB path。然后添加 **Level-2 MATLAB S-Function**：

```text
S-function name = gops_validation_bridge
Parameters      = '.pt 文件路径'
```

桥的输入是完整 observation 向量，输出是完整 action 向量。S-Function 只有一个 observation 输入端口，不应按每个状态元素拆成多个端口。

必须保证训练与回灌之间以下契约完全一致：

- observation 元素、顺序、符号、单位和 dtype；
- action 维数、单位和缩放方式；
- 控制采样周期；
- 状态初值、目标值和参数；
- 被控对象动力学；
- 训练评估时不存在探索噪声。

如果训练使用归一化 observation 或 action，回灌模型必须实现完全相同的变换。如果训练使用物理动作，则不能再次缩放。

### 11.2 当前水箱实例

文件：

```text
gops\env\py2slx_tools\gops_validation_bridge.m
gops\env\py2slx_tools\DDPG_rl_water_30500_opt.pt
```

MATLAB 添加路径：

```matlab
toolDir = 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools';
addpath(toolDir);
which gops_validation_bridge
```

S-Function 设置：

```text
S-function name = gops_validation_bridge
```

```matlab
'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools\DDPG_rl_water_30500_opt.pt'
```

水箱 observation 用 Mux 严格组成：

```text
[积分跟踪误差, 目标水位-实际水位, 实际水位]
```

接线：

```text
3 维 observation
→ gops_validation_bridge
→ 1 维物理动作
→ Saturation [-100,100]
→ 水箱 Action
```

水箱训练使用 `action_scale=False`，所以 S-Function 输出后不能再次乘 100。

### 11.3 验证顺序

按以下四层执行，不要直接跳到闭环：

1. **固定点推理一致性**在 Python 和 Simulink 中分别输入相同 observation，例如 `[0,0,10]`，比较 action。
2. **开环接口验证**断开 Plant，用 Constant 或缓慢变化的 observation 驱动桥，确认输入输出维数、dtype 和数值稳定性。
3. **同工况闭环验证**GOPS evaluator 和 Simulink 使用相同初值、目标值、参数、步长、采样周期、动作限幅和时域，比较状态、动作、误差、累计奖励及终止。
4. **鲁棒性验证**
   同工况一致后，再分别改变初始状态、目标、物理参数、测量噪声、执行器饱和、采样延迟和模型不确定性。

### 11.4 验收

- S-Function 能加载 `.pt`；
- 输入/输出 shape 与训练配置一致；
- 固定点推理只有合理浮点误差；
- 同工况下 GOPS 与 Simulink 轨迹基本一致；
- 无重复动作缩放、观察量错序或错误采样节拍。

---

## 12. 修改内容与重新执行范围

| 修改内容                                    | 从哪个环节重新开始    |
| ------------------------------------------- | --------------------- |
| 只改变模型显示布局                          | 通常不需要重新生成    |
| 改动力学、端口、observation、reward 或 done | 第 2 环节，并重新训练 |
| 改模型可调参数接口                          | 第 2 环节，并重新训练 |
| 改`model.toml` 或 `env.toml`            | 第 3 环节             |
| 改求解器或采样周期                          | 第 2 环节，并重新训练 |
| 只改 GOPS 环境适配器                        | 第 6 环节，并重新训练 |
| 只改算法超参数或网络结构                    | 第 8 环节             |
| 选择另一个已有检查点                        | 第 9 环节             |
| 只改`.pt` 在 Simulink 中的路径            | 第 11 环节            |

只要修改会改变策略看到的 observation、输出的 action、状态转移、reward 或时间关系，就必须重新训练。

## 13. 全流程最终产物

```text
Simulink 模型
<SLXPY_PROJECT>\<MODEL_NAME>.slx

Python 原生环境
<SLXPY_PROJECT>\build\lib...\<扩展>.pyd

GOPS 环境
gops\env\env_matlab\simu_<对象名>.py
gops\env\env_matlab\resources\simu_<对象名>\

训练结果
results\<ENV_ID>\<算法_时间戳>\

回灌策略
<控制器名称>.pt

Simulink 闭环
observation → gops_validation_bridge → action → Plant
```

## 14. 主要参考资料

- [GOPS 官方文档首页](https://gops.readthedocs.io/en/latest/)
- [GOPS Introduction、拉取与安装](https://gops.readthedocs.io/en/latest/introduction.html)
- [Simulink to GOPS：前置条件、模型接口、env.toml、生成和编译](https://gops.readthedocs.io/en/latest/slx2py.html)
- [GOPS Training Configuration](https://gops.readthedocs.io/en/latest/example_config.html)
- [GOPS to Simulink：策略导出和 S-Function 回灌](https://gops.readthedocs.io/en/latest/py2slx.html)
- 本地参考文档：`说明文档\image\顺序记录-DDPG水箱从环境配置到训练及Simulink回灌完整流程\1782875921676.docx`

