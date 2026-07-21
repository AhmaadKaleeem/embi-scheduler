#!/usr/bin/env python3
"""
generate_manifest.py

Generates a reproducibility manifest for the artifact evaluation committee.
Computes SHA256 hashes of the canonical datasets and records parameters.

Usage:
    python generate_manifest.py --cleaned-dir ../../data/alibaba/cleaned
"""

import argparse
import os
import hashlib
import json
import datetime

def hash_file(filepath):
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        # Read and update hash in chunks of 4K
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cleaned-dir', type=str, required=True)
    args = parser.parse_args()
    
    manifest = {
        "dataset": "alibaba-cluster-trace-v2021",
        "date_processed": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "tool_version": "1.0.0",
        "cleaning_parameters": {
            "rtt_cap_ms": 5000,
            "tick_resolution_ns": 1000,
            "drop_nan": True,
            "sort_stable": True
        },
        "files": {}
    }
    
    if os.path.exists(args.cleaned_dir):
        for root, dirs, files in os.walk(args.cleaned_dir):
            for file in files:
                if file.endswith(".csv"):
                    filepath = os.path.join(root, file)
                    manifest["files"][file] = {
                        "sha256": hash_file(filepath),
                        "size_bytes": os.path.getsize(filepath)
                    }
                    
    manifest_path = os.path.join(args.cleaned_dir, "..", "manifest.json")
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=4)
        
    print(f"Manifest generated at: {manifest_path}")

if __name__ == "__main__":
    main()
