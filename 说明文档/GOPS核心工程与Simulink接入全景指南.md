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

### 6.1 “根据字符串配置创建组件”是什么意思

训练脚本先提供一组名称：

```text
env_id = simu_aircraftconti
algorithm = DDPG
trainer = off_serial_trainer
sampler_name = off_sampler
buffer_name = replay_buffer
evaluator_name = evaluator
policy_func_type = MLP
value_func_type = MLP
```

这些值最初只是字符串。`create_pkg` 将字符串翻译成真正可运行的 Python 对象：

```text
simu_aircraftconti → Simulink Aircraft 环境对象
DDPG → DDPG 算法对象
MLP → 策略网络和价值网络对象
off_sampler → 离策略采样器对象
replay_buffer → 经验回放池对象
evaluator → 策略评估器对象
off_serial_trainer → 离策略串行训练器对象
```

因此，工厂本质上是一个“名称到组件”的装配系统。训练入口声明想使用什么，工厂负责找到对应实现、传入参数并完成实例化。

### 6.2 Aircraft + DDPG 示例

训练脚本为：

```text
example_train/ddpg/ddpg_mlp_aircraftconti_offserial_slx.py
```

组件创建过程如下：

```text
1. create_env(**args)
   读取 env_id=simu_aircraftconti
   → 找到 env_matlab/simu_aircraftconti.py
   → 创建封装 Aircraft .pyd 的 GymEnv

2. init_args(env, **args)
   → 从环境读取观测维度、动作维度和动作上下界
   → 将这些信息补充到 args

3. create_alg(**args)
   读取 algorithm=DDPG
   → 找到 algorithm/ddpg.py
   → 创建 DDPG 算法对象

4. 创建近似函数
   读取 policy_func_type/value_func_type=MLP
   → 使用 apprfunc/mlp.py
   → 创建策略网络和动作价值网络

5. create_sampler(**args)
   读取 sampler_name=off_sampler
   → 创建离策略采样器
   → 采样器用策略与 Aircraft 环境交互

6. create_buffer(**args)
   读取 buffer_name=replay_buffer
   → 创建经验回放池

7. create_evaluator(**args)
   → 创建独立评估器

8. create_trainer(..., **args)
   读取 trainer=off_serial_trainer
   → 把算法、采样器、经验池和评估器组装起来
```

最终关系为：

```text
Aircraft Simulink 环境
        ↕ reset/step
离策略采样器
        ↓ 交互数据
经验回放池
        ↓ 训练批次
DDPG 算法 + MLP 网络
        ↓ 更新后的策略
评估器和结果保存
```

### 6.3 使用工厂的价值

- 更换环境只需修改 `env_id`；
- 更换算法只需修改 `algorithm`；
- 训练模式可以通过 `trainer` 切换；
- 同一框架可以复用不同环境、网络和算法；
- 新增符合约定的文件后可以被扫描和注册机制发现；
- 实验配置可统一保存到 `config.json`。

工厂本身不负责训练，也不实现算法。它只负责发现、创建和组装组件。

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

### 7.7 这些模块不是并列使用，而是分层协作

#### 7.7.1 四个 `env_*` 目录是否同级

是的，以下四个目录在文件结构和环境注册机制上是同级的：

```text
gops/env/
├── env_gym/
├── env_ocp/
├── env_gen_ocp/
└── env_matlab/
```

但“同级”只表示它们都是环境来源目录，不表示一次训练必须同时使用四个目录。它们更像四个不同类型的环境库：

| 环境库          | 环境从哪里来                      | 典型用途                     |
| --------------- | --------------------------------- | ---------------------------- |
| `env_gym`     | Gym、MuJoCo、Atari 等标准任务     | 通用强化学习算法测试         |
| `env_ocp`     | GOPS 早期集中式 Python 控制模型   | 现有 OCP 示例和旧策略兼容    |
| `env_gen_ocp` | Robot、Context、EnvModel 组合     | 新建复杂跟踪、约束和避障任务 |
| `env_matlab`  | Simulink 模型编译出的 Python 扩展 | 用现有 Simulink 对象交互训练 |

#### 7.7.2 一次运行是否只使用其中一个

通常是。训练脚本通过一个 `env_id` 选择一个具体环境：

