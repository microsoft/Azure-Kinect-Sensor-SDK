from ._bindings import *

# We want "import k4a" to import all symbols in k4atypes.py under k4a.<x>.
from .k4atypes import *

del _bindings
del k4atypes