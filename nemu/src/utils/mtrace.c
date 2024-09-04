#include <trace.h>
#include <memory/paddr.h>

void mtrace_read(paddr_t addr, int len) {
    printf("mtrace_read:  addr = %08x, len = %d\n", addr, len);
}

void mtrace_write(paddr_t addr, int len, word_t data) {
    printf("mtrace_write: addr = %08x, len = %d, data = " FMT_WORD "\n",addr, len, data);
}