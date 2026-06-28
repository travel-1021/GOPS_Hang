# GOPS 项目文件夹结构与模块关系说明

## 1. 文档目的

本文针对项目目录 `E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang` 中的各个文件夹进行说明，重点回答以下问题：

1. 每个文件夹负责什么功能；
2. 哪些目录属于核心源码，哪些属于示例、测试或运行结果；
3. 训练程序、算法、近似函数、环境、训练器和结果目录之间如何协作；
4. 修改或扩展 GOPS 时，通常应该进入哪个目录。

本文只描述当前项目中实际存在的结构。时间戳命名的训练与测试目录可能随着后续运行继续增加。

## 2. 项目总体结构

当前项目根目录的主要文件夹如下：

```text
GOPS_Hang/
├── .git/                 Git 版本库内部数据
├── .vscode/              VS Code 工作区配置
├── example_run/          已训练策略的运行、测试和对比入口
├── example_train/        各类算法的训练入口与参数示例
├── figures/              策略运行后生成的图、表格、数据和视频
├── gops/                 GOPS 核心 Python 源码包
│   ├── algorithm/        强化学习和近似动态规划算法
│   ├── apprfunc/         神经网络、策略和值函数等近似函数
│   ├── create_pkg/       各组件的注册、查找和实例化工厂
│   ├── env/              控制环境、动力学模型、包装器和检查工具
│   ├── sys_simulator/    闭环仿真、绘图和最优控制器对比
│   ├── trainer/          训练流程、采样器和经验回放池
│   └── utils/            日志、参数、绘图和通用工具
├── gops.egg-info/        pip 可编辑安装生成的包元数据
├── results/              训练检查点、评估记录和 TensorBoard 数据
├── tests/                自动化测试
└── 说明文档/             本项目的本地中文说明文档
```

从使用角度，可以将这些目录分成三层：

- **输入和入口层**：`example_train`、`example_run`、`tests`；
- **核心实现层**：`gops`；
- **输出层**：`results`、`figures`。

## 3. 根目录下各文件夹说明

### 3.1 `.git/`：Git 版本控制数据

该目录由 Git 自动维护，用来保存：

- 提交历史和对象数据；
- 分支、标签及引用；
- 本地仓库配置；
- Git 操作日志；
- Git hooks 示例和配置。

常见子目录包括：

- `objects/`：Git 对象数据库；
- `refs/`：分支和标签引用；
- `logs/`：引用变化记录；
- `hooks/`：Git 钩子脚本；
- `info/`：仓库辅助信息。

该目录不参与 GOPS 的训练或仿真计算。一般不应手工修改其中内容，应通过 `git status`、`git add`、`git commit` 等 Git 命令管理。

### 3.2 `.vscode/`：VS Code 配置

用于保存当前项目的 VS Code 工作区级配置，例如：

- Python 解释器选择；
- 调试启动参数；
- 编辑器设置；
- 任务配置。

该目录只影响 IDE 使用体验，不属于 GOPS 算法逻辑。选择 Python 解释器时，应指向安装了 GOPS 的 Conda 环境，例如 `gops_env`。

### 3.3 `example_train/`：训练入口与实验配置

这是用户启动训练时最常使用的目录。这里的脚本通常负责：

1. 定义环境名称 `env_id`；
2. 选择算法 `algorithm`；
3. 选择策略/价值函数结构；
4. 配置学习率、采样、经验池和训练轮数；
5. 选择串行、同步或异步训练器；
6. 调用 `gops.create_pkg` 创建各个组件；
7. 启动训练并将结果写入 `results/`。

根目录中的 `alg_apprfunc_environ_trainer.py` 是较完整的训练参数模板，展示了如下标准创建顺序：

```text
create_env
  → init_args
  → create_alg
  → create_sampler
  → create_buffer
  → create_evaluator
  → create_trainer
  → trainer.train()
```

训练结束后，脚本通常还会调用绘图和 TensorBoard 数据导出工具。

#### `example_train/ddpg/`

DDPG（Deep Deterministic Policy Gradient）训练示例，主要用于连续动作控制问题。当前包含摆、连续 CartPole、MuJoCo、Aircraft/Simulink 等示例。

文件名中的典型含义：

- `mlp`：使用多层感知机；
- `poly`：使用多项式近似函数；
- `offserial`：离策略串行训练；
- `async`：异步并行训练；
- `slx`：涉及 MATLAB/Simulink 环境或模型。

