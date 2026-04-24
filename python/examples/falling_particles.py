import particle_methods


def main() -> None:
    positions = particle_methods.run_dem_scene(
        scene_name="pile_formation",
        particle_count=256,
        steps=300,
        seed=123,
        use_cuda=False,
        substeps=2,
    )

    print("particle count:", len(positions))
    print("first five rows:")
    for row in positions[:5]:
        print(row)


if __name__ == "__main__":
    main()
