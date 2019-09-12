/*
 * multigraph.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef MULTIGRAPH_H_
#define MULTIGRAPH_H_

#include "basic_types.h"
#include "taskgraph.h"
#include "processorgraph.h"

#include <vector>

namespace scheduler {

	typedef struct MultiGraphVertexInfo_t : public ProcessorVertexInfo {
		MultiGraphVertexInfo_t() : ProcessorVertexInfo(), taskVertexInfo(TaskVertexInfo()), ngrid_cells(0), t_idle(0), x(0), y(0) {};
		MultiGraphVertexInfo_t(TaskVertexInfo &taskVertexInfo_, ProcessorVertexInfo &processorVertexInfo_) :
			ProcessorVertexInfo(processorVertexInfo_), taskVertexInfo(TaskVertexInfo(taskVertexInfo_)), ngrid_cells(0), t_idle(0), x(0), y(0) {};
		MultiGraphVertexInfo_t(TaskVertexInfo &taskVertexInfo_, ProcessorVertexInfo &processorVertexInfo_, int ngrid_cells_) :
			ProcessorVertexInfo(processorVertexInfo_), taskVertexInfo(TaskVertexInfo(taskVertexInfo_)), ngrid_cells(ngrid_cells_), t_idle(0), x(0), y(0) {};
		MultiGraphVertexInfo_t(const MultiGraphVertexInfo_t &copy) :
			ProcessorVertexInfo(copy.name, copy.id, copy.t_c, copy.e_c, copy.p0, copy.p_idle, copy.color),
			taskVertexInfo(TaskVertexInfo(copy.taskVertexInfo)), ngrid_cells(copy.ngrid_cells), t_idle(copy.t_idle), x(copy.x), y(copy.y) {
			for (auto it = copy.taskVertexVectorMappedMap.begin(); it != copy.taskVertexVectorMappedMap.end(); ++it) {
				taskVertexVectorMappedMap.insert(std::pair<int, TaskVertex>(it->first, it->second));
			}
		};
		TaskVertexInfo taskVertexInfo;
		long ngrid_cells;
		long long t_idle;
		int x;
		int y;
		// list of mapped tasks
		//std::vector<TaskVertexInfo> taskVertexInfoVectorMapped;
		std::map<int, TaskVertexInfo> taskVertexInfoVectorMappedMap;
		std::map<int, TaskVertex> taskVertexVectorMappedMap;
	} MultiGraphVertexInfo;

	typedef struct MultiGraphEdgeInfo_t : public ProcessorEdgeInfo {
		MultiGraphEdgeInfo_t() : ProcessorEdgeInfo() {};
		MultiGraphEdgeInfo_t(TaskEdgeInfo &taskEdgeInfo_, ProcessorEdgeInfo &processorEdgeInfo_) :
			ProcessorEdgeInfo(processorEdgeInfo_) {
			taskEdgeInfo.v1 = taskEdgeInfo_.v1;
			taskEdgeInfo.v2 = taskEdgeInfo_.v2;
			taskEdgeInfo.d = taskEdgeInfo_.d;
		};
		MultiGraphEdgeInfo_t(const MultiGraphEdgeInfo_t &copy) :
			ProcessorEdgeInfo(copy.name1, copy.name2, copy.t_e, copy.e_e)
		{
			taskEdgeInfo.v1 = copy.taskEdgeInfo.v1;
			taskEdgeInfo.v2 = copy.taskEdgeInfo.v2;
			taskEdgeInfo.d = copy.taskEdgeInfo.d;
		};
		TaskEdgeInfo taskEdgeInfo;
	} MultiGraphEdgeInfo;

	typedef Koala::Graph<MultiGraphVertexInfo, MultiGraphEdgeInfo> MultiGraph;
	typedef MultiGraph::PVertex MultiGraphVertex;
	typedef MultiGraph::PEdge MultiGraphEdge;

} /* namespace scheduler */

#endif /* MULTIGRAPH_H_ */