```text
env_id=gym_pendulum
  → 选择 env_gym/gym_pendulum.py

env_id=pyth_idpendulum
  → 选择 env_ocp/pyth_idpendulum.py

env_id=idpendulum
  → 选择 env_gen_ocp/idpendulum.py

env_id=simu_aircraftconti
  → 选择 env_matlab/simu_aircraftconti.py
```

因此，一次普通训练不会同时把四种环境混合成一个环境。`create_env.py` 虽然会扫描四个目录并建立完整环境注册表，但实际创建对象时只按当前 `env_id` 选择一个具体环境。

这里有两个容易混淆的例外：

1. ==sampler== 和 evaluator 可能各自创建同一个 `env_id` 的独立实例，但它们仍表示同一个任务；
2. 模型驱动算法可能同时使用“数据环境”和同任务的 `env_model`，但这不是选择两个不同任务，而是分别用于真实交互和内部预测。

#### 7.7.3 为什么需要分成四类

因为不同来源的模型在创建方式、数据接口和依赖上有本质差异：

- Gym 环境通常调用标准 Gym 接口；
- `env_ocp` 直接用 Python/NumPy/PyTorch 描述控制问题；
- `env_gen_ocp` 将机器人动力学和任务上下文拆开，便于复用；
- `env_matlab` 需要加载由 Simulink 编译得到的 `.pyd`。

如果全部混在一个目录中，会出现以下问题：

- 很难判断环境依赖 Gym、Python 方程还是 MATLAB/Simulink；
- 新旧 GOPS 环境架构混杂；
- 数据环境和可求导模型难以配对；
- Simulink 的 `.slx/.toml/.pyd` 资产难以管理；
- 不同环境的安装和部署要求无法清晰区分。

所以这种细分主要是为了管理不同来源和架构，并不是为了让一次训练同时使用更多环境。

#### 7.7.4 最简单的理解方式

可以把四个目录理解成四个“环境供应商”：

```text
训练脚本
  ↓ 提供 env_id
create_env 环境工厂
  ↓ 从四个环境库的注册表中选一个
某个具体环境
  ↓ 统一包装
sampler 与它执行 reset/step 交互
```

如果当前目标是将自己的 Simulink 模型接入 GOPS，主要关注：

```text
env_matlab/
env_matlab/resources/
create_env.py
example_train/*_slx.py
```

一般不需要同时修改 `env_gym`、`env_ocp` 和 `env_gen_ocp`。

`env` 内部可以按四层理解：

```text
第一层：具体任务来源
  env_gym / env_ocp / env_gen_ocp / env_matlab
          ↓ 选择其中一个具体环境

第二层：统一接口和数据含义
  wrapper
          ↓ 包装 reset、step、动作、观测、奖励

第三层：运行方式
  单环境 或 vector 同步/异步多环境
          ↓

第四层：验证工具
  inspector 检查接口、动力学和环境模型一致性
```

四个 `env_*` 目录不是必须同时使用的四个部件，而是四种“环境来源”。一次实验通常选择其中一个具体环境：

```text
经典强化学习任务       → env_gym
旧式 Python 控制模型    → env_ocp
组件化新控制模型       → env_gen_ocp
Simulink 编译模型       → env_matlab
```

选择环境之后，`wrapper` 再把它整理成 GOPS 统一接口；如需并行采样，最外面再套 `vector`；`inspector` 只用于检查，不参与正式训练循环。

### 7.8 环境系统有两条不同的数据路线

#### 路线一：数据环境路线

```text
外部真实/仿真对象
  ↓
某个 env_* 数据环境
  ↓
wrapper
  ↓
可选 vector
  ↓
sampler
  ↓
buffer / trainer / algorithm
```

这条路线负责产生真实训练样本。无论底层是 Simulink 还是 Python，最终都输出：

```text
(当前观测, 动作, 奖励, 下一观测, 是否终止, 附加信息)
```

#### 路线二：环境模型路线

```text
env_*/env_model/<环境模型>
  ↓
create_env_model
  ↓
模型包装器
  ↓
FHADP / INFADP / MPC 等模型驱动算法
```

这条路线不负责与真实仿真器逐步交互，而是在算法内部预测“给定状态和动作后会发生什么”。它通常使用 PyTorch Tensor，可以批量计算并支持梯度。

两条路线的区别：

