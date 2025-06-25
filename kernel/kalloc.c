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

#define index_rfc(pa) ((pa - KERNBASE) >> 12)

static uint16 pgrfc[(PHYSTOP - KERNBASE) / PGSIZE];
struct spinlock rfc_lock;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&rfc_lock, "pgrfc");
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

  acquire(&rfc_lock);
  uint16 rfc = get_pgrfc((uint64)pa);
  if(rfc > 1){
    add_pgrfc((uint64)pa, -1);
    release(&rfc_lock);
    return;
  }
  release(&rfc_lock);

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
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    printf("kalloc %d", pgrfc[index_rfc((uint64)r)]); 
    printf("\n");
    add_pgrfc((uint64)r, 1);
  }
  return (void*)r;
}

int
add_pgrfc(uint64 pa, int n)
{
  if((int)pgrfc[index_rfc(pa)] + n < 0 ||
     (int)pgrfc[index_rfc(pa)] + n > 65535){
    panic("set_pgrfc: too big or too small");
    return -1;
  }

  pgrfc[index_rfc(pa)] += n;
  return 0;
}

void
set_pgrfc(uint64 pa, int n){
  pgrfc[index_rfc(pa)] = n;
}

uint16
get_pgrfc(uint64 pa)
{
  return pgrfc[index_rfc(pa)];
}
