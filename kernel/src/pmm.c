#include <common.h>

#define MIN(a, b) ((a)<(b)?(a):(b))
#define GET_HEADER(p) ((void*)((uintptr_t)(p) - sizeof(Header)))
#define REMOVE_HEADER(p) ((void*)((uintptr_t)(p) + sizeof(Header)))

#define MAX_ORDER 28           // 800最大是28
// 空闲链表节点
typedef struct FreeNode { 
	size_t size; // Block size, 不加头
	struct FreeNode *next; // Next free block 
} FreeNode;

int a;

// 头块(大小和节点一样, 方便相互转换)
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
  size_t head=0, tail=0, i=0;   // index
  FreeNode *mid = NULL;
  FreeNode *free_block = NULL;
  printf("%d\n", a);
  Header *res = NULL;           // 返回的分配内存的头块
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
    memset(&(res->magic), 0xfd, sizeof(void*));
    return REMOVE_HEADER(res);
  }
  return NULL;    // 内存不够
}

static void kfree(void *ptr) {
  // 放对应大小的链表里，遍历，有相邻的合并
  Buddy *free_area = (Buddy*)heap.start;
  size_t index = 0;
  Header *mem_to_free = GET_HEADER(ptr);
  void *magic = NULL;
  memset(&magic, 0xfd, sizeof(void*));
  assert(memcmp(&magic, &(mem_to_free->magic), sizeof(void*))==0); // magic被人改了就完了
  FreeNode *free_block = (FreeNode*)mem_to_free;    // 待释放的内存
  FreeNode *pnode = NULL;   // 
  FreeNode *prev = NULL;
  FreeNode *friend = NULL;  // 伙伴, 即要合并的空闲内存
  size_t total_size = free_block->size+sizeof(Header);
  while((1<<index)<total_size) index++;
  assert((1<<index)==total_size);   // 要是2的指数次

  int flag=1;  // 升级没失败
  while (flag){
    // 申请index的锁
    friend = (FreeNode*)((uintptr_t)ptr ^ (uintptr_t)(1<<index));   // 寻找伙伴
    pnode = free_area[index].head;
    prev = NULL;
    while(pnode){
      if (REMOVE_HEADER(pnode)==friend){
        if (prev==NULL){
          free_area[index].head = pnode->next;
        }else{
          prev->next = pnode->next;
        }
        // 释放index锁
        ptr = (void*)MIN((uintptr_t)ptr, (uintptr_t)friend);
        free_block = GET_HEADER(ptr);
        index++;
        free_block->size = (1<<index)-sizeof(Header);
        break;
      }
      prev = pnode;
      pnode = pnode->next;
    }
    flag = 0;
  }
  // 申请index锁
  free_block->next = free_area[index].head;
  free_area[index].head = free_block;
  // 释放index锁
}

static void pmm_init() {
  assert(sizeof(Header)==sizeof(FreeNode));
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
