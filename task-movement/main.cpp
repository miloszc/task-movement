/*
 * main.cpp
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#include <iostream>

// uncomment to get more output
//#define PRINT_DEBUG 1
#include "main.h"

#include <cstdlib>
#include <stdio.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <cstring>
#include <climits>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iomanip>

#include "SchedulerConfig.h"
#include "basic_types.h"
#include "taskgraph.h"
#include "processorgraph.h"
#include "multigraph.h"
#include "schedparams.h"
#include "read.h"
#include "scheduling.h"
#include "cost.h"

#include "Koala/coloring/edge.h"
#include "Koala/io/graphml.h"

using namespace Koala;
using namespace std;
using namespace scheduler;

/*
 * Program starts by reading task and processor graphs.
 * Creates mulitGraph from processor and task graphs.
 */
int main(int argc, char *argv[]) {
    TaskGraph taskGraph;
    ProcessorGraph processorGraph;

    string path;
    SchedParams schedParams;
    if(argc != 5) {
        printf("Version %d.%d: scheduler <path> <idx> <order> <load_factor>\n\
                -path\t\tthe input path for the task and processor graphs\n\
                -idx\t\tto select algorithm; possible values 1(LB)|3(M)|4(DM)|5(NA)|6(MM)|7(LS)|8(TM)\n\
                -order\t\tsorting order of input tasks; used in alg. 1\n\
                \tIJK = 0,\n\
                \tJIK = 1,\n\
                \tKIJ = 2,\n\
                \tJKI = 3,\n\
                \tRANDOM = 4\n\
                -load_factor\t\tload factor; used in alg. 5\n\
                \tpossible values [0.0,1.0]\n",
				Scheduler_VERSION_MAJOR,
				Scheduler_VERSION_MINOR);
        return 1;
    } else {
        path = string(argv[1]);
        sscanf(argv[2], "%d", &schedParams.alg_idx);
        sscanf(argv[3], "%d", &schedParams.task_order);
        sscanf(argv[4], "%f", &schedParams.f);
#ifdef PRINT_DEBUG
        printf("%s %d %d %f\n", path.c_str(), schedParams.alg_idx, schedParams.task_order, schedParams.f);
#endif
    }

    srand( time( NULL ) );

    Vector3i gridSize;

    // Reads task and processor graphs
    read_graph(path, taskGraph, processorGraph, gridSize);

    MultiGraph multiGraph;
    scheduling(taskGraph, processorGraph, multiGraph, gridSize, schedParams);
    scheduler::Cost cost;
    calculate_cost(multiGraph/*, schedParams*/, cost, VIZING, true);

	Koala::AssocArray<MultiGraphEdge, int> colors;
	colors.clear();

	std::cout << cost << std::endl;
	print_n_edges(multiGraph);
	print_n_pus(multiGraph);

    color_task_vertices(taskGraph, multiGraph);

	write_GrapML(taskGraph, processorGraph, multiGraph);

    return 0;
}
