# Facmaker

A program for simulating input/output graphs

Meant to be used in games such as modded Minecraft, Factorio, etc, but can adapt to any scenario.

## Installing

Cloning the repo:

```
git clone --recursive https://github.com/aleokdev/facmaker
cd facmaker
```

Setting up the build directory:

```
cmake --list-presets
# Replace {Preset} with any of the presets listed
cmake --preset {Preset}
```

Building the repo:

```
cmake --build --list-presets
# Replace {Preset} with any of the presets listed
cmake --build --preset {Preset}
```

If it doesn't work, check the CMake presets and try again.

Executing the program:

```
./facmaker
```