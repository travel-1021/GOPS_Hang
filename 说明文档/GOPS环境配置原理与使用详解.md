# GOPS 环境配置原理与使用详解

## 1. 环境配置主线

GOPS 的环境配置不是集中在单个配置文件，而由五层共同决定：

1. `example_train/*.py` 或 `example_run/*.py` 选择环境并提供参数；
2. `gops/create_pkg/create_env.py` 根据 `env_id` 创建环境并添加包装器；
3. `gops/env/env_*/<环境名>.py` 定义真实交互环境；
4. `gops/env/env_*/env_model/<环境名>_model.py` 定义算法内部使用的环境模型；
5. `gops/utils/init_args.py` 从最终环境推导网络维度和结果路径。

```text
训练脚本参数 → create_env → 原始环境与包装器 → init_args → 算法和训练器
```

必须区分环境参数、负责真实采样的数据环境，以及供模型驱动算法和 MPC 使用的 Tensor 环境模型。

## 2. `env_id` 如何选择环境

训练脚本通常执行：

```python
parser.add_argument(--env_id, type=str, default=pyth_idpendulum)
args = vars(parser.parse_args())
env = create_env(**args)
args = init_args(env, **args)
```

`create_env.py` 扫描 `gops/env` 下所有以 `env_` 开头的目录，将 `.py` 文件名注册为环境 ID：

```text
pyth_idpendulum ↔ gops/env/env_ocp/pyth_idpendulum.py
idpendulum      ↔ gops/env/env_gen_ocp/idpendulum.py
gym_pendulum    ↔ gops/env/env_gym/gym_pendulum.py
```

环境模块通常提供：

```python
def env_creator(**kwargs):
    return YourEnvironment(**kwargs)
```

GOPS 使用内部注册表，不是单纯依赖 `gym.make()`。文件不存在、导入失败或缺少创建入口时，会出现 `No registered env with id`。其他参数会通过 `**kwargs` 传给环境，具体环境通过 `kwargs.get()` 或 `kwargs.pop()` 读取专用参数。

## 3. 四类环境

### 3.1 `env_ocp`

`gops/env/env_ocp/` 是原有 Python 最优控制环境。典型 ID 包括 `pyth_idpendulum`、`pyth_aircraftconti`、车辆、移动机器人和线性二次系统。

它通常在单个环境文件中集中定义状态、动作、动力学、奖励、终止条件和渲染；对应模型在 `env_ocp/env_model/`。当前 FHADP 倒立双摆案例使用这一体系。

### 3.2 `env_gen_ocp`

`gops/env/env_gen_ocp/` 将任务拆成：

```text
robot/       被控对象、动力学和物理参数
context/     参考轨迹、平衡点、障碍物和约束
env_model/   Tensor 环境模型
*.py         将 Robot 与 Context 组合成任务
```

完整状态包含：

```python
State(
    robot_state=...,
    context_state=ContextState(
        reference=...,
        constraint=...,
        t=...,
    ),
)
```

- `robot_state`：物理系统状态；
- `reference`：目标或参考轨迹；
- `constraint`：场景约束；
- `t`：轨迹时间索引。

这种结构更适合复杂跟踪、避障和约束任务。

### 3.3 `env_gym`

`gops/env/env_gym/` 封装 CartPole、Pendulum、MountainCar、LunarLander、MuJoCo、Atari 和 CarRacing 等经典环境。部分环境有对应模型。

### 3.4 `env_matlab`

`gops/env/env_matlab/` 连接 Python 与 MATLAB/Simulink。`resources/` 保存 `.slx`、`.m`、`.toml` 和预编译 `.pyd`。名称带 `_slx` 的训练示例通常使用此类环境，还需关注 MATLAB、Python 和预编译模块的版本兼容性。

## 4. 原始环境接口

### 4.1 观测空间

`observation_space` 定义策略看到的数据形状和范围。`init_args()` 据此生成 `obsv_dim`，直接决定策略和值网络输入维度。

```python
self.observation_space = gym.spaces.Box(
    low=np.array([-np.inf] * 6, dtype=np.float32),
    high=np.array([np.inf] * 6, dtype=np.float32),
)
```

### 4.2 动作空间

`action_space` 定义动作类型、范围和维度。连续 `Box` 空间生成 `action_type=continu`、`action_dim` 和上下界；离散 `Discrete(n)` 生成 `action_num=n`。

### 4.3 `reset()` 与 `step()`

```python
obs, info = env.reset(...)
next_obs, reward, done, info = env.step(action)
```

旧环境若只由 `reset()` 返回 `obs`，`ResetInfoData` 会转成 `(obs,{})`。

