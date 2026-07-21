import os
import hashlib
from pathlib import Path

def compute_sha256_fast(file_path: Path, chunk_size: int = 8192 * 1024, max_bytes: int = 100 * 1024 * 1024) -> str:
    """
    Computes SHA256 of the file. For extremely large files, to save time,
    we can just hash the first `max_bytes`. If max_bytes is 0, hash the whole file.
    """
    sha256_hash = hashlib.sha256()
    bytes_read = 0
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(chunk_size), b""):
            sha256_hash.update(byte_block)
            bytes_read += len(byte_block)
            if max_bytes > 0 and bytes_read >= max_bytes:
                break
    return sha256_hash.hexdigest()
