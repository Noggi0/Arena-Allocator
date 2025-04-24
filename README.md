# Arena-Allocator

This repository contains a custom Arena Allocator implemented in C++, a python script that runs a couple of benchmarks. The script also creates graphs to visualize the difference between default allocations and allocations using the Arena.

## Setup
To run the code, just run this command : `xmake build && xmake run`
I target x64 architecture because x86 (32-bits) limits the user address space to 2GB, and can cause crashes in big benchmarks.

To run the benchmark script, just run : `python run_benchmark.py [NUM_ALLOCATIONS=1000000]`
You can play with the NUM_ALLOCATIONS argument, and the default is 1000000. I use Matplotlib to generate graphs, and I store .png files in `/result/`.