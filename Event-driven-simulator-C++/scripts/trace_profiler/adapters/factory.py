from trace_profiler.core.registry import DatasetDescriptor
from .base_adapter import ITraceAdapter
from .alibaba import AlibabaAdapter
from .azure import AzureAdapter
from .google import GoogleJSONAdapter
from .pyarrow_csv_adapter import PyArrowCSVAdapter

class AdapterFactory:
    @staticmethod
    def get_adapter(descriptor: DatasetDescriptor) -> ITraceAdapter:
        name_lower = descriptor.name.lower()
        if "alibaba" in name_lower:
            return AlibabaAdapter(descriptor)
        elif "azure" in name_lower:
            return AzureAdapter(descriptor)
        elif "google" in name_lower:
            return GoogleJSONAdapter(descriptor)
        else:
            # Fallback based on parser type
            if descriptor.parser == "pyarrow.csv":
                return PyArrowCSVAdapter(descriptor)
            else:
                raise NotImplementedError(f"No adapter available for {descriptor.name} with parser {descriptor.parser}")
