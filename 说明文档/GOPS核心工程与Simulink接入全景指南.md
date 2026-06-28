# GOPS 核心工程与 Simulink 接入全景指南

## 1. 阅读目标

本文不讨论具体函数如何实现，也不逐行解释代码，而是回答：

1. `gops` 整个核心工程由哪些部分组成；
2. 每类文件在系统中扮演什么角色；
3. 训练脚本、算法、环境、Simulink 模型和结果目录如何连接；
4. 把一个现有 Simulink 模型转成 GOPS 可交互训练环境时，应该按什么顺序操作。

## 2. 先从宏观看 GOPS

GOPS 可以理解为一条控制策略生产线：

```text
控制问题/仿真模型
    ↓
统一环境接口 reset/step
    ↓
采样器让策略与环境交互
    ↓
训练器组织数据与参数更新
    ↓
算法更新策略网络和值网络
    ↓
results 保存训练模型
    ↓
PolicyRunner 做闭环验证
    ↓
figures 保存曲线和视频
```

其中 `gops` 是核心引擎，根目录中的 `example_train` 和 `example_run` 是用户入口。

## 3. `gops` 顶层目录全景

```text
gops/
├── algorithm/       算法：决定怎样更新策略和值函数
├── apprfunc/        近似函数：决定用什么网络/函数表示策略和值
├── create_pkg/      工厂：根据字符串配置寻找并创建组件
├── env/             环境：定义智能体与控制对象如何交互
├── sys_simulator/   验证：加载策略，执行闭环仿真，绘图和对比
├── trainer/         训练：组织采样、经验池、更新、评估和保存
├── utils/           公共工具：参数、日志、绘图、路径等
└── __init__.py      Python 包入口和版本信息
```

七个核心模块的关系：

```text
create_pkg
 ├─创建→ env
 ├─创建→ algorithm ─使用→ apprfunc
 └─创建→ trainer ─组织→ sampler/buffer/evaluator

trainer ─调用→ env + algorithm
sys_simulator ─加载→ results 中的策略
sys_simulator ─调用→ env
utils ─被所有模块共用
```

## 4. `algorithm`：算法文件地图

该目录回答“拿到样本后，如何更新神经网络”。每个主要文件对应一种算法或变体：

- `base.py`：算法公共基础接口；
- `ddpg.py`：DDPG，连续动作、模型自由、离策略；
- `td3.py`：TD3，DDPG 的双价值网络改进；
- `sac.py`：SAC，随机策略、最大熵、离策略；
- `dsac.py`：分布式 SAC；
- `dsact.py`：DSAC 相关变体；
- `dqn.py`：DQN，离散动作；
- `ppo.py`：PPO，同策略；
- `trpo.py`：TRPO，同策略；
- `mac.py`：Mixed Actor-Critic；
- `mpg.py`：Mixed Policy Gradient；
- `spil.py`：面向约束控制的 SPIL；
- `rpi.py`：策略迭代类控制算法；
- `fhadp.py`、`fhadp2.py`：有限时域 ADP；
- `infadp.py`：无限时域 ADP；
- `fhadp_exterior.py`、`fhadp_interior.py`：不同约束处理形式；
- `fhadp_lagrangian.py`、`fhadp_lagrangiannet.py`：拉格朗日相关 FHADP；
- `__init__.py`：算法包入口。

与 Simulink 的关系：

- DDPG、TD3、SAC、DSAC、PPO 等可直接把编译后的 Simulink 环境当黑盒采样；
- FHADP、INFADP、MPC 等通常还需要可求导的 Python 环境模型，只有 Simulink `.pyd` 不够。

## 5. `apprfunc`：网络与函数表示文件地图

该目录回答“策略和值函数用什么结构表示”：

- `mlp.py`：多层感知机，控制任务最常用；
- `cnn.py`：卷积网络，用于图像观测；
- `cnn_shared.py`：策略和值函数共享卷积特征；
- `rnn.py`：循环网络，用于时序或部分可观问题；
- `poly.py`：多项式近似；
- `gauss.py`：高斯基函数；
- `lipsnet.py`：Lipschitz 相关网络；
- `__init__.py`：包入口。

Simulink 环境只决定观测和动作的形状；算法与 `apprfunc` 根据这些形状自动构造网络。Simulink 模型本身不需要包含神经网络。

## 6. `create_pkg`：组件创建文件地图

这是配置字符串与实际对象之间的连接层：

