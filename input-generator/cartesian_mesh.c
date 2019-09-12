#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#define N_BUFF 16
#define SUMMIT 0
#define PIZ_DAINT 1
#define FUGAKU 2


typedef struct Vector3i_t {
	int x;
	int y;
	int z;
} Vector3i;

typedef struct Stencil6i_t {
	int l;
	int r;
	int b;
	int t;
	int g;
	int s;
} Stencil6i;

// Greatest common divisor
int gcd(int m, int n)
{
	int tmp;
	while (m) { tmp = m; m = n % m; n = tmp; }
	return n;
}

// Least common multiple
int lcm(int m, int n)
{
	return m / gcd(m, n) * n;
}

// Reads line from FILE
void read_line(FILE *fp, char *line, int nbuff) {
	if (fgets(line, nbuff, fp) == NULL) perror("Could not read file!");
}

// Returns int from char array
int get_int(char *line, int nbuff) {
	int val = 0;
	sscanf(line, "%d", &val);
	return val;
}

// Read input parameters
void read_graph_parameters(Vector3i *grid, Stencil6i *stencil, int *n_processors, int *vertex_default_weight, Vector3i *edge_default_weights) {
	// x
	// y
	// z
	FILE *fgrid = fopen("grid.txt", "r");

	// l
	// r
	// b
	// t
	// g
	// s
	FILE *fstencil = fopen("stencil.txt", "r");

	// n
	FILE *fnprocessors = fopen("nprocessors.txt", "r");

	// weight
	FILE *fvertex_default_weight = fopen("vertex_default_weight.txt", "r");

	// weight i
	// weight j
	// weight k
	FILE *fedge_default_weights = fopen("edge_default_weights.txt", "r");

	char line[N_BUFF];
	// grid.x; grid.y; grid.z
	read_line(fgrid, line, N_BUFF);
	grid->x = get_int(line, N_BUFF);
	read_line(fgrid, line, N_BUFF);
	grid->y = get_int(line, N_BUFF);
	read_line(fgrid, line, N_BUFF);
	grid->z = get_int(line, N_BUFF);

	// stencil.l, stencil.r, ..
	read_line(fstencil, line, N_BUFF);
	stencil->l = get_int(line, N_BUFF);
	read_line(fstencil, line, N_BUFF);
	stencil->r = get_int(line, N_BUFF);
	read_line(fstencil, line, N_BUFF);
	stencil->b = get_int(line, N_BUFF);
	read_line(fstencil, line, N_BUFF);
	stencil->t = get_int(line, N_BUFF);
	read_line(fstencil, line, N_BUFF);
	stencil->g = get_int(line, N_BUFF);
	read_line(fstencil, line, N_BUFF);
	stencil->s = get_int(line, N_BUFF);

	// nprocessors
	read_line(fnprocessors, line, N_BUFF);
	*n_processors = get_int(line, N_BUFF);

	// weight
	read_line(fvertex_default_weight, line, N_BUFF);
	*vertex_default_weight = get_int(line, N_BUFF);

	// edge_default_weights.x
	// edge_default_weights.y
	// edge_default_weights.z
	read_line(fedge_default_weights, line, N_BUFF);
	edge_default_weights->x = get_int(line, N_BUFF);
	read_line(fedge_default_weights, line, N_BUFF);
	edge_default_weights->y = get_int(line, N_BUFF);
	read_line(fedge_default_weights, line, N_BUFF);
	edge_default_weights->z = get_int(line, N_BUFF);

	fclose(fgrid);
	fclose(fstencil);
	fclose(fnprocessors);
}

//int file_exists(const char *filename) {
//    return access(filename, F_OK) != -1;
//}

FILE *file_open(const char *filename) {
	/*if(!file_exists(filename)) { */return fopen(filename, "w");/* };*/
	return NULL;
}

