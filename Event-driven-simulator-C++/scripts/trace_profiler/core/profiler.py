from pathlib import Path
import json
from trace_profiler.core.registry import DatasetRegistry
from trace_profiler.core.discovery import FileAnalyzer
from trace_profiler.core.schema import SchemaInferencer
from trace_profiler.core.quality import QualityChecker
from trace_profiler.core.statistics import TraceStatistics
from trace_profiler.core.semantic_mapper import SemanticMapper
from trace_profiler.core.benchmark import Benchmark
from trace_profiler.core.manifest import ManifestGenerator
from trace_profiler.core.optimizer import Optimizer
from trace_profiler.config import DATASET_DIR, CACHE_DIR, PROFILER_DIR

class TraceProfiler:
    def __init__(self):
        self.registry = DatasetRegistry(CACHE_DIR / "dataset_index.json")
        self.analyzer = FileAnalyzer(self.registry)
        self.reports_dir = PROFILER_DIR / "output_reports"

    def run(self):
        print("Starting Trace Profiler...")
        # 1. Discovery
        print("Running Discovery...")
        datasets = self.analyzer.discover_datasets(DATASET_DIR)
        
        for descriptor in datasets:
            print(f"\nProcessing {descriptor.name}...")
            bench = Benchmark()
            bench.start()
            
            # 2. Schema Inference
            if not descriptor.schema:
                print("  Inferring schema...")
                inferencer = SchemaInferencer(descriptor)
                descriptor.schema = inferencer.infer_schema()
                self.registry.save()

            # 3. Quality Check
            if not descriptor.profile:
                print("  Running quality checks...")
                qc = QualityChecker(descriptor)
                descriptor.profile = qc.generate_report()
                self.registry.save()

            # 4. Statistics
            if not descriptor.statistics:
                print("  Computing statistics...")
                stat = TraceStatistics(descriptor)
                descriptor.statistics = stat.compute()
                self.registry.save()

            # 5. Semantic Mapping
            print("  Semantic Mapping...")
            mapper = SemanticMapper(descriptor.schema, {})
            mapping = mapper.map_fields()

            bench.stop()
            res = bench.get_results()
            print(f"  Processed in {res['duration_seconds']:.2f}s, Peak RAM: {res['peak_memory_mb']:.2f}MB")

            # Manifest Generation
            ManifestGenerator.generate(
                dataset_name=descriptor.name,
                sha256=descriptor.sha256,
                runtime=res['duration_seconds'],
                peak_ram_mb=res['peak_memory_mb'],
                reports=["schema", "quality", "statistics", "mapping"],
                output_dir=self.reports_dir / descriptor.name
            )

        print("\nProfiling Complete.")
