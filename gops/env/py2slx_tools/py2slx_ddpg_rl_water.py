"""将训练完成的 DDPG 水箱控制策略转换为可供 Simulink 调用的模型。

本脚本针对以下训练结果编写：

    results/simu_rl_water/DDPG_260630-063942

脚本将加载该结果目录中的最优策略文件：

    apprfunc/apprfunc_30500_opt.pkl

随后，GOPS 的 :class:`Py2slxRunner` 会完成以下工作：

1. 读取训练目录中的 ``config.json``，恢复算法、网络和环境配置；
2. 创建 DDPG 近似函数网络，并加载指定的策略检查点；
3. 从水箱环境中取得一条示例观测，用于执行 ``torch.jit.trace``；
4. 将策略导出为 TorchScript 格式的 ``.pt`` 文件；
5. 检查本机 MATLAB 版本，并尝试从导出目录启动 MATLAB。

最终生成的控制器文件为：

    gops/env/py2slx_tools/DDPG_rl_water_30500_opt.pt

注意：运行本脚本时，应使用安装了 GOPS、PyTorch、NumPy 和水箱环境扩展
的 Python 环境。MATLAB 后续也必须连接到与之兼容且安装了 PyTorch 的
Python 环境，否则 Simulink 中的 ``gops_validation_bridge.m`` 无法加载模型。
"""

from pathlib import Path

from gops.env.py2slx_tools.py2slx import Py2slxRunner


# ---------------------------------------------------------------------------
# 路径与导出参数配置
# ---------------------------------------------------------------------------
#
# 所有路径均以当前脚本的绝对位置为基准自动计算，而不是依赖 PowerShell、
# CMD 或 IDE 的当前工作目录。这样，无论从项目根目录还是其他目录启动脚本，
# 都能定位到同一份训练结果。

# 当前 py2slx_tools 目录。
# 导出的 .pt 文件会保存到此目录，并与 gops_validation_bridge.m 放在一起，
# 便于 MATLAB 添加路径或复制到 Simulink 工程目录。
TOOL_DIR = Path(__file__).resolve().parent

# GOPS_Hang 项目根目录。
# 当前脚本层级为：GOPS_Hang/gops/env/py2slx_tools/本文件.py，
# 因此 parents[3] 对应 GOPS_Hang。
PROJECT_ROOT = Path(__file__).resolve().parents[3]

# 本次需要转换的完整训练结果目录。
# 该目录中必须同时存在 config.json 和 apprfunc 子目录。
POLICY_DIR = (
    PROJECT_ROOT / "results" / "simu_rl_water" / "DDPG_260630-063942"
)

# 策略迭代标识。
# Py2slxRunner 会根据此字符串拼出：
# apprfunc/apprfunc_30500_opt.pkl
# 因此这里只填写 "30500_opt"，不要填写前缀、后缀或完整路径。
POLICY_ITERATION = "30500_opt"

# 导出控制器的基本文件名。
# Py2slxRunner 会自动添加 .pt 后缀，最终文件名为：
# DDPG_rl_water_30500_opt.pt
CONTROLLER_NAME = "DDPG_rl_water_30500_opt"

# 根据上述目录和迭代标识得到完整检查点路径。
# 此变量主要用于运行转换前的显式检查和提示；实际加载仍由 Py2slxRunner 完成。
CHECKPOINT = POLICY_DIR / "apprfunc" / f"apprfunc_{POLICY_ITERATION}.pkl"


def main() -> None:
    """检查输入文件并将水箱 DDPG 策略导出为 TorchScript 控制器。"""

    # config.json 保存训练时的算法名称、网络结构、观测维度、动作维度等参数。
    # Py2slxRunner 需要依靠它重新创建与训练阶段完全一致的 DDPG 网络。
    config_path = POLICY_DIR / "config.json"
    if not config_path.is_file():
        raise FileNotFoundError(f"Training configuration not found: {config_path}")

    # 提前检查最优策略检查点是否存在，可避免在网络创建完成后才得到较难理解的
    # torch.load 文件错误。
    if not CHECKPOINT.is_file():
        raise FileNotFoundError(f"Policy checkpoint not found: {CHECKPOINT}")

    # Py2slxRunner 的四个参数均使用列表，是因为该工具原本支持一次转换多组策略。
    # 本脚本只转换一个水箱策略，所以每个列表中都只有一个元素：
    #
    # log_policy_dir_list：训练结果目录，不是 apprfunc 子目录；
    # trained_policy_iteration_list：检查点迭代标识；
    # export_controller_name：导出文件名，不包含 .pt 后缀；
    # save_path：导出文件所在目录，该目录必须已经存在。
    runner = Py2slxRunner(
        log_policy_dir_list=[str(POLICY_DIR)],
        trained_policy_iteration_list=[POLICY_ITERATION],
        export_controller_name=[CONTROLLER_NAME],
        save_path=[str(TOOL_DIR)],
    )

    # 此路径仅用于在转换前向用户明确显示预计生成的模型位置。
    output_path = TOOL_DIR / f"{CONTROLLER_NAME}.pt"
    print(f"Loading policy checkpoint: {CHECKPOINT}")
    print(f"Exporting Simulink controller to: {output_path}")

    # 执行完整转换流程：
    # 1. 恢复网络并加载策略权重；
    # 2. 检查策略是否可被 PyTorch JIT trace；
    # 3. 保存 TorchScript 模型；
    # 4. 检查 MATLAB 并尝试启动 MATLAB。
    #
    # 若 MATLAB 未安装、版本过低或 matlab 命令不在 PATH 中，最后一步可能报错，
    # 但应同时检查 output_path 是否已经生成，以判断前面的策略导出是否成功。
    runner.py2simulink()


# 仅当直接运行本文件时执行转换；如果其他模块只导入本文件，则不会自动加载策略、
# 导出模型或启动 MATLAB。
if __name__ == "__main__":
    main()
