#include "numvc.h"

static int try_step = 100;

// Heap functions
void NumvcSolver::my_heap_swap(int a, int b) {
    int t = my_heap[a];
    my_heap[a] = my_heap[b];
    pos_in_my_heap[my_heap[a]] = a;
    my_heap[b] = t;
    pos_in_my_heap[my_heap[b]] = b;
}

bool NumvcSolver::my_heap_is_leaf(int pos) {
    return (pos >= my_heap_count / 2) && (pos < my_heap_count);
}

// These are static helpers, not part of the class state
int my_heap_left_child(int pos) {
    return (2 * pos + 1);
}

int my_heap_right_child(int pos) {
    return (2 * pos + 2);
}

int my_heap_parent(int pos) {
    return (pos - 1) / 2;
}

void NumvcSolver::my_heap_shiftdown(int pos) {
    while (!my_heap_is_leaf(pos)) {
        int j = my_heap_left_child(pos);
        int rc = my_heap_right_child(pos);
        if ((rc < my_heap_count) &&
            (dscore[my_heap[rc]] > dscore[my_heap[j]]))
            j = rc;
        if (dscore[my_heap[pos]] > dscore[my_heap[j]])
            return;
        my_heap_swap(pos, j);
        pos = j;
    }
}

void NumvcSolver::my_heap_insert(int v) {
    int curr = my_heap_count++;
    my_heap[curr] = v;
    pos_in_my_heap[v] = curr;

    while (curr != 0 && dscore[my_heap[curr]] >
           dscore[my_heap[my_heap_parent(curr)]]) {
        my_heap_swap(curr, my_heap_parent(curr));
        curr = my_heap_parent(curr);
    }
}

int NumvcSolver::my_heap_remove_first() {
    my_heap_swap(0, --my_heap_count);
    if (my_heap_count != 0)
        my_heap_shiftdown(0);
    return my_heap[my_heap_count];
}

int NumvcSolver::my_heap_remove(int pos) {
    if (pos == (my_heap_count - 1))
        my_heap_count--;
    else {
        my_heap_swap(pos, --my_heap_count);
        while ((pos != 0) && (dscore[my_heap[pos]] >
               dscore[my_heap[my_heap_parent(pos)]])) {
            my_heap_swap(pos, my_heap_parent(pos));
            pos = my_heap_parent(pos);
        }
        if (my_heap_count != 0)
            my_heap_shiftdown(pos);
    }
    return my_heap[my_heap_count];
}

// Core MVC functions
void NumvcSolver::update_best_sol() {
    for (int i = 1; i <= v_num; i++) {
        best_v_in_c[i] = v_in_c[i];
    }
    best_c_size = c_size;
    finish_time_hr = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(finish_time_hr - start_time_hr);
    best_comp_time = duration.count() / 1000000.0;
    best_step = step;
}

// Private helper function: allocate memory
void NumvcSolver::allocate_memory() {
    edge = new Edge[e_num];
    edge_weight = new int[e_num];
    uncov_stack = new int[e_num];
    index_in_uncov_stack = new int[e_num];
    for (int i = 0; i < e_num; i++) {
        index_in_uncov_stack[i] = -1;
    }
    dscore = new int[v_num + 1];
    time_stamp = new long long[v_num + 1];
    v_edges = new int*[v_num + 1];
    v_adj = new int*[v_num + 1];
    v_edge_count = new int[v_num + 1];
    v_in_c = new int[v_num + 1];
    remove_cand = new int[v_num + 1];
    index_in_remove_cand = new int[v_num + 1];
    best_v_in_c = new int[v_num + 1];
    conf_change = new int[v_num + 1];
    my_heap = new int[v_num + 1];
    pos_in_my_heap = new int[v_num + 1];
    my_heap_count = 0;

    for (int v = 1; v <= v_num; v++)
        v_edge_count[v] = 0;
}

// Private helper function: build adjacency structure
void NumvcSolver::build_adjacency() {
    for (int v = 1; v <= v_num; v++) {
        v_adj[v] = new int[v_edge_count[v]];
        v_edges[v] = new int[v_edge_count[v]];
    }

    int* v_edge_count_tmp = new int[v_num + 1];
    for (int v = 1; v <= v_num; v++)
        v_edge_count_tmp[v] = 0;

    for (int e = 0; e < e_num; e++) {
        int v1 = edge[e].v1;
        int v2 = edge[e].v2;

        v_edges[v1][v_edge_count_tmp[v1]] = e;
        v_edges[v2][v_edge_count_tmp[v2]] = e;

        v_adj[v1][v_edge_count_tmp[v1]] = v2;
        v_adj[v2][v_edge_count_tmp[v2]] = v1;

        v_edge_count_tmp[v1]++;
        v_edge_count_tmp[v2]++;
    }

    delete[] v_edge_count_tmp;
}