#### `example_train/dqn/`

DQN（Deep Q Network）训练示例，面向离散动作空间，例如离散 CartPole。

#### `example_train/dsac/`

DSAC（Distributional Soft Actor-Critic）示例。它使用分布式价值表示，目录中覆盖摆、倒立双摆、车辆、Humanoid 和 CartPole 等任务。

#### `example_train/dsact/`

DSACT 相关示例，当前用于 MuJoCo 和原始图像 CarRacing 等任务。

#### `example_train/fhadp/`

FHADP（Finite-Horizon Approximate Dynamic Programming）训练示例，是当前倒立双摆案例使用的训练目录。

它包括：

- 不同维度的线性二次系统；
- 倒立双摆；
- 二自由度、三自由度车辆；
- 车辆跟踪、避障和约束任务；
- MLP 与多项式近似函数；
- 串行和异步配置。

例如 `fhadp_mlp_idpendulum_serial.py` 表示使用 MLP、倒立双摆环境和串行训练器的 FHADP 实验。

#### `example_train/fhadp_lagrangiannet/`

使用 Lagrangian Network 相关结构的 FHADP 训练示例，主要针对具有约束或特殊动力学结构的控制任务。

#### `example_train/infadp/`

INFADP（Infinite-Horizon Approximate Dynamic Programming）示例，用于无限时域近似动态规划问题。目录覆盖倒立双摆、摆、CartPole、车辆和线性二次系统等任务，并包含 MLP、多项式与 LipsNet 等近似函数配置。

#### `example_train/mac/`

MAC（Mixed Actor-Critic）训练示例，包含摆和连续 CartPole 的串行或异步配置。

#### `example_train/mpg/`

MPG（Mixed Policy Gradient）训练示例，包含摆和连续 CartPole 的离策略串行或异步配置。

#### `example_train/ppo/`

PPO（Proximal Policy Optimization）训练示例。目录中包含 MLP、CNN、多项式近似，以及串行、同步、向量环境和 Simulink 等配置。

#### `example_train/rpi/`

RPI 相关策略迭代示例，当前包括飞机、振子和悬架等连续控制系统，常使用在线串行训练。

#### `example_train/sac/`

SAC（Soft Actor-Critic）训练示例，覆盖倒立双摆、摆、CartPole、车辆、MuJoCo 和 Humanoid 等环境。

#### `example_train/spil/`

SPIL（Separated Proportional-Integral Lagrangian）示例，主要面向带约束的移动机器人和车辆控制任务。

#### `example_train/td3/`

TD3（Twin Delayed DDPG）训练示例，包含摆、连续 CartPole 和 MuJoCo 环境。

#### `example_train/trpo/`

TRPO（Trust Region Policy Optimization）训练示例，包括离散或连续 CartPole 和摆环境。

### 3.4 `example_run/`：策略运行、测试和对比入口

该目录不负责从头训练网络，主要用于加载 `results/` 中已有的策略检查点并进行闭环仿真。

其功能包括：

- 加载一个或多个训练策略；
- 指定初始状态；
- 在对应环境中执行策略；
- 记录状态、动作和奖励；
- 可选地调用 MPC 或全时域最优控制器作为基准；
- 生成曲线、CSV、Excel、NumPy 数据和视频到 `figures/`。

当前文件可以分为以下几类：

#### `run_*.py`

正式的策略运行和对比脚本。例如：

- `run_idp_fhadp.py`：运行倒立双摆 FHADP 策略，并可与 MPC 比较；
- `run_idp_infadp.py`：运行倒立双摆 INFADP 策略；
- `run_idp_sac_dsac.py`：比较 SAC 与 DSAC；
- `run_idp_sac_fhadp_infadp.py`：比较多种算法；
- `run_aircraftconti_rpi.py`：运行飞机连续控制 RPI 策略；
- `run_mobilerobot_spil.py`：运行移动机器人 SPIL 策略；
- `run_veh3dof_tracking*.py`：运行三自由度车辆跟踪或避障任务。

#### `test_*_open.py`

用于测试开放式环境接口或基础环境行为，通常侧重环境能否正常创建、复位、步进和渲染。

#### `test_*_close.py`

用于测试闭环控制行为，通常会将控制器或策略接入环境后执行完整回合。

#### `template_*.py`

新建环境测试或算法运行脚本时使用的模板，包括开放环境、闭环环境和环境-算法组合模板。

