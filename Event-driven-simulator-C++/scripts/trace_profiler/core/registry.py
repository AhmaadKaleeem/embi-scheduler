import json
from pathlib import Path
from typing import Dict, Optional, Any
from dataclasses import dataclass, asdict
import hashlib

@dataclass
class DatasetDescriptor:
    name: str
    path: str
    file_type: str  # csv, json, txt, etc.
    size_bytes: int
    modified_time: float
    sha256: str
    parser: str
    version: str
    estimated_rows: Optional[int] = None
    delimiter: Optional[str] = None
    encoding: Optional[str] = None
    compression: Optional[str] = None
    schema: Optional[Dict[str, Any]] = None
    statistics: Optional[Dict[str, Any]] = None
    profile: Optional[Dict[str, Any]] = None

class DatasetRegistry:
    def __init__(self, cache_file: Path):
        self.cache_file = cache_file
        self.datasets: Dict[str, DatasetDescriptor] = {}
        self.load()

    def load(self):
        if self.cache_file.exists():
            try:
                with open(self.cache_file, "r") as f:
                    data = json.load(f)
                    for k, v in data.items():
                        self.datasets[k] = DatasetDescriptor(**v)
            except Exception as e:
                print(f"Warning: Failed to load dataset registry cache: {e}")

    def save(self):
        self.cache_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.cache_file, "w") as f:
            json.dump({k: asdict(v) for k, v in self.datasets.items()}, f, indent=2)

    def register(self, descriptor: DatasetDescriptor):
        self.datasets[descriptor.name] = descriptor
        self.save()

    def lookup(self, name: str) -> Optional[DatasetDescriptor]:
        return self.datasets.get(name)

    def get_all(self) -> Dict[str, DatasetDescriptor]:
        return self.datasets