- `create_env.py`：根据 `env_id` 扫描和创建数据环境，并添加包装器；
- `create_env_model.py`：创建模型驱动算法需要的 Tensor 环境模型；
- `create_alg.py`：根据算法名创建算法；
- `create_apprfunc.py`：创建策略函数、价值函数等网络；
- `create_sampler.py`：创建采样器；
- `create_buffer.py`：创建经验回放池；
- `create_evaluator.py`：创建评估器；
- `create_trainer.py`：创建训练器；
- `__init__.py`：包入口。

对 Simulink 接入最关键的是 `create_env.py`：只要在 `gops/env/env_matlab/` 中新增符合命名约定的 Python 环境文件，它就能自动把文件名注册为新的 `env_id`。

## 7. `env`：环境模块全景

```text
gops/env/
├── env_gym/        经典 Gym 环境
├── env_ocp/        旧式 GOPS Python 控制环境
├── env_gen_ocp/    组件化控制环境
├── env_matlab/     Simulink 编译环境的 Python 接口
├── wrapper/        环境接口增强与统一
├── vector/         多环境同步/异步运行
├── inspector/      环境正确性检查
├── py2slx_tools/   训练后把 GOPS 策略放回 Simulink
└── __init__.py     环境包入口
```

### 7.1 `env_gym`

顶层 `gym_*.py` 文件分别包装标准任务，例如 CartPole、Pendulum、MountainCar、LunarLander、MuJoCo、Atari 和 CarRacing。

`env_gym/env_model/` 中的文件是少数 Gym 环境的显式模型版本：

- `gym_cartpoleconti_model.py`；
- `gym_pendulum_model.py`；
- `gym_mountaincarconti_model.py`；
- `__init__.py`。

### 7.2 `env_ocp`

顶层 `pyth_*.py` 是原有 GOPS 控制环境：飞机、倒立双摆、车辆、移动机器人、振子、悬架和线性二次系统等。

- `pyth_base_env.py`：这类数据环境的公共基础；
- `env_model/pyth_base_model.py`：模型环境公共基础；
- `env_model/pyth_*_model.py`：每个数据环境的 Tensor 模型；
- `resources/`：参考轨迹和线性系统配置；
- `__init__.py`：包入口。

### 7.3 `env_gen_ocp`

顶层文件如 `idpendulum.py`、`pendulum.py`、`cartpoleconti.py`、`veh3dof_tracking.py` 表示完整任务。

- `pyth_base.py`：Robot、Context、State 和 Env 的公共抽象；
- `robot/`：摆、车辆、四旋翼等被控对象动力学；
- `context/`：平衡点、参考轨迹、障碍物和约束；
- `env_model/`：每个任务的 Tensor 模型；
- `__init__.py`：包入口。

### 7.4 `wrapper`

这些文件不定义新物理系统，而是在已有环境外叠加统一行为：

- `reset_info.py`：统一 reset 返回值；
- `convert_type.py`：统一 NumPy 数据类型；
- `unify_state.py`：保证环境具有统一 state 表达；
- `action_repeat.py`：一个策略动作执行多个仿真步；
- `scale_action.py`：策略动作与物理动作范围映射；
- `scale_observation.py`：观测缩放；
- `shaping_reward.py`：奖励平移和缩放；
- `noise_action.py`、`noise_observation.py`：添加噪声；
- `clip_action.py`、`clip_observation.py`：裁剪；
- `mask_at_done.py`：模型终止掩码；
- `transform_constraint.py`：约束形式转换；
- `gym2gymnasium.py`：接口适配；
- `base.py`、`__init__.py`：公共基础和包入口。

### 7.5 `vector`

- `vector_env.py`：向量环境公共接口；
- `sync_vector_env.py`：同步推进多个环境；
- `async_vector_env.py`：异步推进多个环境；
- `__init__.py`：包入口。

### 7.6 `inspector`

- `env_data_checker.py`：检查观测空间、动作空间和 reset/step 接口；
- `env_dynamic_checker.py`：检查动力学行为；
- `env_model_checker.py`：比较数据环境与模型环境；
- `__init__.py`：包入口。

## 8. `env_matlab`：Simulink 相关每类文件

```text
gops/env/env_matlab/
├── simu_aircraftconti.py
├── simu_lqs2a1conti.py
├── simu_veh3dofconti.py
├── env_model/
├── resources/
└── __init__.py
```

### 8.1 三个 Python 环境入口

