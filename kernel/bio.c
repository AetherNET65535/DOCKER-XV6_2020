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

#define HASHI(x) (x & (NBUC - 1))

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;

  // hash bucket
  struct buf buc[NBUC];
  struct spinlock buc_lock[NBUC];
} bcache;

void
binit(void)
{
  struct buf *b;
  int i;

  initlock(&bcache.lock, "bcache");
  for(i = 0; i < NBUC; i++){
    initlock(&bcache.buc_lock[i], "bcache.buc");

    bcache.buc[i].next = &bcache.buc[i];
    bcache.buc[i].prev = &bcache.buc[i];
  }

  // Create hash table of buffers
  // Head insert
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int x = HASHI(b->blockno);

    b->next = bcache.buc[x].next;
    b->prev = &bcache.buc[x];

    bcache.buc[x].next->prev = b;
    bcache.buc[x].next = b;

    b->ticks = ticks;

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
  int x = HASHI(blockno);

  // Is the block already cached?
  acquire(&bcache.buc_lock[x]);
  for(b = bcache.buc[x].next; b != &bcache.buc[x]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buc_lock[x]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached for now
  release(&bcache.buc_lock[x]);

  acquire(&bcache.lock);

  // Check again
  // Is the block already cached?
  acquire(&bcache.buc_lock[x]);
  for(b = bcache.buc[x].next; b != &bcache.buc[x]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buc_lock[x]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached
  release(&bcache.buc_lock[x]);

  // Init for LRU
  struct buf *minb = 0;
  uint mticks = ~0;

  // Recycle the LRU unused buffer
  for(int i = 0; i < NBUC; i++){
    acquire(&bcache.buc_lock[i]);
    int find = 0;
    
    // Find lastest ununsed buffer
    for(b = bcache.buc[i].next; b != &bcache.buc[i]; b = b->next){
      if(b->refcnt == 0 && b->ticks < mticks){

        if(minb != 0){
          int last = HASHI(minb->blockno);
          if(last != i)
            release(&bcache.buc_lock[last]);

        }

        mticks = b->ticks;
        minb = b;
        find = 1;

      }
    }
    // No buf in this bucket
    if(!find)
      release(&bcache.buc_lock[i]);
  }

  // No buf in ALL bucket
  if(minb == 0)
    panic("bget: no buffers");

  int minb_x = HASHI(minb->blockno);

  minb->dev = dev;
  minb->blockno = blockno;
  minb->valid = 0;
  minb->refcnt = 1;

  if(minb_x != x){
    minb->prev->next = minb->next;
    minb->next->prev = minb->prev;
  }
  release(&bcache.buc_lock[minb_x]);

  if(minb_x != x){
    acquire(&bcache.buc_lock[x]);

    minb->next = bcache.buc[x].next;
    minb->prev = &bcache.buc[x];
    bcache.buc[x].next->prev = minb;
    bcache.buc[x].next = minb;

    release(&bcache.buc_lock[x]);
  }
  
  release(&bcache.lock);
  acquiresleep(&minb->lock);
  return minb;
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

  int x = HASHI(b->blockno);
  acquire(&bcache.buc_lock[x]);
  b->refcnt--;
  if (b->refcnt == 0) 
    b->ticks = ticks;
  release(&bcache.buc_lock[x]);
}

void
bpin(struct buf *b) {
  int x = HASHI(b->blockno);

  acquire(&bcache.buc_lock[x]);
  b->refcnt++;
  release(&bcache.buc_lock[x]);
}

void
bunpin(struct buf *b) {
  int x = HASHI(b->blockno);

  acquire(&bcache.buc_lock[x]);
  b->refcnt--;
  release(&bcache.buc_lock[x]);
}


