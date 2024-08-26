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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define REMAINING_SPACE(buf) (sizeof(buf) - strlen(buf) - 1)
#define MAX_EXPR_LEN 50000
// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
static int bracket_cnt = 0;
enum uint8_t {
  GENERATION_TYPE_NUM = 0,
  GENERATION_TYPE_OP = 1,
  GENERATION_TYPE_BRACKET = 2,
};

static void gen_space(uint8_t pre_generation_type) {
  int num = rand() % 10;
  char str[10] = "         ";
  str[num] = '\0';
  switch (pre_generation_type) {
    case GENERATION_TYPE_NUM:
    case GENERATION_TYPE_BRACKET:
      if (REMAINING_SPACE(buf) < bracket_cnt + 1) return; // 空间不足
      strncat(buf, str, REMAINING_SPACE(buf) - bracket_cnt);
      break;
    case GENERATION_TYPE_OP:
      if (REMAINING_SPACE(buf) < bracket_cnt + 2) return; // 空间不足
      strncat(buf, str, REMAINING_SPACE(buf) - bracket_cnt - 1);
      break;
  }
}

static inline void gen(char *str) {
  assert(REMAINING_SPACE(buf) > strlen(str));
  if (strcmp(str, "(") == 0) {
    if (REMAINING_SPACE(buf) < bracket_cnt + 3) return; // 空间不足
    bracket_cnt ++;
    strncat(buf, "(", 1);
  } else {
    if (bracket_cnt < 1) return;  // 括号不匹配
    bracket_cnt --;
    strncat(buf, ")", 1);
  }
  gen_space(GENERATION_TYPE_BRACKET);
}

static void gen_num() {
  uint32_t num = (uint32_t) rand();
  char str[12];
  str[0] = '\0';
  sprintf(str, "%uu", num);
  assert(REMAINING_SPACE(buf) > bracket_cnt + 1);
  strncat(buf, str, REMAINING_SPACE(buf) - bracket_cnt);
  gen_space(GENERATION_TYPE_NUM);
}

static void gen_rand_op() {
  if (REMAINING_SPACE(buf) < bracket_cnt + 2) return; // 空间不足
  switch (rand() % 4) {
    case 0: 
      strncat(buf, "+", 1);
      break;
    case 1: 
      strncat(buf, "-", 1);
      break;
    case 2:       
      strncat(buf, "*", 1);
      break;
    case 3:       
      strncat(buf, "/", 1);
      break;
  }
  gen_space(GENERATION_TYPE_OP);
}

static void gen_rand_expr() {
  // buf[0] = '\0';
  if (REMAINING_SPACE(buf) < MAX_EXPR_LEN) {
    gen_num();
    return;
  }
  if (REMAINING_SPACE(buf) < bracket_cnt + 3) return; // 空间不足
  switch (rand() % 3) {
    case 0: gen_num(); break;
    case 1:
      gen("(");
      gen_rand_expr();
      gen(")");
      break;
    default:
      gen_rand_expr();
      gen_rand_op();
      gen_rand_expr();
      break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf[0] = '\0';
    bracket_cnt = 0;
    gen_rand_expr();

    // printf("expr = %s\n", buf);
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr -Werror 2> /dev/null");
    if (ret != 0) {
      i--;
      continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    uint32_t result;
    ret = fscanf(fp, "%u", &result);
    // 过滤除0的情况
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