int NumvcSolver::build_instance(char *filename) {
    char line[1024];
    char tempstr1[10];
    char tempstr2[10];

    ifstream infile(filename);
    if (!infile.is_open())
        return 0;

    infile.getline(line, 1024);
    while (line[0] != 'p')
        infile.getline(line, 1024);
    sscanf(line, "%s %s %d %d", tempstr1, tempstr2, &v_num, &e_num);

    allocate_memory();

    for (int e = 0; e < e_num; e++) {
        char tmp;
        int v1, v2;
        infile >> tmp >> v1 >> v2;
        v_edge_count[v1]++;
        v_edge_count[v2]++;
        edge[e].v1 = v1;
        edge[e].v2 = v2;
    }
    infile.close();

    build_adjacency();
    return 1;
}

int NumvcSolver::build_instance_from_edges(const std::vector<std::pair<int, int>>& edges, int vertex_count) {
    if (vertex_count <= 0 || edges.empty()) {
        v_num = vertex_count;
        e_num = 0;
        return 1;
    }

    v_num = vertex_count;
    e_num = edges.size();

    allocate_memory();

    for (int e = 0; e < e_num; e++) {
        int v1 = edges[e].first + 1;
        int v2 = edges[e].second + 1;

        v_edge_count[v1]++;
        v_edge_count[v2]++;

        edge[e].v1 = v1;
        edge[e].v2 = v2;
    }

    build_adjacency();
    return 1;
}

void NumvcSolver::free_memory() {
    for (int v = 1; v <= v_num; v++) {
        delete[] v_adj[v];
        delete[] v_edges[v];
    }
    delete[] conf_change;
    delete[] best_v_in_c;
    delete[] index_in_remove_cand;
    delete[] remove_cand;
    delete[] v_in_c;
    delete[] v_edge_count;
    delete[] v_adj;
    delete[] v_edges;
    delete[] time_stamp;
    delete[] dscore;
    delete[] index_in_uncov_stack;
    delete[] uncov_stack;
    delete[] edge_weight;
    delete[] edge;
    delete[] my_heap;
    delete[] pos_in_my_heap;
}

void NumvcSolver::reset_remove_cand() {
    int j = 0;
    for (int v = 1; v <= v_num; v++) {
        if (v_in_c[v] == 1) {
            remove_cand[j] = v;
            index_in_remove_cand[v] = j;
            j++;
        } else
            index_in_remove_cand[v] = 0;
    }
    remove_cand_size = j;
}

void NumvcSolver::update_target_size() {
    c_size--;
    int max_improvement = -100000000;
    int max_vertex = 1;

    for (int v = 1; v <= v_num; ++v) {
        if (v_in_c[v] == 0)
            continue;
        if (dscore[v] > max_improvement) {
            max_improvement = dscore[v];
            max_vertex = v;
        }
    }
    remove(max_vertex);
    reset_remove_cand();
}

void NumvcSolver::update_best_cov_v() {
    best_cov_v = remove_cand[0];
    for (int i = 1; i < remove_cand_size; ++i) {
        int v = remove_cand[i];
        if (v == tabu_remove)
            continue;
        if (dscore[v] < dscore[best_cov_v])
            continue;
        else if (dscore[v] > dscore[best_cov_v])
            best_cov_v = v;
        else if (time_stamp[v] < time_stamp[best_cov_v])
            best_cov_v = v;
    }
}

inline void NumvcSolver::uncover(int e) {
    if (e < 0 || e >= e_num) {
    std::cerr << "ERROR: uncover() called with invalid edge index: " << e << std::endl;
    std::abort();
    }
    if (index_in_uncov_stack[e] >= 0) {
        return;
    }
    index_in_uncov_stack[e] = uncov_stack_fill_pointer;
    numvc_push(e, uncov_stack); // Macro uses uncov_stack_fill_pointer member
}