| 对比项                        | 数据环境                                 | 环境模型                      |
| ----------------------------- | ---------------------------------------- | ----------------------------- |
| 主要使用者                    | ==sampler==、evaluator、PolicyRunner | 模型驱动算法、MPC             |
| 主要接口                      | `reset()`、`step()`                  | 状态预测、奖励和终止计算      |
| 常见数据                      | NumPy                                    | PyTorch Tensor                |
| 是否产生真实样本              | 是                                       | 否，产生内部预测              |
| 是否要求可求导                | 通常不要求                               | 通常要求                      |
| Simulink`.pyd` 能否直接提供 | 能提供数据环境                           | 通常不能提供 PyTorch 梯度模型 |

模型自由算法只使用第一条路线；模型驱动算法通常两条路线都使用。

#### 7.8.1 什么是模型自由算法

模型自由算法（Model-Free）不要求算法获得环境的显式状态转移方程。它通过反复与环境交互，收集：

```text
(当前观测, 动作, 奖励, 下一观测, 是否终止)
```

再根据这些样本更新策略或价值函数。

它的基本逻辑是：

```text
策略尝试动作
  → 环境返回结果
  → 算法判断这个动作好不好
  → 更新策略
  → 继续试验
```

算法只需要能够调用环境的 `reset()` 和 `step()`，不需要知道下一状态具体由什么微分方程计算，也不需要对环境本身求梯度。

本项目中的典型模型自由算法包括：

- DQN；
- DDPG；
- TD3；
- SAC、DSAC；
- PPO；
- TRPO；
- MAC、MPG 等主要依赖交互样本的算法。

以 DDPG + Simulink Aircraft 为例：

```text
DDPG 输出 action
  → 编译后的 Simulink 环境执行 action
  → 返回 observation、reward、done
  → 样本进入 replay buffer
  → DDPG 从样本中更新策略和值函数
```

DDPG 不需要读取 Aircraft 的质量矩阵、气动力方程或 Simulink 内部结构。对它来说，Aircraft 环境可以是一个黑盒。

模型自由并不表示算法内部没有神经网络或“模型”一词。例如 DDPG 的策略网络、价值网络都是函数近似器，但它们不是环境动力学模型。这里的“模型自由”专指算法不依赖显式的环境状态转移模型。

模型自由算法的主要特点：

| 特点               | 说明                               |
| ------------------ | ---------------------------------- |
| 接入要求           | 只需可靠的 reset/step 交互接口     |
| 环境可否是黑盒     | 可以                               |
| 是否要求环境可求导 | 不要求                             |
| Simulink 接入难度  | 相对较低，编译为 GymEnv 后即可采样 |
| 数据需求           | 通常需要较多交互样本               |
| 模型误差           | 不受单独环境模型误差影响           |

#### 7.8.2 什么是模型驱动算法

模型驱动或基于模型的算法（Model-Based）不仅使用真实交互样本，还需要一个可以在算法内部调用的环境模型。这个模型描述：

```text
给定当前状态 x 和动作 u
  → 下一状态 x_next 是什么
  → 奖励 reward 是什么
  → 是否终止 done
  → 约束值是多少
```

其基本逻辑是：

```text
先在真实/数据环境中获得状态
  → 在内部环境模型中尝试或预测多个动作
  → 利用预测结果、梯度或优化器选择更好的策略
  → 再回到数据环境验证
```

在 GOPS 中，模型驱动算法常通过 `create_env_model()` 获得与数据环境配对的 Tensor 模型。该模型通常需要：

- 支持批量状态和动作；
- 使用 PyTorch Tensor；
- 与数据环境具有相同状态顺序和动作含义；
- 具有一致的动力学、奖励、终止条件和约束；
- 在需要策略梯度或最优控制求解时能够参与自动微分。

本项目中典型的模型驱动用途包括：

- FHADP；
- INFADP；
- 部分 RPI 或基于已知系统模型的控制算法；
- `sys_simulator` 中的 MPC/OPT 最优控制器。

以倒立双摆 FHADP 为例：

```text
pyth_idpendulum.py
  → 数据环境，负责真实 reset/step 和采样

pyth_idpendulum_model.py
  → 环境模型，负责算法内部批量预测和梯度计算

FHADP
  → 同时使用两者训练和验证策略
```

模型驱动算法的主要特点：

