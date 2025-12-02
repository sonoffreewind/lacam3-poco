# LaCAM3-POCO: Experimental Validation Framework

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

This repository serves as the **experimental validation framework** for the paper:
> **"Hostile, Compatible, or Free: A Constant Time Classification of Pairwise Shortest Path Conflicts in Obstacle-Free MAPF"**

It integrates the **[POCO Library](https://github.com/sonoffreewind/poco)** (our core contribution) into the state-of-the-art **LaCAM3** solver to empirically validate the effectiveness of $O(1)$ conflict classification and MVC-based heuristics.

## 🎯 Purpose

The primary goal of this project is to **experimentally validate the performance** of the POCO library, rather than to propose a new LaCAM variant. 

By using LaCAM3 as a host platform, we aim to:
- **Verify Theoretical Claims**: Demonstrate that POCO's $O(1)$ conflict classification translates into tangible search efficiency.
- **Assess Heuristic Impact**: Measure how MVC-based heuristics (derived from POCO) influence search guidance compared to standard heuristics.
- **Analyze Trade-offs**: Quantify the computational cost of POCO's conflict detection versus the savings in search node expansions.

## 🏗️ Project Structure

This repository uses a modular structure to ensure reproducibility:

- **`poco/`**: The core conflict detection library (Git Submodule).
- **`lacam3/`**: The modified LaCAM3 planner source code supporting multiple heuristics.
- **`numvc/`**: Encapsulated C++ wrapper for the NuMVC solver (used for exact heuristics).
- **`scripts/`**: The Julia-based experimental evaluation framework.

## 📦 Solver Variants

We provide four specific solver variants to isolate the impact of different components, consistent with the paper's definitions:

| Binary Name | Description | Role in Paper |
|:---|:---|:---|
| **`lacam3_fixed`** | **Original LaCAM3** implementation with minimal bug fixes. | **Reference (Original)** |
| **`lacam3_base`** | **Optimized Baseline**. Includes our pruning optimizations and stability fixes. | **Primary Baseline** |
| **`lacam3_poco_lb`** | Integrated with POCO's fast **Lower Bound (LB)** heuristic. | **Efficiency Validation** |
| **`lacam3_poco`** | Integrated with POCO's exact **NuMVC** heuristic. | **Accuracy Validation** |

## 🛠️ Build Instructions

### Prerequisites
- CMake ≥ 3.16
- C++17 compatible compiler (GCC/Clang)
- Julia ≥ 1.7 (only required for running large-scale benchmarks)

### Compilation
```bash
# 1. Clone with submodules (Important!)
git clone --recursive https://github.com/sonoffreewind/lacam3-poco.git

cd lacam3-poco

# 2. Build all variants
cmake -B build && make -C build
```

*The executable binaries will be generated in `build`.*

## 🧪 Quick Run

You can test individual scenarios directly from the command line:

```bash
# 1. Run the POCO-enhanced solver (Accuracy Validation)
./build/lacam3_poco -i assets/random-32-32-10-random-1.scen -m assets/random-32-32-10.map -N 300 -v 1

# 2. Run the Baseline (Baseline Comparison)
./build/lacam3_base -i assets/random-32-32-10-random-1.scen -m assets/random-32-32-10.map -N 300 -v 1

# 3. Check Help
./build/lacam3_poco --help
```
## ⚙️ Experimental Configuration

### Solver Parameters
Key parameters for controlled experiments:

- `-N`: Number of agents (for scalability testing)
- `-t`: Time limit (seconds)
- `-s`: Random seed (for reproducibility)
- `-v`: Verbosity level
- `--no-star`: Disable anytime search
- `--pibt-num`: Monte-Carlo configuration count

### Experiment Setup
Customize experiments via YAML files in `scripts/config/`:
```yaml
solver_name: "lacam3_poco"
time_limit_sec: 30
num_min_agents: 10
num_max_agents: 500
maps: ["random-32-32-10", "empty-16-16"]
```

## 📊 Reproducing Experiments (Julia)

We use the Julia framework (inherited from the original LaCAM3) for batch experiments and data analysis.

### 1. Setup Environment
```bash
# Install required Julia packages
sh scripts/setup.sh
```
**Note**: For detailed Julia environment setup and troubleshooting, please refer to the [original LaCAM3 documentation](https://github.com/Kei18/lacam3#experimental-evaluation).

### 2. Run Benchmarks
```julia
# Start Julia with multi-threading
julia --project=scripts/ --threads=auto

# Load the evaluation script
include("scripts/eval.jl")

#julia: Single experiment validation
main("scripts/config/mapf-bench.yaml")

#julia: Comprehensive validation suite
run_aggregate_results("scripts/config/exp_heu")
```

## 🔗 Related Repositories

- **[POCO](https://github.com/sonoffreewind/poco)**: The standalone C++ library for Pairwise Obstacle-free Conflict Oracle. If you want to use our algorithm in your own solver (e.g., CBS), use this library.
- **[LaCAM3](https://github.com/Kei18/lacam3)**: The original solver by Keisuke Okumura.

## 📚 Citation

If you use this code or the POCO library in your research, please cite our paper:

```bibtex
@article{Guo2025POCO,
  title={Hostile, Compatible, or Free: A Constant Time Classification of Pairwise Shortest Path Conflicts in Obstacle-Free MAPF},
  author={Lifeng Guo and Changhong Lu},
  journal={Under Review},
  year={2025}
}
```

**Please also cite the original LaCAM3 work:**
```bibtex
@inproceedings{okumura_Engineering_2024,
    author = {Okumura, Keisuke},
    title = {Engineering LaCAM*: Towards Real-time, Large-scale, and Near-optimal Multi-agent Pathfinding},
    year = {2024},
    booktitle = {Proceedings of the 23rd International Conference on Autonomous Agents and Multiagent Systems},
    pages = {1501–1509},
    numpages = {9}
}
```
**and the NuMVC work:**
```bibtex
  @article{cai_2013_numvc,
    title = {NuMVC: An Efficient Local Search Algorithm for Minimum Vertex Cover},
    author = {Cai, S. and Su, K. and Luo, C. and Sattar, A.},
    journal = {JAIR},
    volume = {46},
    year = {2013}
  }
```

## 🙏 Acknowledgments & Attribution

This project stands on the shoulders of giants:

1.  **LaCAM3**: The base planner is derived from [Keisuke Okumura's LaCAM3](https://github.com/Kei18/lacam3).
2.  **NuMVC**: The exact MVC solver (`numvc/`) is adapted from the work of Cai et al.
    * *Original Source*: [NuMVC Project](https://lcs.ios.ac.cn/~caisw/VC.html)
    * *Modification*: We encapsulated the original C implementation into a thread-safe C++ class structure (`numvc.cpp/h`) to allow integration into the LaCAM search loop.        

## 📄 License

- **LaCAM3-POCO**: MIT License.
- **Original LaCAM3**: MIT License.
- **NuMVC Module**: Adapted from the original implementation by Cai et al.