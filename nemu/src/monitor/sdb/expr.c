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

#include "common.h"
#include "debug.h"
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sdb.h"

enum {
  TK_NOTYPE = 256, TK_EQ, TK_LINE,
  TK_PLUS = '+', TK_MINUS = '-', TK_MUL = '*', TK_DIV = '/', TK_LP = '(', TK_RP = ')', TK_NUM = '0'
  /* TODO: Add more token types */
  // *:42,+:43,-:45,/:47,(:40,):41
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"-", TK_MINUS},// minus
  {"\\*", TK_MUL},         // multiply
  {"/", TK_DIV},           // divide
  {"\\(", TK_LP},         // left bracket
  {"\\)", TK_RP},         // right bracket
  {"==", TK_EQ},        // equal
  {"(0x|0X|-)?[0-9]+(u|U)?", TK_NUM},   // number
  {" *\n *", TK_LINE}  // new line
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  // Log("strlen = %lu\n", strlen(e));
  // Log("e[strlen(e)] = '%c'", e[strlen(e) - 1]);
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            // i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NUM: 
            // TODO: handle the number that is tAssert(substr_len < 32, "The length of the number is too long");
            Assert(substr_len < 32, "The length of the number is too long");
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // Ensure null-termination
            
            case TK_PLUS: case TK_MINUS: case TK_MUL: case TK_DIV: case TK_LP: case TK_RP: 
              tokens[nr_token].type = rules[i].token_type;
              nr_token++;
              // Log("tokens[%d].type = %c, tokens[%d].str = %s\n", 
                // nr_token - 1, tokens[nr_token - 1].type, nr_token - 1, tokens[nr_token - 1].str);
              case TK_NOTYPE: case TK_LINE:
                break;

          default: Assert(false, "Unexpected token type.");
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }

  }
  // Log("nr_token(number of tokens) = %d\n", nr_token);
  return true;
}

static int check_parentness(int p, int q) {
  int cnt = 0;
  bool bracket_flag = true;
  for (int i = p; i < q; i++) {
    if (tokens[i].type == TK_LP) {
      cnt++;
    } else if (tokens[i].type == TK_RP) {
      cnt--;
    } else if (cnt == 0 && (tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS || tokens[i].type == TK_MUL || tokens[i].type == TK_DIV)) {
      bracket_flag = false;
    }
    if (cnt < 0) {
      return -1;
    }
  }
  bool right_format = (cnt - (tokens[q].type == TK_RP) == 0);
  return right_format ? bracket_flag : -1;
}

static int dominant_op(int p, int q) {
  // TODO: 处理负数
  int op_pos = -1, op_type = -1, bracket_cnt = 0;
  for (int i = p; i <= q; i++) {  // FIXME: 循环起点与条件
    switch (tokens[i].type) {
      case TK_NUM:
        continue;
      case TK_LP:
        bracket_cnt++;
        break;
      case TK_RP:
        bracket_cnt--;
        break;
      case TK_PLUS: case TK_MINUS: case TK_DIV: case TK_MUL:
        if (bracket_cnt != 0) break;
        switch (tokens[i].type) {
          case '*': case '/':
            if (op_type == '+' || op_type == '-') break;
          case '+': case '-':
            if (tokens[i].type == '-' && 
                  ( i == 0 || tokens[i - 1].type == TK_LP 
                    || tokens[i - 1].type == TK_PLUS || tokens[i - 1].type == TK_MINUS 
                    || tokens[i - 1].type == TK_MUL || tokens[i - 1].type == TK_DIV)) 
              { continue;; } // 处理负数
            op_pos = i;
            op_type = tokens[i].type;
            break;
          default:
            Assert(false, "Unexpected token type.");
        }
        break;
      default:
        Assert(0, "Unknown error");
    }
  }
  return op_pos;
}

static uint32_t eval(int p, int q, bool *success) {
  // TODO: 处理表达式非法的情况：
  //        1. 括号不匹配 [x]
  //        2. 运算符不合法： 除号、乘号前面没有数字， 连续多个运算符 6 * / + 5, (* 5 + 6)
  //        3. 数字不合法： 0x 0X
  if (p > q) {
    // 非法表达式
    *success = false;
    return 0;
  } else if (p == q) {
    // single token
    *success = true;
    uint32_t num = 0;
    sscanf(tokens[p].str, "%u", &num);
    return num;
  } else {
    if (tokens[p].type == '-') return -eval(p + 1, q, success); // 处理负数
    switch (check_parentness(p, q)) {
      case 1:
        // remove the outermost brackets
        return eval(p + 1, q - 1, success);
      case 0: {
        int main_op = dominant_op(p, q);  // find the dominant operator
        bool val1_status = true, val2_status = true;
        uint32_t val1 = eval(p, main_op - 1, &val1_status);
        uint32_t val2 = eval(main_op + 1, q, &val2_status);
        // Log("main_op pos: %d; val1: %u; val2: %u\n", main_op, val1, val2);
        if (!val1_status || !val2_status) {
          *success = false;
          return 0;
        }
        switch (tokens[main_op].type) {
          case TK_PLUS:
            return val1 + val2;
          case TK_MINUS:
            return val1 - val2;
          case TK_MUL:
            return val1 * val2;
          case TK_DIV:
            return val1 / val2;
          default:
            *success = false;
            return 0;
        }
      }
      case -1:
        // invalid expression
        *success = false;
        return 0;
      default:
        Assert(0, "Unknown error");
    }
  }
}

word_t expr(char *e, bool *success) {
  nr_token = 0;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  int p = 0, q = nr_token - 1;
  return eval(p, q, success);
}
