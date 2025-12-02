# experimental scripts for the MAPF benchmark

import CSV
import Dates
import Base.Threads
import YAML
import Glob: glob
using DataFrames
using Query
import Statistics: mean, median

# round
r = (x) -> round(x, digits = 3)

function main(config_files...)
    # load experimental setting
    config = merge(map(YAML.load_file, config_files)...)
    exec_file = get(config, "exec_file", joinpath(@__DIR__, "..", "build", "main"))
    seed_start = get(config, "seed_start", 1)
    seed_end = get(config, "seed_end", seed_start)
    time_limit_sec = get(config, "time_limit_sec", 10)
    time_limit_sec_force = get(config, "time_limit_sec_force", time_limit_sec + 30)
    scen = get(config, "scen", "scen-random")
    num_min_agents = get(config, "num_min_agents", 10)
    num_max_agents = get(config, "num_max_agents", 1000)
    num_interval_agents = get(config, "num_interval_agents", 10)
    solver_name = get(config, "solver_name", "unnamed")
    solver_options = get(config, "solver_options", [])
    maps = get(config, "maps", Vector{String}())
    date_str = replace(string(Dates.now()), ":" => "-")
    root_dir = joinpath(get(config, "root", joinpath(pwd(), "..", "data", "exp", "heuristic")), date_str)
    !isdir(root_dir) && mkpath(root_dir)

    # save configuration file
    io = IOBuffer()
    versioninfo(io, verbose = true)
    additional_info = Dict(
        "git_hash" => read(`git log -1 --pretty=format:"%H"`, String),
        "date" => date_str,
        "nthreads" => Threads.nthreads(),
        "env" => String(take!(io)),
    )
    YAML.write_file(joinpath(root_dir, "config.yaml"), merge(config, additional_info))

    # generate iterators
    loops =
        readdir(joinpath(@__DIR__, "scen", scen)) |>
        @filter(x -> !isnothing(match(r".scen$", x))) |>
        @map(x -> joinpath(@__DIR__, "scen", scen, x)) |>
        @filter(
            x -> last(split(match(r"\d+\t(.+).map\t.+", readlines(x)[2])[1], "/")) in maps
        ) |>
        @map(
            x -> begin
                lines = readlines(x)
                N_max = min(length(lines) - 1, num_max_agents)
                map_name = joinpath(
                    @__DIR__,
                    "map",
                    last(split(match(r"\d+\t(.+).map\t(.+)", lines[2])[1], "/")) * ".map",
                )
                agents = collect(num_min_agents:num_interval_agents:N_max)
                (isempty(agents) || last(agents) != N_max) && push!(agents, N_max)
                vcat(
                    Iterators.product(
                        [x],
                        [map_name],
                        agents,
                        collect(seed_start:seed_end),
                    )...,
                )
            end
        ) |>
        collect |>
        x -> vcat(x...) |> x -> enumerate(x) |> collect

    # prepare tmp directory
    tmp_dir = joinpath(@__DIR__, "tmp")
    !isdir(tmp_dir) && mkpath(tmp_dir)

    # main loop
    num_total_tasks = length(loops)
    cnt_fin = Threads.Atomic{Int}(0)
    cnt_solved = Threads.Atomic{Int}(0)
    result = Vector{Any}(undef, num_total_tasks)
    t_start = Base.time_ns()

    Threads.@threads for (k, (scen_file, map_file, N, seed)) in loops
        output_file = joinpath(tmp_dir, "result-$(k).txt")
        command = [
            "timeout",
            "$(time_limit_sec_force)s",
            exec_file,
            "-m",
            map_file,
            "-i",
            scen_file,
            "-N",
            N,
            "-o",
            output_file,
            "-t",
            time_limit_sec,
            "-s",
            seed,
            "-l",
            solver_options...,
        ]
        try
            run(pipeline(`$command`))
        catch e
            nothing
        end

        # store results
        row = Dict(
            :solver => solver_name,
            :num_agents => N,
            :map_name => last(split(map_file, "/")),
            :scen => last(split(scen_file, "/")),
            :seed => seed,
            :solved => 0,
            :comp_time => 0,
            :soc => 0,
            :soc_lb => 0,
            :makespan => 0,
            :makespan_lb => 0,
            :sum_of_loss => 0,
            :sum_of_loss_lb => 0,
            :checkpoints => "",
            :comp_time_initial_solution => 0,
            :cost_initial_solution => 0,
            :search_iteration => 0,
            :num_high_level_node => 0,
            :num_low_level_node => 0,
            :updategraph_time_us => 0,
            :cal_mvc_time_us => 0,
            :poco_call_count => 0,
            :poco_result => 0,
        )
        if isfile(output_file)
            for line in readlines(output_file)
                m = match(r"soc=(\d+)", line)
                !isnothing(m) && (row[:soc] = parse(Int, m[1]))
                m = match(r"soc_lb=(\d+)", line)
                !isnothing(m) && (row[:soc_lb] = parse(Int, m[1]))
                m = match(r"makespan=(\d+)", line)
                !isnothing(m) && (row[:makespan] = parse(Int, m[1]))
                m = match(r"makespan_lb=(\d+)", line)
                !isnothing(m) && (row[:makespan_lb] = parse(Int, m[1]))
                m = match(r"sum_of_loss=(\d+)", line)
                !isnothing(m) && (row[:sum_of_loss] = parse(Int, m[1]))
                m = match(r"sum_of_loss_lb=(\d+)", line)
                !isnothing(m) && (row[:sum_of_loss_lb] = parse(Int, m[1]))
                m = match(r"comp_time=(\d+)", line)
                !isnothing(m) && (row[:comp_time] = parse(Int, m[1]))
                m = match(r"solved=(\d+)", line)
                if !isnothing(m)
                    row[:solved] = parse(Int, m[1])
                    (row[:solved] == 1) && (Threads.atomic_add!(cnt_solved, 1))
                end
                m = match(r"checkpoints=(.+)", line)
                !isnothing(m) && (row[:checkpoints] = m[1])
                m = match(r"comp_time_initial_solution=(\d+)", line)
                !isnothing(m) && (row[:comp_time_initial_solution] = parse(Int, m[1]))
                m = match(r"cost_initial_solution=(\d+)", line)
                !isnothing(m) && (row[:cost_initial_solution] = parse(Int, m[1]))
                m = match(r"search_iteration=(\d+)", line)
                !isnothing(m) && (row[:search_iteration] = parse(Int, m[1]))
                m = match(r"num_high_level_node=(\d+)", line)
                !isnothing(m) && (row[:num_high_level_node] = parse(Int, m[1]))
                m = match(r"num_low_level_node=(\d+)", line)
                !isnothing(m) && (row[:num_low_level_node] = parse(Int, m[1]))
                m = match(r"updategraph_time_us=(\d+)", line)
                !isnothing(m) && (row[:updategraph_time_us] = parse(Int, m[1]))
                m = match(r"cal_mvc_time_us=(\d+)", line)
                !isnothing(m) && (row[:cal_mvc_time_us] = parse(Int, m[1]))
                m = match(r"poco_call_count=(\d+)", line)
                !isnothing(m) && (row[:poco_call_count] = parse(Int, m[1]))
                m = match(r"poco_result=(\d+)", line)
                !isnothing(m) && (row[:poco_result] = parse(Int, m[1]))
            end
            rm(output_file)
        end
        result[k] = NamedTuple{Tuple(keys(row))}(values(row))
        Threads.atomic_add!(cnt_fin, 1)
        print(
            "\r" *
            "$(r((Base.time_ns() - t_start) / 1.0e9)) sec" *
            "\t$(cnt_fin[])/$(num_total_tasks) " *
            "($(r(cnt_fin[]/num_total_tasks*100))%)" *
            " tasks have been finished, " *
            "solved: $(cnt_solved[])/$(cnt_fin[]) ($(r(cnt_solved[]/cnt_fin[]*100))%)",
        )
    end

    # save result
    result_file = joinpath(root_dir, "result.csv")
    CSV.write(result_file, result)
    println()
    rm(tmp_dir; recursive = true)