inline void NumvcSolver::cover(int e) {
    if (e < 0 || e >= e_num) {
    std::cerr << "ERROR: cover() called with invalid edge index: " << e
                << " (e_num=" << e_num << ")" << std::endl;
    std::abort();
    }

    // 检查：确保这条边 *确实* 是未覆盖的
    int index = index_in_uncov_stack[e];
    if (index < 0 || index >= uncov_stack_fill_pointer) { // 严格检查
        fprintf(stderr, "FATAL: index_in_uncov_stack[%d] = %d is out of bounds (stack size = %d)\n",
                e, index, uncov_stack_fill_pointer);
        std::abort();
    }

    int last_uncov_edge = numvc_pop(uncov_stack); // Macro uses uncov_stack_fill_pointer member
    // 如果弹出的边恰好是我们要覆盖的边，我们什么都不用做
    // (除了在函数末尾将 e 的索引设为 -1)
    if (e != last_uncov_edge) {
        if (last_uncov_edge < 0 || last_uncov_edge >= e_num) {
            fprintf(stderr, "FATAL: popped invalid edge %d from uncov_stack\n", last_uncov_edge);
            std::abort();
        }
        uncov_stack[index] = last_uncov_edge;
        index_in_uncov_stack[last_uncov_edge] = index;
    }
    index_in_uncov_stack[e] = -1;
}

void NumvcSolver::init_sol() {
    for (int v = 1; v <= v_num; v++) {
        v_in_c[v] = 0;
        dscore[v] = 0;
        conf_change[v] = 1;
        time_stamp[v] = 0;
    }

    for (int e = 0; e < e_num; e++) {
        edge_weight[e] = 1;
        dscore[edge[e].v1] += edge_weight[e];
        dscore[edge[e].v2] += edge_weight[e];
    }

    uncov_stack_fill_pointer = 0;
    for (int e = 0; e < e_num; e++)
        uncover(e);

    for (int v = 1; v <= v_num; v++)
        my_heap_insert(v);

    int i = 0;
    while (uncov_stack_fill_pointer > 0) {
        int best_v = my_heap[0];
        if (dscore[best_v] > 0) {
            add_init(best_v);
            i++;
        } else {
            break;
        }
    }

    c_size = i;
    update_best_sol();
    reset_remove_cand();
    update_best_cov_v();
}

void NumvcSolver::add(int v) {
    v_in_c[v] = 1;
    dscore[v] = -dscore[v];

    int edge_count = v_edge_count[v];
    for (int i = 0; i < edge_count; ++i) {
        int e = v_edges[v][i];
        int n = v_adj[v][i];

        if (v_in_c[n] == 0) {
            dscore[n] -= edge_weight[e];
            conf_change[n] = 1;
            cover(e);
        } else {
            dscore[n] += edge_weight[e];
        }
    }
}

void NumvcSolver::add_init(int v) {
    int pos = pos_in_my_heap[v];
    my_heap_remove(pos);

    v_in_c[v] = 1;
    dscore[v] = -dscore[v];

    int edge_count = v_edge_count[v];
    for (int i = 0; i < edge_count; ++i) {
        int e = v_edges[v][i];
        int n = v_adj[v][i];

        if (v_in_c[n] == 0) {
            dscore[n] -= edge_weight[e];
            conf_change[n] = 1;

            pos = pos_in_my_heap[n];
            my_heap_remove(pos);
            my_heap_insert(n);

            cover(e);
        } else {
            dscore[n] += edge_weight[e];
        }
    }
}

void NumvcSolver::remove(int v) {
    v_in_c[v] = 0;
    dscore[v] = -dscore[v];
    conf_change[v] = 0;

    int edge_count = v_edge_count[v];
    for (int i = 0; i < edge_count; ++i) {
        int e = v_edges[v][i];
        int n = v_adj[v][i];

        if (v_in_c[n] == 0) {
            dscore[n] += edge_weight[e];
            conf_change[n] = 1;
            uncover(e);
        } else {
            dscore[n] -= edge_weight[e];
        }
    }
}

int NumvcSolver::check_solution() {
    for (int e = 0; e < e_num; ++e) {
        if (best_v_in_c[edge[e].v1] != 1 &&
            best_v_in_c[edge[e].v2] != 1) {
            return 0;
        }
    }
    return 1;
}

void NumvcSolver::forget_edge_weights() {
    int new_total_weight = 0;

    for (int v = 1; v <= v_num; v++)
        dscore[v] = 0;

    for (int e = 0; e < e_num; e++) {
        edge_weight[e] = edge_weight[e] * p_scale;
        new_total_weight += edge_weight[e];

        if (v_in_c[edge[e].v1] + v_in_c[edge[e].v2] == 0) {
            dscore[edge[e].v1] += edge_weight[e];
            dscore[edge[e].v2] += edge_weight[e];
        }
        else if (v_in_c[edge[e].v1] + v_in_c[edge[e].v2] == 1) {
            if (v_in_c[edge[e].v1] == 1)
                dscore[edge[e].v1] -= edge_weight[e];
            else
                dscore[edge[e].v2] -= edge_weight[e];
        }
    }
    ave_weight = new_total_weight / e_num;
}

