#ifndef INCLUDE_STACK_H
#define INCLUDE_STACK_H

typedef struct Stack {
    void** data;
    unsigned int count;
    unsigned int capacity;
} Stack;

void stack_init(Stack* stack);
void push(Stack* stack, void* val);
void* pop(Stack* stack);
void* peek(Stack* stack);
void stack_destroy(Stack* stack);

#endif