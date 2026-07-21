import ijson
from typing import Iterator, Dict, Any, List
from .base_adapter import ITraceAdapter

class GoogleJSONAdapter(ITraceAdapter):
    def get_sample_chunks(self, head_rows: int, middle_rows: int, tail_rows: int) -> List[Dict[str, Any]]:
        # For JSON, we stream through and extract the first N objects
        sample = []
        try:
            with open(self.file_path, "rb") as f:
                # We assume the json is an array of objects, or just a stream of objects
                # Using ijson.items to extract objects
                parser = ijson.items(f, 'item')
                for i, obj in enumerate(parser):
                    if i >= head_rows:
                        break
                    sample.append(obj)
        except Exception as e:
            print(f"Error sampling JSON: {e}")
        return sample

    def iter_batches(self, batch_size: int = 100_000) -> Iterator[List[Dict[str, Any]]]:
        with open(self.file_path, "rb") as f:
            # We assume it's an array of items for Google JSON traces usually
            # Could also be line-delimited JSON, in which case we use ijson or jsonlines
            try:
                parser = ijson.items(f, 'item')
                batch = []
                for obj in parser:
                    batch.append(obj)
                    if len(batch) >= batch_size:
                        yield batch
                        batch = []
                if batch:
                    yield batch
            except Exception:
                # Fallback to line-delimited JSON
                f.seek(0)
                import json
                batch = []
                for line in f:
                    if line.strip():
                        try:
                            batch.append(json.loads(line))
                            if len(batch) >= batch_size:
                                yield batch
                                batch = []
                        except:
                            pass
                if batch:
                    yield batch
