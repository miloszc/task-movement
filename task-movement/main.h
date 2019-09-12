/*
 * main.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "basic_types.h"
#include "taskgraph.h"
#include "processorgraph.h"
#include "multigraph.h"
#include "schedparams.h"
#include "cost.h"

#include "Koala/coloring/edge.h"
#include "Koala/io/graphml.h"

#include <chrono>
#include <string>

namespace scheduler {
	enum ColoringAlgorithm { SIMPLE = 1, GREEDY, VIZING };

	// slow but more precise method
	int get_vizing_ncolors(MultiGraph &multiGraph, Koala::AssocArray<MultiGraphEdge, int> &colors)
	{
		//if we don't know if the graph is simple we should use vizing
		if (multiGraph.getEdgeNo(Koala::EdAll) > 0) {
			return Koala::SeqEdgeColoring::vizing(multiGraph, colors) + 1;
		}

		return 0;
	}

	int get_greedy_ncolors(MultiGraph &multiGraph, Koala::AssocArray<MultiGraphEdge, int> &colors)
	{
		//if we don't know if the graph is simple we should use vizing
		if (multiGraph.getEdgeNo(Koala::EdAll) > 0) {
			return Koala::SeqEdgeColoring::greedy(multiGraph, colors) + 1;
		}

		return 0;
	}

	// Vizing
	// X`(G) = Delta(G) + mu(G)
	int get_simple_ncolors(MultiGraph &multiGraph)
	{
		return multiGraph.Delta() + multiGraph.mu();
	}

	int ColorGraph(MultiGraph &multiGraph, Koala::AssocArray<MultiGraphEdge, int> &colors, ColoringAlgorithm cl)
	{
		int ncolors = 0;

		if (cl == SIMPLE)
		{
			ncolors = get_simple_ncolors(multiGraph);
		}
		else if (cl == GREEDY)
		{
			ncolors = get_greedy_ncolors(multiGraph, colors);
		}
		else
		{
			ncolors = get_vizing_ncolors(multiGraph, colors);
		}

		return ncolors;
	}

	long long get_n_intranode_edges(MultiGraph &multiGraph)
	{
		long long n_intranode_edges = 0;

		for (MultiGraphEdge ee = multiGraph.getEdge(); ee; ee = multiGraph.getEdgeNext(ee)) {
			MultiGraphVertex v1 = multiGraph.getEdgeEnd1(ee);
			MultiGraphVertex v2 = multiGraph.getEdgeEnd2(ee);

			if ((v1->info.id / 8) == (v2->info.id / 8))
			{
				++n_intranode_edges;
			}
		}

		return n_intranode_edges;
	}

	long long get_n_internode_edges(MultiGraph &multiGraph, long long n_intranode_edges)
	{
		return multiGraph.getEdgeSet().size() - n_intranode_edges;
	}

	long long get_used_n_cpus(MultiGraph &multiGraph)
	{
		int n_cpus = 0;

		for (MultiGraphVertex mv = multiGraph.getVert(); mv; mv = multiGraph.getVertNext(mv)) {
			if ((mv->info.id % 8 < 2) && (mv->info.ngrid_cells > 0))
			{
				++n_cpus;
			}
		}

		return n_cpus;
	}

	long long get_used_n_gpus(MultiGraph &multiGraph)
	{
		int n_gpus = 0;

		for (MultiGraphVertex mv = multiGraph.getVert(); mv; mv = multiGraph.getVertNext(mv)) {
			if ((mv->info.id % 8 > 1) && (mv->info.ngrid_cells > 0))
			{
				++n_gpus;
			}
		}

		return n_gpus;
	}

	void print_n_pus(MultiGraph &multiGraph)
	{
		std::cout << "n_cpus: " << get_used_n_cpus(multiGraph) <<
			"\nn_gpus" << get_used_n_gpus(multiGraph) << std::endl;
	}

	void print_n_edges(MultiGraph &multiGraph)
	{
		int n_intranode_edges = get_n_intranode_edges(multiGraph);

		std::cout << "intranode edges: " << n_intranode_edges <<
			"\ninternode edges: " << get_n_internode_edges(multiGraph, n_intranode_edges) << std::endl;
	}

	void calculate_cost(MultiGraph &multiGraph/*, SchedParams &schedParams*/, Cost &cost, ColoringAlgorithm cl, bool calculateDeltaMu, bool calculateColors = true) {
		// communicaiton time
		// number of colors
		int res;
		Koala::AssocArray<MultiGraphEdge, int> colors;
		colors.clear();

		auto start = std::chrono::high_resolution_clock::now();

		int ncolors = calculateColors ? ColorGraph(multiGraph, colors, cl) : 0;

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

#ifdef PRINT_DEBUG
		std::cout << "vizing time elapsed: " << elapsed.count() << " ns" << std::endl;

		std::cout << "max deg: " << multiGraph.Delta() << " mul:" << +multiGraph.mu() << " " << std::endl;
#endif

		long long communication_time = 0;
#ifdef PRINT_DEBUG
		std::cout << "Number of colors: " << ncolors << "\n";
#endif
		// here we assume that all edges are equal: have the same bandwidth and weight
		// solution: we have to multiply the number of edges added to MultiGraph from TaskGraph based on bandwidth and weight
		// then communication_time == ncolors
		communication_time = ncolors;
#ifdef PRINT_DEBUG
		printf("Communication time:%lld\n", communication_time);
#endif
		// computation time
		long long max_computation_time = 0;
		for (MultiGraphVertex mv = multiGraph.getVert(); mv; mv = multiGraph.getVertNext(mv)) {
			if (max_computation_time < (mv->info.ngrid_cells * mv->info.t_c)) max_computation_time = (mv->info.ngrid_cells * mv->info.t_c);
		}
#ifdef PRINT_DEBUG
		printf("Computation time:%lld\n", max_computation_time);
#endif
		long long max_time = communication_time > max_computation_time ? communication_time : max_computation_time;
#ifdef PRINT_DEBUG
		printf("Summary time: %lld\n", max_time);
#endif
		// energy cost for computation
		long long energy_computation = 0;
		for (MultiGraphVertex mv = multiGraph.getVert(); mv; mv = multiGraph.getVertNext(mv)) {
			mv->info.t_idle = max_time - (mv->info.ngrid_cells * mv->info.t_c);
			energy_computation += mv->info.e_c * mv->info.ngrid_cells +
				mv->info.t_c * mv->info.ngrid_cells * mv->info.p0 +
				mv->info.t_idle * mv->info.p_idle;
		}
#ifdef PRINT_DEBUG
		printf("Energy computation: %ld\n", energy_computation);
#endif
		// energy cost for communication
		long long energy_communication = 0;
		for (MultiGraphEdge ee = multiGraph.getEdge(); ee; ee = multiGraph.getEdgeNext(ee)) {
			energy_communication += (ee->info.e_e / ee->info.t_e) * ee->info.taskEdgeInfo.d;
		}

#ifdef PRINT_DEBUG
		printf("Energy communication: %lld #edges:%d\n", energy_communication, multiGraph.getEdgeNo());
#endif
		long long energy_cost = energy_computation + energy_communication;
		// summary energy usage
#ifdef PRINT_DEBUG
		printf("Energy usage: %lld\n", energy_cost);
#endif

		/*char alg_name[32];
		snprintf(alg_name, sizeof alg_name, "Alg_%d", schedParams.alg_idx);

		cost.alg_name.append(alg_name);
		cost.task_order = schedParams.task_order;
		cost.f = schedParams.f;*/
		cost.nedges = multiGraph.getEdgeNo();
		if (calculateDeltaMu)
		{
			cost.delta = multiGraph.Delta();
			cost.mu = multiGraph.mu();
		}
		cost.ncolors = ncolors;
		cost.summary_energy = energy_cost;
		cost.summary_time = max_time;
		cost.computation_time = max_computation_time;
		cost.communication_time = communication_time;
		cost.computation_energy = energy_computation;
		cost.communication_energy = energy_communication;

#ifdef PRINT_DEBUG
		std::cout << cost << std::endl;
#endif
	}

	void write_TaskGraph_to_GraphML(TaskGraph &taskGraph) {
		Koala::IO::GraphML gml;
		Koala::IO::GraphMLGraph *gmlg;

		//put into GraphML
		gmlg = gml.createGraph("TaskGraph");
		gmlg->writeGraph(taskGraph,
			Koala::IO::gmlIntField(&TaskVertexInfo::id, "vid")
			& Koala::IO::gmlIntField(&TaskVertexInfo::w, "vw")
			& Koala::IO::gmlIntField(&TaskVertexInfo::x, "x")
			& Koala::IO::gmlIntField(&TaskVertexInfo::y, "y")
			& Koala::IO::gmlStringField(&TaskVertexInfo::color, "color")
			& Koala::IO::gmlStringField(&TaskVertexInfo::pid, "pid"),
			Koala::IO::gmlIntField(&TaskEdgeInfo::v1, "ev1")
			& Koala::IO::gmlIntField(&TaskEdgeInfo::v2, "ev2")
			& Koala::IO::gmlIntField(&TaskEdgeInfo::d, "ed")
		);
		//write GraphML to a file
		gml.writeFile("TaskGraph.graphml");
	}

	void write_ProcessorGraph_to_GraphML(ProcessorGraph &processorGraph) {
		Koala::IO::GraphML gml;
		Koala::IO::GraphMLGraph *gmlg;

		//put into GraphML
		gmlg = gml.createGraph("ProcessorGraph");
		gmlg->writeGraph(processorGraph, Koala::IO::gmlStringField(&ProcessorVertexInfo::name, "vname")
			& Koala::IO::gmlIntField(&ProcessorVertexInfo::t_c, "vt_c")
			& Koala::IO::gmlStringField(&ProcessorVertexInfo::color, "color"),
			Koala::IO::gmlStringField(&ProcessorEdgeInfo::name1, "ename1")
			& Koala::IO::gmlStringField(&ProcessorEdgeInfo::name2, "ename2")
			& Koala::IO::gmlIntField(&ProcessorEdgeInfo::t_e, "et_e")
		);
		//write GraphML to a file
		gml.writeFile("ProcessorGraph.graphml");
	}

	void write_MultiGraph_to_GraphML(MultiGraph &multiGraph, std::string graphName) {
		Koala::IO::GraphML gml;
		Koala::IO::GraphMLGraph *gmlg;

		//put into GraphML
		gmlg = gml.createGraph("MultiGraph");;
		gmlg->writeGraph(multiGraph, Koala::IO::gmlStringField(&MultiGraphVertexInfo::name, "vname")
			& Koala::IO::gmlIntField(&MultiGraphVertexInfo::t_c, "vt_c")
			& Koala::IO::gmlIntField(&MultiGraphVertexInfo::ngrid_cells, "vngrid_cells")
			& Koala::IO::gmlIntField(&MultiGraphVertexInfo::t_idle, "vt_idle"),
			Koala::IO::gmlStringField(&MultiGraphEdgeInfo::name1, "ep1")
			& Koala::IO::gmlStringField(&MultiGraphEdgeInfo::name2, "ep2")
			& Koala::IO::gmlIntField(&MultiGraphEdgeInfo::t_e, "et_e")
		);
		//write GraphML to a file
		gml.writeFile(std::string(graphName + ".graphml").c_str());
	}

	void write_GrapML(TaskGraph &taskGraph, ProcessorGraph &processorGraph, MultiGraph &multiGraph)
	{
		write_TaskGraph_to_GraphML(taskGraph);
		write_ProcessorGraph_to_GraphML(processorGraph);
		write_MultiGraph_to_GraphML(multiGraph, "MultiGraph");
	}

} /* namespace scheduler */

#endif /* MAIN_H_ */