| 特点               | 说明                                         |
| ------------------ | -------------------------------------------- |
| 接入要求           | reset/step 数据环境 + 一致的内部环境模型     |
| 环境可否完全是黑盒 | 通常不可以，除非另外学习代理模型             |
| 是否要求环境可求导 | GOPS 中许多此类算法要求或受益于可求导模型    |
| Simulink 接入难度  | 较高，`.pyd` 通常不能直接提供 PyTorch 梯度 |
| 数据需求           | 正确模型可减少真实交互需求                   |
| 主要风险           | 环境模型与真实 Simulink 不一致会产生模型偏差 |

#### 7.8.3 两者对 Simulink 接入的直接影响

如果只有一个现成 Simulink 模型，建议先判断目标算法属于哪一类：

```text
使用 DDPG/SAC/PPO 等模型自由算法
  → 将 .slx 编译为 .pyd/GymEnv
  → 实现 env_matlab 数据环境包装
  → 可以开始交互训练

使用 FHADP/INFADP/MPC 等模型驱动方法
  → 除上述数据环境外
  → 还要实现同任务的 Python/PyTorch env_model
  → 验证 env_model 与 Simulink 逐步一致
  → 才能可靠训练或优化
```

因此，对于首次将自有 Simulink 模型接入 GOPS，模型自由算法通常是更合理的第一步。先把观测、动作、奖励、终止和 reset 跑通，再决定是否值得额外维护一套可求导环境模型。

#### 7.8.4 “模型驱动”不等于模型一定已知

环境模型可能来自：

1. 已知物理方程，直接用 PyTorch 重写；
2. 与 Simulink 同源的数学模型；
3. 使用 Simulink 交互数据辨识出的代理模型。

但第三种方式会引入模型误差，算法可能利用代理模型的误差得到在模型中很好、在真实 Simulink 中较差的策略。因此需要持续使用数据环境进行一致性验证和闭环测试。

### 7.9 `create_env` 是环境模块对训练系统的统一出口

训练脚本和 ==sampler== 通常不直接导入 `env_ocp` 或 `env_matlab` 中的具体类，而是调用：

```text
create_env(env_id, 其他参数)
```

它完成以下工作：

```text
1. 根据 env_id 找到具体环境
2. 创建原始环境
3. 统一 reset 返回值
4. 添加最大回合步数
5. 统一数据类型和 state 表达
6. 按配置处理动作、观测、奖励和噪声
7. 按需创建同步或异步向量环境
8. 返回最终环境对象
```

因此外部训练模块只面对一个稳定接口，不需要理解底层来自 Gym、Python 方程还是 Simulink。

### 7.10 外界如何与环境交互

环境对外只暴露少量概念：

```text
observation_space：智能体能看到什么、维度是多少
action_space：智能体能输出什么、范围是多少
reset：开始一个新回合
step：执行一次动作并推进环境
render：可选的画面输出
close：释放资源
```

完整交互顺序为：

```text
trainer 要求 sampler 收集数据
        ↓
sampler 调用 env.reset()
        ↓
环境返回初始 observation
        ↓
sampler 把 observation 交给策略网络
        ↓
策略网络输出 action
        ↓
sampler 对 action 添加探索噪声并按 action_space 裁剪
        ↓
sampler 调用 env.step(action)
        ↓
环境把 action 传给动力学/Simulink
        ↓
环境返回 next_observation、reward、done、info
        ↓
sampler 把这次交互记录为 Experience
        ↓
done=False：继续下一步
done=True：再次 reset，开始新回合
```

`trainer` 不直接操作 Simulink，`algorithm` 也不直接调用 Simulink。真正执行交互的是 ==sampler== 与最终环境对象。

### 7.11 wrapper 在交互中的位置

wrapper 像逐层套在原始环境外面的转换器：

```text
策略输出动作
  ↓ ScaleAction：归一化动作转物理动作
原始环境/Simulink 执行动作
  ↓ 得到原始状态和奖励
ScaleObservation：变换观测
ShapingReward：变换奖励
Noise：可选添加观测噪声
  ↓
sampler 得到最终 observation 和 reward
```

例如策略输出范围为 `[-1,1]`，而执行器实际范围为 `[-500,500]`，动作包装器可以完成映射。观测包装器也可以将不同量纲归一化后再交给网络。

wrapper 不改变底层模型文件，而是在输入和输出边界上转换数据。因此训练和部署必须使用相同的缩放和预处理。