/// single processor
void update_processor(int i, int n_processors, int n_dimensions, FILE *fprocessors, FILE *fprocessors_speeds, FILE *fprocessors_capacities, FILE *fprocessors_energy, FILE *fprocessors_p0, FILE *fprocessors_pidle, FILE *fprocessors_exchange_time, FILE *fprocessors_exchange_energy, FILE *fprocessors_setup_time)
{
	fprintf(fprocessors, "p%d\n", i);
	if (fprocessors_speeds) { fprintf(fprocessors_speeds, "p%d;1\n", i); };
	if (fprocessors_capacities) { fprintf(fprocessors_capacities, "p%d;100000\n", i); };
	if (fprocessors_energy) { fprintf(fprocessors_energy, "p%d;1\n", i); };
	if (fprocessors_p0) { fprintf(fprocessors_p0, "p%d;1\n", i); };
	if (fprocessors_pidle) { fprintf(fprocessors_pidle, "p%d;1\n", i); };
	for (int j = 0; j < n_processors; ++j) {
		if (i != j) {
			if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;2\n", i, j); };
			if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;1\n", i, j); };
			for (int d = 0; d < n_dimensions; ++d) {
				// 105 - i ascii code
				if (fprocessors_setup_time) { fprintf(fprocessors_setup_time, "p%d;p%d;%c;1\n", i, j, 105 + d); };
			}
		}
	}
}