### 3.5 `gops/`：核心 Python 包

这是整个项目最重要的源码目录。执行 `python -m pip install -e .` 后，其他项目中的 `import gops` 实际引用的就是该目录。

训练脚本和运行脚本只是配置入口，真正的算法、环境、网络、训练器和仿真器都在这里实现。

#### 3.5.1 `gops/algorithm/`：算法实现

该目录实现 GOPS 支持的强化学习和近似动态规划算法。

主要文件包括：

- `base.py`：算法公共基类或公共接口；
- `ddpg.py`：DDPG；
- `dqn.py`：DQN；
- `td3.py`：TD3；
- `sac.py`：SAC；
- `dsac.py`、`dsact.py`：DSAC 系列；
- `ppo.py`：PPO；
- `trpo.py`：TRPO；
- `fhadp.py`、`fhadp2.py`：FHADP 系列；
- `fhadp_exterior.py`、`fhadp_interior.py`：约束 FHADP 的不同处理形式；
- `fhadp_lagrangian.py`、`fhadp_lagrangiannet.py`：拉格朗日相关 FHADP；
- `infadp.py`：INFADP；
- `mac.py`：MAC；
- `mpg.py`：MPG；
- `rpi.py`：RPI；
- `spil.py`：SPIL。

每个算法模块一般同时定义：

- 算法更新逻辑；
- 损失函数；
- 优化器；
- 与策略/价值网络关联的 `ApproxContainer`；
- 训练与推理需要的参数。

`gops/create_pkg/create_alg.py` 会扫描本目录并自动注册算法，因此训练脚本只需传入类似 `FHADP`、`DDPG` 的算法名称。

#### 3.5.2 `gops/apprfunc/`：近似函数

用于构造算法需要的策略函数、状态价值函数、动作价值函数或分布式价值函数。

主要实现包括：

- `mlp.py`：多层感知机；
- `cnn.py`：卷积神经网络；
- `cnn_shared.py`：共享特征提取层的 CNN；
- `rnn.py`：循环神经网络；
- `poly.py`：多项式近似；
- `gauss.py`：高斯基函数近似；
- `lipsnet.py`：具有 Lipschitz 相关结构的网络。

算法目录负责“如何学习”，本目录负责“用什么函数表示策略和值函数”。两者通过 `create_apprfunc.py` 和算法中的 `ApproxContainer` 组合。

#### 3.5.3 `gops/create_pkg/`：组件工厂和注册中心

该目录将字符串配置转换为实际 Python 对象，是示例脚本与底层实现之间的连接层。

主要文件包括：

- `create_alg.py`：扫描并创建算法；
- `create_apprfunc.py`：创建策略/价值近似函数；
- `create_env.py`：扫描、注册、包装并创建环境；
- `create_env_model.py`：创建可微分或预测环境模型；
- `create_trainer.py`：创建训练器；
- `create_sampler.py`：创建采样器；
- `create_buffer.py`：创建经验回放池；
- `create_evaluator.py`：创建评估器。

该设计使训练脚本可以通过参数选择组件，而不需要直接硬编码每个具体类。

#### 3.5.4 `gops/env/`：环境系统

该目录负责控制任务建模，是项目中层级最丰富的模块。它既包含传统 Gym 环境，也包含 GOPS 自定义最优控制环境和 MATLAB/Simulink 接口。

##### `gops/env/env_gen_ocp/`

新一代组合式最优控制问题环境。它把一个完整控制任务拆分成：

- 机器人或被控对象动力学；
- 任务上下文、参考轨迹或障碍物；
- 环境接口；
- 可供算法预测使用的环境模型。

该目录中的顶层文件，如 `idpendulum.py`、`pendulum.py`、`cartpoleconti.py`、`veh3dof_tracking.py`，负责将各组件组装成具体任务。

其子目录如下：

- `robot/`：物理系统和动力学，例如摆、倒立双摆、车辆、四旋翼和线性二次系统；
- `context/`：参考轨迹、平衡点、跟踪误差、静态障碍物和约束等任务上下文；
- `env_model/`：与真实环境对应的模型，用于模型驱动算法、梯度计算、预测和 MPC。

##### `gops/env/env_gym/`

对 Gym/Gymnasium 经典任务的封装与扩展，例如：

- CartPole、Pendulum、MountainCar；
- LunarLander、BipedalWalker；
- MuJoCo 的 Ant、Hopper、Humanoid、Walker2d 等；
- Atari 类任务；
- CarRacing。

