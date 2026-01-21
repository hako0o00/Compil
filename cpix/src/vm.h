#ifndef VM_H
#define VM_H

void vm_init(void);
int  vm_run_block(const char* label); /* runs quads after LABEL until next LABEL or end */
void vm_shutdown(void);

#endif
