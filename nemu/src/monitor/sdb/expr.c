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
#include "memory/vaddr.h"
#include "sdb.h"

enum {
  TK_NOTYPE = 256, TK_REG, TK_NUM = 101,
  TK_LP = 102, TK_RP = 103, 
  TK_LINE = 104,

  BOOL_NOT = 10, BYTE_NOT = 11, TK_NEGA = 12, DEREF = 13, // 单目运算符
  TK_MUL = 20, TK_DIV = 21,
  TK_PLUS = 30, TK_MINUS = 31, 
  BYTE_LM = 40, BYTE_RM = 41,
  RE_GT = 50, RE_LT = 51, RE_GE = 52, RE_LE =53,
  RE_EQ = 61, RE_NEQ = 62, 
  BYTE_AND = 70, 
  BYTE_OR = 80, 
  BOOL_AND = 90, 
  BOOL_OR = 100,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  {" +", TK_NOTYPE},    // spaces  [Done]
  {"\\(", TK_LP},         // left bracket  [Done]
  {"\\)", TK_RP},         // right bracket  [Done]

  {"!", BOOL_NOT},  // 逻辑非
  {"~", BYTE_NOT},  // 按位取反

  {"\\*", TK_MUL},         // 乘 or 指针解引用 
  {"/", TK_DIV},           // divide  [Done]

  {"\\+", TK_PLUS},         // plus  [Done]
  {"-", TK_MINUS},          // minus  [Done]

  {">>", BYTE_RM},      // 右移
  {"<<", BYTE_LM},      // 左移

  {">=", RE_GE},        // greater or equal
  {">", RE_GT},         // greater 
  {"<=", RE_LE},        // less or equal
  {"<", RE_LT},         // less

  {"==", RE_EQ},        // equal 
  {"!=", RE_NEQ},       // not equal

  {"&&", BOOL_AND},     // 逻辑与 
  {"\\|\\|", BOOL_OR},  // 逻辑或

  {"&", BYTE_AND},      // 按位与
  {"\\|", BYTE_OR},     // 按位或

  {"(((0x|0X)[0-9,a-f,A-F]+)|([0-9]+(u|U)?))", TK_NUM},   // number  [DOne]
  {"\\$(\\$0|ra|sp|gp|tp|t[0-2]|s[0-1]|a[0-7]|s[2-9]|s1[01]|t[3-6])", TK_REG},  // register [test]
  {" *\n *", TK_LINE}  // new line [Done]
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