其中 `env_model/` 保存部分 Gym 环境的模型版本，例如连续 CartPole、Pendulum 和连续 MountainCar 模型。

##### `gops/env/env_ocp/`

GOPS 原有的 Python 最优控制环境实现。文件名通常以 `pyth_` 开头，例如：

- `pyth_idpendulum.py`：倒立双摆；
- `pyth_aircraftconti.py`：飞机连续控制；
- `pyth_veh2dofconti.py`、`pyth_veh3dofconti.py`：车辆模型；
- `pyth_mobilerobot.py`：移动机器人；
- `pyth_lq.py`：线性二次系统。

其子目录包括：

- `env_model/`：各环境对应的预测/可微模型；
- `resources/`：参考轨迹、线性二次系统配置和共享资源。

`env_ocp` 与 `env_gen_ocp` 都提供最优控制问题，但建模组织方式不同。前者以一个环境文件集中实现为主，后者更强调 robot、context 和 model 的组件化组合。

##### `gops/env/env_matlab/`

连接 Python 与 MATLAB/Simulink 的环境接口。

顶层 Python 文件负责包装具体仿真模型，例如飞机、车辆和线性二次系统。子目录包括：

- `env_model/`：Simulink 环境对应的 Python 模型接口；
- `resources/`：`.slx` 模型、`.m` 参数脚本、`.toml` 配置、预编译 `.pyd` 模块及类型提示文件。

`resources/` 中按对象划分的目录包括：

- `simu_aircraft/`、`simu_aircraft_v2/`：飞机模型；
- `simu_cartpole/`、`simu_cartpole_v2/`：CartPole；
- `simu_doublemass/`、`simu_doublemass_v2/`：双质量系统；
- `simu_lqs2a1/`：线性二次系统；
- `simu_vehicle3dof/`、`simu_vehicle3dof_v2/`：三自由度车辆。

带 `_v2` 的目录通常包含较新的接口形式、配置文件或预编译模块。此部分可能依赖特定 Python、MATLAB 和 Windows 版本。

##### `gops/env/wrapper/`

环境包装器。在不修改原始环境的情况下，对输入输出行为进行统一或增强。

主要功能包括：

- `action_repeat.py`：重复执行动作；
- `clip_action.py`、`clip_observation.py`：裁剪动作或观测；
- `scale_action.py`、`scale_observation.py`：缩放动作或观测；
- `noise_action.py`、`noise_observation.py`：注入噪声；
- `shaping_reward.py`：奖励平移与缩放；
- `reset_info.py`：统一 reset 信息；
- `convert_type.py`：统一数据类型；
- `unify_state.py`：统一状态表达；
- `mask_at_done.py`：终止状态掩码；
- `transform_constraint.py`：约束表达转换；
- `gym2gymnasium.py`：Gym 与 Gymnasium 接口适配；
- `base.py`：包装器基础定义。

`create_env.py` 会根据训练参数按需应用这些包装器。

##### `gops/env/vector/`

向量化环境实现，用于同时运行多个环境实例以提高采样吞吐量。

- `vector_env.py`：向量环境基础接口；
- `sync_vector_env.py`：同步向量环境；
- `async_vector_env.py`：异步向量环境。

##### `gops/env/inspector/`

环境检查工具，用于在正式训练之前验证环境和模型接口是否一致。

- `env_data_checker.py`：检查环境数据、空间和接口；
- `env_dynamic_checker.py`：检查动力学行为；
- `env_model_checker.py`：检查真实环境与环境模型的一致性。

##### `gops/env/py2slx_tools/`

Python 到 Simulink 的导出与验证工具。

它包含：

- Python 模型导出逻辑；
- Python/Simulink 桥接代码；
- MATLAB 验证脚本；
- 示例与专门的 README。

该目录主要服务于控制器或模型向 Simulink 部署、联调和一致性验证。

#### 3.5.5 `gops/trainer/`：训练编排

训练器负责组织“采样—存储—更新—评估—保存”循环。算法定义单次更新规则，训练器决定这些规则在整个实验中如何被执行。

主要训练器包括：

- `off_serial_trainer.py`：离策略串行训练；
- `off_async_trainer.py`：离策略异步训练；
- `off_sync_trainer.py`：离策略同步并行训练；
- `on_serial_trainer.py`：在线/同策略串行训练；
- `on_sync_trainer.py`：在线/同策略同步并行训练；
- `evaluator.py`：定期独立评估策略并保存评估结果。

