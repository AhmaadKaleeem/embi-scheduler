from typing import Dict, Any
import os
from trace_profiler.config import DEFAULT_CHUNK_SIZE, MAX_WORKERS, MEMORY_BUDGET_GB

class Optimizer:
    @staticmethod
    def get_performance_profile(size_bytes: int) -> Dict[str, Any]:
        """
        Dynamically adjusts tuning parameters based on dataset size.
        """
        gb = size_bytes / (1024**3)
        
        if gb < 1:
            profile = "Small"
            chunk_size = DEFAULT_CHUNK_SIZE
            workers = min(2, MAX_WORKERS)
            budget = min(1.0, MEMORY_BUDGET_GB)
        elif gb < 10:
            profile = "Medium"
            chunk_size = DEFAULT_CHUNK_SIZE * 5
            workers = min(4, MAX_WORKERS)
            budget = min(4.0, MEMORY_BUDGET_GB)
        elif gb < 100:
            profile = "Large"
            chunk_size = DEFAULT_CHUNK_SIZE * 10
            workers = min(8, MAX_WORKERS)
            budget = min(16.0, MEMORY_BUDGET_GB)
        else:
            profile = "Very Large"
            chunk_size = DEFAULT_CHUNK_SIZE * 20
            workers = MAX_WORKERS
            budget = max(32.0, MEMORY_BUDGET_GB)
            
        return {
            "profile": profile,
            "chunk_size": chunk_size,
            "workers": workers,
            "memory_budget_gb": budget
        }
