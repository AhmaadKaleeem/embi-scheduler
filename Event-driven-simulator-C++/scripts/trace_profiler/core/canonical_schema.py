import pyarrow as pa

# Define the canonical trace record abstract schema using PyArrow
CanonicalTraceRecordSchema = pa.schema([
    pa.field('timestamp', pa.int64(), nullable=True),
    pa.field('job_id', pa.string(), nullable=True),
    pa.field('task_id', pa.string(), nullable=True),
    pa.field('machine_id', pa.string(), nullable=True),
    pa.field('event_type', pa.string(), nullable=True),
    pa.field('cpu_usage', pa.float64(), nullable=True),
    pa.field('memory_usage', pa.float64(), nullable=True),
    pa.field('status', pa.string(), nullable=True),
    pa.field('priority', pa.int32(), nullable=True),
])

class CanonicalSchema:
    @staticmethod
    def get_arrow_schema() -> pa.Schema:
        return CanonicalTraceRecordSchema