##### `gops/trainer/sampler/`

负责与环境交互并产生训练样本：

- `base.py`：采样器基础接口；
- `off_sampler.py`：离策略采样；
- `on_sampler.py`：同策略采样。

##### `gops/trainer/buffer/`

保存和抽取离策略训练数据：

- `replay_buffer.py`：普通经验回放池；
- `prioritized_replay_buffer.py`：优先级经验回放池。

同策略训练通常直接使用最新轨迹；离策略训练通常通过 buffer 重复利用历史数据。

#### 3.5.6 `gops/sys_simulator/`：闭环运行、最优控制和结果绘制

该目录主要被 `example_run/` 调用。

- `sys_run.py`：`PolicyRunner` 的核心实现，负责加载策略、创建环境、执行回合、保存数据、录制视频和绘图；
- `opt_controller.py`：基于环境模型的 MPC/OPT 最优控制器；
- `opt_controller_for_gen_env.py`：面向 `env_gen_ocp` 环境的最优控制器；
- `call_terminal_cost.py`：终端代价相关处理。

当运行 `example_run/run_idp_fhadp.py` 时，该目录会：

1. 从 `results/` 加载 FHADP 策略；
2. 从 `gops/env` 创建倒立双摆环境；
3. 执行策略闭环仿真；
4. 根据 `use_opt=True` 调用 MPC；
5. 比较策略与 MPC 的状态、动作和奖励；
6. 将结果保存到 `figures/`。

#### 3.5.7 `gops/utils/`：通用工具

该目录提供多个模块共用的基础能力：

- `init_args.py`：补全、检查和规范化训练参数；
- `gops_path.py`：统一项目内部路径；
- `gops_typing.py`：类型定义；
- `common_utils.py`：通用函数；
- `math_utils.py`：数学工具；
- `explore_noise.py`：探索噪声；
- `act_distribution_cls.py`、`act_distribution_type.py`：动作概率分布；
- `log_data.py`：训练数据记录；
- `tensorboard_setup.py`：TensorBoard 启动与数据导出；
- `plot_evaluation.py`：训练和评估曲线绘制；
- `parallel_task_manager.py`：并行任务管理；
- `pkl2onnx.py`：将模型检查点转换为 ONNX。

#### 3.5.8 `gops/**/__pycache__/`：Python 字节码缓存

这些目录由 Python 在导入模块时自动生成，其中保存 `.pyc` 字节码缓存，用来加快后续导入。

它们不是源码，也不是实验结果，不应在代码中依赖其具体内容。出现于 `gops` 及多个子目录中是正常现象。

### 3.6 `gops.egg-info/`：安装元数据

该目录通常由以下命令生成：

```powershell
python -m pip install -e .
```

它保存：

- 包名和版本；
- 依赖信息；
- 源文件列表；
- 顶层 Python 包信息；
- 安装工具使用的其他元数据。

它与 `gops/` 的区别是：

- `gops/` 是实际源码；
- `gops.egg-info/` 是安装系统生成的描述信息。

业务代码不应直接写入该目录。

### 3.7 `results/`：训练结果与模型检查点

该目录主要由 `example_train/` 中的训练脚本生成，也包含项目已有的示例策略。

常见组织方式有两种：

```text
results/<算法>/<环境>/...
```

以及：

```text
results/<环境>/<算法_时间戳>/...
```

当前可见的算法或任务目录包括：

- `DDPG/`；
- `DSAC/`；
- `FHADP/`；
- `INFADP/`；
- `RPI/`；
- `SAC/`；
- `SPIL/`；
- `pyth_idpendulum/`。

其内部常见子目录如下：

#### `apprfunc/`

保存策略函数和值函数检查点，通常为 `.pkl` 文件。名称中的数字一般表示训练迭代次数。例如 `apprfunc_54000_opt.pkl` 表示某个迭代点保存的近似函数参数。

`example_run/` 通过策略目录和迭代名称定位这些文件。

#### `evaluator/`

保存训练过程中定期评估产生的数据，例如奖励、回合长度或其他评估指标。

#### `videos/`

某些训练或评估过程保存的视频。是否存在取决于脚本是否启用了渲染和录制。

#### 时间戳目录

例如 `FHADP_260626-015315/`，表示一次独立训练运行。时间戳用于避免不同实验相互覆盖。

