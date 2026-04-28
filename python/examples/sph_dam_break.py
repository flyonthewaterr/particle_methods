import particle_methods


def main() -> None:
    positions = particle_methods.run_sph_scene(
        scene_name="dam_break",
        particle_count=300,
        steps=200,
        seed=7,
        substeps=1,
    )

    print("particle count:", len(positions))
    print("first five rows:")
    for row in positions[:5]:
        print(row)


if __name__ == "__main__":
    main()
