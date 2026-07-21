import os
from pathlib import Path

# Paths
WORKSPACE_DIR = Path(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DATASET_DIR = Path(r"d:\Ahmad\Startup\EMBI Scheduling Framework\dataset")
PROFILER_DIR = WORKSPACE_DIR / "scripts" / "trace_profiler"
CACHE_DIR = PROFILER_DIR / "cache"

# Performance Tuning
# These values can be overridden by the optimizer based on dataset size
DEFAULT_CHUNK_SIZE = 100_000
MAX_WORKERS = os.cpu_count() or 4
MEMORY_BUDGET_GB = 4.0

# Sampling Config for Schema Inference
SAMPLE_HEAD_ROWS = 100_000
SAMPLE_MIDDLE_ROWS = 100_000
SAMPLE_TAIL_ROWS = 100_000

# File Types Supported
SUPPORTED_EXTENSIONS = {".csv", ".txt", ".json", ".parquet", ".arrow"}
