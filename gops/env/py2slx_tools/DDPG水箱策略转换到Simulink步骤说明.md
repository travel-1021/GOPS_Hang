# DDPG 水箱策略转换到 Simulink 步骤说明

本文针对以下训练结果：

```text
E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\results\simu_rl_water\DDPG_260630-063942
```

经检查，该目录中已经存在最优策略文件：

```text
apprfunc\apprfunc_30500_opt.pkl
```

因此，转换时应将 `trained_policy_iteration_list` 设置为 `"30500_opt"`，不要填写完整文件名。

## 1. 转换前准备

1. 安装 MATLAB 和 Simulink。`gops_validation_bridge.m` 使用 `pyrun`，MATLAB 最低版本为 R2021b，建议使用较新的版本。
2. 确认 MATLAB 版本支持 GOPS 当前 Python 环境的 Python 版本。兼容关系应以对应 MATLAB 版本的 MathWorks 文档为准。
3. 在用于转换和启动 MATLAB 的同一个 Conda 环境中安装以下内容：
   - GOPS；
   - PyTorch；
   - NumPy；
   - 本水箱环境运行所需的依赖及已编译扩展。
4. 确认训练结果目录中的 `config.json` 和 `apprfunc` 文件夹没有被移动或删除。

在 Anaconda Prompt 或 PowerShell 中可先执行：

```powershell
conda activate gops_env
python -c "import sys, torch, numpy; print(sys.executable); print(torch.__version__); print(numpy.__version__)"
```

记录输出的 Python 可执行文件路径，稍后用它核对 MATLAB 的 `pyenv`。

## 2. 配置转换脚本

打开：

```text
E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools\py2slx_example.py
```

将其中 `runner = Py2slxRunner(...)` 部分改成下面的配置：

```python
runner = Py2slxRunner(
    log_policy_dir_list=[
        r"E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\results\simu_rl_water\DDPG_260630-063942"
    ],
    trained_policy_iteration_list=["30500_opt"],
    export_controller_name=["DDPG_rl_water_30500_opt"],
    save_path=[
        r"E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools"
    ],
)
```

四个参数的含义如下：

- `log_policy_dir_list`：训练结果目录，其中必须包含 `config.json` 和 `apprfunc` 文件夹。
- `trained_policy_iteration_list`：检查点编号。本次为 `30500_opt`，对应 `apprfunc_30500_opt.pkl`。
- `export_controller_name`：导出的 TorchScript 控制器名称；工具会自动添加 `.pt`。
- `save_path`：导出目录，必须事先存在。本说明将模型导出到 `py2slx_tools` 目录，便于和桥接文件放在一起。

这些参数均为列表；四个列表中的项目必须一一对应。

## 3. 执行转换

在 GOPS 项目根目录执行：

```powershell
cd E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang
conda activate gops_env
python gops\env\py2slx_tools\py2slx_example.py
```

转换过程会完成以下操作：

1. 读取训练时的 `config.json`；
2. 创建 DDPG 网络并加载 `apprfunc_30500_opt.pkl`；
3. 用水箱环境产生一条示例观测；
4. 检查策略能否被 `torch.jit.trace`；
5. 生成以下文件：

```text
E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools\DDPG_rl_water_30500_opt.pt
```

6. 检查 MATLAB 并尝试从该目录启动 MATLAB。

看到策略创建、策略加载成功，且上述 `.pt` 文件已经生成，即表示 Python 到 TorchScript 的核心转换已经完成。MATLAB 自动启动失败不代表 `.pt` 一定导出失败，应先检查该文件是否存在。

## 4. 在 Python 中验证导出模型

在项目根目录执行：

```powershell
python -c "import torch; p=r'gops\env\py2slx_tools\DDPG_rl_water_30500_opt.pt'; m=torch.jit.load(p); m.eval(); y=m(torch.zeros(3)); print('action=', y, 'shape=', tuple(y.shape))"
```

本策略的输入应为长度 3 的观测向量，输出应为长度 1 的动作向量。该检查只验证模型能被加载和推理，不代替闭环仿真。

## 5. 从正确的 Python 环境启动 MATLAB

推荐从已激活的 Conda 环境启动 MATLAB：

```powershell
conda activate gops_env
matlab
```

在 MATLAB 命令窗口中执行：

```matlab
pyenv
pyrun("import sys, torch, numpy; print(sys.executable); print(torch.__version__); print(numpy.__version__)")
```

`pyenv` 中的 `Executable`、`Version`、`Library` 和 `Home` 应对应前面用于转换且装有 PyTorch 的 Python 环境。如果 MATLAB 已经加载了错误的 Python，通常需要关闭 MATLAB，从正确的 Conda 环境重新启动；也可在 Python 尚未加载时按 MATLAB 文档用 `pyenv` 指定解释器。

## 6. 在 Simulink 中加入 GOPS 控制器

