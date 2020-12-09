'''k4atyps.py

Defines Python ctypes equivalent structures to those defined in k4atypes.h. 

Credit given to hexops's github contribution for the
ctypes.Structure definitions and ctypes function bindings.
https://github.com/hexops/Azure-Kinect-Python
'''

# Add top-level k4a directory to path.
import sys
import os

k4a_top_level = os.path.dirname(os.path.dirname(__file__))
sys.path.insert(0, k4a_top_level)

import ctypes
import struct
    
import k4a