- `next_obs`：下一观测；
- `reward`：本步奖励；
- `done`：回合是否结束；
- `info`：约束、原始状态等附加信息。

`render()` 负责显示或返回 RGB 图像，`close()` 释放资源，`max_episode_steps` 限制单回合最大步数。

## 5. `create_env()` 的包装顺序

```text
原始环境 env_creator(**kwargs)
 → ResetInfoData
 → TimeLimit（若有最大步数）
 → ActionRepeatData（若 repeat_num 非 None）
 → ConvertType
 → StateData
 → ShapingRewardData（若设置奖励变换）
 → NoiseData（若设置观测噪声）
 → ScaleObservationData（若设置观测变换）
 → ScaleActionData（连续动作且 action_scale=True）
 → Gym2Gymnasium（若启用）
 → SyncVectorEnv / AsyncVectorEnv（若启用）
```

算法面对的是最外层最终环境，包装顺序会影响数据含义。

## 6. 通用环境参数

### 6.1 `max_episode_steps`

指定回合最大步数；为 `None` 时读取环境自身属性。

### 6.2 `reward_shift`、`reward_scale`

```text
最终奖励 = (原始奖励 + reward_shift) × reward_scale
```

原始奖励保存到 `info[raw_reward]`。缩放会真实改变算法的学习信号。

### 6.3 `repeat_num`、`sum_reward`

同一策略动作连续执行 n 个环境步。`sum_reward=True` 返回奖励总和，否则只返回最后一步奖励。它不同于动力学内部积分的 `discrete_num`，两者可能叠加。

### 6.4 `obs_shift`、`obs_scale`

```text
最终观测 = (原始观测 + obs_shift) × obs_scale
```

原始观测保存在 `info[raw_obs]`。数据环境和环境模型必须使用相同变换。

### 6.5 观测噪声

`obs_noise_type` 支持 `normal` 和 `uniform`，`obs_noise_data` 分别为 `[mean,std]` 或 `[low,high]`。数组长度必须等于观测维度。噪声只加入数据环境观测。

### 6.6 动作缩放

`action_scale=True` 时，策略输出 `[min_action,max_action]`（默认 `[-1,1]`）线性映射到原始动作空间：

```text
真实动作 = 原始下界 + (原始上界-原始下界)
         × (策略动作-min_action)/(max_action-min_action)
```

若原始动作也是 `[-1,1]`，数值不变，但仍会裁剪。

### 6.7 Gymnasium 和向量环境

`gym2gymnasium=True` 用于接口适配。当前旧示例主要使用 Gym 0.23 风格，不应无理由开启。

`vector_env_num` 指定环境数量，`vector_env_type` 可为 `sync` 或 `async`。向量环境应与采样器和训练器的数据形状配套。

## 7. 数据环境和环境模型

### 7.1 数据环境

由 `create_env()` 创建，使用 NumPy 数据，通过 `reset/step` 为 sampler、evaluator 和 PolicyRunner 产生真实轨迹。

### 7.2 环境模型

由 `create_env_model(env_id)` 创建，通常使用 PyTorch Tensor、支持批量和梯度，用于 FHADP、INFADP、MPC 等模型驱动方法。

它按名称查找：

```text
pyth_idpendulum
 → env_ocp/pyth_idpendulum.py
 → env_ocp/env_model/pyth_idpendulum_model.py
```

模型文件需要提供 `env_model_creator(**kwargs)`。

### 7.3 模型包装器

`create_env_model()` 依次应用 `MaskAtDoneModel`、动作重复、奖励缩放、观测缩放、观测裁剪、动作裁剪和动作缩放。

数据环境与模型的动力学、奖励、终止条件、时间步、动作/观测缩放必须一致，否则模型梯度、MPC 预测和真实轨迹会不一致。

## 8. `init_args()` 从环境推导什么

创建环境后，`init_args()` 自动得到：

- `obsv_dim`：观测维度；
- `action_type`：连续或离散；
- `action_dim`、`action_num`；
- `action_low_limit`、`action_high_limit`；
- 若存在约束，则得到 `constraint_dim`。

如果 `save_folder=None`，还会创建：

```text
results/<env_id>/<algorithm_时间戳>/
├── apprfunc/
├── evaluator/
└── config.json
```

`config.json` 保存经过补全后的实际实验参数，是复核某次训练环境配置的关键文件。

## 9. 当前倒立双摆环境的完整配置

FHADP 训练脚本选择：

```python
env_id=pyth_idpendulum
```

对应：

