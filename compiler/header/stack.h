#ifndef INCLUDE_STACK_H
#define INCLUDE_STACK_H

#define STACK_CAPACITY_STEP 512

typedef struct Stack {
    unsigned int bsize;
    unsigned int bcapacity;
    void* bdata;
} Stack;

void init_stack(Stack* stack);

void push_chr(Stack* stack, char val);
void push_sht(Stack* stack, short val);
void push_int(Stack* stack, int val);
void push_lng(Stack* stack, long val);
void push_llg(Stack* stack, long long val);
void push_ptr(Stack* stack, void* ptr);

char peek_chr(Stack* stack);
short peek_sht(Stack* stack);
int peek_int(Stack* stack);
long peek_lng(Stack* stack);
long long peek_llg(Stack* stack);
void* peek_ptr(Stack* stack);

char pop_chr(Stack* stack);
short pop_sht(Stack* stack);
int pop_int(Stack* stack);
long pop_lng(Stack* stack);
long long pop_llg(Stack* stack);
void* pop_ptr(Stack* stack);

void destroy_stack(Stack* stack);
void* stack_disown(Stack* stack);

#endif