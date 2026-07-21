from typing import Dict, Any, Tuple
import re

class SemanticMapper:
    def __init__(self, inferred_schema: Dict[str, str], sample_data: Dict[str, Any]):
        self.schema = inferred_schema
        self.sample = sample_data

    def map_fields(self) -> Dict[str, Dict[str, Any]]:
        """
        Maps fields to canonical names with confidence scores.
        """
        mapping = {}
        
        for col_name, dtype in self.schema.items():
            name_lower = col_name.lower()
            
            # Timestamp mapping
            if "time" in name_lower or "ts" in name_lower:
                confidence = 0.8
                reason = "Column name contains time/ts"
                if dtype in ["int", "int64", "float", "float64"]:
                    confidence += 0.15
                    reason += " and is numeric"
                mapping[col_name] = {"canonical": "timestamp", "confidence": confidence, "reason": reason}
            
            # CPU mapping
            elif "cpu" in name_lower or "core" in name_lower:
                mapping[col_name] = {"canonical": "cpu_usage", "confidence": 0.9, "reason": "Name implies CPU"}
                
            # Memory mapping
            elif "mem" in name_lower or "ram" in name_lower:
                mapping[col_name] = {"canonical": "memory_usage", "confidence": 0.9, "reason": "Name implies Memory"}
                
            # ID mapping
            elif "id" in name_lower:
                if "job" in name_lower:
                    mapping[col_name] = {"canonical": "job_id", "confidence": 0.9, "reason": "Name implies Job ID"}
                elif "task" in name_lower or "instance" in name_lower:
                    mapping[col_name] = {"canonical": "task_id", "confidence": 0.8, "reason": "Name implies Task/Instance ID"}
                elif "machine" in name_lower or "node" in name_lower:
                    mapping[col_name] = {"canonical": "machine_id", "confidence": 0.9, "reason": "Name implies Machine ID"}
                    
        return mapping
