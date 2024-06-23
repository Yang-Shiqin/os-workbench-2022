#include <common.h>

// spinlock_t lock = spin_init("Big Kernel Lock");

// 空闲内存链表（改成数组记录不同大小的内存链表，然后里面是类似大小的内存链表？）
typedef struct FreeNode{
  uintptr_t pmsize;
  struct FreeNode * next;
  // TODO: 锁(不知道有没有现成的，还是得自己实现)
}FreeNode;

static void *kalloc(size_t size) {
  // 全局锁方案
  // spin_lock(&lock);
  // // 申请内存
  // spin_unlock(&lock);
  // 申请当前节点的锁
  // 检查大小是否够(要从特定位置开始读), 够则分裂, 剩下更新到空闲(前面的整个申请, 但返回整的地址; 还是前面空闲的也加到链表中)
  // 不够申请下一节点锁, 更新完才释放当前节点锁
  return NULL;
}

static void kfree(void *ptr) {
  // 合并连续空闲
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