/// single processor summit
void update_processor_summit(int i, int n_processors, int n_dimensions, FILE *fprocessors, FILE *fprocessors_speeds, FILE *fprocessors_capacities, FILE *fprocessors_energy, FILE *fprocessors_p0, FILE *fprocessors_pidle, FILE *fprocessors_exchange_time, FILE *fprocessors_exchange_energy, FILE *fprocessors_setup_time)
{
	int i_processor = i % 8;
	int i_is_CPU = i_processor == 0 || i_processor == 1;
	int i_is_GPU = i_processor == 2 || i_processor == 3 || i_processor == 4 || i_processor == 5 || i_processor == 6 || i_processor == 7;
	int i_node = i / 8;
	int i_fat_tree_level1 = i_node / 2;
	int i_fat_tree_level2 = i_fat_tree_level1 / 2;

	fprintf(fprocessors, "p%d\n", i);
	if (i_is_CPU) if (fprocessors_speeds) { fprintf(fprocessors_speeds, "p%d;114\n", i); };
	if (i_is_GPU) if (fprocessors_speeds) { fprintf(fprocessors_speeds, "p%d;22\n", i); };
	if (fprocessors_capacities) { fprintf(fprocessors_capacities, "p%d;100000\n", i); };
	if (i_is_CPU) if (fprocessors_energy) { fprintf(fprocessors_energy, "p%d;5530\n", i); };
	if (i_is_GPU) if (fprocessors_energy) { fprintf(fprocessors_energy, "p%d;505\n", i); };
	if (i_is_CPU) if (fprocessors_p0) { fprintf(fprocessors_p0, "p%d;70\n", i); };
	if (i_is_GPU) if (fprocessors_p0) { fprintf(fprocessors_p0, "p%d;70\n", i); };
	if (i_is_CPU) if (fprocessors_pidle) { fprintf(fprocessors_pidle, "p%d;10\n", i); };
	if (i_is_GPU) if (fprocessors_pidle) { fprintf(fprocessors_pidle, "p%d;30\n", i); };
	for (int j = 0; j < n_processors; ++j) {
		if (i != j) {
			int j_processor = j % 8;
			int j_is_CPU = j_processor == 0 || j_processor == 1;
			int j_is_GPU = j_processor == 2 || j_processor == 3 || j_processor == 4 || j_processor == 5 || j_processor == 6 || j_processor == 7;
			int j_node = j / 8;
			int j_fat_tree_level1 = j_node / 2;
			int j_fat_tree_level2 = j_fat_tree_level1 / 2;

			// intranode
			if (i_node == j_node)
			{
				if (i_is_CPU) {
					// 4.77 * 1-^-7
					if (j_is_CPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;5\n", i, j); }; }
					if (j_is_GPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;6\n", i, j); }; }
				}

				if (i_is_GPU) {
					if (j_is_CPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;6\n", i, j); }; }
					if (j_is_GPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;6\n", i, j); }; }
				}

				// the same for all types of PUs
				if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;328\n", i, j); };
			} // other node
			else {
				if (i_fat_tree_level1 == j_fat_tree_level1)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;12\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;328000\n", i, j); };
				}
				else if (i_fat_tree_level2 == j_fat_tree_level2)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;12\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;656000\n", i, j); };
				}
				else
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;12\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;983000\n", i, j); };
				}
			}

			for (int d = 0; d < n_dimensions; ++d) {
				// 105 - i ascii code
				if (fprocessors_setup_time) { fprintf(fprocessors_setup_time, "p%d;p%d;%c;1\n", i, j, 105 + d); };
			}
		}
	}
}

/// single processor piz daint
void update_processor_piz_daint(int i, int n_processors, int n_dimensions, FILE *fprocessors, FILE *fprocessors_speeds, FILE *fprocessors_capacities, FILE *fprocessors_energy, FILE *fprocessors_p0, FILE *fprocessors_pidle, FILE *fprocessors_exchange_time, FILE *fprocessors_exchange_energy, FILE *fprocessors_setup_time)
{
	int i_processor = i % 2;
	int i_is_CPU = i_processor == 0;
	int i_is_GPU = i_processor == 1;
	int i_node = i / 2;
	int i_fat_tree_level1 = i_node / 8;
	int i_fat_tree_level2 = i_fat_tree_level1 / 2;

	fprintf(fprocessors, "p%d\n", i);
	if (i_is_CPU) if (fprocessors_speeds) { fprintf(fprocessors_speeds, "p%d;114\n", i); };
	if (i_is_GPU) if (fprocessors_speeds) { fprintf(fprocessors_speeds, "p%d;27\n", i); };
	if (fprocessors_capacities) { fprintf(fprocessors_capacities, "p%d;100000\n", i); };
	if (i_is_CPU) if (fprocessors_energy) { fprintf(fprocessors_energy, "p%d;4410\n", i); };
	if (i_is_GPU) if (fprocessors_energy) { fprintf(fprocessors_energy, "p%d;842\n", i); };
	if (i_is_CPU) if (fprocessors_p0) { fprintf(fprocessors_p0, "p%d;90\n", i); };
	if (i_is_GPU) if (fprocessors_p0) { fprintf(fprocessors_p0, "p%d;70\n", i); };
	if (i_is_CPU) if (fprocessors_pidle) { fprintf(fprocessors_pidle, "p%d;10\n", i); };
	if (i_is_GPU) if (fprocessors_pidle) { fprintf(fprocessors_pidle, "p%d;30\n", i); };
	for (int j = 0; j < n_processors; ++j) {
		if (i != j) {
			int j_processor = j % 2;
			int j_is_CPU = j_processor == 0;
			int j_is_GPU = j_processor == 1;
			int j_node = j / 2;
			int j_fat_tree_level1 = j_node / 8;
			int j_fat_tree_level2 = j_fat_tree_level1 / 2;

			// intranode
			if (i_node == j_node)
			{
				if (i_is_CPU) {
					// 1.93 * 10^-6
					if (j_is_CPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;19\n", i, j); }; }
					if (j_is_GPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;19\n", i, j); }; }
				}

				if (i_is_GPU) {
					if (j_is_CPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;19\n", i, j); }; }
					if (j_is_GPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;19\n", i, j); }; }
				}

				// the same for all types of PUs
				if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;328\n", i, j); };
			} // other node
			else {
				if (i_fat_tree_level1 == j_fat_tree_level1)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;153\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;328000\n", i, j); };
				}
				else if (i_fat_tree_level2 == j_fat_tree_level2)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;174\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;656000\n", i, j); };
				}
				else
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;195\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;983000\n", i, j); };
				}
			}

			for (int d = 0; d < n_dimensions; ++d) {
				// 105 - i ascii code
				if (fprocessors_setup_time) { fprintf(fprocessors_setup_time, "p%d;p%d;%c;1\n", i, j, 105 + d); };
			}
		}
	}
}

/// single processor fugaku
void update_processor_fugaku(int i, int n_processors, int n_dimensions, FILE *fprocessors, FILE *fprocessors_speeds, FILE *fprocessors_capacities, FILE *fprocessors_energy, FILE *fprocessors_p0, FILE *fprocessors_pidle, FILE *fprocessors_exchange_time, FILE *fprocessors_exchange_energy, FILE *fprocessors_setup_time)
{
	int i_processor = i;
	int i_is_CPU = 1;
	int i_node = i;
	int i_fat_tree_level1 = i_node / 6;
	int i_fat_tree_level2 = i_fat_tree_level1 / 2;
	int i_fat_tree_level3 = i_fat_tree_level2 / 2;

	fprintf(fprocessors, "p%d\n", i);
	if (i_is_CPU) if (fprocessors_speeds) { fprintf(fprocessors_speeds, "p%d;19\n", i); };
	if (fprocessors_capacities) { fprintf(fprocessors_capacities, "p%d;100000\n", i); };
	if (i_is_CPU) if (fprocessors_energy) { fprintf(fprocessors_energy, "p%d;932\n", i); };
	if (i_is_CPU) if (fprocessors_p0) { fprintf(fprocessors_p0, "p%d;50\n", i); };
	if (i_is_CPU) if (fprocessors_pidle) { fprintf(fprocessors_pidle, "p%d;5\n", i); };
	for (int j = 0; j < n_processors; ++j) {
		if (i != j) {
			int j_processor = j;
			int j_is_CPU = 1;
			int j_node = j;
			int j_fat_tree_level1 = j_node / 6;
			int j_fat_tree_level2 = j_fat_tree_level1 / 2;
			int j_fat_tree_level3 = j_fat_tree_level2 / 2;

			// intranode
			if (i_node == j_node)
			{
				if (i_is_CPU) {
					// 1.93 * 10^-6
					if (j_is_CPU) { if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;19\n", i, j); }; }
				}

				// the same for all types of PUs
				if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;328\n", i, j); };
			} // other node
			else {
				if (i_fat_tree_level1 == j_fat_tree_level1)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;7\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;328000\n", i, j); };
				}
				else if (i_fat_tree_level2 == j_fat_tree_level2)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;7\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;656000\n", i, j); };
				}
				else if (i_fat_tree_level3 == j_fat_tree_level3)
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;7\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;983000\n", i, j); };
				}
				else
				{
					if (fprocessors_exchange_time) { fprintf(fprocessors_exchange_time, "p%d;p%d;7\n", i, j); };
					if (fprocessors_exchange_energy) { fprintf(fprocessors_exchange_energy, "p%d;p%d;1311000\n", i, j); };
				}
			}

			for (int d = 0; d < n_dimensions; ++d) {
				// 105 - i ascii code
				if (fprocessors_setup_time) { fprintf(fprocessors_setup_time, "p%d;p%d;%c;1\n", i, j, 105 + d); };
			}
		}
	}
}

