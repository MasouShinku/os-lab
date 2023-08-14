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
  int pageNum;
  char *refPage;
  char *cEnd;
} kmem;


// cow lab5
// 计算引用数
int 
pageCountCal(void *pa_start,void *pa_end)
{
  char *p;
  int cnt=0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    cnt++;
  }
  return cnt;
    
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // cow lab5
  kmem.pageNum=pageCountCal(end, (void*)PHYSTOP);
  kmem.refPage=end;
  for(int i =0;i<kmem.pageNum;i++){
    kmem.refPage[i]=0;
  }
  kmem.cEnd=kmem.refPage+kmem.pageNum;

  freerange(kmem.cEnd, (void*)PHYSTOP);
}

// 获得索引序号
int
getIndex(void *pa)
{
  uint64 upa=(uint64)pa;
  upa=PGROUNDDOWN(upa);

  int res=(upa-(uint64)kmem.cEnd)/PGSIZE;
  if(res<0||res>=kmem.pageNum){
    panic("index illegal");
  }
  return res;
}

//增加引用
void 
refIncr(void* pa)
{
  int index=getIndex(pa);

  acquire(&kmem.lock);
  kmem.refPage[index]++;
  release(&kmem.lock);
}

//减少引用
void 
refDecr(void* pa)
{
  int index=getIndex(pa);

  acquire(&kmem.lock);
  kmem.refPage[index]--;
  release(&kmem.lock);
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
  int index=getIndex(pa);
  if(kmem.refPage[index]>=1){
    refDecr(pa);
    if(kmem.refPage[index]>0)
      return;
  }

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  //cow lab5

  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    refIncr(r);
  }
  
  return (void*)r;
}
