/* Unit tests for spx_call_stack */
#include "test_framework.h"
#include "../../src/spx_call_stack.h"
#include "../../src/spx_limits.h"
#include <string.h>

TEST(call_stack_create_destroy) {
    spx_limits_init();
    spx_call_stack_t *stack = spx_call_stack_create(100);
    ASSERT_NOT_NULL(stack);
    spx_call_stack_destroy(stack);
}

TEST(call_stack_push_pop) {
    spx_limits_init();
    spx_call_stack_t *stack = spx_call_stack_create(100);
    ASSERT_NOT_NULL(stack);

    spx_call_stack_frame_t frame1 = {"function1", "Class1", 10};
    int result = spx_call_stack_push(stack, &frame1);
    ASSERT_EQ(result, 0);

    size_t depth = spx_call_stack_depth(stack);
    ASSERT_EQ(depth, 1);

    const spx_call_stack_frame_t *top = spx_call_stack_top(stack);
    ASSERT_NOT_NULL(top);
    ASSERT_STR_EQ(top->function, "function1");

    spx_call_stack_pop(stack);
    depth = spx_call_stack_depth(stack);
    ASSERT_EQ(depth, 0);

    spx_call_stack_destroy(stack);
}

TEST(call_stack_multiple_frames) {
    spx_limits_init();
    spx_call_stack_t *stack = spx_call_stack_create(100);
    ASSERT_NOT_NULL(stack);

    spx_call_stack_frame_t f1 = {"func1", "Class1", 10};
    spx_call_stack_frame_t f2 = {"func2", "Class2", 20};
    spx_call_stack_frame_t f3 = {"func3", "Class3", 30};

    spx_call_stack_push(stack, &f1);
    spx_call_stack_push(stack, &f2);
    spx_call_stack_push(stack, &f3);

    ASSERT_EQ(spx_call_stack_depth(stack), 3);

    const spx_call_stack_frame_t *top = spx_call_stack_top(stack);
    ASSERT_STR_EQ(top->function, "func3");

    spx_call_stack_pop(stack);
    top = spx_call_stack_top(stack);
    ASSERT_STR_EQ(top->function, "func2");

    spx_call_stack_destroy(stack);
}

TEST(call_stack_capacity) {
    spx_limits_init();
    spx_call_stack_t *stack = spx_call_stack_create(3);
    ASSERT_NOT_NULL(stack);

    spx_call_stack_frame_t f = {"func", "Class", 10};

    ASSERT_EQ(spx_call_stack_push(stack, &f), 0);
    ASSERT_EQ(spx_call_stack_push(stack, &f), 0);
    ASSERT_EQ(spx_call_stack_push(stack, &f), 0);

    /* Stack is full, next push should fail */
    int result = spx_call_stack_push(stack, &f);
    ASSERT_NE(result, 0);

    spx_call_stack_destroy(stack);
}

TEST(call_stack_empty) {
    spx_limits_init();
    spx_call_stack_t *stack = spx_call_stack_create(100);
    ASSERT_NOT_NULL(stack);

    ASSERT_EQ(spx_call_stack_depth(stack), 0);

    const spx_call_stack_frame_t *top = spx_call_stack_top(stack);
    ASSERT_NULL(top);

    /* Popping empty stack should not crash */
    spx_call_stack_pop(stack);

    spx_call_stack_destroy(stack);
}

TEST(call_stack_at) {
    spx_limits_init();
    spx_call_stack_t *stack = spx_call_stack_create(100);
    ASSERT_NOT_NULL(stack);

    spx_call_stack_frame_t f1 = {"func1", "Class1", 10};
    spx_call_stack_frame_t f2 = {"func2", "Class2", 20};
    spx_call_stack_frame_t f3 = {"func3", "Class3", 30};

    spx_call_stack_push(stack, &f1);
    spx_call_stack_push(stack, &f2);
    spx_call_stack_push(stack, &f3);

    const spx_call_stack_frame_t *frame;

    frame = spx_call_stack_at(stack, 0);
    ASSERT_NOT_NULL(frame);
    ASSERT_STR_EQ(frame->function, "func1");

    frame = spx_call_stack_at(stack, 1);
    ASSERT_NOT_NULL(frame);
    ASSERT_STR_EQ(frame->function, "func2");

    frame = spx_call_stack_at(stack, 2);
    ASSERT_NOT_NULL(frame);
    ASSERT_STR_EQ(frame->function, "func3");

    frame = spx_call_stack_at(stack, 3);
    ASSERT_NULL(frame);

    spx_call_stack_destroy(stack);
}

BEGIN_TEST_SUITE(SPX_Call_Stack)
    RUN_TEST(call_stack_create_destroy);
    RUN_TEST(call_stack_push_pop);
    RUN_TEST(call_stack_multiple_frames);
    RUN_TEST(call_stack_capacity);
    RUN_TEST(call_stack_empty);
    RUN_TEST(call_stack_at);
END_TEST_SUITE()
