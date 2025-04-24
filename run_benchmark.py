import subprocess
import re
import sys
import os
import matplotlib.pyplot as plt

def run_benchmarks(arg: str):
    result = subprocess.run(["xmake", "build"], shell=True, cwd=os.getcwd())
    result = subprocess.run(["./build/windows/x64/release/arena.exe", arg], capture_output=True, text=True)
    return result.stdout

def parse_benchmark_output(output):
    benchmarks = {}

    benchmark_section_pattern = r"=== (.*?) ==="
    benchmark_results_pattern = r"([^\n:]+)\s*:\s*(\d+\.\d+)\s*ms"
    object_size_subsection_pattern = r"^(Small|Medium|Large) Objects:"

    current_section = None
    current_subsection = None

    for line in output.splitlines():
        line = line.strip()

        match_section = re.match(benchmark_section_pattern, line)
        if match_section:
            current_section = match_section.group(1).strip()
            current_subsection = None
            continue

        if current_section and current_section.startswith("Object Size Benchmark"):
            match_subsection = re.match(object_size_subsection_pattern, line)
            if match_subsection:
                subsection = match_subsection.group(1)
                current_subsection = f"{current_section} - {subsection}"
                benchmarks[current_subsection] = []
                continue

        match_result = re.match(benchmark_results_pattern, line)
        if match_result:
            name = match_result.group(1).strip()
            time = float(match_result.group(2))
            section = current_subsection if current_subsection else current_section
            if section not in benchmarks:
                benchmarks[section] = []
            benchmarks[section].append((name, time))

    return benchmarks

def plot_benchmark_results(benchmarks):
    if os.path.isdir("results") == False:
        os.mkdir("results")
    for section, results in benchmarks.items():
        names = [result[0] for result in results]
        times = [result[1] for result in results]
        
        plt.figure(figsize=(10, 6))
        plt.barh(names, times, color='skyblue')
        plt.xlabel('Time (ms)')
        plt.ylabel('Benchmark Method')
        plt.title(f'{section} Performance')
        plt.tight_layout()
        clean_section = re.sub(r"\s*\(.*?\)", "", section)
        plt.savefig(f"./results/{clean_section}.png")
        #plt.show()

def main():
    arguments = sys.argv
    iterations = "1000000"

    if len(arguments) > 1:
        iterations = arguments[1]
    else:
        print("Running benchmark with default iterations: 1000000")
    output = run_benchmarks(iterations)
    
    benchmark_data = parse_benchmark_output(output)
    
    plot_benchmark_results(benchmark_data)

if __name__ == "__main__":
    main()