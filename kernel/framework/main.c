#include <kernel.h>
#include <klib.h>
typedef struct Header { 
	size_t size; // Block size, 不加头
	void* magic; // canary, 0xfdfdfdfd为正常
} Header;

void stress_test() {
  void *ptr = pmm->alloc(66);
  Header* h = ptr-sizeof(Header);
  printf("%d %x %p\n", h->size, ((int*)(h->magic))[1], ptr);
  ptr = pmm->alloc(12996);
  h = ptr-sizeof(Header);
  printf("%d %x %p\n", h->size, ((int*)(h->magic))[1], ptr);
  ptr = pmm->alloc(62326);
  h = ptr-sizeof(Header);
  printf("%d %x %p\n", h->size, ((int*)(h->magic))[1], ptr);
  ptr = pmm->alloc(6126);
  h = ptr-sizeof(Header);
  printf("%d %x %p\n", h->size, ((int*)(h->magic))[1], ptr);
}

int main() {
  os->init();
  mpe_init(stress_test);
  return 1;
}