- `simu_aircraftconti.py`：导入 Aircraft 的预编译 `.pyd`，创建 GymEnv；
- `simu_lqs2a1conti.py`：包装线性二次 Simulink 环境，负责 reset 参数写入；
- `simu_veh3dofconti.py`：较完整示例，负责初始状态、参考轨迹、动作/观测缩放、动作保持和奖励处理；
- `__init__.py`：包入口。

这些文件是“GOPS 与 Simulink 编译包之间的适配层”，不是 Simulink 物理模型本身。

### 8.2 `env_model`

- `simu_lqs2a1conti_model.py`：为模型驱动算法提供线性二次系统的 Python 模型；
- `__init__.py`：包入口。

它说明数据采样可以来自 Simulink，而梯度/预测模型可以由等价 Python 数学模型提供。

### 8.3 `resources` 中每种资产的作用

每个 `simu_*` 子目录代表一个可独立使用的 Simulink 环境资产包：

- `simu_aircraft/`、`simu_aircraft_v2/`：飞机；
- `simu_cartpole/`、`simu_cartpole_v2/`：CartPole；
- `simu_doublemass/`、`simu_doublemass_v2/`：双质量系统；
- `simu_lqs2a1/`：线性二次系统；
- `simu_vehicle3dof/`、`simu_vehicle3dof_v2/`：三自由度车辆。

其中不同扩展名分别表示：

| 文件 | 作用 |
|---|---|
| `*.slx` | 原始 Simulink 模型 |
| `model.toml` | slxpy 的模型代码生成配置 |
| `env.toml` | Action/State/Reward/Done 和 Gym 空间配置 |
| `*.pyd` | 编译后的 Windows Python 扩展，训练时实际执行 |
| `*.m` | 模型参数初始化脚本 |
| `*.pyi` | 自动生成的类型提示，帮助查看扩展提供的类和参数 |
| `.gitignore` | 该资产目录的 Git 忽略规则 |

带 `_v2` 的目录通常是较新的 slxpy 工程形式，包含 `.toml` 和/或预编译 `.pyd`。旧目录主要保存 `.slx` 和 `.m`。

### 8.4 Aircraft 资产链

```text
aircraft.slx
 + model.toml
 + env.toml
      ↓ slxpy 代码生成与编译
aircraft.cp38-win_amd64.pyd
      ↓ 被导入
simu_aircraftconti.py
      ↓ 注册为
env_id=simu_aircraftconti
      ↓ 被使用
ddpg_mlp_aircraftconti_offserial_slx.py
```

这条链就是 Simulink 接入 GOPS 的最小参考。

## 9. `trainer`：训练流程文件地图

顶层训练器：

- `off_serial_trainer.py`：离策略串行训练，首次接入 Simulink 最适合从这里开始；
- `off_sync_trainer.py`：离策略同步并行；
- `off_async_trainer.py`：离策略异步并行；
- `on_serial_trainer.py`：同策略串行训练；
- `on_sync_trainer.py`：同策略同步训练；
- `evaluator.py`：定期独立测试策略并保存结果；
- `__init__.py`：包入口。

`sampler/`：

- `base.py`：定义策略输出动作、调用环境 step、记录 Experience 的通用循环；
- `off_sampler.py`：离策略采样；
- `on_sampler.py`：同策略轨迹采样；
- `__init__.py`：包入口。

`buffer/`：

- `replay_buffer.py`：普通经验回放；
- `prioritized_replay_buffer.py`：优先级经验回放；
- `__init__.py`：包入口。

Simulink 模型与智能体真正发生交互的位置是 sampler 调用 `env.reset()` 和 `env.step(action)`。训练器只负责何时采样、何时更新和何时保存。

## 10. `sys_simulator`：策略验证文件地图

- `sys_run.py`：PolicyRunner，加载策略、创建环境、执行回合、保存图表和视频；
- `opt_controller.py`：旧式环境的 MPC/OPT 对比控制器；
- `opt_controller_for_gen_env.py`：组件化环境的最优控制器；
- `call_terminal_cost.py`：终端代价处理；
- `__init__.py`：包入口。

它用于训练完成后的闭环测试，不负责网络训练。

## 11. `utils`：公共工具文件地图

- `init_args.py`：读取环境空间，补全观测/动作维度并创建结果目录；
- `gops_path.py`：统一核心目录路径；
- `gops_typing.py`：公共类型定义；
- `common_utils.py`：随机种子、数据转换等通用功能；
- `math_utils.py`：数学辅助；
- `explore_noise.py`：连续动作噪声和离散探索；
- `act_distribution_cls.py`、`act_distribution_type.py`：动作分布；
- `log_data.py`：训练日志；
- `tensorboard_setup.py`：TensorBoard 和数据导出；
- `plot_evaluation.py`：训练/评估曲线；
- `parallel_task_manager.py`：并行任务辅助；
- `pkl2onnx.py`：检查点转 ONNX；
- `__init__.py`：包入口。

