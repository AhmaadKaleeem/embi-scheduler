import json
import time
import platform
import subprocess
from pathlib import Path
from typing import Dict, Any

class ManifestGenerator:
    @staticmethod
    def get_git_commit() -> str:
        try:
            return subprocess.check_output(["git", "rev-parse", "HEAD"]).decode("utf-8").strip()
        except Exception:
            return "unknown"

    @staticmethod
    def generate(dataset_name: str, sha256: str, runtime: float, peak_ram_mb: float, reports: list, output_dir: Path):
        manifest = {
            "dataset": dataset_name,
            "sha256": sha256,
            "profiler_version": "1.0.0",
            "git_commit": ManifestGenerator.get_git_commit(),
            "python_version": platform.python_version(),
            "os": platform.system() + " " + platform.release(),
            "timestamp": time.time(),
            "runtime_seconds": runtime,
            "peak_ram_mb": peak_ram_mb,
            "generated_reports": reports
        }
        
        output_dir.mkdir(parents=True, exist_ok=True)
        with open(output_dir / "manifest.json", "w") as f:
            json.dump(manifest, f, indent=2)
            
        return manifest
