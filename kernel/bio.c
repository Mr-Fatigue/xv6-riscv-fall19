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

#define NBUCKETS 13



struct {
  struct buf buf[4*NBUF];
  struct buf hashbucket[NBUCKETS];//替代原先的head，作为每一个双向链表的头
  struct spinlock lock[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0 ; i < NBUCKETS ; i++){
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
    initlock(&bcache.lock[i],"bcache");
  }

  // Create linked list of buffers
  
  for(int i = 0; i < 4*NBUF; i++){
    b = &bcache.buf[i];
    b->next = bcache.hashbucket[i%NBUCKETS].next;
    b->prev = &bcache.hashbucket[i%NBUCKETS];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[i%NBUCKETS].next->prev = b;
    bcache.hashbucket[i%NBUCKETS].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  acquire(&bcache.lock[blockno%NBUCKETS]);
  // Is the block already cached?
  for(b = bcache.hashbucket[blockno%NBUCKETS].next; b != &bcache.hashbucket[blockno%NBUCKETS]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[blockno%NBUCKETS]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached; recycle an unused buffer.
  for(b = bcache.hashbucket[blockno%NBUCKETS].prev; b != &bcache.hashbucket[blockno%NBUCKETS]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[blockno%NBUCKETS]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[blockno%NBUCKETS]);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
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
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  releasesleep(&b->lock);
  acquire(&bcache.lock[b->blockno%NBUCKETS]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[b->blockno%NBUCKETS].next;
    b->prev = &bcache.hashbucket[b->blockno%NBUCKETS];
    bcache.hashbucket[b->blockno%NBUCKETS].next->prev = b;
    bcache.hashbucket[b->blockno%NBUCKETS].next = b;
  }
  release(&bcache.lock[b->blockno%NBUCKETS]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock[b->blockno%NBUCKETS]);
  b->refcnt++;
  release(&bcache.lock[b->blockno%NBUCKETS]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock[b->blockno%NBUCKETS]);
  b->refcnt--;
  release(&bcache.lock[b->blockno%NBUCKETS]);
}


