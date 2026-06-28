# GOPS 环境安装与跨项目使用说明

本文说明如何根据本项目提供的 `gops_environment.win.yml` 创建 Conda 环境，将当前 GOPS 项目以可编辑方式安装到环境中，并在其他目录下的项目中使用 GOPS。

## 1. 创建 Conda 环境

在 Anaconda Prompt 或已配置 Conda 的 PowerShell 中，进入 GOPS 项目根目录：

```powershell
cd E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang
```

推荐通过 `-n` 参数指定新环境名称，无需修改 `gops_environment.win.yml` 中的 `name`：

【本质上用该项目文件夹更改一定环境名后即可作为项目的文件夹，部分无用的demo可以去除即可；另外环境名一定要合适】

```powershell
conda env create -f gops_environment.win.yml -n 你的环境名
```

例如：

```powershell
conda env create -f gops_environment.win.yml -n my_gops_env
```

也可以直接执行以下命令，使用 YAML 文件中定义的默认环境名 `gops_env`：

```powershell
conda env create -f gops_environment.win.yml
```

## 2. 激活环境

```powershell
conda activate my_gops_env
```

可通过以下命令确认当前 Python 来自目标 Conda 环境：

```powershell
python -c "import sys; print(sys.executable)"
```

## 3. 以可编辑方式安装 GOPS

确保当前目录为 GOPS 项目根目录，即包含 `setup.py` 的目录，然后执行：

```powershell
python -m pip install -e .
```

这里的 `-e` 表示 editable（可编辑）安装。安装后，Python 环境不会复制一份独立的 GOPS 源码，而是引用当前目录下的源码。

因此：

- 修改 `GOPS_Hang` 中的 GOPS 源码后，使用该环境运行程序时会直接使用修改后的代码；
- 不需要在每次修改源码后重新安装；
- 不应随意移动或重命名 `GOPS_Hang` 文件夹，否则可编辑安装的路径可能失效；
- 如果移动或重命名了源码目录，需要在新目录下重新执行 `python -m pip install -e .`。

## 4. 在其他项目中使用 GOPS

其他项目可以放在任意目录。运行该项目之前，只需激活已经安装 GOPS 的 Conda 环境。

例如：

```powershell
conda activate my_gops_env
cd E:\你的其他项目目录
python your_script.py
```

在其他项目的 Python 代码中可以直接导入 GOPS：

```python
import gops
```

其他项目不需要复制 GOPS 源码，也不需要与 `GOPS_Hang` 位于同一个目录中。能否导入 GOPS 取决于运行程序时使用的 Python 环境，而不是当前工作目录。

## 5. 验证安装结果

激活目标环境后执行：

```powershell
python -c "import sys, gops; print('Python:', sys.executable); print('GOPS:', gops.__file__)"
```

正常情况下：

- `Python` 应指向刚创建的 Conda 环境；
- `GOPS` 应指向 `E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang` 下的 GOPS 源码。

也可以检查安装信息：

```powershell
python -m pip show gops
```

## 6. 多个新项目如何选择环境

多个项目可以共用同一个 GOPS Conda 环境，也可以为每个项目分别创建环境。

### 共用一个环境

适用于多个项目所需依赖版本一致的情况。只需创建一次环境并执行一次可编辑安装。

### 每个项目使用独立环境

适用于不同项目依赖版本可能冲突，或者需要保证实验环境彼此隔离的情况。每创建一个新环境，都需要在该环境中执行一次：

```powershell
cd E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang
python -m pip install -e .
```

同一个 GOPS 源码目录可以同时被多个 Conda 环境以可编辑方式引用。

## 7. 推荐的完整操作流程

```powershell
# 进入 GOPS 项目根目录
cd E:\6_PHD_Research_Group\10_Python_Code\GOPS_Hang

# 创建新环境
conda env create -f gops_environment.win.yml -n my_gops_env

# 激活环境
conda activate my_gops_env

# 可编辑安装 GOPS
python -m pip install -e .

# 验证 Python 和 GOPS 的实际路径
python -c "import sys, gops; print('Python:', sys.executable); print('GOPS:', gops.__file__)"

# 进入其他项目并运行程序
cd E:\你的其他项目目录
python your_script.py
```

## 8. 常见问题

### 无法执行 `conda activate`

先初始化 PowerShell，然后重新打开终端：

```powershell
conda init powershell
```

### 出现 `ModuleNotFoundError: No module named 'gops'`

依次确认：

1. 当前是否已经激活正确的 Conda 环境；
2. 是否在该环境中执行过 `python -m pip install -e .`；
3. GOPS 源码目录是否被移动或重命名；
4. IDE 配置的 Python 解释器是否与终端中的 Conda 环境一致。

### IDE 中能运行，终端中不能运行，或反之

这通常表示 IDE 和终端使用了不同的 Python 解释器。分别检查：

```powershell
python -c "import sys; print(sys.executable)"
```

并将 IDE 的 Python 解释器设置为目标 Conda 环境中的 `python.exe`。

## 总结

本项目支持以下使用方式：

1. 使用 `gops_environment.win.yml` 创建 Conda 环境；
2. 在目标环境中运行 `python -m pip install -e .`；
3. 在其他目录的项目中激活该环境；
4. 直接通过 `import gops` 使用当前 GOPS 项目。

这种方式适合在持续修改 GOPS 源码的同时，让多个实验项目直接使用最新代码。