void NumvcSolver::update_edge_weight() {
    for (int i = 0; i < uncov_stack_fill_pointer; ++i) {
        int e = uncov_stack[i];
        edge_weight[e] += 1;
        dscore[edge[e].v1] += 1;
        dscore[edge[e].v2] += 1;
    }

    delta_total_weight += uncov_stack_fill_pointer;
    if (delta_total_weight >= e_num) {
        ave_weight += 1;
        delta_total_weight -= e_num;
    }

    if (ave_weight >= threshold) {
        forget_edge_weights();
    }
}

void NumvcSolver::cover_LS() {
    int best_add_v;
    int e, v1, v2;

    step = 1;

    while (1) {
        if (uncov_stack_fill_pointer == 0) {
            update_best_sol();

            if (c_size == optimal_size)
                return;

            update_target_size();
            continue;
        }

        if (step % try_step == 0) {
            auto current_time = high_resolution_clock::now();
            auto elapsed_duration = duration_cast<microseconds>(current_time - start_time_hr);
            double elap_time = elapsed_duration.count() / 1000000.0;
            if (elap_time >= cutoff_time)
                return;
        }

        update_best_cov_v();

        remove(best_cov_v);

        e = uncov_stack[rng() % uncov_stack_fill_pointer];
        v1 = edge[e].v1;
        v2 = edge[e].v2;

        if (conf_change[v1] == 0)
            best_add_v = v2;
        else if (conf_change[v2] == 0)
            best_add_v = v1;
        else {
            if (dscore[v1] > dscore[v2] ||
                (dscore[v1] == dscore[v2] && time_stamp[v1] < time_stamp[v2]))
                best_add_v = v1;
            else
                best_add_v = v2;
        }

        add(best_add_v);

        int index = index_in_remove_cand[best_cov_v];
        index_in_remove_cand[best_cov_v] = 0;

        remove_cand[index] = best_add_v;
        index_in_remove_cand[best_add_v] = index;

        time_stamp[best_add_v] = time_stamp[best_cov_v] = step;
        tabu_remove = best_add_v;

        update_edge_weight();
        step++;
    }
}

// Public API implementations
MVCResult numvc_solve_mvc(const std::vector<std::pair<int, int>>& edges,
                   int vertex_count, int optimal_size,
                   double cutoff_time, int random_seed) {

    MVCResult result;
    result.success = false;
    if(edges.empty() || vertex_count <= 0) {
        result.success = true;
        result.mvc_size = 0;
        result.solve_time = 0.0;
        result.steps = 0;
        return result;
    }
    NumvcSolver solver;
    solver.optimal_size = optimal_size;
    solver.cutoff_time = cutoff_time;
    solver.threshold = vertex_count / 2;

    solver.rng.seed(random_seed);

    if (solver.build_instance_from_edges(edges, vertex_count) != 1) {
        return result;
    }

    solver.start_time_hr = high_resolution_clock::now();

    solver.init_sol();

    if (solver.c_size + solver.uncov_stack_fill_pointer > optimal_size) {
        solver.cover_LS();
    }

    if (solver.check_solution() == 1) {
        result.success = true;
        result.mvc_size = solver.best_c_size;
        result.solve_time = solver.best_comp_time;
        result.steps = solver.best_step;

        // for (int i = 1; i <= solver.v_num; i++) {
        //     if (solver.best_v_in_c[i] == 1) {
        //         result.mvc.push_back(i);
        //     }
        // }
    }

    solver.free_memory();
    return result;
}

MVCResult solve_mvc_from_file(const char* filename, int optimal_size,
                             double cutoff_time, int random_seed) {

    MVCResult result;
    result.success = false;

    NumvcSolver solver;
    solver.optimal_size = optimal_size;
    solver.cutoff_time = cutoff_time;

    solver.rng.seed(random_seed);

    if (solver.build_instance(const_cast<char*>(filename)) != 1) {
        return result;
    }

    solver.threshold = solver.v_num / 2;

    solver.start_time_hr = high_resolution_clock::now();

    solver.init_sol();

    if (solver.c_size + solver.uncov_stack_fill_pointer > optimal_size) {
        solver.cover_LS();
    }

    if (solver.check_solution() == 1) {
        result.success = true;
        result.mvc_size = solver.best_c_size;
        result.solve_time = solver.best_comp_time;
        result.steps = solver.best_step;

        // for (int i = 1; i <= solver.v_num; i++) {
        //     if (solver.best_v_in_c[i] == 1) {
        //         result.mvc.push_back(i);
        //     }
        // }
    }

    solver.free_memory();
    return result;
}
