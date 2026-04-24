---
description: "Use this prompt to implement or extend particle_methods milestones (DEM/SPH/MPM, CUDA, Python interface) in practical, testable increments."
---

# Implement Particle Methods Milestone

You are implementing the `particle_methods` repository.

## Context

- Language stack: C++ + CUDA + Python bindings.
- Strategy: 2D first, simple scenes first, correctness first, then performance.
- Current baseline includes a DEM 2D falling-particles example and skeleton SPH/MPM solvers.
- The roadmap is in `docs/ROADMAP.md`.

## Task

Implement the next milestone in `docs/ROADMAP.md` with production-quality code and tests.

## Requirements

1. Keep all public APIs backward compatible unless explicitly migrating.
2. Use modular architecture:
- `include/particle_methods/core`
- `include/particle_methods/solvers`
- `src/core`, `src/solvers`, `src/cuda`, `bindings`
3. Add or update tests for new behavior.
4. Prefer CPU reference implementation before CUDA kernel port.
5. Add Python exposure for any new solver capability.
6. Update documentation for build/run and new scene examples.

## Minimum Output for Each Milestone

1. Code changes implementing the feature.
2. Validation checks:
- build result
- test result
- short runtime smoke test
3. Notes on numerical assumptions and known limitations.
4. Updated roadmap status if milestone completion changed.

## Implementation Checklist

1. Read `docs/ROADMAP.md` and choose the next unfinished deliverable.
2. Implement CPU path.
3. Add tests and deterministic example.
4. Port/accelerate with CUDA if requested by the milestone.
5. Expose API in Python bindings.
6. Run build/test and summarize results.

## Quality Bar

- No placeholder TODOs without an issue or roadmap reference.
- No silent failure paths.
- Errors should be explicit and actionable.
- Keep docs aligned with actual commands and file paths.
