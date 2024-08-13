/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si", "Step one instruction exactly", cmd_si},
  {"info", "Print program status.\n\tinfo r\t\tprint registers' info\n\tinfo w\t\tprint watchpoints' info", cmd_info},
  {"x", "Scan memory", cmd_x},
  {"p", "Print value of expression", cmd_p},
  {"w", "Set watchpoint", cmd_w},
  {"d", "Delete watchpoint", cmd_d},
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    // exec one command
    cpu_exec(1);
  } else {
    for(int i = 0; i < strlen(arg); i++) {
      if(arg[i] < '0' || arg[i] > '9') {
        // syntax error: invalid argument
        printf("Invalid argument '%s'\n", arg);
        return 0;
      }
    }
    // transform the string to int
    int n = atoi(arg);
    // examine the syntax
    if((arg = strtok(NULL, " ")) != NULL) {
      // syntax error: too many arguments
      printf("A syntax error in expression: too many arguments\n");
    } else {
      // exec n commands
      cpu_exec(n);
    }
  }
  return 0;
}

static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    // syntax error: no argument
    printf("%s - %s\n", cmd_table[4].name, cmd_table[4].description);
  } else if (strcmp(arg, "r") == 0) {
    // print the registers
    isa_reg_display();
  } else if (strcmp(arg, "w") == 0) {
    // print the watchpoints
  }
  return 0;
}

static int cmd_x(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    // syntax error: no argument
    printf("%s - %s\n", cmd_table[5].name, cmd_table[5].description);
  } else {
    // test the first argument
    for(int i = 0; i < strlen(arg); i++) {
      if(arg[i] < '0' || arg[i] > '9') {
        // syntax error: invalid argument
        printf("Invalid argument '%s'\n", arg);
        return 0;
      }
    }
    // transform the string to int
    word_t n = strtoul(arg, NULL, 10);
    // test the second argument
    // the second argument should be an expression
    // assume that the format is 0x????????
    char *exp = strtok(NULL, " ");
      // transform the expression to a number
      // bool success = true;
      // word_t addr = expr(exp, &success);
    word_t mem_data = 0, addr = 0;
    sscanf(exp, "0x%x", &addr);
    for(int i = 0; i < n; i++) {
      // print the memory
      mem_data = paddr_read(addr, 4);
      printf("0x%.8x: 0x%.8x\n", addr, mem_data);
      addr += 4;
    }
  }
  return 0;
}

static int cmd_p(char *args) {
  /* extract the first argument */
  // char *arg = strtok(NULL, " ");
  return 0;
}

static int cmd_w(char *args) {
  /* extract the first argument */
  // char *arg = strtok(NULL, " ");
  return 0;
}

static int cmd_d(char *args) {
  /* extract the first argument */
  // char *arg = strtok(NULL, " ");
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