        switch (rules[i].token_type) {
          case TK_NUM: 
            // FIXME: handle the number that is tAssert(substr_len < 32, "The length of the number is too long");
            Assert(substr_len < 32, "The length of the number is too long");
          case TK_REG:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // Ensure null-termination
            
            case TK_PLUS: case TK_MINUS: case TK_MUL: case TK_DIV: case TK_LP: case TK_RP: 
            case BYTE_LM: case BYTE_RM:
            case RE_GE: case RE_GT: case RE_LE: case RE_LT:
            case RE_EQ: case RE_NEQ:             
            case BYTE_AND: case BYTE_OR:
            case BOOL_AND: case BOOL_OR:
            case BOOL_NOT: case BYTE_NOT:
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
  return true;
}

/* test if the expression is syntax right*/
static bool test_expr(char *e) {
  // TODO: 添加更多校验规则
  int cnt = 0;
  for (int i = 0; i < nr_token; i++) {
    switch (tokens[i].type) {
      case TK_LP:
        cnt++;
        break;
      case TK_RP:
        cnt--;
        break;
      default:
        break;
    }
    if (cnt < 0) {
      return false;
    }
  }
  return true;
}

static int check_parentness(int p, int q) {
  int cnt = 0;
  for (int i = p; i <= q; i++) {
    switch (tokens[i].type) {
      case TK_LP:
        cnt++;
        break;
      case TK_RP:
        cnt--;
        break;
      default:
        if (cnt == 0 && tokens[i].type >= 10 && tokens[i].type <= 100) {
          return false;
        }
    }
    if (cnt < 0) {
      return false;
    }
  }
  Assert(cnt == 0, "Invaild brackets.");
  return true;
}

static int dominant_op(int p, int q) {
  int op_pos = -1, op_type = -1, bracket_cnt = 0;
  for (int i = p; i <= q; i++) {  
    switch (tokens[i].type) {
      case TK_NUM: case TK_REG: case TK_NEGA: case DEREF:
      case BOOL_NOT: case BYTE_NOT:
        break;
      case TK_LP:
        bracket_cnt++;
        break;
      case TK_RP:
        bracket_cnt--;
        break;
      case TK_MINUS:case TK_MUL:
      case TK_PLUS: case TK_DIV: 
      case BYTE_LM: case BYTE_RM:
      case RE_GE: case RE_GT: case RE_LE: case RE_LT:
      case RE_EQ: case RE_NEQ:             
      case BYTE_AND: case BYTE_OR:
      case BOOL_AND: case BOOL_OR:
        if (bracket_cnt != 0) break; // 跳过括号中的运算符
        if (op_type == -1) {
          op_pos = i;
          op_type = tokens[i].type;
          break;
        }
        if (tokens[i].type == op_type || (tokens[i].type / 10 % 10) == op_type / 10 % 10
              || ((tokens[i].type / 10 % 10) != op_type / 10 % 10 && (tokens[i].type / 10 % 10) > op_type / 10 % 10)) {
          op_pos = i;
          op_type = tokens[i].type;
        }
        break;
      default:
        Assert(0, "Unknown error");
    }
  }
  if (op_pos == -1 && tokens[p].type <= DEREF) {
    op_pos = p;
  }
  return op_pos;
}

static uint32_t eval(int p, int q, bool *success) {
  // FIXME: 处理表达式非法的情况；错误消息或错误状态返回逻辑
  //        1. 括号不匹配 [x]
  //        2. 运算符不合法： 除号、乘号前面没有数字， 连续多个运算符 6 * / + 5, (* 5 + 6)
  //        3. 数字不合法： 0x 0X
  if (p > q) {
    // 非法表达式
    Assert(false, "p > q");
    *success = false;
    return 0;
  } else if (p == q) {
    // single token
    *success = true;
    uint32_t num = 0;
    if (tokens[p].type == TK_NUM) {
      // 数字
      if (strlen(tokens[p].str) > 2 && (tokens[p].str[1] == 'x' || tokens[p].str[1] == 'X')) 
        sscanf(tokens[p].str, "%x", &num);
      else
        sscanf(tokens[p].str, "%u", &num);
    } else if (tokens[p].type == TK_REG) {
      // 寄存器
      bool reg_value = true;
      num = isa_reg_str2val(tokens[p].str, &reg_value);
      Assert(reg_value, "Unknown error when get reg value.");
    }
    return num;
  } else {
    switch (check_parentness(p, q)) {
      case true:
        // remove the outermost brackets
        return eval(p + 1, q - 1, success);
      case false: {
        int main_op = dominant_op(p, q);  // find the dominant operator
        if (main_op == p) {
          // 单目运算符
          bool val_status = true;
          uint32_t val = eval(p + 1, q, &val_status);
          if (!val_status) {
            *success = false;
            return 0;
          }
          Assert(*success, "Unknown error when eval single operation.");
          switch (tokens[p].type) {
            case BOOL_NOT:
              return !val;
            case BYTE_NOT:
              return ~val;
            case TK_NEGA:
              return -val;
            case DEREF:
              {
                // 指针解引用
                uint32_t addr, mem_data;
                addr = eval(p + 1, q, success);
                if (!*success) {
                  Log("Unexpected value when calculate the address.");
                  return 0;
                }
                mem_data = vaddr_read(addr, 4);
                return mem_data;
              }
            default:
              Assert(false, "Unexpected single operation type.");
          }
        } else {
          bool val1_status = true, val2_status = true;
          uint32_t val1 = eval(p, main_op - 1, &val1_status);
          uint32_t val2 = eval(main_op + 1, q, &val2_status);
          // Log("main op pos: %d, main op index: %d\nval1: %u, val2: %u\n", main_op, tokens[main_op].type, val1, val2);
          Assert(val1_status && val2_status, "Unknown error when eval double operation.");
          // Log("main_op pos: %d; val1: %u; val2: %u\n", main_op, val1, val2);
          if (!val1_status || !val2_status) {
            *success = false;
            return 0;
          }
          switch (tokens[main_op].type) {
            /* + - * / */
            case TK_PLUS:
              return val1 + val2;
            case TK_MINUS:
              return val1 - val2;
            case TK_MUL:
              return val1 * val2;
            case TK_DIV:
              return val1 / val2;
            // << >>
            case BYTE_LM:
              return val1 << val2;
            case BYTE_RM:
              return val1 >> val2;
            /* == != >= > <= < */
            case RE_EQ:
              return val1 == val2;
            case RE_NEQ:
              return val1 != val2;
            case RE_GE:
              return val1 >= val2;
            case RE_GT:
              return val1 > val2;
            case RE_LE:
              return val1 <= val2;
            case RE_LT:
              return val1 < val2;
            // & |
            case BYTE_AND:
              return val1 & val2;
            case BYTE_OR:
              return val1 | val2;
            /* && || */
            case BOOL_AND:
              return val1 && val2;
            case BOOL_OR:
              return val1 || val2;
            default:
              Assert(false, "Unexpected main operation type.");
          }
        }
      }
      default:
        Assert(0, "Unknown error");
    }
  }
}

word_t expr(char *e, bool *success) {
  nr_token = 0;
  if (!make_token(e) || !test_expr(e)) {
    *success = false;
    return 0;
  }

  // 标记负号与解引用
  for (int i = 0; i < nr_token; i++) {
    if (i == 0 || tokens[i - 1].type == TK_LP || (tokens[i - 1].type >= 10 && tokens[i - 1].type <= 100)) { 
      switch (tokens[i].type) {
        case TK_MINUS:
          tokens[i].type = TK_NEGA;
          break;
        case TK_MUL:
          tokens[i].type = DEREF;
          break;
        default:
          break;
      }
    }
  }

  int p = 0, q = nr_token - 1;
  return eval(p, q, success);
}
