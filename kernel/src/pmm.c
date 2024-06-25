#include <common.h>

#define MAX_ORDER 28           // 800最大是28
// 空闲链表节点
typedef struct FreeNode { 
	size_t size; // Block size, 不加头
	struct FreeNode *next; // Next free block 
} FreeNode;


// 头块(大小和节点一样, 方便相互转换), 大小为8字节
typedef struct Header { 
	size_t size; // Block size, 不加头
	void* magic; // canary, 0xfdfdfdfd为正常
} Header;

// 伙伴系统分配器数组
typedef struct Buddy { 
	unsigned int index; // 加头
	//  SDL_mutex* lock;
	struct FreeNode *head; // Next free block 
} Buddy;

static void *kalloc(size_t size) {
  printf("call kalloc\n");
  size_t total_size = size+sizeof(Header);
  size_t head=0, tail=0, i=0;
  FreeNode *mid = NULL;
  FreeNode *free_block = NULL;
  Header *res = NULL;
  Buddy *free_area = (Buddy*)heap.start;
  while((1<<head)<total_size) head++;
  for (tail=head; tail<MAX_ORDER; tail++){
    // tail上锁
    if(free_area[tail].head==NULL){
      // tail下锁
      continue;
    }
    free_block = free_area[tail].head;
    free_area[tail].head = free_area[tail].head->next;
    // tail释放锁
    for(i=tail-1; i>=head; i--){
      // 分裂
      mid = free_block+(1<<i);
      mid->size = (1<<i)-sizeof(FreeNode);
      mid->next = NULL;
      // tail-1上锁
      free_area[i].head = mid;
      // tail-1下锁
    }
    res = (Header*)free_block;
    res->size = (1<<(i+1))-sizeof(Header);
    memset((res->magic), 0xfd, sizeof(void*));
    return (void*)((uintptr_t)res+sizeof(Header));
  }
  return NULL;    // 内存不够
}

static void kfree(void *ptr) {
  // 放对应大小的链表里，遍历，有相邻的合并
  // Buddy *free_area = (Buddy*)heap.start;
  // size_t index = 0;
  // Header *mem_to_free = (uintptr_t)ptr-sizeof(Header);
  // void *magic = NULL;
  // memset(&magic, 0xfd, sizeof(void*));
  // assert(memcmp(&magic, )==0);
  // FreeNode *free_block = (FreeNode*)mem_to_free;  // 待释放的内存
  // FreeNode *pnode = NULL;  // 要合并的空闲内存
  // size_t total_size = free_block->size+sizeof(Header);
  // while((1<<index)<total_size) index++;
  // assert((1<<index)==total_size);
  // // while升级没失败
  // // 申请index的锁
  // ptr ^ (1<<index);
  // pnode = free_area[index].head;

  // while(pnode){
  //   pnode = pnode->next;
  // }
  // Header *res = NULL;
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
  offset_ptr = (char*)heap.start+(1<<19)-sizeof(FreeNode);
  header = (FreeNode*)offset_ptr;
  header->size = (1<<19)-sizeof(FreeNode);
  header->next = NULL;
  free_area[19].head = header;
  for (i=22; i<27; i++){ // 应该是算出来的
    offset_ptr = (char*)((uintptr_t)(1<<i)-sizeof(FreeNode));
    header = (FreeNode*)offset_ptr;
    header->size = (1<<i)-sizeof(FreeNode);
    header->next = NULL;
    free_area[i].head = header;
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
