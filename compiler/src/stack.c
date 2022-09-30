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

void pushchr(Stack* stack, char val) implement_push(char)
void pushsht(Stack* stack, short val) implement_push(short)
void pushint(Stack* stack, int val) implement_push(int)
void pushlng(Stack* stack, long val) implement_push(long)
void pushllg(Stack* stack, long long val) implement_push(long long)
void pushptr(Stack* stack, void* val) implement_push(void*)

#define implement_peek(type) {\
    if (stack->bsize < sizeof(type)) return (type) 0;\
    return ((type*) stack->bdata)[ceil_div(stack->bsize, sizeof(type)) - 1];\
}

char peekchr(Stack* stack) implement_peek(char)
short peeksht(Stack* stack) implement_peek(short)
int peekint(Stack* stack) implement_peek(int)
long peeklng(Stack* stack) implement_peek(long)
long long peekllg(Stack* stack) implement_peek(long long)
void* peekptr(Stack* stack) implement_peek(void*)

#define implement_pop(type) {\
    if (stack->bsize < sizeof(type)) return (type) 0;\
    stack->bsize = ceil(stack->bsize, sizeof(type));\
    stack->bsize -= sizeof(type);\
    return ((type*) stack->bdata)[ceil_div(stack->bsize, sizeof(type))];\
}

char popchr(Stack* stack) implement_pop(char)
short popsht(Stack* stack) implement_pop(short)
int popint(Stack* stack) implement_pop(int)
long poplng(Stack* stack) implement_pop(long)
long long popllg(Stack* stack) implement_pop(long long)
void* popptr(Stack* stack) implement_pop(void*)

void destroy_stack(Stack* stack) {
    stack->bsize = 0;
    stack->bcapacity = 0;
    if (stack->bdata) mfree(stack->bdata);
    stack->bdata = NULL;
}

void* stack_adopt_array(Stack* stack) {
    void* to_ret = stack->bdata;
    to_ret = mrealloc(to_ret, stack->bsize);
    stack->bsize = 0;
    stack->bcapacity = 0;
    stack->bdata = NULL;
    return to_ret;
}