#include<stack.h>
#include<stdlib.h>
#include<mmemory.h>

#define ceil_div(x, y) ((x + y - 1) / y)
#define ceil(x, ceil) (ceil_div(x, ceil) * ceil)

void init_stack(Stack* stack) {
    stack->bcapacity = STACK_CAPACITY_STEP;
    stack->bsize = 0;
    stack->bdata = mmalloc(stack->bcapacity);
}

static inline void mrealloc_on_full(Stack* stack, int push_s) {
    if (ceil_div(stack->bsize, push_s) * push_s < stack->bcapacity) {
        stack->bcapacity += STACK_CAPACITY_STEP;
        stack->bdata = mrealloc(stack->bdata, stack->bcapacity);
    }
}

#define implement_push(type) {\
    mrealloc_on_full(stack, sizeof(type));\
    ((type*) stack->bdata)[ceil_div(stack->bsize, sizeof(type))] = val;\
    stack->bsize += sizeof(type);\
}

void push_chr(Stack* stack, char val) implement_push(char)
void push_sht(Stack* stack, short val) implement_push(short)
void push_int(Stack* stack, int val) implement_push(int)
void push_lng(Stack* stack, long val) implement_push(long)
void push_llg(Stack* stack, long long val) implement_push(long long)
void push_ptr(Stack* stack, void* val) implement_push(void*)

#define implement_peek(type) {\
    if (stack->bsize < sizeof(type)) return (type) 0;\
    return ((type*) stack->bdata)[ceil_div(stack->bsize, sizeof(type)) - 1];\
}

char peek_chr(Stack* stack) implement_peek(char)
short peek_sht(Stack* stack) implement_peek(short)
int peek_int(Stack* stack) implement_peek(int)
long peek_lng(Stack* stack) implement_peek(long)
long long peek_llg(Stack* stack) implement_peek(long long)
void* peek_ptr(Stack* stack) implement_peek(void*)

#define implement_pop(type) {\
    if (stack->bsize < sizeof(type)) return (type) 0;\
    stack->bsize = ceil(stack->bsize, sizeof(type));\
    stack->bsize -= sizeof(type);\
    return ((type*) stack->bdata)[ceil_div(stack->bsize, sizeof(type))];\
}

char pop_chr(Stack* stack) implement_pop(char)
short pop_sht(Stack* stack) implement_pop(short)
int pop_int(Stack* stack) implement_pop(int)
long pop_lng(Stack* stack) implement_pop(long)
long long pop_llg(Stack* stack) implement_pop(long long)
void* pop_ptr(Stack* stack) implement_pop(void*)

void destroy_stack(Stack* stack) {
    stack->bsize = 0;
    stack->bcapacity = 0;
    if (stack->bdata) mfree(stack->bdata);
    stack->bdata = NULL;
}

void* stack_disown(Stack* stack) {
    void* to_ret = stack->bdata;
    to_ret = mrealloc(to_ret, stack->bsize);
    stack->bsize = 0;
    stack->bcapacity = 0;
    stack->bdata = NULL;
    return to_ret;
}