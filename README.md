# particle_methods

Particle-based simulation framework with C++, CUDA acceleration, and Python bindings.

Current focus:
- 2D-first development
- practical, incremental milestones
- starting with a simple falling-particles example

Methods planned:
- DEM (Discrete Element Method)
- SPH (Smoothed Particle Hydrodynamics)
- MPM (Material Point Method, concise CPU baseline available)

## Current Status

- Core particle data model and solver interfaces are set up.
- DEM phase-1 CPU path is implemented:
	- particle-particle contact (normal + damping + tangential friction)
	- uniform-grid neighbor search (cell list)
	- configurable boundaries (floor, walls, box)
	- deterministic seeded benchmark scene initialization
- MPM phase-3 concise CPU baseline is implemented:
	- background grid transfer loop (P2G -> grid update -> G2P)
	- linear-elastic stress update with configurable material parameters
	- deterministic falling-block benchmark scene initialization
- CUDA backend path exists for DEM stepping.
- Python binding provides 2D DEM APIs and an MPM scene runner.
- SPH remains scaffolded for upcoming implementation.

## Repository Layout

```
include/particle_methods/core/      # shared particle types and containers
include/particle_methods/solvers/   # DEM/SPH/MPM solver interfaces
include/particle_methods/cuda/      # CUDA backend interfaces
src/core/                           # core implementations
src/solvers/                        # solver implementations
src/cuda/                           # CUDA kernels/wrappers
src/examples/                       # C++ executable demos
bindings/                           # pybind11 Python module
python/                             # Python package and examples
docs/ROADMAP.md                     # practical implementation roadmap
.github/prompts/                    # reusable prompts for future agent runs
```

## Build (CPU Only)

```bash
cmake -S . -B build -DPM_ENABLE_CUDA=OFF -DPM_ENABLE_PYTHON=OFF -DPM_BUILD_TESTS=ON
cmake --build build -j
```

Run the phase-1 benchmark demo:

```bash
./build/falling_particles_2d --scene pile_formation --particles 400 --steps 500 --seed 42
```

Available scenes:
- `single_bounce`
- `pile_formation`
- `hopper`

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Build With CUDA and Python

```bash
cmake -S . -B build -DPM_ENABLE_CUDA=ON -DPM_ENABLE_PYTHON=ON
cmake --build build -j
```

Notes:
- CUDA build requires a working CUDA toolkit and compatible compiler.
- Python binding target is generated as `_core` (via pybind11).

## Python Usage (After Building Python Module)

```python
import particle_methods

sim = particle_methods.FallingParticles2D(particle_count=256, spacing=0.05, start_height=0.2)
sim.step(steps=400, substeps=4)
positions = sim.positions()
print(len(positions))
print(positions[:3])

scene_positions = particle_methods.run_dem_scene(
	scene_name="hopper",
	particle_count=300,
	steps=300,
	seed=7,
	use_cuda=False,
	substeps=2,
)
print(len(scene_positions))

mpm_positions = particle_methods.run_mpm_scene(
	scene_name="falling_block",
	particle_count=196,
	steps=250,
	seed=7,
	substeps=1,
)
print(len(mpm_positions))
```

## Practical Roadmap

See `docs/ROADMAP.md` for the phased implementation plan and exit criteria.

## Saved Prompt for Future Agent Implementation

Use:

`.github/prompts/implement-particle-methods.prompt.md`

This prompt is intended to drive milestone-by-milestone implementation in future agent sessions.
