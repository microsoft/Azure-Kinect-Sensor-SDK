from k4a._bindings._k4a import *

# We want "import k4a" to import all symbols in k4atypes.py under k4a.<x>.
from k4a.k4atypes import *

# If k4atypes is not deleted, it shows up in dir(k4a). We want to hide
# k4atypes and only present its contents to k4a.
del k4atypes