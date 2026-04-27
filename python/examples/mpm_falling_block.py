import particle_methods


def main() -> None:
    positions = particle_methods.run_mpm_scene(
        scene_name="falling_block",
        particle_count=196,
        steps=250,
        seed=7,
        substeps=1,
    )

    print("particle count:", len(positions))
    print("first five rows:")
    for row in positions[:5]:
        print(row)


if __name__ == "__main__":
    main()
