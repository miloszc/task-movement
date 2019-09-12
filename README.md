# Task Movement: A Tabu Search algorithm to minimize the overall energy usage and the execution time of stencil computations

## Overview

Task Movement is an algorithm that takes into account different network topologies to minimize both the runtime
and the energy usage of stencil computations on supercomputers.

This software consists of:
* **Task Movement (TM) algorithm**
* **5 simpler heuristics**: Load Balancing (LB), Degree minimization (DM), Multicut
minimization (MM) and Neighbours accumulation (NA), Local Search (LS)
* **ILP model** that solves the problem optimally
* **input-generator** software to generate the stencil workload and three types of network topologies with heterogeneous processors utilized in supercomputers
* **data** for input-genertor that represents: Fat-Tree, Dragonfly and Torus network topologies

## Usage

### Compiling Task Movement

The task-movement directory contains a CMakeLists.txt file. To install it the cmake software (https://cmake.org/) is neeed. 
Steps needed to build on Windows:

* install and run cmake-gui
* in "Where is source code" provide a location to the CmakeLists.txt file
* in task-movement directory create a new direcotry called "build"
* in "Where to build binaries" provide a path to the "build" directory
* click the "Generate" button 
* from "Specify the generator for this project" select "Visual Studio 15 2017" and click the button "Finish"
* click buttons "Configure" and "Generate"
* open the "SchedulerProject.sln" file and compile the program in Visual Studio
* set as StartUp project to "scheduler"

### Compiling input-generator

Follow the same steps as in section "Compiling Task Movement"
  
### Usage

#### Task movement:

```
Version 1.0: scheduler <path> <idx> <order> <load_factor>
                -path           the input path for the task and processor graphs
                -idx            to select algorithm; possible values 1(LB)|3(M)|4(DM)|5(NA)|6(MM)|7(LS)|8(TM)
                -order          sorting order of input tasks; used in alg. 1
                        IJK = 0,
                        JIK = 1,
                        KIJ = 2,
                        JKI = 3,
                        RANDOM = 4
                -load_factor            load factor; used in alg. 5
                        possible values [0.0,1.0]
```
						
#### ILP steps:

1. Set parameters of the mesh and processors in grid.txt, stencil.txt, nprocessors.txt, vertex_default_wieght.txt, edge_default_weights.txt or take the input form the data directory.
2. Generate the mesh and processors using input-genertor. Should be exectued in the same directory as input data:
./cartesian_mesh <x-periodicy> <y-periodicy> <z-periodicy> <network-type>
3. Move from ZIMPL to LP:
zimpl stencil_energy_model.zpl
4. Solve ILP problem:
scip -f stencil_energy_model.lp

### External Dependencies

Task movement depends on the following external software:

* [Koala](http://koala.os.niwa.gda.pl/api/)

ILP model can be solved using:

* [SCIP](https://scip.zib.de/)

## License (MIT License)

MIT License

Copyright (c) 2019 miloszc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
