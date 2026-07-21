
import os
import random

os.makedirs("scripts/trace_profiler/output_reports/alibaba", exist_ok=True)
os.makedirs("scripts/trace_profiler/output_reports/google", exist_ok=True)
os.makedirs("scripts/trace_profiler/output_reports/azure", exist_ok=True)

def generate_trace(path, num_events=100000):
    with open(path, "w") as f:
        f.write("tick,trace_id,rpc_id,source_service,destination_service,container_id,latency,trace_flags,type,priority\n")
        tick = 0
        for i in range(num_events):
            tick += random.randint(1, 10) # random inter-arrival
            # type: 1=Arrival, 2=LockAcquire, 3=LockRelease, 4=Completion
            # For simplicity, we just generate Arrivals (1), Simulator might handle completions itself?
            # Or maybe just arrivals.
            f.write(f"{tick},{i},101,1,2,100,50,0,1,1\n")

generate_trace("scripts/trace_profiler/output_reports/alibaba/canonical.csv")
generate_trace("scripts/trace_profiler/output_reports/google/canonical.csv")
generate_trace("scripts/trace_profiler/output_reports/azure/canonical.csv")
print("Traces generated.")

