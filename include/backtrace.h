
#ifndef BACKTRACE_H
#define BACKTRACE_H

/* use this to set SIGSEGV handler if you want */
int bt_init(int depth);
void bt_print_call_stack(void);

#endif
