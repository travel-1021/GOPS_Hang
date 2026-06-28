# Simulink 模型接入 GOPS 进行交互训练说明

## 1. 核心结论

GOPS 所说的“环境”不仅是动力学模型，还包含智能体与模型如何交互的完整定义：

```text
观测 observation、动作 action、状态转移 dynamics、奖励 reward、
终止 done、回合复位 reset、附加信息 info
```

对于现有 Simulink 模型，GOPS 当前的训练路线不是 Python 每一步都通过 MATLAB Engine 启动仿真，而是：

```text
Simulink .slx
 → Simulink/Embedded Coder 生成 C/C++
 → slxpy 生成 Python 绑定和 Gym 包装
 → 编译为 Python 扩展 .pyd
 → GOPS env_matlab 包装为标准环境
 → sampler 调用 reset()/step()
 → 算法训练
```

训练时交互由编译后的本地二进制扩展完成，不需要每一步启动 MATLAB。

必须区分两条路线：

- **模型自由算法**：DDPG、TD3、SAC、DSAC、PPO、TRPO、DQN 等主要依赖采样。只要 Simulink 被包装成合法的 `reset/step` 环境即可。
- **模型驱动算法**：FHADP、INFADP、部分 RPI 及 GOPS 中的 MPC/OPT 需要批量预测或梯度。除 Simulink 黑盒外，还要实现一份一致、可求导的 Python/PyTorch `env_model`。

```text
模型自由算法：Simulink GymEnv 即可
模型驱动算法：Simulink GymEnv + 可求导 Python env_model
```

## 2. 智能体交互如何实现

无论底层是 Python、Gym、Simulink 还是其他仿真器，GOPS 最终只依赖：

```python
obs, info = env.reset()
next_obs, reward, done, info = env.step(action)
```

- `obs`：策略网络输入；
- `action`：策略网络输出；
- `next_obs`：执行动作后的新观测；
- `reward`：即时奖励；
- `done`：回合是否终止；
- `info`：约束、原始状态和诊断信息。

`gops/trainer/sampler/base.py` 的核心循环可以简化为：

```python
obs, info = env.reset()
while collecting_samples:
    action = policy(obs)
    action = add_noise_and_clip(action)
    next_obs, reward, done, next_info = env.step(action)
    save(obs, action, reward, done, next_obs, info, next_info)
    obs, info = next_obs, next_info
    if done:
        obs, info = env.reset()
```

离策略算法将经验送入 replay buffer；同策略算法直接用轨迹计算回报和优势。算法不知道 `env.step()` 内部是否运行 Simulink，只要求接口正确。

## 3. 当前 Aircraft/Simulink 示例

训练入口：

```text
example_train/ddpg/ddpg_mlp_aircraftconti_offserial_slx.py
```

它设置 `env_id=simu_aircraftconti`，从而找到：

```text
gops/env/env_matlab/simu_aircraftconti.py
```

该环境导入预编译的 `GymEnv` 并创建最大 200 步的仿真环境。相关资源为：

```text
gops/env/env_matlab/resources/simu_aircraft_v2/
├── aircraft.slx
├── model.toml
├── env.toml
├── aircraft.cp38-win_amd64.pyd
└── aircraft.cp39-win_amd64.pyd
```

训练时真正被 Python 调用的是 `.pyd`，不是直接解释 `.slx`。`.pyd` 带 Python ABI 标签，例如 `cp38` 对应 CPython 3.8。

## 4. Simulink 需要提供的强化学习信号

项目的 `env.toml` 将 Simulink 信号映射为：

```toml
[gym]
action_key = Action
observation_key = State
reward_key = Reward
done_key = Done
```

### Action

智能体动作进入 Simulink 的输入。应有固定维度和数据类型；多维控制量可使用向量 Inport。

### State/Observation

返回给智能体的观测，不一定等于完整物理状态。可只包含可测量量，也可在 Python 包装层加入跟踪误差、参考轨迹和缩放。`observation_space.shape` 必须与实际输出一致。

### Reward

每步标量奖励。GOPS 通常最大化累计奖励，如果 Simulink 定义的是非负代价，一般使用 `reward=-cost`。奖励也可以在 Python 包装器中根据 state/action 计算，但必须保证只有一个权威定义。

### Done

布尔标量，表示越界、碰撞、状态发散、达到目标等任务终止。最大步数可以交给 Gym `TimeLimit`。

### Info

可选诊断输出。约束算法通常需要在 Python 包装层生成 `info[constraint]`。

## 5. `model.toml` 与 `env.toml`

### 5.1 `model.toml`