需要明确区分：`results/` 保存的是训练阶段输出和可复用模型；`figures/` 主要保存加载模型进行闭环运行后得到的展示与对比结果。

### 3.8 `figures/`：运行、对比和绘图结果

该目录主要由 `gops/sys_simulator/sys_run.py` 在执行 `example_run/` 脚本时生成。

当前结构包括：

```text
figures/
└── FHADP-pyth_idpendulum/
    ├── 260626-191002/
    ├── 260626-204835/
    ├── 260626-223233/
    └── 260626-224014/
```

其中：

- `FHADP-pyth_idpendulum/` 表示算法与环境组合；
- `260626-224014/` 等目录表示每次独立运行的时间戳；
- 每次新运行通常创建新目录，从而保留之前的结果。

典型输出文件包括：

- `Reward.png`、`Reward.csv`：奖励曲线及数据；
- `Action-*.png`、`Action-*.csv`：控制动作；
- `State-*.png`、`State-*.csv`：状态变量；
- `* error.png`、`* error.csv`：策略与最优控制器之间的误差；
- `Error-result.xlsx`：误差汇总；
- `eval_dict_opt.npy`：最优控制器评估数据；
- `tracking_dict_opt.npy`：跟踪相关数据；
- `videos/`：策略和 MPC 的 MP4 视频及元数据。

`figures/` 中的文件属于已经完成的实验结果。查看 PNG、CSV、Excel 或 MP4 不会触发重新计算。

### 3.9 `tests/`：自动化测试

该目录用于验证核心代码的正确性，而不是保存训练结果。

当前主要子目录是 `tests/env_gen_ocp/`，其中的 `test_consistency.py` 用来检查 `env_gen_ocp` 相关真实环境与模型之间的一致性。

测试目录与 `gops/env/inspector/` 的关系是：

- `inspector/` 提供可复用的检查工具；
- `tests/` 将这些检查组织成可以自动执行和判断通过/失败的测试用例。

### 3.10 `说明文档/`：本地中文文档

该目录用于集中保存针对当前代码副本编写的中文说明，例如：

- 安装与环境配置；
- 项目结构；
- 训练与运行流程；
- 常见问题；
- 实验结果说明。

它不参与 GOPS 运行，可用于团队内部知识沉淀。

## 4. 各目录之间的关系

### 4.1 训练流程

```text
example_train/<算法>/<训练脚本>.py
        │
        ├── gops/create_pkg/create_env.py
        │       └── gops/env/*
        │
        ├── gops/create_pkg/create_alg.py
        │       ├── gops/algorithm/*
        │       └── gops/apprfunc/*
        │
        ├── gops/create_pkg/create_sampler.py
        │       └── gops/trainer/sampler/*
        │
        ├── gops/create_pkg/create_buffer.py
        │       └── gops/trainer/buffer/*
        │
        ├── gops/create_pkg/create_evaluator.py
        │       └── gops/trainer/evaluator.py
        │
        └── gops/create_pkg/create_trainer.py
                └── gops/trainer/*_trainer.py
                        │
                        └── results/<环境或算法>/<本次实验>/
```

具体职责为：

1. `example_train` 提供实验参数；
2. `create_pkg` 根据参数查找并创建组件；
3. `env` 提供被控对象和任务；
4. `algorithm` 提供学习规则；
5. `apprfunc` 提供策略/价值函数表示；
6. `sampler` 与环境交互；
7. `buffer` 保存离策略样本；
8. `trainer` 编排整个训练循环；
9. `evaluator` 定期测试策略；
10. 训练检查点和日志进入 `results`。

### 4.2 策略运行与结果生成流程

```text
example_run/run_*.py
        │
        └── gops/sys_simulator/sys_run.py
                ├── 从 results/ 加载策略检查点
                ├── 从 gops/env/ 创建环境
                ├── 从 gops/algorithm/ 恢复网络结构
                ├── 可选调用 opt_controller.py 计算 MPC/OPT
                └── 向 figures/ 写入图、数据、表格和视频
```

以 `run_idp_fhadp.py` 为例：

```text
results/FHADP/idpendulum
        ↓ 加载 54000_opt 策略
gops/sys_simulator/PolicyRunner
        ↓ 创建 pyth_idpendulum 环境并执行策略
gops/sys_simulator/OptController
        ↓ 计算 MPC 对比结果
figures/FHADP-pyth_idpendulum/<时间戳>/
```