### 7.12 vector 与普通环境的关系

普通环境一次推进一个仿真实例：

```text
一个 action → 一个环境 → 一个 next_obs
```

向量环境同时推进多个独立实例：

```text
一批 actions → N 个环境 → 一批 next_obs
```

- `SyncVectorEnv`：等待所有环境完成当前 step 后统一返回；
- `AsyncVectorEnv`：多个环境通常在不同执行单元中运行。

向量环境只改变“同时运行多少个环境”，不改变单个环境的 observation、action、reward 和 done 定义。

对新 Simulink 环境，应先完成单环境串行训练；只有确认 `.pyd` 能安全创建多个独立实例后，才使用向量或异步环境。

### 7.13 inspector 与正式训练的关系

inspector 是环境的质量检查工具：

```text
环境开发完成
  → inspector 检查空间和接口
  → 检查多步动力学是否合理
  → 如有 env_model，再检查数据环境与模型一致性
  → 检查通过后进入正式训练
```

它不会更新策略，也不会保存训练模型。它的作用是避免把环境接口或动力学错误误判为算法训练失败。

### 7.14 Simulink Aircraft 的完整环境交互例子

```text
训练脚本设置 env_id=simu_aircraftconti
        ↓
create_env 找到 env_matlab/simu_aircraftconti.py
        ↓
该文件加载 aircraft.cp38-win_amd64.pyd
        ↓
.pyd 内部执行由 aircraft.slx 生成的模型代码
        ↓
create_env 在外层添加 GOPS wrapper
        ↓
off_sampler 获得最终环境
        ↓
DDPG 策略根据 aircraft observation 输出 action
        ↓
env.step(action) 将动作送入编译后的 Simulink 模型
        ↓
模型推进一个控制步并返回 State、Reward、Done
        ↓
wrapper 和环境入口整理返回值
        ↓
sampler 保存 Experience 到 replay buffer
        ↓
DDPG 从 replay buffer 取批次更新网络
```

这里各模块的边界很明确：

- `.slx/.pyd`：负责飞机仿真；
- `env_matlab/simu_aircraftconti.py`：负责把仿真包装为 GOPS 环境；
- `wrapper`：负责统一接口和数据范围；
- `sampler`：负责反复交互并记录数据；
- `buffer`：负责保存历史数据；
- `algorithm`：负责更新网络；
- `trainer`：负责组织整个周期。

### 7.15 倒立双摆的双路线例子

```text
数据路线：
pyth_idpendulum.py
 → create_env
 → sampler
 → 产生真实交互样本

模型路线：
env_model/pyth_idpendulum_model.py
 → create_env_model
 → FHADP 或 MPC
 → 批量预测和计算梯度
```

两个文件描述同一个物理问题，但服务对象不同。数据环境负责“实际走一步”，环境模型负责“算法内部预测一步”。二者的状态顺序、动作范围、动力学、奖励、终止条件和时间步必须一致。

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

| 文件           | 作用                                           |
| -------------- | ---------------------------------------------- |
| `*.slx`      | 原始 Simulink 模型                             |
| `model.toml` | slxpy 的模型代码生成配置                       |
| `env.toml`   | Action/State/Reward/Done 和 Gym 空间配置       |
| `*.pyd`      | 编译后的 Windows Python 扩展，训练时实际执行   |
| `*.m`        | 模型参数初始化脚本                             |
| `*.pyi`      | 自动生成的类型提示，帮助查看扩展提供的类和参数 |
| `.gitignore` | 该资产目录的 Git 忽略规则                      |

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

### 8.5 Aircraft 模型中的强化学习接口

对 `simu_aircraft_v2/aircraft.slx` 的只读检查表明，模型顶层已经提供：

```text
输入：
  Action
  AdverAction

输出：
  State
  Reward
  Done
```

`env.toml` 选择其中四个信号构成 Gym 环境：

```text
Action → 智能体动作
State  → 智能体观测
Reward → 单步奖励
Done   → 回合物理终止信号
```

`AdverAction` 虽存在于模型，但当前 `env.toml` 没有把它配置为标准 Gym action，因此普通 Aircraft DDPG 训练不会使用该输入。

当前空间定义为：

```text
action_space.shape = (1,)
action ∈ [-1,1]

observation_space.shape = (2,)
observation ∈ (-∞,+∞)
```