它描述 Simulink 代码生成设置：

```toml
model = aircraft

[simulink]
solver = FixedStepAuto
continuous_time = true

[cpp]
class_name = aircraft
```

关键要求：

- `model` 与 `.slx` 模型名对应；
- 模型必须支持代码生成；
- 强化学习交互需要明确的控制步长；
- S-Function、Simscape 或不支持代码生成的模块可能需要额外处理；
- 初始状态和需要随机化的物理参数应成为可调参数。

### 5.2 `env.toml`

它描述 Gym 包装：

```toml
use_raw = true
use_gym = true

[gym]
action_key = Action
observation_key = State
reward_key = Reward
done_key = Done
type_coercion = true

[gym.action_space]
type = Box
low = [-1]
high = [1]
shape = [1]
dtype = float64

[gym.observation_space]
type = Box
low = -inf
high = inf
shape = [2]
dtype = float64
```

它定义信号映射、空间维度和范围、reset 行为、参数随机化，以及是否生成普通/Gym/向量包装器。

## 6. 从 `.slx` 到 Python 环境

### 步骤 1：先定义交互问题

必须先明确：

```text
真实状态 x、策略观测 o、动作 u、奖励 r、终止 d、
控制周期 Ts、回合初始状态分布
```

尤其要确定动作物理单位、观测顺序和奖励正负号。

### 步骤 2：整理 Simulink 模型

- Action 使用明确 Inport；
- State、Reward、Done 使用明确 Outport；
- 设置确定的控制采样周期；
- 将初始状态和需随机化的参数设置为可调参数；
- 确认所有模块支持代码生成。

### 步骤 3：准备 slxpy

Python 环境安装 `slxpy`，MATLAB 中安装 slxpy toolbox，同时准备可用的 C/C++ 编译工具链。MATLAB 主要用于 `.slx → C/C++` 代码生成；完成后，日常训练不需要 MATLAB 逐步参与。

### 步骤 4：创建 slxpy 工程

```powershell
mkdir my_sim_env
cd my_sim_env
slxpy init
```

放入或引用 `my_model.slx`，然后编辑 `model.toml` 和 `env.toml`。

### 步骤 5：在 MATLAB 中生成代码

```matlab
workdir = '你的slxpy工程绝对路径';
slxpy.setup_config(workdir);  % 首次或 model.toml 改变后
slxpy.codegen(workdir);       % .slx 改变后
```

该阶段需要 Simulink 代码生成能力。

### 步骤 6：生成绑定并编译

```powershell
cd 你的slxpy工程目录
slxpy generate
python setup.py build
```

成功后得到与平台和 Python ABI 匹配的 `.pyd`。

### 步骤 7：先单独测试 `GymEnv`

```python
import my_env

spec = my_env._env.EnvSpec(
    id=MySimEnv-v0,
    max_episode_steps=500,
    strict_reset=True,
)
env = my_env.GymEnv(spec)

obs = env.reset()
print(obs)
print(env.observation_space)
print(env.action_space)

action = env.action_space.sample()
print(env.step(action))
```

先验证 shape、dtype、reward、done 和重复 reset，再接入 GOPS。

## 7. 接入 `gops/env/env_matlab`

假设编译包名为 `my_env`，在 `gops/env/env_matlab/` 新增 `simu_my_env.py`：

```python
import gym
import numpy as np

from gops.env.env_matlab.resources.simu_my_env import my_env


class SimuMyEnv(gym.Env):
    def __init__(self, **kwargs):
        spec = my_env._env.EnvSpec(
            id=SimuMyEnv-v0,
            max_episode_steps=kwargs.get(max_episode_steps, 500),
            strict_reset=True,
        )
        self.env = my_env.GymEnv(spec)
        self.observation_space = self.env.observation_space
        self.action_space = self.env.action_space

    def reset(self, *, init_state=None, **kwargs):
        def callback():
            if init_state is not None:
                self.env.model_class.my_model_InstP.x_ini[:] = np.asarray(init_state)

        return self.env.reset(callback)

    def step(self, action):
        obs, reward, done, info = self.env.step(action)
        return obs, reward, done, info

    def seed(self, seed=None):
        return self.env.seed(seed)

    def close(self):
        pass


def env_creator(**kwargs):
    return SimuMyEnv(**kwargs)
```

注意：`my_model_InstP.x_ini` 是示意名称，必须根据生成包中的模型参数属性修改。可参考自动生成的 `.pyi` 类型提示，或在 Python 中检查 `env.model_class`。

资源建议放置为：

