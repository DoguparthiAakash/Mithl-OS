#ifndef _SETJMP_H
#define _SETJMP_H

// Simple buffer
typedef struct {
    unsigned int regs[6];
} jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif
