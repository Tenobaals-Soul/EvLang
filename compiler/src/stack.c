#include<stack.h>
#include<stdlib.h>

#define STACK_ALLOC_STEP_SIZE 128

void stack_init(Stack* stack) {
    stack->capacity = 0;
    stack->count = 0;
    stack->data = NULL;
}

void push(Stack* stack, void* val) {
    if (stack->count >= stack->capacity) {
        stack->data = realloc(stack->data, (stack->capacity += STACK_ALLOC_STEP_SIZE));
    }
    stack->data[stack->count++] = val;
}


void* pop(Stack* stack) {
    if (stack->count == 0) return NULL;
    return stack->data[--stack->count];
}

void* peek(Stack* stack) {
    if (stack->count == 0) return NULL;
    return stack->data[stack->count - 1];
}

void stack_destroy(Stack* stack) {
    if (stack->data) free(stack->data);
}