以下两个文件必须能被 MATLAB 找到：

```text
gops_validation_bridge.m
DDPG_rl_water_30500_opt.pt
```

它们当前都位于：

```text
E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools
```

可以将它们复制到水箱 `.slx` 模型所在目录，也可以在 MATLAB 中添加该目录：

```matlab
toolDir = 'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools';
addpath(toolDir);
```

然后在 Simulink 模型中：

1. 从库浏览器加入 **Level-2 MATLAB S-Function** 模块。
2. 双击该模块，将 **S-function name** 设置为：

   ```text
   gops_validation_bridge
   ```
3. 将 **Parameters** 设置为模型文件路径。推荐先使用绝对路径：

   ```matlab
   'E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang\gops\env\py2slx_tools\DDPG_rl_water_30500_opt.pt'
   ```

   如果 `.pt` 文件与 `.slx` 文件在同一目录，而且 MATLAB 当前目录也是该目录，也可填写：

   ```matlab
   'DDPG_rl_water_30500_opt.pt'
   ```
4. 将 3 个观测量用 Mux 组成宽度为 3 的一维向量，并严格按训练时的顺序接入 S-Function：

   ```text
   [0] 积分跟踪误差（训练模型中限制为 [-10, 10]）
   [1] 跟踪误差 = 目标水位 - 实际水位
   [2] 实际水位
   ```
5. S-Function 输出为 1 维物理流量动作。该训练配置采用 `action_scale = false`，因此不要再次执行 GOPS 的归一化动作反变换。
6. 建议在动作输出后增加 **Saturation** 模块，将动作限制为训练环境的安全范围 `[-100, 100]`，再连接水箱对象。
7. 确认 Simulink 中积分器初值、误差符号、采样周期和训练环境一致，然后运行闭环仿真。

## 7. 闭环接线检查

推荐按以下信号关系检查：

```text
目标水位 ──┐
           ├─ 误差及积分计算 ── [积分误差, 跟踪误差, 实际水位]
实际水位 ──┘                                      │
                                                  ▼
                                  gops_validation_bridge
                                                  │ 1 维动作
                                                  ▼
                                      Saturation [-100, 100]
                                                  │
                                                  ▼
                                              水箱对象
```

训练模型中的对象关系为：

```text
dh/dt = 0.25 * action - 0.1 * sqrt(h)
```

如果待验证的 Simulink 模型采用不同的动作增益、单位、误差符号或观测顺序，即使 `.pt` 能正常运行，闭环结果也会与训练评估明显不同。

## 8. 常见问题

### 找不到 `apprfunc_30500_opt.pkl`

确认 `log_policy_dir_list` 指向 `DDPG_260630-063942` 本身，而不是它的 `apprfunc` 子目录；确认 iteration 写成 `30500_opt`。

### 提示模型不能被 trace

先确认使用了本次训练时兼容的 GOPS/PyTorch 代码和环境。内置 MLP 策略通常可以转换；如果网络结构后来被自定义修改，则需满足 PyTorch JIT trace 的限制。

### MATLAB 找不到或版本检查失败

确认 `matlab` 命令已加入 PATH，MATLAB 版本不低于 R2021b。也可以先确认 `.pt` 已生成，再手动从正确的 Conda 环境启动 MATLAB。

### MATLAB 中提示没有 `torch` 或 `numpy`

MATLAB 使用了错误的 Python。用 `pyenv` 检查解释器，关闭 MATLAB后从装有 GOPS/PyTorch 的 Conda 环境重新启动。

### 找不到 `gops_validation_bridge`

将 `gops_validation_bridge.m` 复制到 `.slx` 所在目录，或使用 `addpath` 添加 `py2slx_tools` 目录，并执行：

```matlab
which gops_validation_bridge
```

### 模型输入维度错误

控制器要求宽度为 3 的一维观测信号，顺序必须是 `[积分误差, 跟踪误差, 实际水位]`。不要把它接成三个独立输入端口，也不要额外添加 batch 维度。

### 动作数值异常或闭环发散

依次检查观测顺序、误差正负号、单位、积分器初值、采样周期、动作增益以及 `[-100, 100]` 限幅。还应先用与 GOPS evaluator 相同的初始条件和目标值进行对比。

## 9. 完成标准

满足以下条件即可认为部署链路已经完成：

1. `DDPG_rl_water_30500_opt.pt` 成功生成；
2. Python 能加载该文件并对 3 维观测输出 1 维动作；
3. MATLAB 的 `pyenv` 指向装有 PyTorch 和 NumPy 的正确环境；
4. Simulink 能加载 `gops_validation_bridge.m` 和 `.pt` 文件；
5. S-Function 输入、输出维度分别为 3 和 1；
6. 闭环仿真无 Python、维度或路径错误，且控制效果与 GOPS 评估结果在相同工况下基本一致。