/// processors
void update_processors(int supercomputer, int n_processors, int n_dimensions, FILE *fprocessors, FILE *fprocessors_speeds, FILE *fprocessors_capacities, FILE *fprocessors_energy, FILE *fprocessors_p0, FILE *fprocessors_pidle, FILE *fprocessors_exchange_time, FILE *fprocessors_exchange_energy, FILE *fprocessors_setup_time)
{
	for (int i = 0; i < n_processors; ++i) {
		if (supercomputer == SUMMIT)
		{
			update_processor_summit(i, n_processors, n_dimensions, fprocessors, fprocessors_speeds, fprocessors_capacities, fprocessors_energy, fprocessors_p0, fprocessors_pidle, fprocessors_exchange_time, fprocessors_exchange_energy, fprocessors_setup_time);
		} else if (supercomputer == PIZ_DAINT)
		{
			update_processor_piz_daint(i, n_processors, n_dimensions, fprocessors, fprocessors_speeds, fprocessors_capacities, fprocessors_energy, fprocessors_p0, fprocessors_pidle, fprocessors_exchange_time, fprocessors_exchange_energy, fprocessors_setup_time);
		}
		else if (supercomputer == FUGAKU)
		{
			update_processor_fugaku(i, n_processors, n_dimensions, fprocessors, fprocessors_speeds, fprocessors_capacities, fprocessors_energy, fprocessors_p0, fprocessors_pidle, fprocessors_exchange_time, fprocessors_exchange_energy, fprocessors_setup_time);
		}
		else {
			update_processor(i, n_processors, n_dimensions, fprocessors, fprocessors_speeds, fprocessors_capacities, fprocessors_energy, fprocessors_p0, fprocessors_pidle, fprocessors_exchange_time, fprocessors_exchange_energy, fprocessors_setup_time);
		}
	}
}

