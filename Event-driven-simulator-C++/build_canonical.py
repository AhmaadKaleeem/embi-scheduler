import os
import csv
import json
import hashlib

print("Starting build_canonical script...")

def hash_str(s):
    return int(hashlib.md5(str(s).encode()).hexdigest(), 16) % 100000

def generate_canonical(name, input_files, parser_fn, max_rows=50000):
    output_file = f"scripts/trace_profiler/output_reports/{name}/canonical.csv"
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    print(f"Building {name} trace -> {output_file}")
    
    with open(output_file, 'w', newline='') as f_out:
        writer = csv.writer(f_out)
        writer.writerow(['tick', 'trace_id', 'rpc_id', 'source_service', 'destination_service', 'container_id', 'latency', 'trace_flags', 'type', 'priority'])
        all_rows = []
        for input_file in input_files:
            print(f"  Parsing {input_file}...")
            all_rows.extend(parser_fn(input_file, max_rows))
            
        all_rows.sort(key=lambda x: x[0])  # Sort chronologically by tick
        if all_rows:
            min_tick = all_rows[0][0]
            for r in all_rows:
                r[0] = r[0] - min_tick
                writer.writerow(r)

def parse_alibaba(input_file, max_rows):
    rows = []
    with open(input_file, 'r', encoding='utf-8') as f_in:
        reader = csv.reader(f_in)
        next(reader) # header
        count = 0
        for row in reader:
            if not row or len(row) < 6: continue
            tick = int(row[0])
            trace_id = hash_str(row[1])
            rpc_id = count
            source = hash_str(row[1])
            dest = hash_str(row[3])
            container = hash_str(row[2])
            try:
                lat_float = float(row[4])
                import math
                if math.isnan(lat_float):
                    latency = 10
                else:
                    latency = max(10, int(abs(lat_float) * 1000))
            except:
                latency = 10
            rows.append([tick, trace_id, rpc_id, source, dest, container, latency, 0, 1, 1])
            count += 1
            if count >= max_rows: break
    return rows

def parse_azure(input_file, max_rows):
    rows = []
    with open(input_file, 'r', encoding='utf-8') as f_in:
        reader = csv.reader(f_in)
        next(reader) # header
        count = 0
        for row in reader:
            if not row or len(row) < 4: continue
            try:
                end_ts = float(row[2])
                dur = float(row[3])
                tick = int((end_ts - dur) * 1000)
                if tick < 0: tick = 0
                trace_id = hash_str(row[0])
                rpc_id = count
                source = hash_str(row[1])
                dest = hash_str(row[0])
                container = hash_str(row[1])
                latency = max(10, int(abs(dur) * 1000))
                rows.append([tick, trace_id, rpc_id, source, dest, container, latency, 0, 1, 1])
                count += 1
                if count >= max_rows: break
            except:
                pass
    return rows

def parse_google(input_file, max_rows):
    rows = []
    with open(input_file, 'r', encoding='utf-8') as f_in:
        count = 0
        for line in f_in:
            if not line.strip(): continue
            try:
                data = json.loads(line)
                tick = int(data.get("time", 0)) // 1000000
                trace_id = hash_str(data.get("collection_id", ""))
                rpc_id = count
                source = hash_str(data.get("machine_id", ""))
                dest = hash_str(data.get("alloc_collection_id", ""))
                container = hash_str(str(data.get("instance_index", 0)))
                res_req = data.get("resource_request") or {}
                cpus = res_req.get("cpus", 0.0) if isinstance(res_req, dict) else 0.0
                latency = max(10, int(abs(cpus) * 1000))
                try:
                    priority = max(1, int(data.get("priority", 1)))
                except (ValueError, TypeError):
                    priority = 1
                rows.append([tick, trace_id, rpc_id, source, dest, container, latency, 0, 1, priority])
                count += 1
                if count >= max_rows: break
            except:
                pass
    return rows

if __name__ == '__main__':
    generate_canonical("alibaba", [r"../Dataset/Ali Baba_0to30_Min.csv", r"../Dataset/AliBaba_31-60_Min.csv"], parse_alibaba)
    generate_canonical("azure", [r"../Dataset/AzureFunctionsInvocationTraceForTwoWeeksJan2021.txt"], parse_azure)
    generate_canonical("google", [r"../Dataset/Google_Instance_Events_2019_Cluster_Data.json"], parse_google)
    print("Done generating canonical traces!")
