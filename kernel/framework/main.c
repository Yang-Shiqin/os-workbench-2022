#include <kernel.h>
#include <klib.h>
typedef struct Header { 
	size_t size; // Block size, 不加头
	int magic; // canary, 0xfdfdfdfd为正常
} Header;

void stress_test() {
                void *ptr = pmm->alloc(30);
                Header* h = ptr-sizeof(Header);
                printf("%d %d\n", h->size, h->magic);

    // while (1) {
    //     // 根据 workload 生成操作
    //     struct malloc_op op = random_op();

    //     switch (op.type) {
    //         case OP_ALLOC: {
    //             void *ptr = pmm->alloc(op.sz);
    //             alloc_check(ptr, op.sz);
    //             break;
    //         }
    //         case OP_FREE:
    //             free(op.addr);
    //             break;
    //     }
    // }
}

int main() {
  os->init();
  mpe_init(stress_test);
  return 1;
}