```text
数据环境：gops/env/env_ocp/pyth_idpendulum.py
环境模型：gops/env/env_ocp/env_model/pyth_idpendulum_model.py
```

### 9.1 状态和观测

六维状态为：

```text
[p, theta1, theta2, pdot, theta1dot, theta2dot]
```

1. `p`：小车水平位置；
2. `theta1`：第一根杆角度；
3. `theta2`：第二根杆角度；
4. `pdot`：小车水平速度；
5. `theta1dot`：第一根杆角速度；
6. `theta2dot`：第二根杆角速度。

观测就是这六维状态，因此 `observation_space.shape=(6,)`，算法得到 `obsv_dim=6`。

### 9.2 初始状态范围

默认范围为：

```text
p             ∈ [-5, 5]
theta1        ∈ [-0.1, 0.1]
theta2        ∈ [-0.1, 0.1]
pdot          ∈ [-0.3, 0.3]
theta1dot     ∈ [-0.3, 0.3]
theta2dot     ∈ [-0.3, 0.3]
```

`PythBaseEnv` 支持 `train_space`、`work_space` 和 `initial_distribution`。训练模式从 `train_space` 采样，测试模式从 `work_space` 采样；未提供 `train_space` 时两者相同。初始分布支持 `uniform` 和 `normal`。

运行脚本也可固定初始状态：

```python
init_info={init_state: [-1, 0.05, 0.05, 0, 0.1, 0.1]}
```

### 9.3 动作

策略动作是一维连续量：

```text
action ∈ [-1,1]
```

环境动力学内部实际使用 `500 × action`，即归一化动作会转换为最大约 ±500 的驱动力量级。这个 `500` 是具体环境内部的物理比例，不是通用 `ScaleActionData`。

### 9.4 时间尺度

环境定义：

```text
dt = 0.01 s
discrete_num = 5
max_episode_steps = 500
```

一次 `env.step()` 的控制周期为 0.01 s，但内部划分为 5 个 0.002 s 的积分子步。最大 500 步对应约 5 s 物理时间。

如果再设置 `repeat_num=4`，一次策略输出将维持四个环境步，即约 0.04 s。修改 `repeat_num` 会改变闭环控制频率。

### 9.5 奖励函数

```text
reward = 10
       - 5 × theta1²
       - 10 × theta2²
       - 0.5 × pdot²
       - 0.5 × theta1dot²
       - 1 × theta2dot²
       - action²
```

小车位置 `p` 的当前奖励权重为 0，因此奖励不直接惩罚小车偏离中心，但终止条件限制小车不能无限移动。

训练脚本设置 `reward_scale=1` 且未设置 reward shift，因此最终奖励与原始奖励相同。

### 9.6 终止条件

满足任一条件即结束：

1. 第二根杆末端高度 `point2y <= 1.0`；
2. 小车位置 `abs(p) >= 15`；
3. 达到 TimeLimit 的 500 步上限。

### 9.7 当前训练脚本实际包装

`fhadp_mlp_idpendulum_serial.py` 使用：

```python
reward_scale=1
repeat_num=None
sum_reward=False
is_render=False
is_adversary=False
is_constrained=False
```

最终结构大致是：

```text
ScaleActionData
└── ShapingRewardData
    └── StateData
        └── ConvertType
            └── TimeLimit(500)
                └── ResetInfoData
                    └── PythInverteddoublependulum
```

`repeat_num=None`，所以没有 ActionRepeat；`reward_scale=1` 虽添加奖励包装器但数值不变；连续动作默认启用动作缩放；`is_render=False` 仅表示训练时不主动显示动画。

## 10. 训练环境与运行环境必须一致

训练和运行必须保持以下定义一致：

- `env_id`；
- 状态维度、顺序和物理含义；
- 动作维度、范围和物理比例；
- 动力学参数与时间步；
- 奖励和终止条件；
- 观测/动作缩放与动作重复；
- 环境模型定义。

如果训练后修改状态顺序或动作含义，旧策略可能仍能加载，但输出已经不再对应原训练问题，通常必须重新训练。

## 11. 如何查看一次训练实际用了什么配置

### 第一步：检查训练脚本

重点查找：

```text
env_id, reward_scale, reward_shift, repeat_num, sum_reward,
obs_scale, obs_shift, obs_noise_type, action_scale,
max_episode_steps, vector_env_num, vector_env_type
```

### 第二步：按 `env_id` 找环境文件

重点阅读环境的：

- `__init__()`；
- `observation_space`、`action_space`；
- `reset()`、`step()`；
- 奖励和终止逻辑；
- `dt`、`discrete_num`、`max_episode_steps`。

### 第三步：检查同名环境模型

