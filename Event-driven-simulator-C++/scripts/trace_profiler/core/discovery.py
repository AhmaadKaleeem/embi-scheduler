import os
import csv
from pathlib import Path
from typing import List, Tuple
from trace_profiler.config import DATASET_DIR, SUPPORTED_EXTENSIONS
from .registry import DatasetRegistry, DatasetDescriptor
from .metadata import compute_sha256_fast

# Optional dependency for encoding detection, fallback if not installed
try:
    import chardet
except ImportError:
    chardet = None

class FileAnalyzer:
    def __init__(self, registry: DatasetRegistry):
        self.registry = registry

    def _detect_encoding(self, file_path: Path, sample_size: int = 100_000) -> str:
        if chardet is None:
            return "utf-8"
        with open(file_path, "rb") as f:
            rawdata = f.read(sample_size)
            result = chardet.detect(rawdata)
            return result['encoding'] or "utf-8"

    def _detect_delimiter(self, file_path: Path, encoding: str) -> str:
        try:
            with open(file_path, "r", encoding=encoding) as f:
                sample = f.read(100_000)
                sniffer = csv.Sniffer()
                dialect = sniffer.sniff(sample)
                return dialect.delimiter
        except Exception:
            return "," # fallback

    def _estimate_rows(self, file_path: Path, file_size: int, encoding: str) -> int:
        try:
            with open(file_path, "r", encoding=encoding) as f:
                sample = f.read(100_000)
                lines = sample.count('\n')
                if lines > 0:
                    avg_line_length = len(sample) / lines
                    return int(file_size / avg_line_length)
        except Exception:
            pass
        return 0

    def discover_datasets(self, directory: Path) -> List[DatasetDescriptor]:
        discovered = []
        for p in directory.rglob("*"):
            if p.is_file() and p.suffix.lower() in SUPPORTED_EXTENSIONS:
                # Check cache first
                existing = self.registry.lookup(p.name)
                mtime = p.stat().st_mtime
                size = p.stat().st_size
                
                if existing and existing.modified_time == mtime and existing.size_bytes == size:
                    discovered.append(existing)
                    continue
                
                print(f"Discovering metadata for {p.name}...")
                
                sha256 = compute_sha256_fast(p)
                encoding = self._detect_encoding(p)
                
                delimiter = None
                if p.suffix.lower() in [".csv", ".txt"]:
                    delimiter = self._detect_delimiter(p, encoding)
                
                est_rows = self._estimate_rows(p, size, encoding)
                
                parser_map = {
                    ".csv": "pyarrow.csv",
                    ".txt": "pyarrow.csv",
                    ".json": "ijson",
                    ".parquet": "pyarrow.parquet",
                    ".arrow": "pyarrow.ipc"
                }
                
                descriptor = DatasetDescriptor(
                    name=p.name,
                    path=str(p.absolute()),
                    file_type=p.suffix.lower(),
                    size_bytes=size,
                    modified_time=mtime,
                    sha256=sha256,
                    parser=parser_map.get(p.suffix.lower(), "unknown"),
                    version="1.0",
                    estimated_rows=est_rows,
                    delimiter=delimiter,
                    encoding=encoding,
                    compression=None # Add logic later if compressed
                )
                self.registry.register(descriptor)
                discovered.append(descriptor)
        return discovered
