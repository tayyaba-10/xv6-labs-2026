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

#define PA2INDEX(pa) (((uint64)(pa) - KERNBASE) / PGSIZE)
#define MAX_PAGES   ((PHYSTOP - KERNBASE) / PGSIZE)

struct {
  struct spinlock lock;
  int count[MAX_PAGES];
} ref_cnt;

void
refcnt_init(void)
{
  initlock(&ref_cnt.lock, "ref_cnt");
}

void
inc_ref(uint64 pa)
{
  if(pa < KERNBASE || pa >= PHYSTOP)
    return;
  acquire(&ref_cnt.lock);
  ref_cnt.count[PA2INDEX(pa)]++;
  release(&ref_cnt.lock);
}

int
get_ref(uint64 pa)
{
  if(pa < KERNBASE || pa >= PHYSTOP)
    return 0;
  acquire(&ref_cnt.lock);
  int c = ref_cnt.count[PA2INDEX(pa)];
  release(&ref_cnt.lock);
  return c;
}
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
  refcnt_init();
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

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&ref_cnt.lock);
  int idx = PA2INDEX(pa);
  if(ref_cnt.count[idx] > 1) {
    ref_cnt.count[idx]--;
    release(&ref_cnt.lock);
    return;
  }
  ref_cnt.count[idx] = 0;
  release(&ref_cnt.lock);

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

  if(r)
    {
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&ref_cnt.lock);
    ref_cnt.count[PA2INDEX(r)] = 1;
    release(&ref_cnt.lock);
  }
  return (void*)r;
}
