#ifndef __TRACE_H__
#define __TRACE_H__

#include <common.h>

#ifdef CONFIG_ITRACE
void write_iringbuf(word_t pc, uint32_t inst);
void show_iringbuf();
#endif

#ifdef CONFIG_MTRACE
void mtrace_read(paddr_t addr, int len);
void mtrace_write(paddr_t addr, int len, word_t data);
#endif

#ifdef CONFIG_FTRACE
void init_ftrace(const char *elf_file);
void ftrace_call(vaddr_t pc, vaddr_t target);
void ftrace_ret(vaddr_t pc);
#endif
#endif