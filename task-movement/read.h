/*
 * read.h
 *
 *  Created on: 1 maj 2016
 *      Author: miloszc
 */

#ifndef READ_H_
#define READ_H_

#include "basic_types.h"
#include "taskgraph.h"
#include "processorgraph.h"

#include "Koala/coloring/edge.h"

#include <string>
#include <iomanip>

using namespace Koala;

namespace scheduler {

// Opens file file_name
inline FILE *open_file(const char *file_name) {
    FILE *fp = fopen(file_name, "r");
    if(fp) {
        return fp;
    } else {
        fprintf(stderr, "Could not open: %s!\n", file_name);
        return NULL;
    }
}

// Reads line from FILE
int read_line(FILE *fp, char *line, const int N_BUFF) {
    if(fgets(line, N_BUFF, fp) == NULL) return 1;
    return 0;
}

// Returns int from char array
int get_int(char *line) {
    int val = 0;
    sscanf(line, "%d", &val);
    return val;
}

void read_task_graph_grid(std::string path, Vector3i &grid) {
    FILE *fgrid = open_file(std::string(path + "grid.txt").c_str());
    static const int N_BUFF = 32;
    char line[N_BUFF];
    // 10
    read_line(fgrid, line, N_BUFF);
    grid.x = get_int(line);
    read_line(fgrid, line, N_BUFF);
    grid.y = get_int(line);
    read_line(fgrid, line, N_BUFF);
    grid.z = get_int(line);

    fclose(fgrid);
}

void calculate_task_position(Vector3i gridSize, int taskId, int &x, int &y)
{
    int nShiftPix = 50;
    int mShiftPix = -50;
    int lShiftPix = gridSize.y * mShiftPix;

    // 2x2x2
    // taskId == 9
    // lId == 2
    // mId == 0
    // nId == 1
    int lId = taskId / (gridSize.x * gridSize.y);
    int mId = (taskId - (lId * gridSize.x * gridSize.y)) / gridSize.x;
    int nId = taskId - (mId * gridSize.x + lId * gridSize.x * gridSize.y);
    x = nId * nShiftPix + mId * mShiftPix;
    y = mId * mShiftPix + lId * lShiftPix;
}

// String split
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char **)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void read_task_graph_vertices(std::string path, TaskGraph &taskGraph, std::vector<TaskVertex> &taskVertexVec, Vector3i &grid) {
    typedef std::map<int, TaskVertexInfo> TaskMap_t;
    TaskMap_t taskMap;
    FILE *fvertices = open_file(std::string(path + "vertices.txt").c_str());
    FILE *fvertices_weights = open_file(std::string(path + "vertices_weights.txt").c_str());

    static const int N_BUFF = 32;
    char line[N_BUFF];
    // 0
    while(read_line(fvertices, line, N_BUFF) != 1) {
        int id = get_int(line);
        int x = 0, y = 0;
        calculate_task_position(grid, id, x, y);
        //printf("%d\n", id);
        taskMap.insert(
                std::pair<int, TaskVertexInfo>(
                    id,
                    TaskVertexInfo(id, 1, x, y))
                );
    }
    // 0;1
    while(read_line(fvertices_weights, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int id = 0;
        sscanf(tokens[0], "%d", &id);
        int w = 0;
        sscanf(tokens[1], "%d", &w);
        TaskMap_t::iterator it = taskMap.find(id);
        if(it != taskMap.end()) {
            it->second.w = w;
            //printf("%d %d\n", id, w);
        }
    }
    for(auto const &task : taskMap) {
        const TaskVertexInfo *tinfo = &task.second;
        taskVertexVec.push_back(taskGraph.addVert(*tinfo));
        //printf("%d) w:%d\n", tinfo->id, tinfo->w);
    }
    fclose(fvertices);
    fclose(fvertices_weights);
}

void read_task_graph_edges(std::string path, TaskGraph &taskGraph,
        std::vector<TaskVertex> &taskVertexVec) {
    typedef std::map<std::pair<int, int>, TaskEdgeInfo> TaskEdgeMap_t;
    TaskEdgeMap_t taskEdgeMap;
    FILE *fedges = open_file(std::string(path + "edges.txt").c_str());
    FILE *fedges_weights = open_file(std::string(path + "edges_weights.txt").c_str());

    static const int N_BUFF = 32;
    char line[N_BUFF];

    // 0;1 - directed edge form 0 -> 1
    while(read_line(fedges, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int v1 = 0;
        sscanf(tokens[0], "%d", &v1);
        int v2 = 0;
        sscanf(tokens[1], "%d", &v2);
        //printf("%d,%d\n", v1, v2);
        taskEdgeMap.insert(
                std::pair<std::pair<int, int>, TaskEdgeInfo>(
                        std::pair<int, int>(v1, v2),
                    TaskEdgeInfo(v1, v2, 1))
                );
    }
    // 0;1;1
    while(read_line(fedges_weights, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int v1 = 0;
        sscanf(tokens[0], "%d", &v1);
        int v2 = 0;
        sscanf(tokens[1], "%d", &v2);
        int d = 0;
        sscanf(tokens[2], "%d", &d);
        TaskEdgeMap_t::iterator it = taskEdgeMap.find(std::pair<int, int>(v1, v2));
        if(it != taskEdgeMap.end()) {
            it->second.d = d;
            //printf("%d %d %d\n", v1, v2, d);
        }
    }
    for(auto const &task : taskEdgeMap) {
        const TaskEdgeInfo *tinfo = &task.second;
        // TODO: check should be EdDirIn our Out
        taskGraph.addEdge(taskVertexVec[tinfo->v1], taskVertexVec[tinfo->v2],
                *tinfo, EdDirOut);
        //printf("%d, %d) d:%d\n", tinfo->v1, tinfo->v2, tinfo->d);
    }
    //printf("edgeNo: %d\n", taskGraph.getEdgeNo());
    fclose(fedges);
    fclose(fedges_weights);
}

void read_task_graph(std::string path, TaskGraph &taskGraph, Vector3i &gridSize) {
    std::vector<TaskVertex> taskVertexVec;
    // grid.txt
    /* size
     * n
     * m
     * l
     */
    read_task_graph_grid(path, gridSize);
    // vertices.txt
    /* id
     * 0
     * 1
     * 2
     * 3
     * ..
     */
    // vertices_weights.txt
    /* id;weight
     * 0;2
     * 1;1
     * 2;4
     * 3;8
     * ...
     */
    read_task_graph_vertices(path, taskGraph, taskVertexVec, gridSize);
    // edges.txt
    /* id;id
     * 0;2
     * 3;1
     * 2;4
     * ...
     */
    // edges_weights.txt
    /* id;id;weight
     * 1;2;5
     * 0;3;4
     * 2;4;3
     * ...
     */
    read_task_graph_edges(path, taskGraph, taskVertexVec);
}

// Returns word from char array
void get_word(char *line, char *word) {
    sscanf(line, "%s", word);
}

std::string hexStr(int *data, int len)
{
    std::stringstream ss;
    ss<<std::hex;
    for(int i = 0; i<len; ++i)
        ss<<std::setfill('0')<<std::setw(2)<<data[i];
    return ss.str();
}

std::string makeColorGradient(float frequency1, float frequency2, float frequency3,
                           float phase1, float phase2, float phase3,
                           int step, int center = 0, int width = 0)
{
    if (center == 0)   center = 128;
    if (width == 0)    width = 127;

    int data[3] = {0};
    //red
    data[0] = sin(frequency1*step + phase1) * width + center;
    //green
    data[1] = sin(frequency2*step + phase2) * width + center;
    //blue
    data[2] = sin(frequency3*step + phase3) * width + center;

    return hexStr(data, 3);
}

typedef std::map<std::string, ProcessorVertex> ProcessorVertexMap_t;
void read_processor_graph_vertices(std::string path, ProcessorGraph &processorGraph, ProcessorVertexMap_t &processorVertexMap) {
    typedef std::map<std::string, ProcessorVertexInfo> ProcessorMap_t;
    ProcessorMap_t processorMap;
    FILE *fprocessors = open_file(std::string(path + "processors.txt").c_str());
    FILE *fprocessors_speeds = open_file(std::string(path + "processors_speeds.txt").c_str());
    FILE *fprocessors_energy = open_file(std::string(path + "processors_energy.txt").c_str());
    FILE *fprocessors_p0 = open_file(std::string(path + "processors_p0.txt").c_str());
    FILE *fprocessors_pidle = open_file(std::string(path + "processors_pidle.txt").c_str());

    static const int N_BUFF = 16;
    char line[N_BUFF];
    char word[N_BUFF];
    int id = 0;
    float frequency = 2.4f;

    // p0
    while(read_line(fprocessors, line, N_BUFF) != 1) {
        get_word(line, word);
        processorMap.insert(
                std::pair<std::string, ProcessorVertexInfo>(
                        std::string(word),
                    ProcessorVertexInfo(std::string(word), id, 1, 1, 1, 1, std::string("#" + makeColorGradient(frequency, frequency, frequency, 0, 2, 4, id))))
                );
        //cout << makeColorGradient(frequency, frequency, frequency, 0, 2, 4, id) << endl;
        ++id;
    }
    // p0;1
    while(read_line(fprocessors_speeds, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int t_c = 0;
        sscanf(tokens[1], "%d", &t_c);
        ProcessorMap_t::iterator it = processorMap.find(std::string(tokens[0]));
        if(it != processorMap.end()) {
            processorMap[std::string(tokens[0])].t_c = t_c;
            //printf("%s %d\n", tokens[0], t_c);
        }
    }
    // p0;1
    while(read_line(fprocessors_energy, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int e_c = 0;
        sscanf(tokens[1], "%d", &e_c);
        ProcessorMap_t::iterator it = processorMap.find(std::string(tokens[0]));
        if(it != processorMap.end()) {
            processorMap[std::string(tokens[0])].e_c = e_c;
            //printf("%s %d\n", tokens[0], e_c);
        }
    }
    // p0;1
    while(read_line(fprocessors_p0, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int p0 = 0;
        sscanf(tokens[1], "%d", &p0);
        ProcessorMap_t::iterator it = processorMap.find(std::string(tokens[0]));
        if(it != processorMap.end()) {
            processorMap[std::string(tokens[0])].p0 = p0;
            //printf("%s %d\n", tokens[0], p0);
        }
    }
    // p0;1
    while(read_line(fprocessors_pidle, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        int p_idle = 0;
        sscanf(tokens[1], "%d", &p_idle);
        ProcessorMap_t::iterator it = processorMap.find(std::string(tokens[0]));
        if(it != processorMap.end()) {
            processorMap[std::string(tokens[0])].p_idle = p_idle;
            //printf("%s %d\n", tokens[0], p_idle);
        }
    }
    for(auto const &proc : processorMap) {
        const ProcessorVertexInfo *pinfo = &proc.second;
        processorVertexMap.insert(std::pair<std::string, ProcessorVertex>(pinfo->name, processorGraph.addVert(*pinfo)));
        //printf("%s) t_c:%d e_c:%d p0:%d p_idle:%d\n", pinfo->name.c_str(), pinfo->t_c, pinfo->e_c, pinfo->p0, pinfo->p_idle);
    }
    fclose(fprocessors);
    fclose(fprocessors_speeds);
    fclose(fprocessors_energy);
    fclose(fprocessors_p0);
    fclose(fprocessors_pidle);
}

void read_processor_graph_edges(std::string path, ProcessorGraph &processorGraph, ProcessorVertexMap_t &processorVertexMap) {
    typedef std::map<std::pair<std::string, std::string>, ProcessorEdgeInfo> ProcessorEdgeMap_t;
    ProcessorEdgeMap_t processorEdgeMap;
    FILE *fprocessors_exchange_time = open_file(std::string(path + "processors_exchange_time.txt").c_str());
    FILE *fprocessors_exchange_energy = open_file(std::string(path + "processors_exchange_energy.txt").c_str());

    static const int N_BUFF = 32;
    char line[N_BUFF];

    // 0;1 - directed edge form 0 -> 1
    while(read_line(fprocessors_exchange_time, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        std::string p1(tokens[0]);
        std::string p2(tokens[1]);;
        int t_e = 0;
        sscanf(tokens[2], "%d", &t_e);
        //printf("%s,%s,%d\n", p1.c_str(), p2.c_str(), t_e);
        processorEdgeMap.insert(
                std::pair<std::pair<std::string, std::string>, ProcessorEdgeInfo>(
                        std::pair<std::string, std::string>(p1, p2),
                    ProcessorEdgeInfo(p1, p2, t_e, 0))
                );
    }
    // 0;1;1
    while(read_line(fprocessors_exchange_energy, line, N_BUFF) != 1) {
        char **tokens = str_split(line, ';');
        std::string p1(tokens[0]);
        std::string p2(tokens[1]);;
        int e_e = 0;
        sscanf(tokens[2], "%d", &e_e);
        ProcessorEdgeMap_t::iterator it = processorEdgeMap.find(std::pair<std::string, std::string>(p1, p2));
        if(it != processorEdgeMap.end()) {
            it->second.e_e = e_e;
            //printf("%s %s %d\n", p1.c_str(), p2.c_str(), e_e);
        }
    }
    for(auto const &processor : processorEdgeMap) {
        const ProcessorEdgeInfo *pinfo = &processor.second;
        processorGraph.addEdge(processorVertexMap[pinfo->name1], processorVertexMap[pinfo->name2],
                *pinfo, EdDirOut);
        //printf("%s, %s) t_e:%d e_e:%d\n", pinfo->name1.c_str(), pinfo->name2.c_str(), pinfo->t_e, pinfo->e_e);
    }
    //printf("edgeNo: %d\n", processorGraph.getEdgeNo());
    fclose(fprocessors_exchange_time);
    fclose(fprocessors_exchange_energy);

}

void read_processor_graph(std::string path, ProcessorGraph &processorGraph) {
    ProcessorVertexMap_t processorVertexMap;
    // processors.txt
    /* name
     * p0
     * p1
     */
    // processors_speeds.txt
    /* name;time
     * p0;218
     * p1;28
     */
    // processors_energy.txt
    /* name;energy
     * p0;7645
     * p1;1443
     */
    // processors_p0.txt
    /* name;W
     * p0;90
     * p1;74
     */
    // processors_pidle.txt
    /* name;W
     * p0;10
     * p1;30
     */
    read_processor_graph_vertices(path, processorGraph, processorVertexMap);
    // processors_exchange_time.txt
    /* name1;name2;time
     * p0;p1;10
     * p1;p0;20
     */
    // processors_exchange_energy.txt
    /* name1;name2;energy
     * p0;p1;55768
     * p1;p0;55768
     */
    read_processor_graph_edges(path, processorGraph, processorVertexMap);
}

void read_graph(std::string path, TaskGraph &taskGraph, ProcessorGraph &processorGraph, Vector3i &gridSize) {
    read_task_graph(path, taskGraph, gridSize);
    read_processor_graph(path, processorGraph);
}

} /* namespace scheduler */

#endif /* READ_H_ */