### 4.3 环境与环境模型的关系

```text
真实/仿真环境                 环境模型
env_gym/*.py          ↔       env_gym/env_model/*.py
env_ocp/*.py          ↔       env_ocp/env_model/*.py
env_gen_ocp/*.py      ↔       env_gen_ocp/env_model/*.py
env_matlab/*.py       ↔       env_matlab/env_model/*.py
```

- 环境用于产生实际交互数据；
- 环境模型用于算法内部预测、梯度计算、MPC 或一致性验证；
- `tests/env_gen_ocp` 和 `env/inspector` 用来检查两者是否一致。

### 4.4 `results` 与 `figures` 的区别

| 目录         | 产生阶段              | 主要内容                       | 是否可供后续运行加载 |
| ------------ | --------------------- | ------------------------------ | -------------------- |
| `results/` | 训练阶段              | 网络检查点、训练日志、评估数据 | 是                   |
| `figures/` | 策略测试/闭环仿真阶段 | PNG、CSV、Excel、NPY、MP4      | 通常用于分析和展示   |

简化理解：

```text
训练脚本 → results（模型） → 运行脚本 → figures（分析结果）
```

## 5. 常见任务应该进入哪个目录

### 训练一个已有算法

优先从 `example_train/<算法>/` 选择最接近任务的脚本，复制其参数逻辑并调整环境、网络和训练配置。

### 运行已经训练好的策略

进入 `example_run/`，选择相应的 `run_*.py`，并检查其中的：

- `log_policy_dir_list`；
- `trained_policy_iteration_list`；
- `init_info`；
- `save_render`；
- `use_opt` 和 `opt_args`。

### 新增算法

通常需要：

1. 在 `gops/algorithm/` 新增算法模块；
2. 复用或扩展 `gops/apprfunc/`；
3. 在 `example_train/` 添加训练入口；
4. 添加相应测试或运行脚本。

`create_alg.py` 会扫描算法目录，但新模块仍需满足现有的命名和接口约定。

### 新增控制环境

根据建模方式选择：

- 标准 Gym 封装：`gops/env/env_gym/`；
- 原有 GOPS 最优控制环境：`gops/env/env_ocp/`；
- robot/context 组合式环境：`gops/env/env_gen_ocp/`；
- MATLAB/Simulink 环境：`gops/env/env_matlab/`。

如果算法需要环境模型，还应在相应的 `env_model/` 中提供模型实现，并通过 `inspector` 或 `tests` 验证一致性。

### 修改网络结构

通用网络结构位于 `gops/apprfunc/`；某个算法如何组合策略和值函数，通常位于 `gops/algorithm/<算法>.py` 的 `ApproxContainer` 中。

### 查找训练失败或采样异常

可以按以下路径排查：

1. `example_train` 中的参数；
2. `create_pkg` 是否创建了预期组件；
3. `env` 的空间、reset 和 step 返回值；
4. `sampler` 产生的数据；
5. `buffer` 中的数据形状；
6. `algorithm` 的损失与更新；
7. `trainer` 的调用顺序；
8. `results` 中的日志和评估输出。

## 6. 哪些目录是源码，哪些是生成内容

### 应重点阅读和维护的源码目录

- `gops/`；
- `example_train/`；
- `example_run/`；
- `tests/`。

### 运行或工具自动生成的目录

- `results/`；
- `figures/`；
- `gops.egg-info/`；
- 各级 `__pycache__/`。

### 工具配置和仓库管理目录

- `.git/`；
- `.vscode/`。

### 人工维护的说明目录

- `说明文档/`。

在分析代码时，应优先关注源码目录；在查看实验时，应根据时间戳进入 `results` 或 `figures`，避免把不同批次的输出混在一起。

## 7. 总结

GOPS 的核心组织逻辑可以概括为：

```text
环境定义控制问题
    +
近似函数表示策略和值函数
    +
算法定义参数更新规则
    +
采样器、经验池和训练器组织训练
    ↓
results 保存可复用模型
    ↓
系统仿真器加载模型并进行闭环验证
    ↓
figures 保存曲线、误差数据和视频
```

因此：

- `example_train` 是训练入口；
- `example_run` 是验证入口；
- `gops` 是核心实现；
- `results` 是模型与训练记录；
- `figures` 是闭环运行和对比分析结果；
- `tests` 与 `inspector` 用于保证环境和模型正确。