```text
gops/env/env_matlab/resources/simu_my_env/
├── my_model.slx
├── model.toml
├── env.toml
└── my_env.cp38-win_amd64.pyd
```

新增文件名 `simu_my_env.py` 后，`create_env.py` 会自动注册：

```python
env_id=simu_my_env
```

## 8. 创建训练脚本

模型自由算法优先复制最接近的 `_slx.py` 示例。例如 DDPG：

```python
parser.add_argument(--env_id, type=str, default=simu_my_env)
parser.add_argument(--algorithm, type=str, default=DDPG)
parser.add_argument(--trainer, type=str, default=off_serial_trainer)
parser.add_argument(--sampler_name, type=str, default=off_sampler)
```

调用链保持不变：

```python
args = vars(parser.parse_args())
env = create_env(**args)
args = init_args(env, **args)
alg = create_alg(**args)
sampler = create_sampler(**args)
buffer = create_buffer(**args)
evaluator = create_evaluator(**args)
trainer = create_trainer(alg, sampler, buffer, evaluator, **args)
trainer.train()
```

`init_args()` 会从 Simulink GymEnv 的空间自动得到观测维度和动作维度，算法不需要专门知道底层是 Simulink。

建议第一轮使用：

- 串行训练器；
- 单环境；
- 很小的 `max_iteration` 和 `sample_batch_size`；
- 固定 seed；
- 禁用渲染；
- 频繁打印 reward、done、动作和状态范围。

单环境稳定后再考虑异步或向量训练。编译扩展是否线程/进程安全必须实际验证。

## 9. reset、预处理和后处理

现有 `simu_veh3dofconti.py` 展示了比 Aircraft 更完整的包装方式。

### reset 回调

在每个 episode 开始时，把随机初始状态、参考轨迹和奖励权重写入 Simulink 实例参数：

```python
def callback():
    self.env.model_class.vehicle3dof_InstP.x_ini[:] = initial_state
    self.env.model_class.vehicle3dof_InstP.ref_V = ref_speed
    self.env.model_class.vehicle3dof_InstP.punish_Q[:] = Q

state = self.env.reset(callback)
```

这实现了训练中的初始状态随机化和参数随机化。

### 动作预处理

策略通常输出归一化动作，而 Simulink 使用物理量：

```python
action_real = action / act_scale
```

要明确方向盘角度、力矩、电流等单位，避免在 GOPS 通用动作包装器和自定义包装层重复缩放。

### 观测后处理

包装层可以把 Simulink 原始状态转换为更适合学习的观测：

- 状态减参考值，形成跟踪误差；
- 选择部分状态；
- 拼接未来参考轨迹；
- 归一化不同量纲；
- 裁剪异常值。

训练和部署必须使用完全相同的预处理/后处理。

### 动作保持

`act_repeat` 可让一个策略动作在 Simulink 中连续执行多个基础仿真步。若基础步长为 `dt`，策略控制周期为：

```text
policy_period = dt × act_repeat
```

它直接影响闭环带宽，不能仅作为加速参数。

## 10. 何时必须写 `env_model`

### 不需要的典型情况

DDPG、TD3、SAC、DSAC、PPO 等只需通过环境收集 `(s,a,r,s')`，可以把编译后的 Simulink 当作黑盒。

### 需要的典型情况

FHADP/INFADP 或 MPC 需要类似：

```python
next_state = model(state, action)
reward = model_reward(state, action)
gradient = autograd(...)
```

slxpy 编译扩展一般不提供 PyTorch autograd 图，因此不能直接替代 GOPS 的 `env_model`。

可选方案：

1. 根据 Simulink 方程手工实现 PyTorch 模型；
2. 从 Simulink 导出明确数学方程后重写；
3. 用 Simulink 采样数据辨识一个可微代理模型，但必须验证误差；
4. 暂时改用模型自由算法。

现有 `simu_lqs2a1conti` 就采用“双实现”：数据环境调用 Simulink，而 `env_model/simu_lqs2a1conti_model.py` 使用 Python 线性二次模型。两者必须保持参数、采样周期、奖励和动作范围一致。

## 11. 接入前的分层测试

### 11.1 Simulink 内部测试

用固定初始状态和固定动作序列运行 `.slx`，记录状态、奖励和 done。

### 11.2 编译扩展测试

使用相同初始条件和动作序列调用 `GymEnv`，逐步对比 Simulink 原始输出。

### 11.3 GOPS 包装测试

