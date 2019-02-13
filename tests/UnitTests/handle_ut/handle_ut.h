#include <utcommon.h>

// Module being tested
#include <k4ainternal/handle.h>

K4A_DECLARE_HANDLE(dual_defined_t);

typedef struct
{
    int my;
    int data;
} dual_defined_context_t;

// Declare a handle for the context
K4A_DECLARE_CONTEXT(dual_defined_t, dual_defined_context_t);

// Shared function prototype
int is_handle_in_2nd_file_valid(dual_defined_t handle);
