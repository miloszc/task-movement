# Author: miloszc
#
# stencil_energy_model.zpl
#
# This ILP tries to efficiently distribute the stencil tasks between heterogeneous processors with heterogeneous network connections. 
# It minimizes the sum of the number of grid's cells multiplied by energy on all processors and the energy used by all edges. 
# It assumes that the computations and communications are done in parallel. Each processor can set up single communication
# at a time. Graph G(V,E) with V - represents cells, E - represents communications (stencil). P - processors reperesent complete graph.

# 
# Sets
# Types:
# 1. Conditional sets that satisfy a Boolean expression - with clause
# 2. Indexed sets - [ and ]
#
set V := { read "vertices.txt" as "<1s>" }; # Vertices - gird's cells
do print V;
set E := { read "edges.txt" as "<1s,2s>"}; # Edges
do print E;
set P := { read "processors.txt" as "<1s>" }; # Processors
do print P;
set PP := { <p, k> in P * P }; # P^2
set E_PP := E cross PP; # E x P^2

#
# Parameters
# Defines constatns.
# Types:
# 1. Parameters
# 2. Parameter tables
#
param lv := read "least_common_multiple_degree.txt" as "1n"; # Least common multiple of all degrees in G
do print lv;
param wu[V] :=  read "vertices_weights.txt" as "<1s> 2n"; # Weights of the vertices - number of cells on vertex u
do forall <i> in V do print wu[i];
param degu[V] := read "vertices_degrees.txt" as "<1s> 2n"; # Degrees of the vertices - number of incident edges on vertex u
do forall <i> in V do print degu[i];
param duv[E] := read "edges_weights.txt" as "<1s,2s> 3n"; # Weights of edges - number of cells to be transfered on edge u,v
do forall <u,v> in E do print duv[u,v]; 
param m[P] := read "processors_capacities.txt" as "<1s> 2n"; # Memory size in blocks of the processor
do forall <p> in P do print m[p];
# Time #######################
param tp[P] := read "processors_speeds.txt" as "<1s> 2n"; # Processing time of single cell on processor p 
do forall <i> in P do print tp[i];
param tpk[PP] := read "processors_exchange_time.txt" as "<1s,2s> 3n"; # Exchange times of single cell between processors p and k 
do forall <p,k> in PP with p != k do print tpk[p,k];
# Energy ####################
param ep[P] := read "processors_energy.txt" as "<1s> 2n"; # Energy cost of computing single grid cell on p
do forall <p> in P do print ep[p];
param epk[PP] := read "processors_exchange_energy.txt" as "<1s, 2s> 3n"; # Energy cost of sending single grid cell between p and k
do forall <p,k> in PP with p != k do print epk[p,k];
# Power #####################
param P0[P] := read "processors_p0.txt" as "<1s> 2n"; # Power P0 on processor p
do forall <p> in P do print P0[p];
param Pidle[P] := read "processors_pidle.txt" as "<1s> 2n"; # Idle power on processor p
do forall <p> in P do print Pidle[p];
# Constants used for calculationg maximum number of colors ############
param maxduv := max <u,v> in E : duv[u,v]; # Maximum weight (amount of data to be transeferd) in all edges
do print "maxduv:", maxduv;
param maxtpk := max <p,k> in PP with p != k: tpk[p,k]; # Maximum exchange time in all pairs p,k 
do print "maxtpk:", maxtpk;
param maxdeg := max <u> in V : degu[u]; # Maximum degree of all vertices
do print "maxdeg:", maxdeg;
# basedon on chromatic index(G) <= max degree(G) + max multiplicity(G)
# or chromatic index(G) <= 3/2 max degree(G)
param maxcolors := 6 * maxdeg * maxtpk * maxduv; # Maximum number of colors
do print "maxcolors:", maxcolors;
param td := read "deadline.txt" as "1n"; # Time deadline from max{t[p] * b[p], sum <c> in C : z[c]}
do print "deadline:", td;


set C := { 1..maxcolors }; # {1..|E|}
set PP_C := PP cross C; # P^2 x C

var x[E_PP] binary;  # Is edge e is mapped to p,k slot?
var y[PP_C] binary; # Is edge p,k colored by c?
var z[C] binary;  # Is c used in edge coloring ?
var b[P] integer; # Number of cells on processor p 
var ec integer; # Global energy cost of the communication
var tidle[P] integer; # Idle time of the processor p
var td2 integer; # Time deadline from max{t[p] * b[p], sum <c> in C : z[c]}
var e_cost integer; # Objective value

defset uplus(u) := {<u, i> in E}; # Edge starts in u
defset uminus(u) := {<i, u> in E}; # Edge ends in u
defset vplus(u) := {<v, i> in E}; # Edge starts in v
defset vminus(u) := {<i, v> in E}; # Edge ends in v
# deg of vertex u mapped on processor p
#defnumb deg(u, p) := sum <k> in P :
#                    (sum <u, i> in uplus(u): x[u, i, p, k] + sum <i, u> in uminus(u): x[<i, u>, k, p]);

