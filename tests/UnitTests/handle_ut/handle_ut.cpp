#include <utcommon.h>

// Module being tested
#include <k4ainternal/handle.h>

K4A_DECLARE_HANDLE(foo_t);

typedef struct
{
    int my;
    int data;
} context_t;

typedef struct
{
    int my;
    int data;
} context2_t;

// Declare a handle for the context
K4A_DECLARE_CONTEXT(foo_t, context_t);

// Declare a second handle type for the same context
K4A_DECLARE_HANDLE(bar_t);
K4A_DECLARE_CONTEXT(bar_t, context2_t);

TEST(handle_ut, create_free)
{
    foo_t foo = NULL;
    context_t *context = foo_t_create(&foo);

    EXPECT_NE((context_t *)NULL, context);
    EXPECT_NE((foo_t)NULL, foo);

    foo_t_destroy(foo);
}

TEST(handle_ut, deref_correct)
{
    foo_t foo = NULL;
    context_t *pContext = foo_t_create(&foo);

    pContext = foo_t_get_context(foo);

    EXPECT_NE((context_t *)NULL, pContext);

    foo_t_destroy(foo);
}

TEST(handle_ut_deathtest, deref_null)
{
    EXPECT_EQ(NULL, foo_t_get_context(NULL));
}

TEST(handle_ut_deathtest, deref_incorrect)
{
    bar_t bar = NULL;
    (void)bar_t_create(&bar);

    EXPECT_EQ(NULL, foo_t_get_context((foo_t)bar));

    // Fixes unused function warning in clang
    (void)&bar_t_destroy;
}

TEST(handle_ut_deathtest, use_after_free)
{
    foo_t foo = NULL;
    context_t *pContext = foo_t_create(&foo);

    pContext = foo_t_get_context(foo);

    EXPECT_NE((context_t *)NULL, pContext);

    foo_t_destroy(foo);

    EXPECT_EQ(NULL, foo_t_get_context(foo));
}

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}