// Generate an input data to ILP model
int main(int argc, char *argv[]) {
	// Periodic
	Vector3i periodic = { .x = 0,.y = 0,.z = 0 };
	int supercomputer = 0;
	if (argc == 5) {
		sscanf(argv[1], "%d", &periodic.x);
		sscanf(argv[2], "%d", &periodic.y);
		sscanf(argv[3], "%d", &periodic.z);
		sscanf(argv[4], "%d", &supercomputer);
		printf("Periodic: %d %d %d %d\n", periodic.x, periodic.y, periodic.z, supercomputer);
	}
	else {
		printf("Provide arguments: <int-perx> <int-pery> <int-perz> <int-supercomputer>\n");
		exit(1);
	}

	// Grid size
	Vector3i grid = { .x = 1,.y = 1,.z = 1 };
	// Stencil pattern
	Stencil6i stencil = { .l = 1,.r = 1,.b = 1,.t = 1,.g = 1,.s = 1 };
	// Number of processor
	int n_processors = 1;
	// Default vertex weight (size of the block)
	int vertex_default_weight = 1;
	// Edge default weights
	Vector3i edge_default_weights = { .x = 1,.y = 1,.z = 1 };

	read_graph_parameters(&grid, &stencil, &n_processors, &vertex_default_weight, &edge_default_weights);

	// Prepare output files
	// Generated from nprocessors
	FILE *fprocessors = fopen("processors.txt", "w");
	FILE *fprocessors_speeds = file_open("processors_speeds.txt");
	FILE *fprocessors_capacities = file_open("processors_capacities.txt");
	FILE *fprocessors_exchange_time = file_open("processors_exchange_time.txt");
	FILE *fprocessors_setup_time = file_open("processors_setup_time.txt");
	FILE *fprocessors_energy = file_open("processors_energy.txt");
	FILE *fprocessors_exchange_energy = file_open("processors_exchange_energy.txt");
	FILE *fprocessors_p0 = file_open("processors_p0.txt");
	FILE *fprocessors_pidle = file_open("processors_pidle.txt");
	FILE *fvertices = fopen("vertices.txt", "w");
	FILE *fvertices_weights = fopen("vertices_weights.txt", "w");
	FILE *fvertices_degrees = fopen("vertices_degrees.txt", "w");
	FILE *fdimensions = fopen("dimensions.txt", "w");
	FILE *fedges = fopen("edges.txt", "w");
	FILE *fedges_directions = fopen("edges_directions.txt", "w");
	FILE *flcm_degree = fopen("least_common_multiple_degree.txt", "w");
	FILE *fedges_weights = fopen("edges_weights.txt", "w");

	// Dimensions
	// i
	// j
	// k
	if (grid.x > 1) fprintf(fdimensions, "i\n");
	if (grid.y > 1) fprintf(fdimensions, "j\n");
	if (grid.z > 1) fprintf(fdimensions, "k\n");

	int n_dimensions = (grid.x > 1) + (grid.y > 1) + (grid.z > 1);

	// Processors
	update_processors(supercomputer, n_processors, n_dimensions, fprocessors, fprocessors_speeds, fprocessors_capacities, fprocessors_energy, fprocessors_p0, fprocessors_pidle, fprocessors_exchange_time, fprocessors_exchange_energy, fprocessors_setup_time);

	// Prepare array of the degrees
	int n_vertices = grid.x * grid.y * grid.z;
	int *ver_degrees = (int *)malloc(sizeof(int) * n_vertices);
	memset(ver_degrees, 0, sizeof(int) * n_vertices);

	// Vertices
	for (int k = 0; k < grid.z; ++k) {
		for (int j = 0; j < grid.y; ++j) {
			for (int i = 0; i < grid.x; ++i) {
				fprintf(fvertices, "%d\n", i + j * grid.x + k * grid.x*grid.y);
				fprintf(fvertices_weights, "%d;%d\n", i + j * grid.x + k * grid.x*grid.y, vertex_default_weight);
				// Vertex's ids - middle, left, right, bottom, top, gnd, sky
				int ver_m = i + j * grid.x + k * grid.x*grid.y;
				int ver_l = i - 1 + j * grid.x + k * grid.x*grid.y;
				int ver_r = i + 1 + j * grid.x + k * grid.x*grid.y;
				int ver_b = i + (j - 1)*grid.x + k * grid.x*grid.y;
				int ver_t = i + (j + 1)*grid.x + k * grid.x*grid.y;
				int ver_g = i + j * grid.x + (k - 1)*grid.x*grid.y;
				int ver_s = i + j * grid.x + (k + 1)*grid.x*grid.y;
				// Create edges from perspective of sender
				// Send data from %d -> ver_m 
				if ((i == 0) && (stencil.l == 1) && (periodic.x == 1)) {
					int ver_l_p = grid.x - 1 + j * grid.x + k * grid.x*grid.y;
					++ver_degrees[ver_l_p]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_l_p, ver_m, edge_default_weights.x);
					fprintf(fedges_directions, "%d;%d;i;1\n", ver_l_p, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_l_p, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_l_p, ver_m);
					fprintf(fedges, "%d;%d\n", ver_l_p, ver_m);
				}
				if ((i > 0) && (stencil.l == 1)) {
					++ver_degrees[ver_l]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_l, ver_m, edge_default_weights.x);
					fprintf(fedges_directions, "%d;%d;i;1\n", ver_l, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_l, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_l, ver_m);
					fprintf(fedges, "%d;%d\n", ver_l, ver_m);
				}
				if ((i == (grid.x - 1)) && (stencil.r == 1) && (periodic.x == 1)) {
					int ver_r_p = 0 + j * grid.x + k * grid.x*grid.y;
					++ver_degrees[ver_r_p]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_r_p, ver_m, edge_default_weights.x);
					fprintf(fedges_directions, "%d;%d;i;1\n", ver_r_p, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_r_p, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_r_p, ver_m);
					fprintf(fedges, "%d;%d\n", ver_r_p, ver_m);
				}
				if ((i < (grid.x - 1)) && (stencil.r == 1)) {
					++ver_degrees[ver_r]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_r, ver_m, edge_default_weights.x);
					fprintf(fedges_directions, "%d;%d;i;1\n", ver_r, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_r, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_r, ver_m);
					fprintf(fedges, "%d;%d\n", ver_r, ver_m);
				}
				if ((j == 0) && (stencil.b == 1) && (periodic.y == 1)) {
					int ver_b_p = i + (grid.y - 1)*grid.x + k * grid.x*grid.y;
					++ver_degrees[ver_b_p]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_b_p, ver_m, edge_default_weights.y);
					fprintf(fedges_directions, "%d;%d;j;1\n", ver_b_p, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_b_p, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_b_p, ver_m);
					fprintf(fedges, "%d;%d\n", ver_b_p, ver_m);
				}
				if ((j > 0) && (stencil.b == 1)) {
					++ver_degrees[ver_b]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_b, ver_m, edge_default_weights.y);
					fprintf(fedges_directions, "%d;%d;j;1\n", ver_b, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_b, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_b, ver_m);
					fprintf(fedges, "%d;%d\n", ver_b, ver_m);
				}
				if ((j == (grid.y - 1)) && (stencil.t == 1) && (periodic.y == 1)) {
					int ver_t_p = i + (0)*grid.x + k * grid.x*grid.y;
					++ver_degrees[ver_t_p]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_t_p, ver_m, edge_default_weights.y);
					fprintf(fedges_directions, "%d;%d;j;1\n", ver_t_p, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_t_p, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_t_p, ver_m);
					fprintf(fedges, "%d;%d\n", ver_t_p, ver_m);
				}
				if ((j < (grid.y - 1)) && (stencil.t == 1)) {
					++ver_degrees[ver_t]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_t, ver_m, edge_default_weights.y);
					fprintf(fedges_directions, "%d;%d;j;1\n", ver_t, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_t, ver_m);
					if (grid.z > 1) fprintf(fedges_directions, "%d;%d;k;0\n", ver_t, ver_m);
					fprintf(fedges, "%d;%d\n", ver_t, ver_m);
				}
				if ((k == 0) && (stencil.g == 1) && (periodic.z == 1)) {
					int ver_g_p = i + j * grid.x + (grid.z - 1)*grid.x*grid.y;
					++ver_degrees[ver_g_p]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_g_p, ver_m, edge_default_weights.z);
					fprintf(fedges_directions, "%d;%d;k;1\n", ver_g_p, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_g_p, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_g_p, ver_m);
					fprintf(fedges, "%d;%d\n", ver_g_p, ver_m);
				}
				if ((k > 0) && (stencil.g == 1)) {
					++ver_degrees[ver_g]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_g, ver_m, edge_default_weights.z);
					fprintf(fedges_directions, "%d;%d;k;1\n", ver_g, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_g, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_g, ver_m);
					fprintf(fedges, "%d;%d\n", ver_g, ver_m);
				}
				if ((k == (grid.z - 1)) && (stencil.s == 1) && (periodic.z == 1)) {
					int ver_s_p = i + j * grid.x + (0)*grid.x*grid.y;
					++ver_degrees[ver_s_p]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_s_p, ver_m, edge_default_weights.z);
					fprintf(fedges_directions, "%d;%d;k;1\n", ver_s_p, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_s_p, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_s_p, ver_m);
					fprintf(fedges, "%d;%d\n", ver_s_p, ver_m);
				}
				if ((k < (grid.z - 1)) && (stencil.s == 1)) {
					++ver_degrees[ver_s]; ++ver_degrees[ver_m];
					if (fedges_weights) fprintf(fedges_weights, "%d;%d;%d\n", ver_s, ver_m, edge_default_weights.z);
					fprintf(fedges_directions, "%d;%d;k;1\n", ver_s, ver_m);
					if (grid.y > 1) fprintf(fedges_directions, "%d;%d;j;0\n", ver_s, ver_m);
					if (grid.x > 1) fprintf(fedges_directions, "%d;%d;i;0\n", ver_s, ver_m);
					fprintf(fedges, "%d;%d\n", ver_s, ver_m);
				}
			}
		}
	}

	// Degrees of the vertices
	for (int k = 0; k < grid.z; ++k) {
		for (int j = 0; j < grid.y; ++j) {
			for (int i = 0; i < grid.x; ++i) {
				int ver_m = i + j * grid.x + k * grid.x*grid.y;
				// Vertex_id;degree
				fprintf(fvertices_degrees, "%d;%d\n", ver_m, ver_degrees[ver_m]);
			}
		}
	}

	// Least common multiple of all degrees
	int lcm_degree = 1;
	for (int i = 0; i < n_vertices; ++i)
	{
		lcm_degree = lcm(ver_degrees[i], lcm_degree);
	}
	fprintf(flcm_degree, "%d\n", lcm_degree);

	fclose(fprocessors);
	if (fprocessors_speeds) { fclose(fprocessors_speeds); };
	if (fprocessors_capacities) { fclose(fprocessors_capacities); };
	if (fprocessors_exchange_time) { fclose(fprocessors_exchange_time); };
	if (fprocessors_setup_time) { fclose(fprocessors_setup_time); };
	if (fprocessors_energy) { fclose(fprocessors_energy); };
	if (fprocessors_exchange_energy) { fclose(fprocessors_exchange_energy); };
	if (fprocessors_p0) { fclose(fprocessors_p0); };
	if (fprocessors_pidle) { fclose(fprocessors_pidle); };
	fclose(fvertices);
	fclose(fvertices_weights);
	fclose(fvertices_degrees);
	fclose(fdimensions);
	fclose(fedges);
	fclose(fedges_directions);
	fclose(flcm_degree);
	if (fedges_weights) { fclose(fedges_weights); }

	return 0;
}