end

function aggregate_results(main_dir=".")
    """
    Traverse directory to find all result.csv files, calculate averages and save to main.csv
    """
    # Define column types
    text_cols = ["map_name", "solver"]
    numeric_cols_standard = [
        "makespan", "soc", "sum_of_loss", "cost_initial_solution", 
        "comp_time_initial_solution", "num_low_level_node", "num_high_level_node"
    ]
        
    # Special processing columns
    orig_col_x = "updategraph_time_us"
    orig_col_y = "cal_mvc_time_us"
    col_x = "updategraph_time_ms"
    col_y = "cal_mvc_time_ms"
    col_z = "poco_result"
    col_t = "poco_call_count"
    
    # Required columns (all solvers must have)
    required_columns = vcat(text_cols, numeric_cols_standard)
    
    # Optional columns (only POCO solvers have)
    optional_cols = [ orig_col_x, orig_col_y, col_z, col_t]

    ori_columns = vcat(text_cols, numeric_cols_standard, optional_cols)
    final_columns = vcat(text_cols, numeric_cols_standard, [col_x, col_y, col_z, col_t])
    
    results = []
    
    println("Starting search for result.csv files...")
    
    # Traverse all subdirectories
    for (root, dirs, files) in walkdir(main_dir)
        if "result.csv" in files
            file_path = joinpath(root, "result.csv")
            try
                df = CSV.read(file_path, DataFrame)
                
                # Check if required columns exist
                missing_cols = [col for col in required_columns if !(col in names(df))]
                if !isempty(missing_cols)
                    println("[Skipped] $file_path: Missing columns $missing_cols")
                    continue
                end
                
                if nrow(df) == 0
                    println("[Skipped] $file_path: Empty file")
                    continue
                end
                
                row_data = Dict()
                
                # A. Process text columns - keep if all values are same, otherwise empty
                for col in text_cols
                    unique_vals = unique(df[!, col])
                    row_data[col] = length(unique_vals) == 1 ? unique_vals[1] : ""
                end
                
                # B. Process standard numeric columns - calculate averages
                for col in numeric_cols_standard
                    row_data[col] = mean(skipmissing(df[!, col]))
                end
                
                # C. Process Optional columns - calculate ratios
                has_poco_cols = all(col in names(df) for col in optional_cols)
                if has_poco_cols
                    val_x = mean(skipmissing(df[!, orig_col_x]))
                    val_y = mean(skipmissing(df[!, orig_col_y]))
                    val_z = mean(skipmissing(df[!, col_z]))
                    val_t = mean(skipmissing(df[!, col_t]))
                    
                    if val_t != 0
                        row_data[col_x] = round((val_x / 1000.0) / val_t, digits = 2) # Convert us to ms and divide by call count
                        row_data[col_y] = round((val_y / 1000.0) / val_t, digits = 2) # Convert us to ms and divide by call count
                        row_data[col_z] = round(val_z / val_t, digits = 2)            # Divide by call count
                        row_data[col_t] = val_t
                    else
                        row_data[col_x] = 0.00
                        row_data[col_y] = 0.00
                        row_data[col_z] = 0.00
                        row_data[col_t] = 0.00
                    end
                else
                    # Set POCO columns to 0 for non-POCO solvers
                    row_data[col_x] = 0.00
                    row_data[col_y] = 0.00
                    row_data[col_z] = 0.00
                    row_data[col_t] = 0.00
                end
                push!(results, row_data)
                println("[Processed successfully] $file_path")
            catch e
                println("[Error] Error processing file $file_path: $e")
            end
        end
    end
    
    # Save results
    if !isempty(results)
        # Create DataFrame from results
        result_df = DataFrame()
        for (i, row_data) in enumerate(results)
            if i == 1
                for (key, value) in row_data
                    result_df[!, key] = [value]
                end
            else
                push!(result_df, row_data)
            end
        end
        # Select available columns in correct order
        available_columns = intersect(final_columns, String.(names(result_df)))
        
        if !isempty(available_columns)
            result_df = select(result_df, Symbol.(available_columns))
            
            # Define display names
            display_names = [
                "map", "solver", "m", "soc", "sol", 
                raw"$c_{ini}$", raw"$t_{ini}$(ms)", raw"$n_l$", raw"$n_h$", 
                raw"$\overline{t_u}$(ms)", raw"$\overline{t_{mvc}}$(ms)", 
                raw"$\overline{h_f}$", raw"$n_{hc}$"
            ]
            
            # Rename columns with corresponding display names
            rename!(result_df, Symbol.(available_columns) .=> display_names[1:length(available_columns)])
            
            output_file = joinpath(main_dir, "main.csv")
            CSV.write(output_file, result_df)
            println("\nSuccess! Aggregated results saved to: $output_file")
        else
            println("\nNo matching columns found!")
        end
    else
        println("\nNo valid result.csv files found")
    end

end

function run_aggregate_results(target_dir)
    """
    Run all experiments first, then aggregate results once
    """
    # 1. Run all experiments first
    config_files = filter(x -> !contains(x, "common"), glob("$(target_dir)/*.yaml"))
    
    println("Running all experiments...")
    for config_file in config_files
        println("Processing: $config_file")
        main("$(target_dir)/common.yaml", config_file)
    end
    
    # 2. Aggregate results only once after all experiments are done
    println("All experiments completed. Starting aggregation...")
    aggregate_results("../data/exp/heuristic")
end