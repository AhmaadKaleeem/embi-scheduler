from typing import Dict, Any, List
import pyarrow as pa
from trace_profiler.adapters.factory import AdapterFactory
from trace_profiler.core.registry import DatasetDescriptor

class SchemaInferencer:
    def __init__(self, descriptor: DatasetDescriptor):
        self.descriptor = descriptor
        self.adapter = AdapterFactory.get_adapter(descriptor)

    def infer_schema(self, head_rows: int = 100_000) -> Dict[str, Any]:
        """
        Infers schema by sampling the dataset.
        For CSVs parsed via PyArrow, we can just use PyArrow's inferred schema on the first batch.
        For JSON, we sample the objects and infer types.
        """
        # Get sample data
        sample_data = self.adapter.get_sample_chunks(head_rows, 0, 0)
        
        if not sample_data:
            return {}

        inferred = {}
        # If the sample data is a list of dicts (e.g. from JSON or to_pylist())
        if isinstance(sample_data, list) and len(sample_data) > 0 and isinstance(sample_data[0], dict):
            # basic inference from first non-null values
            for row in sample_data:
                for k, v in row.items():
                    if k not in inferred and v is not None:
                        inferred[k] = type(v).__name__
                        
        # Optionally, PyArrow schemas could be returned directly if available
        # But a standardized dict is often easier for the metadata JSON
        return inferred
