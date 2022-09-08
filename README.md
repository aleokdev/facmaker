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
mkdir build
cd build
cmake -B. -S..
```

Building the repo:

```
cmake --build .
```

Executing the program:

```
./facmaker
```