模型驱动算法还必须检查 `env_model/<env_id>_model.py`，确认动力学、奖励、终止条件、状态维度、动作范围和时间步一致。

### 第四步：查看结果目录的 `config.json`

例如：

```text
results/pyth_idpendulum/FHADP_<时间戳>/config.json
```

它记录该次训练实际保存的参数，比只看脚本当前默认值更可靠。

## 12. 安全修改环境配置

### 12.1 只调整实验参数

初始范围、奖励缩放、动作重复、观测归一化、回合长度和噪声，优先在新的训练脚本中修改，并让新实验使用新时间戳目录。

### 12.2 修改动力学、奖励或终止条件

必须同步修改对应环境模型，并执行数据环境—模型一致性检查。较大变化建议创建新的环境文件和新 `env_id`，避免旧实验含义被悄悄改变。

### 12.3 修改状态维度或顺序

这会同时影响观测空间、网络输入层、模型、检查点和绘图含义，属于高风险修改，通常必须重新训练。

### 12.4 修改动作范围

要同时区分：策略输出范围、原始环境动作空间和动力学内部物理比例。当前倒立双摆三者分别涉及 `[-1,1]`、动作包装器和 `500 × action`。

## 13. 一致性检查工具

`gops/env/inspector/` 用于检查空间、接口、返回值和约束维度。

`tests/env_gen_ocp/test_consistency.py` 会比较：

- 新旧环境的一致性；
- 数据环境和环境模型的一致性；
- 初始观测、多步转移、奖励、done 和约束。

新建或修改模型驱动环境时，应增加自动一致性测试，不能只凭动画判断环境是否正确。

## 14. 当前代码中的注意事项

### 14.1 `is_constrained` 与 `is_constraint`

FHADP 示例定义 `is_constrained`，但旧倒立双摆环境读取 `is_constraint`。两个键名不同，仅修改前者不一定改变环境属性。启用约束前必须核对算法、环境和模型实际读取的参数名。

### 14.2 `argparse` 的 `type=bool`

部分旧脚本使用 `type=bool`，命令行字符串 `False` 的转换可能不符合直觉。修改布尔配置时，直接确认脚本默认值更稳妥；长期维护应改成 `store_true/store_false`。

### 14.3 `is_render` 与 `save_render`

- `is_render`：训练采样时是否显示动画；
- `save_render`：PolicyRunner 测试时是否录制视频。

训练不显示动画，不代表测试不能生成视频。

### 14.4 Gym 警告

项目使用旧 Gym 接口并提供部分 Gymnasium 适配。Gym 停止维护的提示通常是警告，不代表环境创建失败；真正失败原因应看 traceback 最后一行。

## 15. 新建环境的推荐步骤

1. 选择 `env_gym`、`env_ocp`、`env_gen_ocp` 或 `env_matlab`；
2. 创建以新 `env_id` 命名的数据环境文件；
3. 定义空间、reset、step、奖励、终止条件和必要的 render；
4. 模型驱动算法需要创建同名 `_model.py` 和 `env_model_creator()`；
5. 添加数据环境—模型一致性测试；
6. 在 `example_train/<算法>/` 添加独立训练脚本；
7. 用新结果目录训练，不覆盖旧实验语义。

## 16. 排查清单

1. `env_id` 是否与文件名一致；
2. 是否提供 `env_creator()`；
3. 观测/动作空间是否与实际数组形状一致；
4. `reset()`、`step()` 返回值是否合法；
5. 初始范围是否合理；
6. 奖励正负号是否符合最大化奖励约定；
7. done 是否过早或永不触发；
8. `dt`、内部积分和 `repeat_num` 是否形成预期时间尺度；
9. 动作归一化与物理量映射是否正确；
10. 数据环境和模型是否使用相同包装参数；
11. 当前脚本是否与训练保存的 `config.json` 一致；
12. 加载旧策略时环境定义是否已经变化。

## 17. 总结

```text
env_id 选择环境本体
create_env 添加统一包装
init_args 从最终环境推导算法维度
env_model 为模型驱动算法提供一致的 Tensor 预测模型
```

当前倒立双摆案例的关键参数为：

```text
env_id = pyth_idpendulum
状态维度 = 6
动作维度 = 1
策略动作范围 = [-1,1]
动力学输入 = 500 × action
控制周期 = 0.01 s
内部积分周期 = 0.002 s
最大步数 = 500
最大物理时长约 = 5 s
```

只使用已有环境时，重点理解 `env_id` 和包装参数；修改动力学或新建任务时，必须同时维护数据环境、环境模型及一致性测试。
