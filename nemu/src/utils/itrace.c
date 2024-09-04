#include <trace.h>
#include <stdint.h>

#define MAX_IRINGBUF 20

void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

typedef struct {
    word_t pc;
    uint32_t inst;
} ItraceNode;

static ItraceNode iringbuf[MAX_IRINGBUF];
static int p_cur = 0;
static int p_head = 0, p_tail = 0;

void write_iringbuf(word_t pc, uint32_t inst) {
    iringbuf[p_cur].inst = inst;
    iringbuf[p_cur].pc = pc;
    p_head = ((p_cur + 1) % MAX_IRINGBUF >= p_head) ? ((p_cur + 1) % MAX_IRINGBUF + 1) % MAX_IRINGBUF : p_head;
    p_cur = (p_cur + 1) % MAX_IRINGBUF;
    p_tail = p_cur;

}

void show_iringbuf() {
    if (p_head == p_tail) return;
    int i = p_head;
    char buf[128];
    char *p = NULL;
    printf("Most recently executed instructions:\n");
    while(i != p_tail) {
        p = buf;
        p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i+1)%MAX_IRINGBUF==p_tail?" --> ":"     ", iringbuf[i].pc, iringbuf[i].inst);
        disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);
        puts(buf);
        i = (i + 1) % MAX_IRINGBUF;
    }
}