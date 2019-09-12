/*
 * scheduling.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#include "basic_types.h"
#include "taskgraph.h"
#include "processorgraph.h"

#include <conio.h>
#include <chrono>
#include <string>
#ifdef __GNUC__
#include <sys/time.h>
#endif

namespace scheduler {

	typedef std::map<std::string, MultiGraphVertex> MultiGraphProcessorMap;
	typedef std::map<std::pair<std::string, std::string>, ProcessorEdgeInfo> ProcessorEdgeMap;
	typedef std::map<std::pair<int, int>, TaskEdgeInfo> TaskEdgeMap;
	typedef std::map<int, TaskVertex> TaskVertexMap;
	typedef std::map<int, MultiGraphVertex> MultiGraphTaskIdOnProcessorMap;
	typedef std::map<std::pair<int, std::string>, std::pair<int/*tenures*/, int/*frequency*/>> TabuMap;

	// creates deep copy of mutliGraph
	void create_multiGraph_copy(MultiGraph &multiGraph, MultiGraph &multiGraphCopy, bool copyTasks = false)
	{
		multiGraphCopy.clear();

		MultiGraphProcessorMap multiGraphProcessorMap;

		// copy of vertices
		for (MultiGraphVertex mv = multiGraph.getVert(); mv; mv = multiGraph.getVertNext(mv)) {
			MultiGraphVertexInfo multiGraphVertexInfo(mv->info);
			MultiGraphVertex mvCopy = multiGraphCopy.addVert(multiGraphVertexInfo);
			multiGraphProcessorMap.insert(std::pair<std::string, MultiGraphVertex>(multiGraphVertexInfo.name, mvCopy));
			if (copyTasks) {
				for (auto it = mv->info.taskVertexVectorMappedMap.begin(); it != mv->info.taskVertexVectorMappedMap.end(); ++it) {
					mvCopy->info.taskVertexVectorMappedMap.insert(std::pair<int, TaskVertex>(it->first, it->second));
				}
			}
		}

		// copy of edges
		for (MultiGraphEdge ee = multiGraph.getEdge(); ee; ee = multiGraph.getEdgeNext(ee)) {
			MultiGraphVertex mv1 = multiGraph.getEdgeEnd1(ee);
			MultiGraphVertex mv2 = multiGraph.getEdgeEnd2(ee);

			MultiGraphProcessorMap::iterator itMultiGraphProcessorMap1 = multiGraphProcessorMap.find(mv1->info.name);
			MultiGraphProcessorMap::iterator itMultiGraphProcessorMap2 = multiGraphProcessorMap.find(mv2->info.name);

			multiGraphCopy.addEdge(itMultiGraphProcessorMap1->second, itMultiGraphProcessorMap2->second, MultiGraphEdgeInfo(ee->info), multiGraph.getEdgeDir(ee, mv1));
		}
	}

	// Move incident edges from Task to Processor
	void reconnect_incident_edges(MultiGraph &multiGraph,
		MultiGraphVertex &multiGraphProcessorVertex,
		MultiGraphVertex &multiGraphTaskVertex,
		ProcessorEdgeMap &processorEdgeMap) {
		// reconnect edges
		Koala::Set<MultiGraphVertex> multiGraphTaskVertexSet;
		multiGraphTaskVertexSet.add(multiGraphTaskVertex);
		// incident edges
		Koala::Set<MultiGraphEdge> multiGraphTaskEdgeSet = multiGraph.getIncEdgeSet(multiGraphTaskVertexSet, Koala::EdAll, Koala::Directed);
		multiGraphTaskVertexSet.clear();
		// reconnect incident edges to processor
		for (MultiGraphEdge me = multiGraphTaskEdgeSet.first(); me;
			me = multiGraphTaskEdgeSet.next(me)) {
			MultiGraphVertex mv1 = multiGraph.getEdgeEnd1(me);
			MultiGraphVertex mv2 = multiGraph.getEdgeEnd2(me);

			MultiGraphVertex mvNew1;
			MultiGraphVertex mvNew2;

			const Koala::EdgeDirection edgeDirection = me->getDir(mv1);
			// check which an edge's vertex equals to our task and set this vertex to a processor vertex
			if (multiGraphTaskVertex == mv1) {
				mvNew1 = multiGraphProcessorVertex;
				mvNew2 = mv2;
			}
			if (multiGraphTaskVertex == mv2) {
				mvNew1 = mv1;
				mvNew2 = multiGraphProcessorVertex;
			}
			// edge is mapped to the same processor delete the loop
			if (mvNew1 == mvNew2) {
				multiGraph.delEdge(me);
			}
			else {
				ProcessorEdgeMap::iterator it1 = processorEdgeMap.find(std::pair<std::string, std::string>(mvNew1->info.name, mvNew2->info.name));
				// if edge is between processor p1 and p2
				if (it1 != processorEdgeMap.end()) {
					ProcessorEdgeInfo pei = it1->second;
					// multiply the number of edges between processors p1 and p2 for graph coloring
					// here we simply convert each task edge to multiple edges with the same bandwidth and weight
					for (int nedges = 0; nedges < pei.t_e * me->info.taskEdgeInfo.d; ++nedges) {
						// weight equal to 1
						TaskEdgeInfo taskEdgeInfo = TaskEdgeInfo(me->info.taskEdgeInfo.v1, me->info.taskEdgeInfo.v2, /*d*/1);
						// time to send single grid cell equals to 1
						ProcessorEdgeInfo processorEdgeInfo(mvNew1->info.name, mvNew2->info.name, /*t_e*/pei.t_e, pei.e_e);
						/*MultiGraphEdge mge = */multiGraph.addEdge(mvNew1, mvNew2, MultiGraphEdgeInfo(taskEdgeInfo, processorEdgeInfo), edgeDirection);
						//printf("Add edge to MultiGraph: %s %s %d %d\n", mge->info.name1.c_str(), mge->info.name2.c_str(), mge->info.t_e, mge->info.taskEdgeInfo.d);
					}
					multiGraph.delEdge(me);
				}
				else { // move an edge to new vertices mv1 and mv2
					if (multiGraph.moveEdge(me, mvNew1, mvNew2, edgeDirection) == false) {
						printf("Could not move edge (%s,%s)\n", mvNew1->info.name.c_str(), mvNew2->info.name.c_str());
					}
				}
			}
		}
	}

	Vector3i idx_last = {};

	MultiGraphVertex popMultiGraphTaskVertex(
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		Vector3i gridSize,
		TaskOrder task_order,
		int &idx
	) {
		idx = idx_last.x + idx_last.y * gridSize.x +
			idx_last.z * gridSize.x * gridSize.y;

		switch (task_order) {
		case IJK: ++idx_last.x;
			if (idx_last.x == gridSize.x) { ++idx_last.y; idx_last.x = 0; }
			if (idx_last.y == gridSize.y) { ++idx_last.z; idx_last.y = 0; }
			if (idx_last.z == gridSize.z) { idx_last.z = 0; }
			break;
		case JIK: ++idx_last.y;
			if (idx_last.y == gridSize.y) { ++idx_last.x; idx_last.y = 0; }
			if (idx_last.x == gridSize.x) { ++idx_last.z; idx_last.x = 0; }
			if (idx_last.z == gridSize.z) { idx_last.z = 0; }
			break;
		case KIJ: ++idx_last.z;
			if (idx_last.z == gridSize.z) { ++idx_last.x; idx_last.z = 0; }
			if (idx_last.x == gridSize.x) { ++idx_last.y; idx_last.x = 0; }
			if (idx_last.y == gridSize.y) { idx_last.y = 0; }
			break;
		case JKI: ++idx_last.y;
			if (idx_last.y == gridSize.y) { ++idx_last.z; idx_last.y = 0; }
			if (idx_last.z == gridSize.z) { ++idx_last.x; idx_last.z = 0; }
			if (idx_last.x == gridSize.x) { idx_last.x = 0; }
			break;
		case KJI: ++idx_last.z;
			if (idx_last.z == gridSize.z) { ++idx_last.y; idx_last.z = 0; }
			if (idx_last.y == gridSize.y) { ++idx_last.x; idx_last.y = 0; }
			if (idx_last.x == gridSize.x) { idx_last.x = 0; }
			break;
		case IKJ: ++idx_last.x;
			if (idx_last.x == gridSize.x) { ++idx_last.z; idx_last.x = 0; }
			if (idx_last.z == gridSize.z) { ++idx_last.y; idx_last.z = 0; }
			if (idx_last.y == gridSize.y) { idx_last.y = 0; }
			break;
		case RANDOM:
			bool notfound = true;
			while ((multiGraphTaskVertexMap.size() != 0) && (notfound)) {
				int min = 0;
				int max = gridSize.x * gridSize.y * gridSize.z - 1;
				idx = min + (rand() % (int)(max - min + 1));
				typedef std::map<int, MultiGraphVertex> MultiGraphTaskVertexMap_t;
				MultiGraphTaskVertexMap_t::iterator it = multiGraphTaskVertexMap.find(idx);
				if (it != multiGraphTaskVertexMap.end()) {
					notfound = false;
				}
				else {
					notfound = true;
				}
			}
			break;
		}

		typedef std::map<int, MultiGraphVertex> MultiGraphTaskVertexMap_t;
		MultiGraphTaskVertexMap_t::iterator it = multiGraphTaskVertexMap.find(idx);
		if (it != multiGraphTaskVertexMap.end()) {
			return it->second;
		}
		return NULL;
	}

	void map_task_to_MultiGraphVertex(MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap, MultiGraphVertex &multiGraphProcessorVertex,
		MultiGraphVertexInfo &multiGraphTaskVertexInfo, TaskVertexMap &taskVertexMap)
	{
		multiGraphTaskIdOnProcessorMap.insert(std::pair<int, MultiGraphVertex>(multiGraphTaskVertexInfo.taskVertexInfo.id, multiGraphProcessorVertex));

		TaskVertexMap::iterator tvIt = taskVertexMap.find(multiGraphTaskVertexInfo.taskVertexInfo.id);
		TaskVertex tv = tvIt->second;

		/*tv->info.color = multiGraphProcessorVertex->info.color;
		tv->info.pid = multiGraphProcessorVertex->info.name;

		multiGraphTaskVertexInfo.taskVertexInfo.color = multiGraphProcessorVertex->info.color;
		multiGraphTaskVertexInfo.taskVertexInfo.pid = multiGraphProcessorVertex->info.name;*/

		//multiGraphProcessorVertexInfo.taskVertexInfoVectorMapped.push_back(*multiGraphTaskVertexInfo.taskVertexInfo);
		//multiGraphProcessorVertex->info.taskVertexInfoVectorMappedMap.insert(std::pair<int, TaskVertexInfo>(multiGraphTaskVertexInfo.taskVertexInfo.id, tv->info));
		multiGraphProcessorVertex->info.taskVertexVectorMappedMap.insert(std::pair<int, TaskVertex>(multiGraphTaskVertexInfo.taskVertexInfo.id, tv));
	}

	void move_task_to_processor(MultiGraphVertex &pOld, MultiGraphVertex &pNew, int taskId,
		TaskGraph &taskGraph, TaskVertexMap &taskVertexMap, ProcessorEdgeMap &processorEdgeMap, MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		MultiGraph &multiGraph)
	{
		if (pOld->info.name.compare(pNew->info.name) == 0)
		{
			printf("Warning: pOld == pNew %s == %s\n", pOld->info.name.c_str(), pNew->info.name.c_str());
			return;
		}

		//printf("Number of all edges in multiGraph: %d\n", multiGraph.getEdgeNo(Koala::EdAll));

		// multi graph pOld incident edges
		Koala::Set<MultiGraphVertex> multiGraphVertexSet;
		multiGraphVertexSet.add(pOld);
		Koala::Set<MultiGraphEdge> multiGraphEdgeSet = multiGraph.getIncEdgeSet(multiGraphVertexSet, Koala::EdAll, Koala::Directed);

		// map taskId to multi graph processor
		std::map<int, MultiGraphVertex> multiGraphVertexMap;

		// remove edges to/from old processor
		// remove pOld edges for taskId
		for (MultiGraphEdge me = multiGraphEdgeSet.first(); me;
			me = multiGraphEdgeSet.next(me)) {

			MultiGraphVertex mv1 = multiGraph.getEdgeEnd1(me);
			MultiGraphVertex mv2 = multiGraph.getEdgeEnd2(me);

			// edge is mapped to the taskId delete it
			if (me->info.taskEdgeInfo.v1 == taskId || me->info.taskEdgeInfo.v2 == taskId)
			{
				// it does not throw exception when key already exists
				multiGraphVertexMap.insert(std::pair<int, MultiGraphVertex>(me->info.taskEdgeInfo.v1 == taskId ? me->info.taskEdgeInfo.v2 : me->info.taskEdgeInfo.v1, mv1->info.name.compare(pOld->info.name) == 0 ? mv2 : mv1));

				multiGraph.delEdge(me);
			}
		}

		// task graph incident edges
		TaskVertexMap::iterator TaskVertexMap = taskVertexMap.find(taskId);
		Koala::Set<TaskVertex> taskVertexSet;
		taskVertexSet.add(TaskVertexMap->second);
		Koala::Set<TaskEdge> taskEdgeSet = taskGraph.getIncEdgeSet(taskVertexSet, Koala::EdAll, Koala::Directed);

		// add new edges to pNew for taskId
		for (TaskEdge te = taskEdgeSet.first(); te;
			te = taskEdgeSet.next(te)) {
			TaskVertex tv1 = taskGraph.getEdgeEnd1(te);
			TaskVertex tv2 = taskGraph.getEdgeEnd2(te);

			Koala::EdgeDirection edgeDirection = te->getDir(tv1);

			MultiGraphVertex mv1;
			MultiGraphVertex mv2;

			if (tv1->info.id == taskId)
			{
				mv1 = pNew;
				std::map<int, MultiGraphVertex>::iterator it = multiGraphTaskIdOnProcessorMap.find(tv2->info.id);
				mv2 = it->second;
			}

			if (tv2->info.id == taskId)
			{
				std::map<int, MultiGraphVertex>::iterator it = multiGraphTaskIdOnProcessorMap.find(tv1->info.id);
				mv1 = it->second;
				mv2 = pNew;
			}

			// edge is mapped to the same processor do not add new edges
			if (mv1 == mv2) {
				// do nothing
			}
			else {
				ProcessorEdgeMap::iterator it1 = processorEdgeMap.find(std::pair<std::string, std::string>(mv1->info.name, mv2->info.name));
				// if edge is between processor p1 and p2
				if (it1 != processorEdgeMap.end()) {
					ProcessorEdgeInfo pei = it1->second;
					// multiply the number of edges between processors p1 and p2 for graph coloring
					// here we simply convert each task edge to multiple edges with the same bandwidth and weight
					for (int nedges = 0; nedges < pei.t_e * te->info.d; ++nedges) {
						// weight equal to 1
						TaskEdgeInfo taskEdgeInfo = TaskEdgeInfo(te->info.v1, te->info.v2, /*d*/1);
						// time to send single grid cell equals to 1
						ProcessorEdgeInfo processorEdgeInfo(mv1->info.name, mv2->info.name, /*t_e*/pei.t_e, pei.e_e);
						/*MultiGraphEdge mge = */multiGraph.addEdge(mv1, mv2, MultiGraphEdgeInfo(taskEdgeInfo, processorEdgeInfo), Koala::EdDirOut);
						//printf("Add edge to MultiGraph: %s %s %d %d\n", mge->info.name1.c_str(), mge->info.name2.c_str(), mge->info.t_e, mge->info.taskEdgeInfo.d);
					}
				}
			}
		}

		// add task to new processor
		/*std::map<int, TaskVertexInfo>::iterator itVertexInfoMap = pOld->info.taskVertexInfoVectorMappedMap.find(taskId);
		TaskVertexInfo taskVertexInfo = itVertexInfoMap->second;
		pNew->info.taskVertexInfoVectorMappedMap.insert(std::pair<int, TaskVertexInfo>(taskId, taskVertexInfo));
		taskVertexInfo.color = pNew->info.color;
		taskVertexInfo.pid = pNew->info.name;*/

		std::map<int, TaskVertex>::iterator itVertexMap = pOld->info.taskVertexVectorMappedMap.find(taskId);
		TaskVertex taskVertex = itVertexMap->second;
		pNew->info.taskVertexVectorMappedMap.insert(std::pair<int, TaskVertex>(taskId, taskVertex));
		taskVertex->info.color = pNew->info.color;
		taskVertex->info.pid = pNew->info.name;

		MultiGraphTaskIdOnProcessorMap::iterator itMultiGraphTaskIdOnProcessorMap = multiGraphTaskIdOnProcessorMap.find(taskId);
		if (itMultiGraphTaskIdOnProcessorMap != multiGraphTaskIdOnProcessorMap.end())
		{
			itMultiGraphTaskIdOnProcessorMap->second = pNew;
		}
		else {
			std::cout << "error: could not find taskId in multiGraphTaskIdOnProcessorMap!" << std::endl;
		}

		// update ngrid_cells on new processor
		pNew->info.ngrid_cells += taskVertex->info.w;

		// update ngrid_cells on old processor
		pOld->info.ngrid_cells -= taskVertex->info.w;

		if (pNew->info.ngrid_cells < 0 || pOld->info.ngrid_cells < 0)
		{
			std::cout << "error: ngrid_cells smaller than 0! " << pOld->info.ngrid_cells << " " << pNew->info.ngrid_cells << std::endl;
		}

		// remove task from old processor
		pOld->info.taskVertexVectorMappedMap.erase(itVertexMap);
		//return pOld->info.taskVertexInfoVectorMappedMap.erase(itVertexInfoMap);
	}

	void swap_tasks_between_processors(MultiGraphVertex &pOld, int taskIdOld, MultiGraphVertex &pNew, int taskIdNew,
		TaskGraph &taskGraph, TaskVertexMap &taskVertexMap, ProcessorEdgeMap &processorEdgeMap, MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		MultiGraph &multiGraph)
	{
		// move taskIdOld from pOld to pNew
		move_task_to_processor(pOld, pNew, taskIdOld, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);

		// move taskIdNew from pNew to pOld
		move_task_to_processor(pNew, pOld, taskIdNew, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);

		//std::cout << "swap: " << taskIdOld << " " << taskIdNew << std::endl;
	}

	void remove_task_form_MultiGraph(MultiGraph &multiGraph,
		MultiGraphVertex &multiGraphTaskVertex) {
		// delete taskVertex form multiGraph
		if (multiGraph.getEdgeNo(multiGraphTaskVertex) != 0) {
			printf("Not all edges (%d) have been removed for: %d\n",
				multiGraph.getEdgeNo(multiGraphTaskVertex), multiGraphTaskVertex->info.taskVertexInfo.id);
		}
		multiGraph.delVert(multiGraphTaskVertex);
	}

	// Alg. 1 - Balancing load
	void alg1_fill_up(TaskGraph &taskGraph, ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		ProcessorEdgeMap &processorEdgeMap, Vector3i &gridSize,
		double speedup_sum, double w_V,
		SchedParams &schedParams) {
		// alg1 - fill up processors iteratively with tasks
		//int task_id = 0;
		for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
			++mvIt) {
			MultiGraphVertex multiGraphProcessorVertex = *mvIt;
			std::cout << multiGraphProcessorVertex->info.name << std::endl;
			long size = 0;
			MultiGraphVertexInfo *multiGraphProcessorVertexinfo =
				&(multiGraphProcessorVertex->info);

			while (size < (w_V * multiGraphProcessorVertexinfo->speedup / speedup_sum)) {
				int id_selected = 0;
				MultiGraphVertex multiGraphTaskVertex =
					popMultiGraphTaskVertex(multiGraphTaskVertexMap, gridSize,
						schedParams.task_order, id_selected);
				
				if (multiGraphTaskVertex == NULL) break;

				reconnect_incident_edges(multiGraph,
					multiGraphProcessorVertex,
					multiGraphTaskVertex,
					processorEdgeMap);
				TaskVertexInfo taskVertexInfo = multiGraphTaskVertex->info.taskVertexInfo;
				
				// update size/weight for current processor
				size += taskVertexInfo.w;
				map_task_to_MultiGraphVertex(multiGraphTaskIdOnProcessorMap, multiGraphProcessorVertex, multiGraphTaskVertex->info, taskVertexMap);
				remove_task_form_MultiGraph(multiGraph,
					multiGraphTaskVertex);
				
				multiGraphTaskVertexMap.erase(id_selected);
			}
			multiGraphProcessorVertex->info.ngrid_cells = size;
		}
	}

	// Alg. 4 - Small degree assignment
	void alg4_small_degree_assignment(TaskGraph &taskGraph,
		ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		ProcessorEdgeMap &processorEdgeMap,
		double speedup_sum, double w_V) {
		unsigned int task_id = 0;
		// Assign the number of edges to each multiGraphTask
		for (MultiGraphVertex multiGraphTaskVertex = multiGraphTaskVertexMap[task_id]; task_id < multiGraphTaskVertexMap.size(); ++task_id) {
			multiGraphTaskVertex->info.taskVertexInfo.deg = multiGraph.getEdgeNo(multiGraphTaskVertex);
		}
		// Schedule
		for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
			++mvIt) {
			MultiGraphVertex multiGraphProcessorVertex = *mvIt;
			long size = 0;
			MultiGraphVertexInfo *multiGraphProcessorVertexInfo =
				&(multiGraphProcessorVertex->info);
			while (size < (w_V * multiGraphProcessorVertexInfo->speedup / speedup_sum))
			{
				if (multiGraphTaskVertexMap.size() == 0) break;
				// Find argmin deg(T_u)
				// find block with smallest excess
				// For now we just pick last block with min degree
				// unsigned int task_id = 0;
				int task_id_selected = -1;
				int min_deg = INT_MAX;
				MultiGraphVertex multiGraphTaskVertexMinDeg = NULL;
				typedef std::map<int, MultiGraphVertex>::iterator it_type;
				for (it_type iterator = multiGraphTaskVertexMap.begin(); iterator != multiGraphTaskVertexMap.end(); iterator++) {
					int degree_current =
						iterator->second->info.taskVertexInfo.deg;
					if (min_deg >= degree_current) {
						multiGraphTaskVertexMinDeg = iterator->second;
						task_id_selected = iterator->first;
						min_deg = degree_current;
					}
				}

				if (multiGraphTaskVertexMinDeg == NULL) {
					printf("Could not find task with min degree!\n");
				}

				// For each neighbour of multiGraphTaskVertexMinDeg decrease negih_no
				Koala::Set<MultiGraphVertex> multiGraphTaskVertexMinDegNeighSet
					= multiGraph.getNeighSet(multiGraphTaskVertexMinDeg);
				for (MultiGraphVertex mv = multiGraphTaskVertexMinDegNeighSet.first(); mv;
					mv = multiGraphTaskVertexMinDegNeighSet.next(mv)) {
					mv->info.taskVertexInfo.deg -= multiGraph.getEdgeNo(multiGraphTaskVertexMinDeg, mv);
				}
				reconnect_incident_edges(multiGraph,
					multiGraphProcessorVertex,
					multiGraphTaskVertexMinDeg,
					processorEdgeMap);
				// update size/weight for current processor
				size += multiGraphTaskVertexMinDeg->info.taskVertexInfo.w;
				// erase task from map
				multiGraphTaskVertexMap.erase(task_id_selected);

				map_task_to_MultiGraphVertex(multiGraphTaskIdOnProcessorMap, multiGraphProcessorVertex, multiGraphTaskVertexMinDeg->info, taskVertexMap);

				// remove task from multiGraph
				remove_task_form_MultiGraph(multiGraph,
					multiGraphTaskVertexMinDeg);
			}
			multiGraphProcessorVertex->info.ngrid_cells = size;
		}
	}

	// Alg. 5 - Accumulate neighbours
	void alg5_accumulate_neighbours(TaskGraph &taskGraph,
		ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		ProcessorEdgeMap &processorEdgeMap,
		double speedup_sum, double w_V, float f) {
		typedef std::map<int, MultiGraphVertex>::iterator it_type;
		// Assign the number of neighbours to each multiGraphTask
		for (it_type iterator = multiGraphTaskVertexMap.begin(); iterator != multiGraphTaskVertexMap.end(); iterator++) {
			iterator->second->info.taskVertexInfo.deg = multiGraph.getNeighNo(iterator->second);
		}
		// Schedule
		for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
			++mvIt) {
			MultiGraphVertex multiGraphProcessorVertex = *mvIt;
			// Assign the number of neighbours to each multiGraphTask
			for (it_type iterator = multiGraphTaskVertexMap.begin(); iterator != multiGraphTaskVertexMap.end(); iterator++) {
				iterator->second->info.taskVertexInfo.neigh_no = 0;
			}
			long size = 0;
			MultiGraphVertexInfo *multiGraphProcessorVertexInfo =
				&(multiGraphProcessorVertex->info);
			while (size < (w_V * multiGraphProcessorVertexInfo->speedup / speedup_sum))
			{
				if (multiGraphTaskVertexMap.size() == 0) break;

				MultiGraphVertex multiGraphTaskVertexSelected = NULL;
				int task_id_selected = -1;
				if (size <= (f * w_V * multiGraphProcessorVertexInfo->speedup / speedup_sum))
				{
					// Find argmax N(T_u) on current processor
					// find block with smallest excess
					int max_N = 0;
					typedef std::map<int, MultiGraphVertex>::iterator it_type;
					for (it_type iterator = multiGraphTaskVertexMap.begin(); iterator != multiGraphTaskVertexMap.end(); iterator++) {
						int N_current =
							iterator->second->info.taskVertexInfo.neigh_no;
						if (max_N < N_current) {
							multiGraphTaskVertexSelected = iterator->second;
							task_id_selected = iterator->first;
							max_N = N_current;
						}
					}
					// If no unmapped task is adjacent to the tasks currently mapped
					// to p, then the task with max deg is selected
					int max_deg = -1;
					if (multiGraphTaskVertexSelected == NULL) {
						for (it_type iterator = multiGraphTaskVertexMap.begin();
							iterator != multiGraphTaskVertexMap.end();
							iterator++) {
							int deg_current =
								iterator->second->info.taskVertexInfo.deg;
							if (max_deg < deg_current) {
								multiGraphTaskVertexSelected = iterator->second;
								task_id_selected = iterator->first;
								max_deg = deg_current;
							}
						}
					}
				}
				else {
					//// Find argmin deg(T_u) - N(T_u) on current processor
					//// find block with smallest excess
					int min_deg_N = INT_MAX;
					typedef std::map<int, MultiGraphVertex>::iterator it_type;
					for (it_type iterator = multiGraphTaskVertexMap.begin();
						iterator != multiGraphTaskVertexMap.end(); iterator++) {
						int deg_N_current =
							iterator->second->info.taskVertexInfo.deg -
							iterator->second->info.taskVertexInfo.neigh_no;
						if (min_deg_N > deg_N_current) {
							multiGraphTaskVertexSelected = iterator->second;
							task_id_selected = iterator->first;
							min_deg_N = deg_N_current;
						}
					}
					// Find argmax N(T_u)/deg(T_u)  on current processor
					// find block with smallest excess
					// If no unmapped task is adjacent to the tasks currently mapped
					// to p, then the task with min deg is selected
					int min_deg = INT_MAX;
					if (multiGraphTaskVertexSelected == NULL) {
						for (it_type iterator = multiGraphTaskVertexMap.begin();
							iterator != multiGraphTaskVertexMap.end();
							iterator++) {
							int deg_current =
								iterator->second->info.taskVertexInfo.deg;
							if (min_deg > deg_current) {
								multiGraphTaskVertexSelected = iterator->second;
								task_id_selected = iterator->first;
								min_deg = deg_current;
							}
						}
					}
				}
				if (multiGraphTaskVertexSelected == NULL) {
					printf("Could not find task with max N!\n");
				}

				// For each neighbour of multiGraphTaskVertexSelected increase negih_no
				Koala::Set<MultiGraphVertex> multiGraphTaskVertexSelectedNeighSet
					= multiGraph.getNeighSet(multiGraphTaskVertexSelected);
				for (MultiGraphVertex mv = multiGraphTaskVertexSelectedNeighSet.first(); mv;
					mv = multiGraphTaskVertexSelectedNeighSet.next(mv)) {
					mv->info.taskVertexInfo.neigh_no++;
				}
				reconnect_incident_edges(multiGraph,
					multiGraphProcessorVertex,
					multiGraphTaskVertexSelected,
					processorEdgeMap);
				// update size/weight for current processor
				size += multiGraphTaskVertexSelected->info.taskVertexInfo.w;
				// erase task from map
				multiGraphTaskVertexMap.erase(task_id_selected);
				map_task_to_MultiGraphVertex(multiGraphTaskIdOnProcessorMap, multiGraphProcessorVertex, multiGraphTaskVertexSelected->info, taskVertexMap);

				// remove task from multiGraph
				remove_task_form_MultiGraph(multiGraph,
					multiGraphTaskVertexSelected);
			}
			multiGraphProcessorVertex->info.ngrid_cells = size;
		}
	}

	// Alg. 6 - Small multicut assignment
	void alg6_small_multicut_assignment(TaskGraph &taskGraph,
		ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		ProcessorEdgeMap &processorEdgeMap,
		double speedup_sum, double w_V) {
		unsigned int task_id = 0;
		// Assign the number of neighbours to each multiGraphTask
		for (MultiGraphVertex multiGraphTaskVertex = multiGraphTaskVertexMap[task_id]; task_id < multiGraphTaskVertexMap.size(); ++task_id) {
			multiGraphTaskVertex->info.taskVertexInfo.deg = multiGraph.getNeighNo(multiGraphTaskVertex);
		}
		// Schedule
		for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
			++mvIt) {
			MultiGraphVertex multiGraphProcessorVertex = *mvIt;
			long size = 0;
			MultiGraphVertexInfo *multiGraphProcessorVertexInfo =
				&(multiGraphProcessorVertex->info);
			while (size < (w_V * multiGraphProcessorVertexInfo->speedup / speedup_sum))
			{
				if (multiGraphTaskVertexMap.size() == 0) break;
				// Find argmin deg(T_u)
				// find block with smallest excess
				// For now we just pick last block with min degree
				int task_id_selected = -1;
				int min_deg = INT_MAX;
				MultiGraphVertex multiGraphTaskVertexMinDeg = NULL;
				typedef std::map<int, MultiGraphVertex>::iterator it_type;
				for (it_type iterator = multiGraphTaskVertexMap.begin(); iterator != multiGraphTaskVertexMap.end(); iterator++) {
					int degree_current =
						iterator->second->info.taskVertexInfo.deg;
					if (min_deg >= degree_current) {
						multiGraphTaskVertexMinDeg = iterator->second;
						task_id_selected = iterator->first;
						min_deg = degree_current;
					}
				}

				if (multiGraphTaskVertexMinDeg == NULL) {
					printf("Could not find task with min degree!\n");
				}

				// For each neighbour of multiGraphTaskVertexMinDeg decrease negih_no
				Koala::Set<MultiGraphVertex> multiGraphTaskVertexMinDegNeighSet
					= multiGraph.getNeighSet(multiGraphTaskVertexMinDeg);
				for (MultiGraphVertex mv = multiGraphTaskVertexMinDegNeighSet.first(); mv;
					mv = multiGraphTaskVertexMinDegNeighSet.next(mv)) {
					mv->info.taskVertexInfo.deg--;
				}
				reconnect_incident_edges(multiGraph,
					multiGraphProcessorVertex,
					multiGraphTaskVertexMinDeg,
					processorEdgeMap);
				// update size/weight for current processor
				size += multiGraphTaskVertexMinDeg->info.taskVertexInfo.w;
				// erase task from map
				multiGraphTaskVertexMap.erase(task_id_selected);
				map_task_to_MultiGraphVertex(multiGraphTaskIdOnProcessorMap, multiGraphProcessorVertex, multiGraphTaskVertexMinDeg->info, taskVertexMap);

				// remove task from multiGraph
				remove_task_form_MultiGraph(multiGraph,
					multiGraphTaskVertexMinDeg);
			}
			multiGraphProcessorVertex->info.ngrid_cells = size;
		}
	}

	// Alg. 7 - Local search
	void alg7_local_search(TaskGraph &taskGraph, ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		TaskEdgeMap &taskEdgeMap,
		ProcessorEdgeMap &processorEdgeMap,
		double speedup_sum, double w_V) {

		// get current cost
		Cost currentCost;
		calculate_cost(multiGraph, currentCost, SIMPLE, true);

		Cost lowestCost(currentCost);
		MultiGraph multiGraphWithLowestCost;
		create_multiGraph_copy(multiGraph, multiGraphWithLowestCost);

		// define deadline
		double deadline = std::numeric_limits<double>::max();

		// iterate over tasks
		for (TaskVertex tv = taskGraph.getVert(); tv; tv = taskGraph.getVertNext(tv))
		{
			MultiGraphVertex oldVertex = NULL;
			MultiGraphVertex bestVertex = NULL;

			// taskId
			int taskId = tv->info.id;

			// iterate over all processors excluding processor mv
			for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
				++mvIt) {
				MultiGraphVertex mvToCheck = *mvIt;
				MultiGraphVertex mv = multiGraphTaskIdOnProcessorMap.find(tv->info.id)->second;

				// exclude processor mv
				if (mv->info.name.compare(mvToCheck->info.name) != 0)
				{
					// apply task move to multigraph
					move_task_to_processor(mv, mvToCheck, taskId, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);

					// calculate cost of task move
					calculate_cost(multiGraph, currentCost, SIMPLE, false);

					// check if energy cost is lowest and if the solution is feasible - meets deadline
					if ((currentCost.summary_energy < lowestCost.summary_energy || (currentCost.summary_energy == lowestCost.summary_energy && currentCost.summary_time < lowestCost.summary_time)) && currentCost.summary_time < deadline)
					{
						lowestCost = Cost(currentCost);

						// save multi graph
						create_multiGraph_copy(multiGraph, multiGraphWithLowestCost);

						// save move
						// save old vertex
						oldVertex = mv;
						// save best vertex
						bestVertex = mvToCheck;

						std::cout << lowestCost << "\n";
					}

					// revert task move
					move_task_to_processor(mvToCheck, mv, taskId, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
				}
			}

			if (oldVertex != NULL && bestVertex != NULL)
			{
				move_task_to_processor(oldVertex, bestVertex, taskId, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
			}

			std::cout << tv->info.id << "\n";
		}
		multiGraph = multiGraphWithLowestCost;
	}

	bool is_tabu(TabuMap &tabuMap, int taskId, std::string processorName)
	{
		TabuMap::iterator itTabuMap = tabuMap.find(std::pair<int, std::string>(taskId, processorName));

		return (itTabuMap != tabuMap.end()) && (itTabuMap->second.first > 0);
	}

	int tabu_tenure = 8;

	void make_tabu(TabuMap &tabuMap, int taskId, std::string processorName)
	{
		TabuMap::iterator itTabuMap = tabuMap.find(std::pair<int, std::string>(taskId, processorName));

		if (itTabuMap != tabuMap.end())
		{
			itTabuMap->second.second++;
			// increase lineary tenure size
			itTabuMap->second.first = /*std::pow(2,*/itTabuMap->second.second/*)*/ * tabu_tenure;
			std::cout << "incerement tabu <taskId, processorName, tenures, frequency>: " << taskId << "," << processorName << " " << itTabuMap->second.first << " " << itTabuMap->second.second << std::endl;
		}
		else {
			tabuMap.insert(std::pair<std::pair<int, std::string>, std::pair<int, int>>(std::pair<int, std::string>(taskId, processorName), std::pair<int, int>(tabu_tenure, 1)));
		}
		std::cout << "make tabu <taskId, processorName>: " << taskId << "," << processorName << std::endl;
	}

	void update_tabu(TabuMap &tabuMap)
	{
		std::vector<std::pair<int, std::string>> movesToRemove;

		for (TabuMap::iterator itTabuMap = tabuMap.begin(); itTabuMap != tabuMap.end(); ++itTabuMap)
		{
			itTabuMap->second.first--;

			// do not move from tabu list
			/*if (itTabuMap->second.first <= 0)
			{
				movesToRemove.push_back(itTabuMap->first);
			}*/
		}

		// do not remove
		//for (auto itMovesToRemove = movesToRemove.begin(); itMovesToRemove != movesToRemove.end(); ++itMovesToRemove)
		//{
		//	tabuMap.erase(std::pair<int, std::string>(itMovesToRemove->first, itMovesToRemove->second));
		//	//std::cout << "remove from tabu <taskId, processorName>: " << itMovesToRemove->first << "," << itMovesToRemove->second << std::endl;
		//}
	}

	void tabu(TabuMap &tabuMap, int taskId, std::string processorName)
	{
		update_tabu(tabuMap);

		make_tabu(tabuMap, taskId, processorName);
	}

	// press "H" key to display best solution during computations
#define KEY_H    72

	bool detect_if_key_pressed()
	{
		if (_kbhit()) {
			int ex;

			switch (ex = getch())
			{
			case KEY_H    /* H */:
				std::cout << std::endl << "H" << std::endl;//key up
				return true;
			default:
				std::cout << std::endl << (char)ex << std::endl;  // not arrow
				break;
			}
		}

		return false;
	}

	void find_task_color(std::map<int, TaskVertex> taskVertexVectorMappedMap, TaskVertex &taskVertex, MultiGraphVertex &mv) {
		int id = taskVertex->info.id;
		for (auto it = taskVertexVectorMappedMap.begin(); it != taskVertexVectorMappedMap.end(); ++it) {
			if (it->second->info.id == id) {
				taskVertex->info.color = mv->info.color;
				taskVertex->info.pid = mv->info.name;
			}
		}
	}

	void color_task_vertices(TaskGraph &taskGraph, MultiGraph &multiGraph) {
		for (TaskVertex taskVertex = taskGraph.getVert(); taskVertex; taskVertex = taskGraph.getVertNext(taskVertex)) {
			for (MultiGraphVertex mv = multiGraph.getVert(); mv; mv = multiGraph.getVertNext(mv)) {
				find_task_color(mv->info.taskVertexVectorMappedMap, taskVertex, mv);
			}
		}
	}

	// Alg. 8 - Tabu search
	void alg8_tabu_search(TaskGraph &taskGraph, ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		TaskEdgeMap &taskEdgeMap,
		ProcessorEdgeMap &processorEdgeMap,
		double speedup_sum, double w_V) {

		// get current cost
		Cost currentCost;
		calculate_cost(multiGraph, currentCost, VIZING, true);

		Cost localLowestCost(currentCost);
		Cost lowestCost(currentCost);
		MultiGraph multiGraphWithLowestCost;
		create_multiGraph_copy(multiGraph, multiGraphWithLowestCost);

		// TODO: define deadline
		long long deadline = 1381;

		// tabu map - <taskId, processorName> to tenure
		TabuMap tabuMap;

		int move_type = 0;

		// accept threshold - make it larger to accpert worse solution if there is no improvement
		long long acceptThresholdEnergy = 0;
		long long acceptThresholdDeadline = 0;

		int maxNumberIterations = 40000;
		for (int iterationId = 0; iterationId < maxNumberIterations; ++iterationId)
		{
			// check if there is any improvement in current iteration
			bool isImproved = false;

			MultiGraphVertex oldVertex = NULL;
			MultiGraphVertex bestVertex = NULL;

			int oldTaskId = -1;
			int bestTaskId = -1;

			// iterate over tasks
			for (TaskVertex tv = taskGraph.getVert(); tv; tv = taskGraph.getVertNext(tv))
			{
				// taskId
				int taskId = tv->info.id;

				// iterate over all processors excluding processor mv
				for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
					++mvIt) {
					MultiGraphVertex mvToCheck = *mvIt;
					MultiGraphVertex mv = multiGraphTaskIdOnProcessorMap.find(tv->info.id)->second;

					// too strong assumption that the processor without tasks is no longer considered in search
					// as the energy usage is minimized as well as the number of processors are
					/*if (mvToCheck->info.ngrid_cells == 0)
					{
						continue;
					}*/

					// exclude processor mv
					if (mv->info.name.compare(mvToCheck->info.name) != 0)
					{
						int taskIdOld = -1;
						int taskIdNew = -1;

						// apply task move to multigraph
						if (move_type == 0)
						{
							move_task_to_processor(mv, mvToCheck, taskId, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
						} // apply task swap
						else if (move_type == 1)
						{
							if (mvToCheck->info.taskVertexVectorMappedMap.size() <= 0)
							{
								continue;
							}

							taskIdOld = taskId;

							auto it = mvToCheck->info.taskVertexVectorMappedMap.begin();
							std::advance(it, rand() % mvToCheck->info.taskVertexVectorMappedMap.size());
							taskIdNew = it->first;

							swap_tasks_between_processors(mv, taskIdOld, mvToCheck, taskIdNew,
								taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
						} // apply edge move
						else if (move_type == 2)
						{
							if (mvToCheck->info.taskVertexVectorMappedMap.size() <= 0)
							{
								continue;
							}

							taskIdOld = taskId;

							auto it = mvToCheck->info.taskVertexVectorMappedMap.begin();
							std::advance(it, rand() % mvToCheck->info.taskVertexVectorMappedMap.size());
							taskIdNew = it->first;

							swap_tasks_between_processors(mv, taskIdOld, mvToCheck, taskIdNew,
								taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
						}

						// calculate cost of task move
						calculate_cost(multiGraph, currentCost, VIZING, true);

						// is move tabu?
						bool isTabu = false;

						if (move_type == 0)
						{
							isTabu = is_tabu(tabuMap, taskId, mvToCheck->info.name);
						}
						else if (move_type == 1)
						{
							isTabu = is_tabu(tabuMap, taskIdOld, mvToCheck->info.name) && is_tabu(tabuMap, taskIdNew, mv->info.name);
						}

						// check if energy cost is lowest or equal but computation time is lower and if the solution is feasible - meets deadline
						if (((currentCost.summary_energy < (localLowestCost.summary_energy + acceptThresholdEnergy)) || (currentCost.summary_energy == localLowestCost.summary_energy && currentCost.summary_time < localLowestCost.summary_time)) && (currentCost.summary_time <= (deadline + acceptThresholdDeadline)))
						{
							bool aspirationCriteria = false;

							// save solution if not tabu or meets aspritation criteria
							if (!isTabu || aspirationCriteria)
							{
								// save cost
								localLowestCost = Cost(currentCost);

								// save move
								if (move_type == 0)
								{
									// save old vertex
									oldVertex = mv;
									// save best vertex
									bestVertex = mvToCheck;

									// save best task id
									bestTaskId = taskId;
								}
								else if (move_type == 1)
								{
									// save old vertex
									oldVertex = mv;
									// save best vertex
									bestVertex = mvToCheck;

									// save old task id
									oldTaskId = taskIdOld;
									// save best task id
									bestTaskId = taskIdNew;
								}
								else if (move_type == 2)
								{
									// save old vertex
									oldVertex = mv;
									// save best vertex
									bestVertex = mvToCheck;

									// save old task id
									oldTaskId = taskIdOld;
									// save best task id
									bestTaskId = taskIdNew;
								}

								//isImproved = true;
								//acceptThresholdEnergy = 0;
								//acceptThresholdDeadline = 0;
							}

							if (((currentCost.summary_energy < (lowestCost.summary_energy)) || (currentCost.summary_energy == lowestCost.summary_energy && currentCost.summary_time < lowestCost.summary_time)) && (currentCost.summary_time <= deadline))
							{
								// save global lowest cost
								lowestCost = Cost(currentCost);

								// save solution with lowest cost
								create_multiGraph_copy(multiGraph, multiGraphWithLowestCost, true);

								std::cout << lowestCost << "\n";
							}
						}

						// revert move
						if (move_type == 0)
						{
							// revert task move
							move_task_to_processor(mvToCheck, mv, taskId, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
						}
						else if (move_type == 1)
						{
							// revert task swap
							swap_tasks_between_processors(mv, taskIdNew, mvToCheck, taskIdOld,
								taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
						}
						else if (move_type == 2)
						{
							// revert edge move
							swap_tasks_between_processors(mv, taskIdNew, mvToCheck, taskIdOld,
								taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
						}
					}

					if (detect_if_key_pressed())
					{
						scheduler::Cost cost;
						calculate_cost(multiGraphWithLowestCost/*, schedParams*/, cost, VIZING, true);
						std::cout << cost << std::endl;
						print_n_edges(multiGraphWithLowestCost);
						print_n_pus(multiGraphWithLowestCost);
						color_task_vertices(taskGraph, multiGraphWithLowestCost);
						write_GrapML(taskGraph, processorGraph, multiGraphWithLowestCost);
					}
				}
			}

			// apply best non tabu move
			if (oldVertex != NULL && bestVertex != NULL)
			{
				if (move_type == 0)
				{
					// make move tabu
					tabu(tabuMap, bestTaskId, oldVertex->info.name);

					move_task_to_processor(oldVertex, bestVertex, bestTaskId, taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
				}
				else if (move_type == 1)
				{
					tabu(tabuMap, oldTaskId, oldVertex->info.name);

					tabu(tabuMap, bestTaskId, bestVertex->info.name);

					swap_tasks_between_processors(oldVertex, oldTaskId, bestVertex, bestTaskId,
						taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
				}
				else if (move_type == 2)
				{
					tabu(tabuMap, oldTaskId, oldVertex->info.name);

					tabu(tabuMap, bestTaskId, bestVertex->info.name);

					swap_tasks_between_processors(oldVertex, oldTaskId, bestVertex, bestTaskId,
						taskGraph, taskVertexMap, processorEdgeMap, multiGraphTaskIdOnProcessorMap, multiGraph);
				}

				isImproved = true;
				acceptThresholdEnergy = 0;
				acceptThresholdDeadline = 0;
			}

			if (!isImproved)
			{
				acceptThresholdEnergy += localLowestCost.summary_energy * 0.001 < 2100000 ? 2100000 : localLowestCost.summary_energy * 0.001;
				acceptThresholdDeadline += (localLowestCost.summary_time * 0.01) < 100 ? 101 : (localLowestCost.summary_time * 0.01);

				std::cout << "accpetThreshold: " << acceptThresholdEnergy << " " << acceptThresholdDeadline << std::endl;

				if (acceptThresholdEnergy > localLowestCost.summary_energy * 0.02)
				{
					if (move_type == 1)
					{
						break;
					}

					++move_type;
					acceptThresholdEnergy = 0;
					acceptThresholdDeadline = 0;
				}
			}

			std::cout << iterationId << std::endl;
		}

		multiGraph = multiGraphWithLowestCost;
	}

	typedef std::map<std::string, std::pair<Vector3i, Vector3i>> TaskIdToProcessorMap;
	TaskIdToProcessorMap taskIdToProcessorMap;

	void filld_taskIdToProcessorMap()
	{
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p0", std::pair<Vector3i, Vector3i>(Vector3i(0, 0, 0), Vector3i(-1, -1, -1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p1", std::pair<Vector3i, Vector3i>(Vector3i(0, 0, 0), Vector3i(-1, -1, -1))));

		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p2", std::pair<Vector3i, Vector3i>(Vector3i(0, 0, 0), Vector3i(3, 1, 1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p3", std::pair<Vector3i, Vector3i>(Vector3i(0, 2, 0), Vector3i(3, 4, 1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p4", std::pair<Vector3i, Vector3i>(Vector3i(0, 5, 0), Vector3i(3, 7, 1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p5", std::pair<Vector3i, Vector3i>(Vector3i(4, 0, 0), Vector3i(7, 1, 1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p6", std::pair<Vector3i, Vector3i>(Vector3i(4, 2, 0), Vector3i(7, 4, 1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p7", std::pair<Vector3i, Vector3i>(Vector3i(4, 5, 0), Vector3i(7, 7, 1))));

		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p8", std::pair<Vector3i, Vector3i>(Vector3i(0, 0, 0), Vector3i(-1, -1, -1))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p9", std::pair<Vector3i, Vector3i>(Vector3i(0, 0, 0), Vector3i(-1, -1, -1))));

		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p10", std::pair<Vector3i, Vector3i>(Vector3i(0, 0, 2), Vector3i(3, 1, 3))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p11", std::pair<Vector3i, Vector3i>(Vector3i(0, 2, 2), Vector3i(3, 4, 3))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p12", std::pair<Vector3i, Vector3i>(Vector3i(0, 5, 2), Vector3i(3, 7, 3))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p13", std::pair<Vector3i, Vector3i>(Vector3i(4, 0, 2), Vector3i(7, 1, 3))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p14", std::pair<Vector3i, Vector3i>(Vector3i(4, 2, 2), Vector3i(7, 4, 3))));
		taskIdToProcessorMap.insert(std::pair<std::string, std::pair<Vector3i, Vector3i>>("p15", std::pair<Vector3i, Vector3i>(Vector3i(4, 5, 2), Vector3i(7, 7, 3))));
	}

	// Alg. 3 - manual
	// for tests - manually provide the mapping 
	void alg3_manual(TaskGraph &taskGraph, ProcessorGraph &processorGraph,
		MultiGraph &multiGraph,
		std::vector<MultiGraphVertex> &multiGraphProcessorVertexVector,
		std::map<int, MultiGraphVertex> &multiGraphTaskVertexMap,
		MultiGraphTaskIdOnProcessorMap &multiGraphTaskIdOnProcessorMap,
		TaskVertexMap &taskVertexMap,
		ProcessorEdgeMap &processorEdgeMap, Vector3i &gridSize,
		double speedup_sum, double w_V,
		SchedParams &schedParams) {

		filld_taskIdToProcessorMap();

		for (int id_selected = 0; id_selected < gridSize.x * gridSize.y * gridSize.z; ++id_selected)
		{
			for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end();
				++mvIt) {
				MultiGraphVertex multiGraphProcessorVertex = *mvIt;
				//std::cout << multiGraphProcessorVertex->info.name << std::endl;
				MultiGraphVertexInfo *multiGraphProcessorVertexinfo =
					&(multiGraphProcessorVertex->info);

				int idz = id_selected / (gridSize.x * gridSize.y);
				int idy = (id_selected % (gridSize.x * gridSize.y)) / gridSize.x;
				int idx = id_selected - idy * gridSize.x - idz * gridSize.x * gridSize.y;

				std::pair<Vector3i, Vector3i> taskOnProcessorRange;

				TaskIdToProcessorMap::iterator itTaskIdToProcessorMap = taskIdToProcessorMap.find(multiGraphProcessorVertexinfo->name);
				if (itTaskIdToProcessorMap != taskIdToProcessorMap.end()) {
					taskOnProcessorRange = itTaskIdToProcessorMap->second;

					if (
						(idx >= taskOnProcessorRange.first.x) && (idx <= taskOnProcessorRange.second.x) &&
						(idy >= taskOnProcessorRange.first.y) && (idy <= taskOnProcessorRange.second.y) &&
						(idz >= taskOnProcessorRange.first.z) && (idz <= taskOnProcessorRange.second.z)
						)
					{
						MultiGraphVertex multiGraphTaskVertex = NULL;

						typedef std::map<int, MultiGraphVertex> MultiGraphTaskVertexMap_t;
						MultiGraphTaskVertexMap_t::iterator it = multiGraphTaskVertexMap.find(id_selected);
						if (it != multiGraphTaskVertexMap.end()) {
							multiGraphTaskVertex = it->second;
						}

						if (multiGraphTaskVertex == NULL) {
							std::cout << "error!" << std::endl;
							break;
						};

						reconnect_incident_edges(multiGraph,
							multiGraphProcessorVertex,
							multiGraphTaskVertex,
							processorEdgeMap);
						TaskVertexInfo taskVertexInfo = multiGraphTaskVertex->info.taskVertexInfo;
						map_task_to_MultiGraphVertex(multiGraphTaskIdOnProcessorMap, multiGraphProcessorVertex, multiGraphTaskVertex->info, taskVertexMap);
						remove_task_form_MultiGraph(multiGraph,
							multiGraphTaskVertex);
						multiGraphTaskVertexMap.erase(id_selected);

						multiGraphProcessorVertex->info.ngrid_cells += taskVertexInfo.w;
					}
				}
			}
		}
	}

	void scheduling(TaskGraph &taskGraph, ProcessorGraph &processorGraph,
		MultiGraph &multiGraph, Vector3i &gridSize, SchedParams &schedParams) {
		typedef std::map<std::string, MultiGraphVertex> MultiGraphVertexMap;
		std::vector<MultiGraphVertex> multiGraphProcessorVertexVector;
		// copy ProcessorGraph to MultiGraph
		for (ProcessorVertex pv = processorGraph.getVert(); pv; pv = processorGraph.getVertNext(pv)) {
			TaskVertexInfo taskVertexInfo = TaskVertexInfo(0, 0, 0, 0);
			multiGraphProcessorVertexVector.push_back(multiGraph.addVert(MultiGraphVertexInfo(taskVertexInfo, pv->info)));
		}

		Koala::Set<MultiGraphVertex> multiGraphProcessorVertexSet = multiGraph.getVertSet();
		int t_c_max = 0;
		// find values for Processors
		// find maximum computation time t_c
		for (std::vector<MultiGraphVertex>::iterator mvIt = multiGraphProcessorVertexVector.begin(); mvIt != multiGraphProcessorVertexVector.end(); ++mvIt) {
			MultiGraphVertex mv = *mvIt;
			if (t_c_max < mv->info.t_c) {
				t_c_max = mv->info.t_c;
			}
		}
		double speedup_sum = 0.0f;
		// calculate speedup for each processor
		for (MultiGraphVertex mv = multiGraphProcessorVertexSet.first(); mv; mv = multiGraphProcessorVertexSet.next(mv)) {
			MultiGraphVertexInfo *minfo = &(mv->info);
			minfo->speedup = t_c_max / minfo->t_c;
			speedup_sum += minfo->speedup;
		}
		// find values for Tasks
		// compute sum of weights w_V of all tasks
		float w_V = 0.0f;
		for (TaskVertex tv = taskGraph.getVert(); tv; tv = taskGraph.getVertNext(tv))
		{
			w_V += tv->info.w;
		}

		std::cout << "max computation time: " << t_c_max << " speedup sum: " << speedup_sum << " sum of weights of all tasks: " << w_V << std::endl;

		// copy TaskGraph to MultiGraph
		// a) copy TaskVertex
		std::map<int, MultiGraphVertex> multiGraphTaskVertexMap;
		for (TaskVertex tv = taskGraph.getVert(); tv; tv = taskGraph.getVertNext(tv)) {
			ProcessorVertexInfo processorVertexInfo = ProcessorVertexInfo();
			processorVertexInfo.name = std::to_string(tv->info.id);
			multiGraphTaskVertexMap.insert(std::pair<int, MultiGraphVertex>(tv->info.id, multiGraph.addVert(MultiGraphVertexInfo(tv->info, processorVertexInfo))));
		}

		// b) copy TaskEdge
		ProcessorEdgeInfo processorEdgeInfo;
		for (TaskEdge ee = taskGraph.getEdge(); ee; ee = taskGraph.getEdgeNext(ee)) {
			TaskVertex v1 = taskGraph.getEdgeEnd1(ee);
			TaskVertex v2 = taskGraph.getEdgeEnd2(ee);
			multiGraph.addEdge(multiGraphTaskVertexMap[v1->info.id], multiGraphTaskVertexMap[v2->info.id], MultiGraphEdgeInfo(ee->info, processorEdgeInfo), ee->getDir(v1));
		}

		// taskEdgeMap with mapping task ids to edges
		TaskEdgeMap taskEdgeMap;
		for (TaskEdge ee = taskGraph.getEdge(); ee; ee = taskGraph.getEdgeNext(ee)) {
			TaskVertex v1 = taskGraph.getEdgeEnd1(ee);
			TaskVertex v2 = taskGraph.getEdgeEnd2(ee);
			taskEdgeMap.insert(
				std::pair<std::pair<int, int>, TaskEdgeInfo>
				(std::pair<int, int>(v1->info.id, v2->info.id), ee->info));
		}

		// processorEdgeMap with mapping processor names to edges
		ProcessorEdgeMap processorEdgeMap;
		for (ProcessorEdge ee = processorGraph.getEdge(); ee; ee = processorGraph.getEdgeNext(ee)) {
			ProcessorVertex v1 = processorGraph.getEdgeEnd1(ee);
			ProcessorVertex v2 = processorGraph.getEdgeEnd2(ee);
			processorEdgeMap.insert(
				std::pair<std::pair<std::string, std::string>, ProcessorEdgeInfo>
				(std::pair<std::string, std::string>(v1->info.name, v2->info.name), ee->info));
		}

		// taskVertexMap with mapping task ids to vertices
		TaskVertexMap taskVertexMap;
		for (TaskVertex tv = taskGraph.getVert(); tv; tv = taskGraph.getVertNext(tv)) {
			taskVertexMap.insert(std::pair<int, TaskVertex>(tv->info.id, tv));
		}

		MultiGraphTaskIdOnProcessorMap multiGraphTaskIdOnProcessorMap;

#ifdef __GNUC__
		timespec ts0, ts1;
		clock_gettime(CLOCK_REALTIME, &ts0);


		struct timeval t0, t1;

		gettimeofday(&t0, 0);
#endif

		auto start = std::chrono::high_resolution_clock::now();

		// Select scheduling algorithm
		if (schedParams.alg_idx == 1 || schedParams.alg_idx == 7 || schedParams.alg_idx == 8) {
			alg1_fill_up(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				processorEdgeMap, gridSize,
				speedup_sum, w_V,
				schedParams);
		}
		else if (schedParams.alg_idx == 3) {
			alg3_manual(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				processorEdgeMap, gridSize,
				speedup_sum, w_V,
				schedParams);
		}
		else if (schedParams.alg_idx == 4) {
			alg4_small_degree_assignment(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				processorEdgeMap,
				speedup_sum, w_V);
		}
		else if (schedParams.alg_idx == 5) {
			alg5_accumulate_neighbours(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				processorEdgeMap,
				speedup_sum, w_V, schedParams.f);
		}
		else if (schedParams.alg_idx == 6) {
			alg6_small_multicut_assignment(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				processorEdgeMap,
				speedup_sum, w_V);
		}

		if (schedParams.alg_idx == 7) {
			alg7_local_search(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				taskEdgeMap, processorEdgeMap,
				speedup_sum, w_V);
		}

		if (schedParams.alg_idx == 8) {
			alg8_tabu_search(taskGraph, processorGraph, multiGraph,
				multiGraphProcessorVertexVector, multiGraphTaskVertexMap,
				multiGraphTaskIdOnProcessorMap,
				taskVertexMap,
				taskEdgeMap, processorEdgeMap,
				speedup_sum, w_V);
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

		std::cout << "scheduling time elapsed: " << elapsed.count() << " ns" << std::endl;

#ifdef __GNUC__
		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

		clock_gettime(CLOCK_REALTIME, &ts1);
		long long elapsed_nano = (ts1.tv_sec - ts0.tv_sec) * 1000000000L + ts1.tv_nsec - ts0.tv_nsec;

		printf("tmie elapsed: %ld %ld\n", elapsed, elapsed_nano);
#endif
		if (multiGraph.getVertNo() != processorGraph.getVertNo()) {
			printf("Number of vertices in MultiGraph is not equal to number of processors: %d != %d!\n", multiGraph.getVertNo(), processorGraph.getVertNo());
			exit(1);
		}
	}

} /* namespace scheduler */

#endif /* SCHEDULING_H_ */
