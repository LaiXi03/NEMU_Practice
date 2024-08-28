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

#include "debug.h"
#include <common.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "monitor/sdb/sdb.h"

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif 

  /* test the expr function */
  // read test data from ../tools/gen-expr/input
  FILE *fp = fopen("./tools/gen-expr/input", "r");
  Assert(fp != NULL, "Can not open input file\n");
  char line_str[65536];
  line_str[0] = '\0';
  uint32_t result = 0;
  int passed_cnt = 0;
  while (fgets(line_str, 65536, fp) != NULL) {
    bool eval_status = true;
    char *result_str = strtok(line_str, " ");
    Assert(result_str != NULL, "Test data format error\n");
    sscanf(result_str, "%u", &result);
    char *expr_str = line_str + strlen(result_str) + 1;
    Assert(expr_str != NULL, "Test data format error\n");
    uint32_t res = expr(expr_str, &eval_status);

    if (res != result) {
      Log("Test failed: expect %u, but got %u.", result, res);
      Log("The expression: %s\n", expr_str);
      expr_str[0] = '\0';
      continue;
    } 
    Assert(eval_status, "eval_status uncorrect.\n");
    passed_cnt++;
    expr_str[0] = '\0';
  }
  Log("passed cnt: %d\n", passed_cnt);
  int ret = fclose(fp);
  Assert(ret == 0, "Close file failed\n");
  /* Start engine. */
  engine_start();

  free_regex();

  return is_exit_status_bad();
}
