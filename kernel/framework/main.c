#include <kernel.h>
#include <klib.h>
typedef struct Header { 
	size_t size; // Block size, 不加头
	void* magic; // canary, 0xfdfdfdfd为正常
} Header;

void stress_test() {
  void *ptr = pmm->alloc(32);
  Header* h = ptr-sizeof(Header);
  printf("%d %x %d\n", h->size, ((int*)(h->magic))[0], sizeof(void*));
}

int main() {
  os->init();
  mpe_init(stress_test);
  return 1;
}
