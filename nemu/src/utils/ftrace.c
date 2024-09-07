#include "debug.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <trace.h>
#include <elf.h>

#define F_NAME_LENGTH  128
#define F_SYM_NUM      1024

typedef struct {
    char name[F_NAME_LENGTH];
    vaddr_t address;
    uint32_t size;
} ftrace_symbol_t;

static ftrace_symbol_t symbol_arr[F_SYM_NUM];
static uint32_t func_symbol_cnt = 0;
static uint32_t call_deep = 0;

static void parse_elf(const char *elf_file, ftrace_symbol_t *symbols) {
    Log("elf file path: %s", elf_file);
    Assert(strlen(elf_file) > 1 && symbol_arr != NULL, "can not parse elf file");
    FILE *fp = fopen(elf_file, "rb");
    Assert(fp != NULL, "can not open '%s'", elf_file);

    // 读取elf header
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    Assert(ehdr != NULL, "malloc failed");
    Assert(fread(ehdr, sizeof(Elf32_Ehdr), 1, fp), "fread failed");
    Assert(ehdr->e_ident[EI_MAG0] == ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] == ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] == ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] == ELFMAG3 , "Not an elf file");

    // 读取section header
    Elf32_Shdr *shdrs = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr) * ehdr->e_shnum);
    Assert(shdrs != NULL, "malloc failed");
    fseek(fp, ehdr->e_shoff, SEEK_SET);
    Assert(fread(shdrs, ehdr->e_shentsize, ehdr->e_shnum, fp), "fread failed");

    // 读取symtab段
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB || shdrs[i].sh_type == SHT_DYNSYM) {
            // 读取symtab并定义相关变量
            Elf32_Shdr sh_symtab = shdrs[i];
            Elf32_Sym *symtab = (Elf32_Sym *)malloc(sh_symtab.sh_size);
            fseek(fp, sh_symtab.sh_offset, SEEK_SET);
            Assert(fread(symtab, 1, sh_symtab.sh_size, fp), "fread symtab failed");
            int sym_num = sh_symtab.sh_size / sizeof(Elf32_Sym);

            // 读取对应的strtab段
            Elf32_Shdr *strtab = shdrs + sh_symtab.sh_link;
            char *sym_strtab = (char *)malloc(strtab->sh_size);
            fseek(fp, strtab->sh_offset, SEEK_SET);
            Assert(fread(sym_strtab, 1, strtab->sh_size, fp), "fread strtab failed");
            
            // 记录type为func的符号
            for (int j = 0; j < sym_num; j++) {
                if (ELF32_ST_TYPE(symtab[j].st_info) == STT_FUNC) {
                    ftrace_symbol_t *symbol = &symbols[func_symbol_cnt];
                    symbol->address = symtab[j].st_value;
                    symbol->size = symtab[j].st_size;
                    strcpy(symbol->name, sym_strtab + symtab[j].st_name);
                    Log("symbol[%d] func_name: %s addr: 0x%08x size: %d", func_symbol_cnt, symbol->name, symbol->address, symbol->size);
                    func_symbol_cnt++;
                }
            }
            free(sym_strtab);
            free(symtab);
            break;
        }
    }
    
    free(shdrs);
    free(ehdr);
    fclose(fp);
}

void init_ftrace(const char *elf_file) {
    parse_elf(elf_file, symbol_arr);
    Log("func_symbol_cnt = %d", func_symbol_cnt);
}

void ftrace_call(vaddr_t pc, vaddr_t target) {
    // Log("call pc = 0x%08x, target = 0x%08x", pc, target);
    for (int i = 0; i < func_symbol_cnt; i++) {
        if (target == symbol_arr[i].address) {
            // Log("call pc = 0x%08x, target = 0x%08x", pc, target);
            printf("0x%08x", symbol_arr[i].address);
            for (int j = 0; j < call_deep; j++) {
                printf("  ");
            }
            printf("call [%s@0x%08x]\n", symbol_arr[i].name, symbol_arr[i].address);
            call_deep++;
            break;
        }
    }
}

void ftrace_ret(vaddr_t pc) {
    // Log("ret pc = 0x%08x", pc);
    for (int i = 0; i < func_symbol_cnt; i++) {
        if (pc >= symbol_arr[i].address && pc < symbol_arr[i].address + symbol_arr[i].size) {
            // Log("ret pc = 0x%08x", pc);
            call_deep--;
            printf("0x%08x", pc);
            for (int j = 0; j < call_deep; j++) {
                printf("  ");
            }
            printf("ret  [%s]\n", symbol_arr[i].name);
            break;
        }
    }
}