import os.path
import sys
import ctypes


# Load the k4a.dll.
lib_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), '_libs')
try:
    _k4a_dll = ctypes.CDLL(os.path.join(lib_dir, 'k4a.dll'))
except Exception as e:
    try:
        _k4a_dll = ctypes.CDLL(os.path.join(lib_dir, 'k4a.so'))
    except Exception as ee:
        print("Failed to load library", e, ee)
        sys.exit(1)

# Define symbols that will be exported with "from .bindings import *".
__all__ = [
]