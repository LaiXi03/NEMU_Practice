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
#include "sdb.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* create new watchpoint */
WP *new_wp(char *str) {
  // 检验是否有空闲的watchpoint
  if (free_ == NULL) {
    Assert(false, "No enough watchpoints");
  }
  if (strlen(str) < 1) {
    printf("Syntax error: expected expression\n");
    return NULL;
  }
  bool expr_status = true;
  uint32_t value = expr(str, &expr_status);
  if (!expr_status) {
    printf("Syntax error: invalid expression\n");
    return NULL;
  }
  // 从free_中取出一个watchpoint
  WP *wp = free_;
  free_ = free_->next;
  wp->expr = strdup(str);
  Assert(wp->expr != NULL, "strdup failed");
  wp->hit_times = 0;
  wp->new_value = value;
  wp->old_value = value;  // FIXME
  wp->next = NULL;
  // 将watchpoint插入到head中
  WP *p = head;
  if (p == NULL) {
    head = wp;
  } else {
    while (p->next != NULL) {
      p = p->next;
    }
    p->next = wp;
  }
  return wp;
}

void free_wp(WP *wp) {
  if (wp == NULL) return;
  if (wp->NO < 0 || wp->NO > NR_WP || head == NULL) {
    printf("Invalid watchpoint\n");
    return;
  }
  // 判断是否为head
  if (head->NO == wp->NO) {
    WP *tmp = head;
    free(tmp->expr);
    head = head->next;
    tmp->next = free_;
    free_ = tmp;
    return;
  }
  WP *p = head; // p指向wp的前一个
  while (p->next != NULL) {
    if (p->next->NO == wp->NO) {
      WP *tmp = p->next;
      free(tmp->expr);
      p->next = p->next->next;
      tmp->next = free_;
      free_ = tmp;
      return;
    }
    p = p->next;
  }
  printf("Invalid watchpoint\n");
  return;
}

/* show the information of watchpoints */
void show_wp() {
  WP *p = head;
  int wp_cnt = 0;
  printf("No\tExpr\tHit Times\tValue\n");
  while (p != NULL) {
    wp_cnt++;
    printf("%d\t%s\t%d\t\t%u\n", p->NO, p->expr, p->hit_times, p->new_value);
    p = p->next;
  }
  printf("--------------------------------\n");
  printf("Total watchpoints: %d\n", wp_cnt);
}

/* scan the wp in head and update the expr value */
void scan_wp(bool *flag) {
  WP *p = head;
  while (p != NULL) {
    bool expr_status = true;
    uint32_t value = expr(p->expr, &expr_status);
    Assert(expr_status, "get new expr value failed"); // FIXME
    p->old_value = p->new_value;
    p->new_value = value;
    if (p->new_value != p->old_value) {
      *flag = true;
      p->hit_times++;
      // 输出监视点信息
      printf("Stopped at watchpoint: %s\nOld value = %u\nNew value = %u\n", p->expr, p->old_value, p->new_value);
      // TODO: 添加堆栈信息
    }
    p = p->next;
  }
}