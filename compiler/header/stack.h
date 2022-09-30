#ifndef INCLUDE_STACK_H
#define INCLUDE_STACK_H

#define STACK_CAPACITY_STEP 512

typedef struct Stack {
    unsigned int bsize;
    unsigned int bcapacity;
    void* bdata;
} Stack;

void init_stack(Stack* stack);

void pushchr(Stack* stack, char val);
void pushsht(Stack* stack, short val);
void pushint(Stack* stack, int val);
void pushlng(Stack* stack, long val);
void pushllg(Stack* stack, long long val);
void pushptr(Stack* stack, void* ptr);

char peekchr(Stack* stack);
short peeksht(Stack* stack);
int peekint(Stack* stack);
long peeklng(Stack* stack);
long long peekllg(Stack* stack);
void* peekptr(Stack* stack);

char popchr(Stack* stack);
short popsht(Stack* stack);
int popint(Stack* stack);
long poplng(Stack* stack);
long long popllg(Stack* stack);
void* popptr(Stack* stack);

void destroy_stack(Stack* stack);
void* stack_adopt_array(Stack* stack);

#endif