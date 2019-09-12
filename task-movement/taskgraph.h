/*
 * taskgraph.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef TASKGRAPH_H_
#define TASKGRAPH_H_

#include "Koala/coloring/edge.h"

#include <string>

namespace scheduler {

/*class taskgraph {
public:
    taskgraph();
    virtual ~taskgraph();
};*/

	typedef struct TaskVertexInfo_t {
		TaskVertexInfo_t() : id(0), w(0), deg(0), neigh_no(0), x(0), y(0), pid("0"), color("#000") {};
		TaskVertexInfo_t(int id_, int w_, int x_, int y_) : id(id_), w(w_), deg(0), neigh_no(0), x(x_), y(y_), pid("0"), color("#000") {};
		TaskVertexInfo_t(const TaskVertexInfo_t &obj) : id(obj.id), w(obj.w), deg(obj.deg), neigh_no(obj.neigh_no), x(obj.x), y(obj.y), pid(obj.pid), color(obj.color) {};
		// id
		int id;
		// weight
		int w;
		// degree
		int deg;
		// neighbours no
		int neigh_no;
		// x position in Zgred
		int x;
		// y position in Zgred
		int y;
		// processor name in Zgred
		std::string pid;
		// color in Zgred
		std::string color;
	} TaskVertexInfo;

std::ostream& operator<<(std::ostream& os, const TaskVertexInfo &arg)
{
    return os << arg.id << ',' << arg.w << ',' << arg.x << ',' << arg.y;
}

typedef struct TaskEdgeInfo_t {
    TaskEdgeInfo_t() : v1(-1), v2(-1), d(0) {};
    TaskEdgeInfo_t(int v1_, int v2_, int d_) : v1(v1_), v2(v2_), d(d_) {};
    // v1
    int v1;
    // v2
    int v2;
    // weight
    int d;
} TaskEdgeInfo;

std::ostream& operator<<(std::ostream& os, const TaskEdgeInfo &arg)
{
    return os << arg.v1 << ',' << arg.v2 << ',' << arg.d;
}

typedef Koala::Graph<TaskVertexInfo, TaskEdgeInfo> TaskGraph;
typedef TaskGraph::PVertex TaskVertex;
typedef TaskGraph::PEdge TaskEdge;

} /* namespace scheduler */

#endif /* TASKGRAPH_H_ */
