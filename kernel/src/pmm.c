#include <common.h>

#define MAX_ORDER 28           // 800最大是28
// 空闲链表节点
typedef struct FreeNode { 
	size_t size; // Block size 
	struct FreeNode *next; // Next free block 
} FreeNode;


// 头块(大小和节点一样, 方便相互转换), 大小为8字节
typedef struct Header { 
	size_t size; // Block size, 不加头
	int magic; // canary, 0xfdfdfdfd为正常
} Header;

// 伙伴系统分配器数组
typedef struct Buddy { 
	unsigned int index; // 加头
	//  SDL_mutex* lock;
	struct FreeNode *head; // Next free block 
} Buddy;

static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  Buddy *free_area = (Buddy*)heap.start;
  FreeNode *header = NULL;
  char* offset_ptr = NULL;
  int i;
  for (i=0; i<MAX_ORDER; i++){
    free_area[i].index = i;
    free_area[i].head = NULL;
  }
  offset_ptr = (char*)heap.start+(1<<19)-8;
  header = (FreeNode*)offset_ptr;
  header->size = (1<<19)-8;
  header->next = NULL;
  free_area[19].head = header;
  for (i=22; i<27; i++){ // 应该是算出来的
    offset_ptr = (char*)((uintptr_t)(1<<i)-8);
    header = (FreeNode*)offset_ptr;
    header->size = (1<<i)-8;
    header->next = NULL;
    free_area[i].head = header;
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
