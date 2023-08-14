// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];
// lock lab8
// 修改内存结构为数组



void
kinit()
{
  // lock lab8
  for (int i=0;i<NCPU;i++){
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}


void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // lock lab8
  push_off(); // 关中断
  int tempCPU=cpuid();
  acquire(&kmem[tempCPU].lock);
  r->next = kmem[tempCPU].freelist;
  kmem[tempCPU].freelist = r;
  release(&kmem[tempCPU].lock);
  pop_off();  // 开中断
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  // lock lab8
  push_off();
  int tempCPU=cpuid();
  acquire(&kmem[tempCPU].lock);

  r = kmem[tempCPU].freelist;
  if(r){
    kmem[tempCPU].freelist = r->next;
  }else{
    // 空闲空间不足，抢夺其他cpu空间
    int id;
    for(id=0;id<NCPU;id++){
      // 跳过当前cpu编号
      if(id==tempCPU)continue;
      acquire(&kmem[id].lock);
      r = kmem[id].freelist;
      if(r) {
        kmem[id].freelist = r->next;
        release(&kmem[id].lock);
        break;
      }
      release(&kmem[id].lock);

    }
  }
  release(&kmem[tempCPU].lock);
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
