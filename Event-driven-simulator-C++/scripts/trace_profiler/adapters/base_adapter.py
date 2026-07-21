from abc import ABC, abstractmethod
from typing import Iterator, Dict, Any, List
from pathlib import Path
from trace_profiler.core.registry import DatasetDescriptor

class ITraceAdapter(ABC):
    def __init__(self, descriptor: DatasetDescriptor):
        self.descriptor = descriptor
        self.file_path = Path(descriptor.path)

    @abstractmethod
    def get_sample_chunks(self, head_rows: int, middle_rows: int, tail_rows: int) -> List[Dict[str, Any]]:
        """
        Returns a sample of the dataset (useful for schema inference without reading the whole file).
        """
        pass

    @abstractmethod
    def iter_batches(self, batch_size: int = 100_000) -> Iterator[Any]:
        """
        Yields batches of data. Depending on the backend (PyArrow vs others), 
        it might yield pyarrow.RecordBatch, pandas.DataFrame, or lists of dicts.
        """
        pass
