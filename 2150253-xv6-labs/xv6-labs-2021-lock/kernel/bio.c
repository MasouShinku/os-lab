// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

extern uint ticks;

// cache lab8
// 定义一下桶结构
struct {
  struct spinlock lock;
  struct buf head;
} bbuc[NBUC];


struct {
  struct spinlock lock;
  struct buf buf[NBUF];  
} bcache;


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for(int i =0;i<NBUC;i++){
    // 初始化桶
    initlock(&bbuc[i].lock,"bbuc");
    bbuc[i].head.prev=&bbuc[i].head;
    bbuc[i].head.next=&bbuc[i].head;
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->tick=-1;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int tempBuc = blockno%NBUC;
  acquire(&bbuc[tempBuc].lock);
  
  // 是否cached
  for(b = bbuc[tempBuc].head.next; b != &bbuc[tempBuc].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->tick = ticks;
      b->refcnt++;
      release(&bbuc[tempBuc].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 否则根据lru原则选择桶存放

  // 首先在本桶内查找
  struct buf *tempBuf=  NULL; 
  for(b = bbuc[tempBuc].head.next; b != &bbuc[tempBuc].head; b = b->next){
    if(b->refcnt ==0){
      if(tempBuf==NULL||tempBuf->tick>b->tick){
        tempBuf=b;
      }

    }
  }
  if(tempBuf!=NULL){
    tempBuf->dev = dev;
    tempBuf->blockno = blockno;
    tempBuf->valid = 0;
    tempBuf->refcnt = 1;
    tempBuf->tick = ticks;
    release(&bbuc[tempBuc].lock);
    acquiresleep(&tempBuf->lock);
    return tempBuf;
  }

  // 否则在所有缓冲区查找 
  int targetBuc;
  int flag = 1;
  while (flag){
    flag = 0;
    tempBuf = 0;
    for (b = bcache.buf; b < bcache.buf+NBUF; b++){
      if (b->refcnt == 0){ 
        flag = 1;
        if(tempBuf==NULL||tempBuf->tick>b->tick){
        tempBuf=b;
        }
      }
    }
    if (tempBuf!=NULL){
      if (tempBuf->tick == -1){
        acquire(&bcache.lock);
        if (tempBuf->refcnt == 0){
          // 正好没有引用
          tempBuf->dev = dev;
	        tempBuf->blockno = blockno;
          tempBuf->valid = 0;
          tempBuf->refcnt = 1;
          tempBuf->tick = ticks;

          // 加入桶队列
          tempBuf->next = bbuc[tempBuc].head.next;
          tempBuf->prev = &bbuc[tempBuc].head;
          bbuc[tempBuc].head.next->prev = tempBuf;
          bbuc[tempBuc].head.next = tempBuf;

          release(&bcache.lock);
          release(&bbuc[tempBuc].lock);
          acquiresleep(&tempBuf->lock);
          return tempBuf;
        } else { 
          // 有引用
          release(&bcache.lock);
        }
      } else { 
        targetBuc = tempBuf->blockno%NBUC;
        acquire(&bbuc[targetBuc].lock);
        if (tempBuf->refcnt == 0){ 
          tempBuf->dev = dev;
          tempBuf->blockno = blockno;
          tempBuf->valid = 0;
          tempBuf->refcnt = 1;
          tempBuf->tick = ticks;

	        tempBuf->next->prev = tempBuf->prev;
          tempBuf->prev->next = tempBuf->next;
	        tempBuf->next = bbuc[tempBuc].head.next;
          tempBuf->prev = &bbuc[tempBuc].head;
	        bbuc[tempBuc].head.next->prev = tempBuf;
	        bbuc[tempBuc].head.next = tempBuf;

	        release(&bbuc[targetBuc].lock);
	        release(&bbuc[tempBuc].lock);
          acquiresleep(&tempBuf->lock);
	        return tempBuf;
        } else { 
	        release(&bbuc[targetBuc].lock);
        }
      }
    }
  }

  panic("bget: no buffers");
}




// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int tempBuc=b->blockno%NBUC;

  acquire(&bbuc[tempBuc].lock);
  b->refcnt--;
  release(&bbuc[tempBuc].lock);

}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