此外，GOPS 的 `simu_aircraftconti.py` 将最大回合长度设为 200 步。回合可能因为 Simulink 的 Done 提前结束，也可能因为达到 200 步而被 TimeLimit 截断。

### 8.6 Aircraft 初始条件如何从 Simulink 传到 GymEnv

当前模型采用参数 `x_ini` 表示初始状态。模型内部两个积分器分别使用：

```text
第一个状态积分器 InitialCondition = x_ini(1)
第二个状态积分器 InitialCondition = x_ini(2)
```

模型同时把 `x_ini` 声明为可调模型参数/模型参数实参。其关系为：

```text
env.toml 生成或设置 x_ini
  ↓
编译后的模型实例参数 x_ini
  ↓
Simulink 两个积分器的 InitialCondition
  ↓
reset 后的初始 Aircraft 状态
```

`env.toml` 当前写法是：

```toml
[reset]
first_step = true

[parameter]
    [parameter.x_ini]
        type = uniform
        low = -0.05
        high = 0.05
```

含义是：

- 每次调用 `GymEnv.reset()` 时，slxpy 重置模型实例；
- 为 `x_ini` 按均匀分布生成初值；
- 每个状态初值位于 `[-0.05,0.05]`；
- 初始化完成后，根据 `first_step=true` 执行首次模型步以取得初始 observation。

所以当前 Aircraft 示例的初始条件不是在训练脚本中定义，而是在 `.slx` 的 `x_ini` 参数和 `env.toml` 的 `[parameter.x_ini]` 两处共同定义。

### 8.7 以 Aircraft 为模板时，各文件如何产生

这些文件并不是一次性自动从 `.slx` 全部生成。它们分为“人工准备”“工具生成”“GOPS 人工适配”三类。

#### 第一步：人工准备 `.slx`

在 Simulink 中完成：

1. 建立 Aircraft 动力学；
2. 设置 `Action` 输入；
3. 设置 `State`、`Reward`、`Done` 输出；
4. 创建初始状态参数 `x_ini`；
5. 各状态积分器的初值引用 `x_ini(i)`；
6. 将 `x_ini` 设置为代码生成后仍可调整的模型参数；
7. 确认模型支持 Simulink Coder/Embedded Coder 代码生成。

这一阶段得到：

```text
aircraft.slx
```

#### 第二步：建立 slxpy 工程配置

运行 `slxpy init` 后形成或填写：

```text
model.toml
env.toml
```

`model.toml` 负责：

- 指定模型名 `aircraft`；
- 指定求解器和连续时间等代码生成特征；
- 指定生成 C++ 类名。

`env.toml` 负责：

- `mkdir` 是“创建文件夹”的命令，来自英文 **make directory**`mkdir奖励：|error| < 0.1：奖励：|error| < 0.1：奖励：|error| < 0.1：+10 否则：-1 越界额外：-100 因此范围 [-101, 10]：正确。+10 否则：-1 越界额外：-100 因此范围 [-101, 10]：正确。+10 否则：-1 越界额外：-100 因此范围 [-101, 10]：正确。` 是“创建文件夹”的命令，来自英文 **make directory**`mkdir` 是“创建文件夹”的命令，来自英文 **make directory**指定 Action/State/Reward/Done 信号；
- 声明 observation/action space；
- 声明 reset 行为；
- 声明 `x_ini` 的初始化分布。

这两个 TOML 是人工配置文件，不是 GOPS 训练脚本自动生成的。

#### 第三步：MATLAB 生成模型代码

在 MATLAB 中执行：

```matlab
workdir = 'simu_aircraft_v2所在的slxpy工程路径';
slxpy.setup_config(workdir);
slxpy.codegen(workdir);
```

这一阶段根据 `.slx + model.toml` 生成 C/C++ 模型代码。

#### 第四步：生成 Python 绑定并编译

在目标 Conda 环境中执行：

```powershell
cd <slxpy工程目录>
slxpy generate
python setup.py build
```

这一阶段根据生成代码和 `env.toml` 形成 Python/Gym 包装，最终产生类似：

```text
aircraft.cp38-win_amd64.pyd
```

`cp38` 表示它适用于 CPython 3.8；换 Python 版本通常需要重新编译对应 ABI 的 `.pyd`。

#### 第五步：人工编写 GOPS 入口

