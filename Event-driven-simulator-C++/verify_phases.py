import subprocess
import json
import os
import sys

BIN = r".\build_release_new\bin\embi_sim.exe"

def run_phase1():
    print("--- Phase 1: Dry Run ---")
    subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File", ".\phase1_dry_run.ps1"], check=True)
    subprocess.run(["python", "aggregate.py", "results/raw_dryrun_2"], check=True)
    print("Phase 1 verified!")

def run_phase2():
    print("--- Phase 2: Warmup Verification ---")
    out_dir = "results/warmup_test"
    os.makedirs(out_dir, exist_ok=True)
    cmd = [BIN, "--scheduler", "embi", "--workload", "uniform", "--arrival-rate", "0.5", 
           "--ticks", "1000000", "--warmup-ticks", "200000", "--log-freq", "1000000", "--output", out_dir]
    subprocess.run(cmd, check=True)
    
    with open(os.path.join(out_dir, "summary.json"), "r") as f:
        data = json.load(f)
    
    measured = data["config"]["measured_ticks"]
    if measured != 800000:
        raise Exception(f"Warmup verification failed: measured_ticks is {measured}")
    print(f"Phase 2 verified: measured_ticks = {measured}")

def run_phase3():
    print("--- Phase 3: Human Trace Verification ---")
    out_dir = "results/human_trace"
    os.makedirs(out_dir, exist_ok=True)
    cmd = [
            ".\\build_release_new\\bin\\embi_sim.exe",
            "--scheduler", "embi",
            "--workload", "trace",
            "--trace", "appendix_a_human_trace.txt",
            "--ticks", "1000",
            "--output", "results/human_trace"
        ]
    # We just run it to make sure it doesn't crash and outputs the trace. 
    subprocess.run(cmd, check=True)
    print("Phase 3 verified: trace ran without crashing!")

if __name__ == "__main__":
    run_phase1()
    run_phase2()
    run_phase3()
