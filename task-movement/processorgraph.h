/*
 * processorgraph.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef PROCESSORGRAPH_H_
#define PROCESSORGRAPH_H_

#include <string>

namespace scheduler {

/*class processorgraph {
public:
    processorgraph();
    virtual ~processorgraph();
};*/

typedef struct ProcessorVertexInfo_t {
    ProcessorVertexInfo_t(): name("0"), id(0), t_c(0), e_c(0), p0(0), p_idle(0), speedup(0.0f), color("") {};
    ProcessorVertexInfo_t(std::string name_, int id_, int t_c_, int e_c_, int p0_, int p_idle_, std::string color_): name(name_), id(id_), t_c(t_c_), e_c(e_c_), p0(p0_), p_idle(p_idle_), speedup(0.0f), color(color_) {};
    ProcessorVertexInfo_t(const ProcessorVertexInfo_t &obj): name(obj.name), id(obj.id),t_c(obj.t_c), e_c(obj.e_c), p0(obj.p0), p_idle(obj.p_idle), speedup(obj.speedup), color(obj.color) {};
    // name
    std::string name;
    // id
    int id;
    // Computing time per grid cell
    long t_c;
    // Computing energy per grid cell
    long e_c;
    // P0
    long p0;
    // Idle power
    long p_idle;

    // Helper vars
    // speedup - relative to slowest processor
    double speedup;
    // color for Zgred
    std::string color;
} ProcessorVertexInfo;

typedef struct ProcessorEdgeInfo_t {
    ProcessorEdgeInfo_t(): name1("-"), name2("-"), t_e(0), e_e(0) {};
    ProcessorEdgeInfo_t(std::string name1_, std::string name2_, int t_e_, int e_e_): name1(name1_), name2(name2_), t_e(t_e_), e_e(e_e_) {};
    ProcessorEdgeInfo_t(const ProcessorEdgeInfo_t &obj): name1(obj.name1), name2(obj.name2), t_e(obj.t_e), e_e(obj.e_e) {};
    // p1
    std::string name1;
    // p2
    std::string name2;
    // Exchange time per grid cell
    int t_e;
    // Exchange energy per grid cell
    int e_e;
} ProcessorEdgeInfo;

typedef Koala::Graph<ProcessorVertexInfo, ProcessorEdgeInfo> ProcessorGraph;
typedef ProcessorGraph::PVertex ProcessorVertex;
typedef ProcessorGraph::PEdge ProcessorEdge;

} /* namespace scheduler */

#endif /* PROCESSORGRAPH_H_ */
