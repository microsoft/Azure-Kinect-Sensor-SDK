// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include "handle_ut.h"

K4A_DECLARE_HANDLE(foo_t);

class cpp_class_for_ctor_dtor_test
{
public:
    cpp_class_for_ctor_dtor_test()
    {
        m_ctor = 1;
        m_dtor = 0;
    };
    ~cpp_class_for_ctor_dtor_test()
    {
        m_dtor = 1;
    };

    static int m_ctor;
    static int m_dtor;
};

typedef struct
{
    int my;
    int data;
} context_t;

typedef struct
{
    int my;
    int data;
    cpp_class_for_ctor_dtor_test ctor_dtor_obj;
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

int cpp_class_for_ctor_dtor_test::m_ctor = 0;
int cpp_class_for_ctor_dtor_test::m_dtor = 0;

TEST(handle_ut, create_free_for_cpp)
{
    bar_t bar = NULL;
    context2_t *context = bar_t_create(&bar);
    context->ctor_dtor_obj.m_dtor = 0;

    EXPECT_NE((context2_t *)NULL, context);
    EXPECT_NE((bar_t)NULL, bar);
    EXPECT_NE(cpp_class_for_ctor_dtor_test::m_ctor, 0);
    EXPECT_EQ(cpp_class_for_ctor_dtor_test::m_dtor, 0);

    // delete bar;
    bar_t_destroy(bar);

    EXPECT_NE(cpp_class_for_ctor_dtor_test::m_dtor, 0);
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

TEST(handle_ut, K4A_DECLARE_CONTEXT_in_shared_header)
{
    dual_defined_t dual = NULL;
    dual_defined_context_t *context = dual_defined_t_create(&dual);

    EXPECT_NE((dual_defined_context_t *)NULL, context);
    EXPECT_NE((dual_defined_t)NULL, dual);

    EXPECT_EQ(context, dual_defined_t_get_context(dual));
    EXPECT_NE(0, is_handle_in_2nd_file_valid(dual));

    EXPECT_EQ(0, is_handle_in_c_file_valid(dual));

    dual_defined_t_destroy(dual);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