## 12. `py2slx_tools`：不要与训练侧接入混淆

```text
gops/env/py2slx_tools/
├── py2slx.py
├── export.py
├── py2slx_example.py
├── gops_validation_bridge.m
├── README.md
└── __init__.py
```

- `py2slx.py`：组织策略加载、兼容性检查和导出；
- `export.py`：将策略包装并导出为 TorchScript；
- `py2slx_example.py`：导出配置示例；
- `gops_validation_bridge.m`：Simulink 中调用导出策略的 S-Function 桥；
- `README.md`：策略回灌操作说明；
- `__init__.py`：包入口。

两条方向必须分清：

```text
训练侧：Simulink .slx → slxpy → .pyd → GOPS env_matlab
部署侧：GOPS 策略 → TorchScript → gops_validation_bridge.m → Simulink
```

`py2slx_tools` 是部署/验证侧工具，不负责把 `.slx` 转为训练环境。

## 13. 核心目录之外的相关文件

### `example_train`

这里的文件是实验配置入口，不是算法实现。例如：

- `ddpg/ddpg_mlp_aircraftconti_offserial_slx.py`：Aircraft + DDPG；
- `ppo/ppo_mlp_veh3dofconti_onserial_slx.py`：车辆 + PPO；
- `sac/sac_mlp_veh3dofconti_offserial_slx.py`：车辆 + SAC；
- `dsac/dsac_mlp_veh3dof_offserial_slx.py`：车辆 + DSAC。

它们负责指定 `env_id`、算法、网络、训练器、采样量和 Simulink 环境参数。

### `results`

保存训练检查点、配置、日志和评估数据。一次训练的 `config.json` 是复现实验的重要文件。

### `example_run`

加载 `results` 中的策略进行闭环测试。

### `figures`

保存运行后的 PNG、CSV、Excel、NPY 和视频。

## 14. 实际转换一个 Simulink 模型的操作顺序

下面是推荐的严格顺序。不要一开始就修改 GOPS 算法代码。

### 阶段 1：定义交互问题

在任何转换前，先写清楚：

1. Action：策略输出什么控制量，维度、范围、单位是什么；
2. Observation：策略看到哪些量，顺序、维度、范围是什么；
3. Reward：希望最大化什么；
4. Done：哪些物理或任务条件结束回合；
5. Reset：每个回合如何设置初始状态和随机参数；
6. Ts：策略多久给一次动作；
7. TimeLimit：一个回合最多多少个策略步。

此阶段输出一张接口表，而不是代码。

### 阶段 2：整理 `.slx`

在 Simulink 中完成：

1. 设置明确的 Action Inport；
2. 设置 State/Observation Outport；
3. 设置标量 Reward Outport；
4. 设置逻辑型 Done Outport；
5. 固定并记录基础仿真步长；
6. 将初始状态和需要随机化的参数设为可调参数；
7. 确认模型及各模块支持代码生成。

完成后，应先在 Simulink 内用固定动作序列得到一份基准输出。

### 阶段 3：建立 slxpy 工程

创建独立目录，例如：

```text
my_sim_env/
├── my_model.slx
├── model.toml
└── env.toml
```

操作：

```powershell
mkdir my_sim_env
cd my_sim_env
slxpy init
```

在 `model.toml` 中配置模型和代码生成，在 `env.toml` 中配置 Action/State/Reward/Done 映射及 Gym 空间。

### 阶段 4：生成 C/C++ 代码

在 MATLAB 中：

```matlab
workdir = 'my_sim_env的绝对路径';
slxpy.setup_config(workdir);
slxpy.codegen(workdir);
```

- 首次创建或修改 `model.toml` 后重新运行 `setup_config`；
- 修改 `.slx` 后重新运行 `codegen`。

### 阶段 5：生成 Python 绑定并编译

在目标 Conda 环境中：

```powershell
cd my_sim_env
slxpy generate
python setup.py build
```

得到与当前 Python/Windows 匹配的 `.pyd`。例如 `cp38-win_amd64` 只对应 64 位 CPython 3.8。

### 阶段 6：脱离 GOPS 单独测试

先直接导入编译包并测试：

