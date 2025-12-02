#ifndef MVC_H
#define MVC_H

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <sys/times.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <random>

using namespace std;
using namespace std::chrono;

#define numvc_pop(stack) stack[--stack##_fill_pointer]
#define numvc_push(item, stack) stack[stack##_fill_pointer++] = item

struct Edge {
    int v1;
    int v2;
};

struct MVCResult {
    int mvc_size;
    // std::vector<int> mvc;
    double solve_time;
    int steps;
    bool success;
};

class NumvcSolver {
public:
    // timing
    high_resolution_clock::time_point start_time_hr;
    high_resolution_clock::time_point finish_time_hr;

    // parameters of algorithm
    long long max_steps;
    double cutoff_time;
    long long step;
    int optimal_size;

    // parameters of the instance
    int v_num;
    int e_num;

    // structures about edge
    Edge *edge;
    int *edge_weight;

    // structures about vertex
    int *dscore;
    long long *time_stamp;
    int best_cov_v;

    // from vertex to it's edges and neighbors
    int **v_edges;
    int **v_adj;
    int *v_edge_count;

    // structures about solution
    int c_size;
    int *v_in_c;
    int *remove_cand;
    int *index_in_remove_cand;
    int remove_cand_size;

    // best solution found
    int best_c_size;
    int *best_v_in_c;
    double best_comp_time;
    long best_step;

    // uncovered edge stack
    int *uncov_stack;
    int uncov_stack_fill_pointer;
    int *index_in_uncov_stack;

    // CC and taboo
    int *conf_change;
    int tabu_remove;

    // smooth
    int ave_weight;
    int delta_total_weight;
    int threshold;
    float p_scale;

    // heap
    int *my_heap;
    int *pos_in_my_heap;
    int my_heap_count;

    std::mt19937 rng;

    // constructor
    NumvcSolver() : tabu_remove(0), ave_weight(1), delta_total_weight(0),
                    p_scale(0.3f), my_heap_count(0), uncov_stack_fill_pointer(0),
                    edge(nullptr), edge_weight(nullptr), dscore(nullptr), time_stamp(nullptr),
                    v_edges(nullptr), v_adj(nullptr), v_edge_count(nullptr), v_in_c(nullptr),
                    remove_cand(nullptr), index_in_remove_cand(nullptr), best_v_in_c(nullptr),
                    conf_change(nullptr), uncov_stack(nullptr), index_in_uncov_stack(nullptr),
                    my_heap(nullptr), pos_in_my_heap(nullptr) {
        start_time_hr = high_resolution_clock::now();
        finish_time_hr = high_resolution_clock::now();
    }

    // Member functions (previously took NumvcSolver* as first parameter)
    int build_instance(char *filename);
    int build_instance_from_edges(const std::vector<std::pair<int, int>>& edges, int vertex_count);
    void init_sol();
    void cover_LS();
    void add(int v);
    void add_init(int v);
    void remove(int v);
    void forget_edge_weights();
    void update_edge_weight();
    int check_solution();
    void free_memory();
    void reset_remove_cand();
    void update_target_size();
    void update_best_cov_v();
    void update_best_sol();
    void uncover(int e);
    void cover(int e);

    // Heap functions
    void my_heap_swap(int a, int b);
    bool my_heap_is_leaf(int pos);
    void my_heap_shiftdown(int pos);
    void my_heap_insert(int v);
    int my_heap_remove_first();
    int my_heap_remove(int pos);
private:
    void allocate_memory();
    void build_adjacency();
};

// Static helper functions (don't need solver instance)
int my_heap_left_child(int pos);
int my_heap_right_child(int pos);
int my_heap_parent(int pos);

// Public API
MVCResult numvc_solve_mvc(const std::vector<std::pair<int, int>>& edges,
                        int vertex_count,
                        int optimal_size = 0,
                        double cutoff_time = 10.0,
                        int random_seed = 1);

MVCResult solve_mvc_from_file(const char* filename,
                             int optimal_size = 0,
                             double cutoff_time = 10.0,
                             int random_seed = 1);

inline int numvc(const std::vector<std::pair<int, int>>& edges, int vertex_count){
    auto result= numvc_solve_mvc(edges, vertex_count, 0, 0.0005, 1);
    return result.mvc_size;
}
#endif
