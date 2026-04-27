# Particle Methods Practical Roadmap

This roadmap is designed for iterative development of DEM, SPH, and MPM with a 2D-first strategy, CUDA acceleration, and Python usability.

## Guiding Principles

1. Build one stable vertical slice first: simulation core + one working method (DEM) + Python API.
2. Keep solver logic modular and method-specific while sharing common particle/container utilities.
3. Validate CPU behavior before moving kernels to CUDA.
4. Require numerical checks and basic performance checks at every milestone.
5. Keep examples small, deterministic, and quick to run.

## Phase 0: Foundation (Current)

Status: complete

Deliverables:
- CMake-based C++ project structure.
- Common particle data model (`Vec2`, `Particle`, `ParticleSystem`, `SimulationConfig`).
- Solver interfaces for DEM/SPH/MPM.
- Working DEM gravity + collision baseline in CPU.
- CUDA DEM stepping path (host-device copy each step for now).
- C++ demo for 2D falling particles.
- Python bindings to run and inspect particle positions.

Exit criteria:
- Repository configures and builds with CPU-only options.
- DEM demo runs and prints particle positions.
- Python module can run a short simulation and return particle positions (`[[x, y], ...]`).

## Phase 1: Robust DEM 2D

Goal: make DEM physically and numerically stable enough for benchmarks.

Status: CPU implementation in progress (core deliverables implemented, CUDA optimization tasks pending)

Tasks:
- Add particle-particle contact force model (normal + damping + tangential friction).
- Add neighbor search (uniform grid / cell list).
- Add configurable boundary types (floor, walls, box).
- Add deterministic seedable initialization helpers.
- Add benchmark scenes: single bounce, pile formation, hopper.

CUDA tasks:
- Move contact detection and contact accumulation to kernels.
- Keep host-device traffic minimal (persistent device buffers).
- Add CUDA validation tests against CPU references.

Exit criteria:
- DEM 2D supports thousands of particles on CPU.
- CUDA DEM is measurably faster than CPU at moderate particle counts.
- Unit tests pass for contact model invariants.

## Phase 2: SPH 2D Baseline

Goal: add incompressible-ish fluid baseline in 2D.

Tasks:
- Implement kernels (poly6/spiky-like) and density/pressure estimation.
- Implement pressure and viscosity forces.
- Add boundary handling (ghost particles or boundary force).
- Add CFL-based timestep limiter.
- Add fluid scenes: dam break and falling droplet.

CUDA tasks:
- Reuse grid neighbor search infrastructure from DEM.
- Add SPH density and force kernels.

Exit criteria:
- Dam-break scene runs with stable density fluctuations.
- SPH has CPU and CUDA backends sharing the same interfaces.
- Python API can run SPH and return trajectories.

## Phase 3: MPM 2D Baseline

Goal: add grid-particle hybrid simulation in 2D.

Status: concise CPU baseline in progress (P2G/grid-update/G2P and falling-block scene available)

Tasks:
- Implement background grid data structures.
- Implement particle-to-grid (P2G), grid update, grid-to-particle (G2P).
- Start with elastic material model.
- Add basic boundary conditions and gravity scene.
- Add standard MPM demos: falling block / column collapse.

CUDA tasks:
- Implement P2G and G2P kernels with atomic strategies.
- Profile memory access and kernel occupancy.

Exit criteria:
- MPM 2D stable for small-to-medium particle counts.
- CPU and CUDA versions produce comparable trajectories.
- Python API supports selecting MPM solver.

## Phase 4: Unified API and Tooling

Goal: make the framework pleasant for research and rapid iteration.

Tasks:
- Unify solver selection in C++ and Python (`dem`, `sph`, `mpm`).
- Add configuration loading from JSON or YAML.
- Add consistent scene runner and output writer (CSV or NPZ).
- Add plotting helper scripts in Python.
- Add CI: CPU build + tests + lint.

Exit criteria:
- One Python entrypoint can run any supported solver in 2D.
- Basic docs cover build, run, and extension points.
- CI protects the main branch from regressions.

## Phase 5: 3D Expansion and Optimization

Goal: extend validated 2D methods into 3D and optimize for larger scale.

Tasks:
- Extend vector/particle/grid data to 3D.
- Port 2D benchmarks to 3D equivalents.
- Add memory pooling and SoA data layout where profitable.
- Optimize CUDA kernels and reduce branch divergence.

Exit criteria:
- 3D scenes run for DEM/SPH/MPM with documented limitations.
- Performance dashboard tracks CPU/CUDA speedups over time.

## Testing Strategy

1. Unit tests
- Math helpers and integration steps.
- Contact force and boundary response.
- SPH density and pressure sanity checks.
- MPM mass/momentum transfer checks.

2. Regression tests
- Fixed-seed scenes with trajectory snapshots.
- CPU vs CUDA tolerance checks on selected frames.

3. Performance tests
- Runtime vs particle count for each method.
- Throughput trends recorded per commit (optional in CI at first).

## Suggested Milestone Sequence (Practical)

1. Harden DEM CPU + neighbor search.
2. Optimize DEM CUDA.
3. Build SPH CPU, then SPH CUDA.
4. Build MPM CPU, then MPM CUDA.
5. Unify Python API and add scene tooling.

## Definition of Done for Each Method

1. Reproducible 2D reference examples.
2. CPU and CUDA implementations with basic numerical agreement.
3. Python-facing API with scene setup and result extraction.
4. Documentation of assumptions, limitations, and recommended timestep ranges.