#
# Objective
#
# Minimize the maximum from {the number of cells b on the most heavily loaded processor,
# the number of communication stages z}
# Minimize the sum from the sum of energy used to compute blocks on all processors and the sum of energy used to communicate cells.
#
minimize cost: e_cost;

subto min_e_cost: e_cost >= sum <p> in P :
                    (b[p] * ep[p] + tp[p] * b[p] * P0[p] + tidle[p] * Pidle[p]) + ec;

#minimize cost: sum <p> in P :
#                    (b[p] * ep[p] + tp[p] * b[p] * P0[p] + tidle[p] * Pidle[p]) + ec;

#
# Constraints
#

# Deadline from max{t[p] * b[p], sum <c> in C : z[c]}
#
subto max_td2_td: td2 <= td;

# Deadline from max{t[p] * b[p], sum <c> in C : z[c]}
#
subto max_tp_bp: forall <p> in P :
                    tp[p] * b[p] <= td2;

# Deadline from max{t[p] * b[p], sum <c> in C : z[c]}
#
subto max_sum_zc: sum <c> in C : z[c] <= td2;

# Idle time on processor p
#
subto idle_p: forall <p> in P :
                td2 - tp[p] * b[p] <= tidle[p]; 

# Each edge is mapped to single processor or between two processors
#
subto map_edges: forall <u, v> in E : 
                    sum <p, k> in PP : 
                        x[u, v, p, k] == 1;
   
# Sum of cells mapped to processor p in P 
subto sum_cells: forall <p> in P : 
                    sum <u, v> in  E:
                        sum <k> in P: 
                            (wu[u] * lv /
                                degu[u] * x[u, v, p, k] + 
                            wu[v] * lv /
                                degu[v] * x[u, v, k, p]) <= lv * b[p];

                           # (wu[u] * lv /
                           #     (sum <l> in P :
                           #         (sum <t, i> in uplus(u): x[t, i, p, l] + sum <i, t> in uminus(u): x[i, t, l, p])) * x[u, v, p, k] + 
                           # wu[v] * lv /
                           #     (sum <l> in P :
                           #         (sum <t, i> in uplus(v): x[t, i, p, l] + sum <i, t> in uminus(v): x[i, t, l, p])) * x[u, v, k, p]) <= b;
# Restrict edges to some slots 
#
# If stencil edge u,v is mapped to processor edge p,k than exists outgoing edge u,i mapped to p,k
#
subto restrict_edges1: forall <u, v> in E :
                        forall <p> in P :
                            forall <t, i> in uplus(u) :
                                sum <k> in P : x[u, v, p, k] - sum <k> in P : x[t, i, p, k] == 0;

# If stencil edge u,v is mapped to processor edge p,k than exists ingoing edge i,u mapped to p,k
#
subto restrict_edges2: forall <u, v> in E :
                        forall <p> in P :
                            forall <i, t> in uminus(u) :
                                sum <k> in P : x[u, v, p, k] - sum <k> in P : x[i, t, k, p] == 0;

# If stencil edge u,v is mapped to processor edge k,p than exists outgoing edge v,i mapped to p,k
#
subto restrict_edges3: forall <u, v> in E :
                        forall <p> in P :
                            forall <t, i> in vplus(v) :
                                sum <k> in P : x[u, v, k, p] - sum <k> in P : x[t, i, p, k] == 0;

# If stencil edge u,v is mapped to processor edge k,p than exists ingoing edge i,v mapped to k,p
#
subto restrict_edges4: forall <u, v> in E :
                        forall <p> in P :
                            forall <i, t> in vminus(v) :
                                sum <k> in P : x[u, v, k, p] - sum <k> in P : x[i, t, k, p] == 0;

# Requires that each edge receives at least as many colors as its multiplicity demands
# Each edge is multiplied by time tpk required to exchange data duv
#
subto multiplicity_colors: forall <p, k> in PP with p != k:
                            sum <u, v> in E : (x[u, v, p, k] * tpk[p, k] * duv[u, v] + x[u, v, k, p] * tpk[k, p] * duv[u, v]) <= sum <c> in C : y[p, k, c];

# Requires that incident edges do not receive the same color and that an edge can only receive a colour that is used
#
subto incident_colors: forall <p> in P:
                        forall <c> in C:
                            sum <k> in P with k != p : y[p, k, c] <= z[c];


# Sum of energy used by communication
#
subto sum_edges: sum <u, v> in E :
                    sum <p, k> in PP with p != k :
                        x[u, v, p, k] * duv[u, v] * epk[p,k] <= ec;

# Restrict b to capacity of the processor
#
subto restrict_capacity: forall <p> in P:
                            b[p] <= m[p];

