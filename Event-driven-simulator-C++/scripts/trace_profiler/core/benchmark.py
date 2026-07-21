import time
import os
import psutil
from typing import Dict, Any

class Benchmark:
    def __init__(self):
        self.start_time = 0
        self.end_time = 0
        self.process = psutil.Process(os.getpid())
        self.peak_memory = 0
        self.bytes_processed = 0
        self.rows_processed = 0

    def start(self):
        self.start_time = time.time()
        self.peak_memory = self.process.memory_info().rss

    def update(self, bytes_added: int, rows_added: int):
        self.bytes_processed += bytes_added
        self.rows_processed += rows_added
        current_mem = self.process.memory_info().rss
        if current_mem > self.peak_memory:
            self.peak_memory = current_mem

    def stop(self):
        self.end_time = time.time()

    def get_results(self) -> Dict[str, Any]:
        duration = self.end_time - self.start_time
        mb_processed = self.bytes_processed / (1024 * 1024)
        return {
            "duration_seconds": duration,
            "mb_per_sec": mb_processed / duration if duration > 0 else 0,
            "rows_per_sec": self.rows_processed / duration if duration > 0 else 0,
            "peak_memory_mb": self.peak_memory / (1024 * 1024),
            "cpu_time": self.process.cpu_times().user
        }