1. 能否创建 `GymEnv`；
2. reset 是否稳定返回正确维度；
3. action space 是否正确；
4. step 是否返回 observation、reward、done、info；
5. 相同初始状态和动作序列是否与 Simulink 基准输出一致；
6. 多次 reset 是否彻底恢复积分器和内部状态。

这一阶段不通过，不能进入 GOPS。

### 阶段 7：放入 GOPS 资源目录

建议创建：

```text
gops/env/env_matlab/resources/simu_my_model/
├── my_model.slx
├── model.toml
├── env.toml
└── my_package.cp38-win_amd64.pyd
```

保留 `.slx` 和配置文件，不能只保留 `.pyd`，否则以后无法追溯和重新编译。

### 阶段 8：添加 GOPS 环境入口

创建：

```text
gops/env/env_matlab/simu_my_model.py
```

该文件只负责以下职责：

1. 导入编译后的 GymEnv；
2. 声明 observation/action space；
3. reset 时写入初始状态和随机参数；
4. step 前做动作物理量转换；
5. step 后做观测、奖励和 info 后处理；
6. 提供 `env_creator()`。

文件名决定新的环境 ID：

```text
simu_my_model.py → env_id=simu_my_model
```

### 阶段 9：通过 GOPS 单独测试环境

使用 `create_env(env_id=simu_my_model)`，先以随机动作运行若干回合，确认：

- 观测和动作形状正确；
- 无 NaN/Inf；
- reward 数量级合理；
- done 和 TimeLimit 行为正确；
- reset 后没有上一回合残留状态；
- 动作缩放没有在两层重复执行。

### 阶段 10：选择算法路线

首次接入建议选择模型自由算法：

```text
连续动作：DDPG、TD3、SAC、PPO
离散动作：DQN、PPO
```

复制最接近的 `_slx.py` 训练脚本，只修改：

- `env_id`；
- 算法和网络配置；
- 环境专用参数；
- 小规模训练参数。

如果使用 FHADP/INFADP/MPC，则必须额外创建：

```text
gops/env/env_matlab/env_model/simu_my_model_model.py
```

并验证它与 Simulink 数据环境逐步一致。

### 阶段 11：短训练

先采用：

- 串行 trainer；
- 单环境；
- 很少的迭代次数；
- 很小的采样批次；
- 固定 seed；
- 不启用渲染；
- 高频记录状态、动作、reward 和 done。

确认采样、buffer、更新和评估完整走通后再扩大规模。

### 阶段 12：正式训练与验证

```text
训练脚本
 → results/<env_id>/<算法_时间戳>/
 → 检查 config.json 和检查点
 → example_run/PolicyRunner 闭环测试
 → figures 保存图和视频
```

最后可使用 `py2slx_tools` 把训练策略导出为 TorchScript，放回原 Simulink 模型进行闭环验证。

## 15. 一个新 Simulink 环境最终需要哪些文件

### 必需文件

```text
my_model.slx
model.toml
env.toml
my_package.<python-abi>.pyd
gops/env/env_matlab/simu_my_model.py
example_train/<algorithm>/<new_training_script>.py
```

### 模型驱动算法额外需要

```text
gops/env/env_matlab/env_model/simu_my_model_model.py
数据环境与模型的一致性测试
```

### 建议保留

```text
Simulink 基准动作序列
基准状态/奖励输出
接口和单位说明
代码生成日志
编译环境版本记录
```

## 16. 最常见的错误顺序

以下顺序会让问题难以定位：

```text
未定义接口
 → 直接编译
 → 直接接入异步训练
 → 发现 reward 不学习
 → 同时修改模型、环境和算法
```

正确顺序应是：

```text
定义接口
 → Simulink 基准测试
 → slxpy 编译
 → GymEnv 独立测试
 → GOPS 环境随机动作测试
 → 串行短训练
 → 正式训练
 → 策略回灌 Simulink
```

## 17. 总结

把 Simulink 模型放入 GOPS 不是简单复制一个 `.slx` 文件。需要形成一条完整资产链：

```text
.slx 负责物理仿真
.toml 负责代码生成和 Gym 信号映射
.pyd 负责训练时高速执行
env_matlab/*.py 负责 GOPS 接口、reset 和数据变换
example_train/*.py 负责选择算法和实验参数
trainer/sampler 负责实际交互训练
results 保存策略
py2slx_tools 负责训练后回灌验证
```

首次迁移应优先使用模型自由算法和串行训练。只有算法确实需要显式预测或梯度时，才额外实现 Python `env_model`。
