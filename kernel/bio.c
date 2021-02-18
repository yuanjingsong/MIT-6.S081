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

int hash(uint num) {
  return num % NBUCKETS;
}

struct {
  struct spinlock locks[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < NBUCKETS; i++) {
    initlock(&bcache.locks[i], "bcache");
    bcache.heads[i].prev = &bcache.heads[i];
    bcache.heads[i].next = &bcache.heads[i];
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.heads[0].next;
    b->prev = &bcache.heads[0];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[0].next->prev = b;
    bcache.heads[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketId = hash(blockno);

  acquire(&bcache.locks[bucketId]);

  // Is the block already cached?
  for(b = bcache.heads[bucketId].next; b != &bcache.heads[bucketId]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[bucketId]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (int j = hash(blockno+1); j != bucketId; j = hash(j+1)) {
    acquire(&bcache.locks[j]);
    for (b = bcache.heads[j].prev; b != &bcache.heads[j]; b = b->prev) {
      if (b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        // Cut off the target buffer
        b->next->prev = b->prev;
        b->prev->next = b->next;

        // Insert buffer
        b->next = bcache.heads[bucketId].next;
        b->prev = &bcache.heads[bucketId];
        bcache.heads[bucketId].next->prev = b;
        bcache.heads[bucketId].next = b;

        release(&bcache.locks[j]);
        release(&bcache.locks[bucketId]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.locks[j]);
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

  int bucketId = hash(b->blockno);
  acquire(&bcache.locks[bucketId]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[bucketId].next;
    b->prev = &bcache.heads[bucketId];
    bcache.heads[bucketId].next->prev = b;
    bcache.heads[bucketId].next = b;
  }
  
  release(&bcache.locks[bucketId]);
}

void
bpin(struct buf *b) {
  int bucketId = hash(b->blockno);

  acquire(&bcache.locks[bucketId]);
  b->refcnt++;
  release(&bcache.locks[bucketId]);
}

void
bunpin(struct buf *b) {
  int bucketId = hash(b->blockno);
  acquire(&bcache.locks[bucketId]);
  b->refcnt--;
  release(&bcache.locks[bucketId]);
}