创建：

```text
gops/env/env_matlab/simu_aircraftconti.py
```

该文件导入 `GymEnv`，设置 `EnvSpec`，并通过 `env_creator()` 交给 `create_env.py` 注册。

#### 第六步：人工编写训练配置

创建或复制：

```text
example_train/ddpg/ddpg_mlp_aircraftconti_offserial_slx.py
```

设置：

```text
env_id=simu_aircraftconti
algorithm=DDPG
```

至此，训练脚本通过 `env_id` 找到 Python 入口，Python 入口加载 `.pyd`，`.pyd` 执行由 `.slx` 生成的模型。

### 8.8 初始条件的两种管理方式

#### 方式 A：完全由 `env.toml` 随机化

当前 Aircraft 示例使用这种方式：

```text
训练脚本调用 env.reset()
  → GymEnv 根据 env.toml 随机生成 x_ini
  → 编译模型用 x_ini 初始化积分器
  → 返回初始 State
```

优点：配置简单，适合固定分布训练。

限制：当前 `simu_aircraftconti.py` 只是直接返回 `GymEnv`，没有接收 `init_state` 的自定义 reset，因此调用者不能方便地指定某个固定初值。

#### 方式 B：由 GOPS Python 包装层显式传入初值

如果希望训练、评估或运行脚本能够指定：

```text
init_state=[x1,x2]
```

就需要参考：

```text
env_matlab/simu_lqs2a1conti.py
env_matlab/simu_veh3dofconti.py
```

在 Aircraft 外再写一层环境类，其 reset 流程应是：

```text
1. reset 接收 init_state
2. 若未提供，则按训练分布采样
3. 在 GymEnv reset callback 中把值写入模型实例的 x_ini
4. 调用底层 GymEnv.reset(callback)
5. 返回初始化后的 observation
```

概念关系为：

```text
run/train 脚本中的 init_state
  ↓
simu_aircraftconti.py 的 reset
  ↓
reset callback
  ↓
编译模型实例参数 x_ini
  ↓
Simulink 积分器初值
```

这种方式更适合：

- 测试固定初始状态；
- 复现实验；
- 分别设置训练和测试初始区域；
- 课程学习；
- 根据场景动态改变初值。

使用方式 B 时，必须确认编译扩展暴露的参数属性名称。Aircraft `.slx` 中参数叫 `x_ini`，但 Python 中的完整访问路径由 slxpy 生成结果决定，应检查生成的 `.pyi` 或模型类属性，不能直接假设与其他模型的 `vehicle3dof_InstP.x_ini` 完全相同。

### 8.9 初始条件设置时最容易犯的错误

1. 只在 MATLAB 工作区定义 `x_ini`，却没有把它设置为可调模型参数，导致编译后无法在 reset 时修改；
2. 积分器仍写死常数初值，没有引用 `x_ini(i)`；
3. `x_ini` 维度与状态积分器数量不一致；
4. TOML 中设置了随机参数，但 GOPS 包装层又覆盖一次，导致初始分布与预期不符；
5. reset 只修改外部参数，没有彻底重置积分器、延迟或滤波器内部状态；
6. 训练使用随机初值，评估却没有记录固定初值，导致结果无法比较；
7. 修改 `.slx` 或模型参数接口后，仍然使用旧 `.pyd`，造成模型与配置不一致。

### 8.10 Aircraft 文件之间的最终关系

```text
aircraft.slx
  ├─定义→ 动力学、Action、State、Reward、Done
  └─定义→ x_ini 与积分器初值关系

model.toml
  └─定义→ 如何把 aircraft.slx 生成 C/C++

env.toml
  ├─定义→ Gym 信号和空间
  └─定义→ reset 时如何生成 x_ini

aircraft.cp38-win_amd64.pyd
  └─实现→ 可被 Python 高速调用的编译环境

simu_aircraftconti.py
  └─实现→ GOPS 环境入口和 EnvSpec

create_env.py
  └─注册→ env_id=simu_aircraftconti

ddpg_mlp_aircraftconti_offserial_slx.py
  └─选择→ Aircraft 环境 + DDPG 训练配置
```

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

Simulink 模型与智能体真正发生交互的位置是 ==sampler== 调用 `env.reset()` 和 `env.step(action)`。训练器只负责何时采样、何时更新和何时保存。

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