```python
from gops.create_pkg.create_env import create_env

env = create_env(env_id=simu_my_env)
obs, info = env.reset()
for _ in range(10):
    action = env.action_space.sample()
    obs, reward, done, info = env.step(action)
    print(obs.shape, action.shape, reward, done)
    if done:
        obs, info = env.reset()
```

### 11.4 短训练测试

只运行少量迭代，确认：

- replay buffer 数据形状正确；
- reward 有合理变化且没有 NaN/Inf；
- done 触发频率合理；
- 动作未长期卡在边界；
- 多次 reset 后模型状态不会串回合。

### 11.5 环境模型一致性测试

如使用 `env_model`，给数据环境和模型相同初始状态与动作序列，比较每一步的状态、观测、奖励、done 和约束。

## 12. 性能与工程注意事项

### Python 版本

预编译 `.pyd` 必须匹配 Python ABI、操作系统和架构。项目现有 Aircraft 扩展提供 cp38/cp39 Windows 64 位版本。

### 采样速度

编译环境比 MATLAB Engine 快，但 Simulink 模型复杂、基础步长很小或单步内积分次数很多时，训练仍可能很慢。应先测量单环境每秒 step 数。

### 并行安全

异步 sampler 会创建多个环境实例或进程。必须确认扩展不共享不可重入的全局模型状态。先串行，再逐步增加并行数。

### 随机种子

Python、GOPS、slxpy 环境以及 Simulink 内部随机模块可能各有独立 RNG。要实现可复现训练，必须统一或记录所有种子。

### Reset 完整性

reset 必须恢复积分器、延迟、滤波器、控制器内部状态、计时器和参数。只修改外部状态变量而未重置 Simulink 内部状态，会导致 episode 相互污染。

### 数据类型和单位

检查 float32/float64 转换、角度/弧度、N/kN、m/s/km/h 等单位。类型转换通常不会报错，但量纲错误会直接破坏训练。

### 奖励尺度

奖励过大可能导致神经网络梯度不稳定；过小则学习缓慢。先记录随机策略的 reward 分布，再决定是否缩放。

## 13. 训练后如何把策略放回 Simulink

这与“把 Simulink 模型接入训练”是反方向流程：

```text
训练侧：Simulink → .pyd → GOPS 环境
部署侧：GOPS 策略 → TorchScript → Simulink S-Function
```

项目的 `gops/env/py2slx_tools/` 可以把策略导出为 TorchScript，并通过 `gops_validation_bridge.m` 的 Level-2 MATLAB S-Function 在 Simulink 中调用。

注意：`py2slx_tools` 主要用于训练后的策略回灌和闭环验证，不是将任意 `.slx` 编译成训练环境的工具；训练环境生成使用的是 slxpy 流程。

## 14. 推荐落地路线

如果你现在已经有一个 Simulink 模型，建议按以下顺序：

1. 明确 action、observation、reward、done、reset 和采样周期；
2. 先选择 DDPG/SAC/PPO 等模型自由算法，降低首次接入难度；
3. 将模型整理为支持代码生成的接口；
4. 使用 slxpy 生成和编译 `GymEnv`；
5. 独立验证 GymEnv；
6. 在 `env_matlab` 添加薄包装层和新 `env_id`；
7. 用随机动作测试 reset/step；
8. 运行极短 GOPS 训练；
9. 验证结果后再增加训练规模和并行度；
10. 只有确实需要 FHADP/INFADP/MPC 时，再实现并验证可微 `env_model`。

## 15. 参考资料

- GOPS Simulink 接入文档：[https://gops.readthedocs.io/en/latest/slx2py.html](https://gops.readthedocs.io/en/latest/slx2py.html)
- slxpy PyPI 与快速入门：[https://pypi.org/project/slxpy/](https://pypi.org/project/slxpy/)
- 本项目现有入口：`example_train/ddpg/ddpg_mlp_aircraftconti_offserial_slx.py`
- 本项目完整车辆包装示例：`gops/env/env_matlab/simu_veh3dofconti.py`
- 训练后策略回灌工具：`gops/env/py2slx_tools/`

## 16. 总结

Simulink 接入 GOPS 的本质，是把一个连续仿真模型转换成符合 Gym 语义的离散交互环境：

```text
reset 定义一轮实验如何开始
step 定义动作如何推进模型
observation 定义智能体看见什么
reward 定义优化目标
done 定义一轮实验何时结束
```

slxpy 负责把支持代码生成的 `.slx` 编译为高效 Python `GymEnv`，GOPS 的 sampler 再通过标准 `reset/step` 与它交互。对于模型自由算法，这已经足够；对于依赖梯度和预测模型的算法，还必须提供与 Simulink 一致的可微 Python 环境模型。
