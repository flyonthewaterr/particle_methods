# GPU DEM 引擎与 Python 模块构建教程（面向 particle_methods）

> 目标：用 C++/CUDA 搭建可用的 DEM 核心，引入 GPU 加速，并通过 pybind11 导出 Python API。

本教程以仓库现有结构为蓝本，讲清楚 **DEM 引擎的内部框架** 与 **GPU + Python 模块的落地路径**。适合想从零搭建可用 DEM 引擎、并最终在 Python 中调用的开发者。

---

## 1. 基础概念与范围

DEM（Discrete Element Method）核心步骤：

1. 粒子/刚体数据建模（位置、速度、半径、质量）
2. 邻域搜索（广义碰撞检测）
3. 精确接触判定（接触点、法向、重叠）
4. 接触力模型（法向/切向、阻尼/摩擦）
5. 数值积分更新状态

本仓库当前目标为 **2D 优先**，先打通 DEM CPU 路径，再平移到 GPU，并提供 Python 入口。

---

## 2. 工程骨架与分层设计

推荐拆分为三层：

- **Core 数据层**：粒子与系统容器（如 `Particle`, `ParticleSystem`, `SimulationConfig`）
- **Solver 逻辑层**：DEM/SPH/MPM 接口与实现（`DEMSolver`）
- **加速层**：CUDA kernel + 持久化 device 缓冲区

结合仓库结构可以这样组织：

```
include/particle_methods/core/      # 通用数据结构
include/particle_methods/solvers/   # DEM/SPH/MPM 接口
include/particle_methods/cuda/      # CUDA 接口
src/core/                           # core 实现
src/solvers/                        # DEM/SPH/MPM 实现
src/cuda/                           # CUDA kernel 和 wrapper
bindings/                           # pybind11 模块
python/                             # Python 包
```

---

## 3. DEM 引擎最小可运行切片

建议先完成一个 **可测可跑的 CPU DEM 版本**：

1. **粒子系统**
   - `positions`, `velocities`, `radii`, `mass`
   - 用 SoA（结构数组）提高缓存局部性

2. **邻域搜索**
   - 2D/3D 均可用 **uniform grid / cell list**
   - 输入：粒子位置
   - 输出：粒子候选邻居列表或 cell offset

3. **接触判定**
   - 计算重叠深度与法向
   - 过滤非接触对

4. **接触力模型**
   - 法向弹簧 + 阻尼
   - 切向摩擦（Coulomb/弹簧-滑移）

5. **积分更新**
   - 半隐式 Euler 是最常用方案
   - 子步（substeps）可提高稳定性

完成上述切片即可做 benchmark（如单粒反弹、堆积、漏斗等）。

---

## 4. GPU 化关键路径

GPU 化的核心不是“把 for 循环搬到 kernel”，而是 **设计高效的数据流和访问模式**。

### 4.1 数据布局

- 将粒子属性拆为 SoA（位置、速度、力等分开数组）
- 在 GPU 上维持持久化缓冲区，避免每步 host-device 往返拷贝

### 4.2 DEM 核心 kernel 拆分

一般拆成 3~4 类 kernel：

1. **构建网格 / binning**
2. **生成候选接触对**
3. **接触判定 + 力计算**
4. **积分更新**

如果 contact 对数不稳定，可额外做一次 **compact/sort** 降低分支与冗余。

### 4.3 GPU 误差与验证

- CPU 作为参考基准
- 允许误差窗口（浮点差异）
- 每个场景固定随机 seed

---

## 5. Python API 设计建议

Python 层应该做到：

- 可以创建/重置场景
- 可配置步数/子步/是否使用 CUDA
- 返回 positions 便于可视化

当前绑定示例：

- `FallingParticles2D` 类
- `run_dem_scene(...)` 函数

建议在 C++ 层封装最少 API，再通过 pybind11 暴露。

---

## 6. 构建步骤（C++ + CUDA + Python）

### 6.1 CPU 版本

```bash
cmake -S . -B build \
  -DPM_ENABLE_CUDA=OFF \
  -DPM_ENABLE_PYTHON=OFF \
  -DPM_BUILD_TESTS=ON
cmake --build build -j
```

### 6.2 GPU + Python 版本

```bash
cmake -S . -B build \
  -DPM_ENABLE_CUDA=ON \
  -DPM_ENABLE_PYTHON=ON
cmake --build build -j
```

**注意：** 需要 CUDA Toolkit 与兼容的 C++ 编译器。

---

## 7. 让 Python 找到 `_core` 模块

当前 pybind11 生成 `_core` 扩展模块，Python 包位于 `python/particle_methods`。

推荐方式之一：将编译输出直接放进 Python 包目录。

```bash
cmake -S . -B build \
  -DPM_ENABLE_CUDA=ON \
  -DPM_ENABLE_PYTHON=ON \
  -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=$PWD/python/particle_methods
cmake --build build -j
```

或者手动拷贝/软链接：

```bash
cp build/_core*.so python/particle_methods/
```

然后：

```bash
export PYTHONPATH=$PWD/python
python python/examples/falling_particles.py
```

---

## 8. Python 使用示例

```python
import particle_methods

sim = particle_methods.FallingParticles2D(
    particle_count=256,
    spacing=0.05,
    start_height=0.2,
    use_cuda=True,
)

sim.step(steps=300, substeps=4)
positions = sim.positions()
print(len(positions), positions[:5])
```

或者直接跑场景：

```python
positions = particle_methods.run_dem_scene(
    scene_name="hopper",
    particle_count=300,
    steps=400,
    seed=7,
    use_cuda=True,
    substeps=2,
)
```

---

## 9. 常见问题排查

- **找不到 `_core`**：检查 `_core*.so` 是否在 `python/particle_methods/` 目录。
- **CUDA 不可用**：确认 `nvcc` 可用，且 CMake 检测到 CUDA。
- **数值发散**：缩小 `dt` 或增加 `substeps`。

---

## 10. 下一步建议

1. 添加持久化 GPU 缓冲区（减少每步 memcpy）
2. 让 contact 列表在 GPU 内部进行 sort/compact
3. 统一 CPU/GPU 数值检查与回归测试
4. 扩展到 3D（复用 grid + contact 逻辑）

---

如果你想继续深化，我可以帮你把：

- GPU kernel 调度细化成可实现的函数列表
- C++ 类设计拆到更清晰的模块
- Python API 封装成完整 SDK（带安装脚本）

直接告诉我你想落地的层级即可。
