/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

//#define DEBUG

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* Predefined macros */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* My macros */
#define WSIZE 4
#define DSIZE 8
#define PAGE mem_pagesize()
#define INIT 1
#define RANGE 0
#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define classToCapacity(x) ((x) == 0 ? 8 : (1 << ((x) + 2)))
#define classToBlock(x) (classToCapacity(x) + DSIZE)
#define getClass(p) (((get(header(p)) & 0x6) << 2) + (get(footer(p)) & 0x7))
#define header(p) (p - WSIZE)
#define footer(p) (p + getBlock(header(p)) - DSIZE)
#define pre(fp) (fp)
#define next(fp) (fp + WSIZE)
#define get(p) (*(unsigned int *)(p))
#define put(p, val) (*(unsigned int *)(p) = (val))
#define getBlock(head) (get(head) & ~0x7)
#define getCapacity(head) (getBlock(head) - DSIZE)
#define isAllocated(head) (get(head) & 0x1)
#define classInHead(x) (((x) & 0x18) >> 2)
#define classInFoot(x) ((x) & 0x7)
#define footToHead(foot) ((foot) - getBlock(foot) + WSIZE)
#define footToBody(foot) ((foot) - getBlock(foot) + DSIZE)
#define beforeFoot(p) ((p) - DSIZE)
#define afterHead(p) (footer(p) + WSIZE)

/* My interface */
static size_t memToClass(size_t size);
static void * coalesce(void * p);
static void * find(size_t class, size_t size);
static size_t split(void * p, size_t size);
static void place(void * p, size_t size);
static void * extend(size_t class);
static void delete(void * p);
static void addFirst(void * p);
static void addLast(void * p);
static void connect(void * father, void * son);
static void createBlock(void * p, size_t class, size_t capacity, int allocated);
static void upgrade(void * p, size_t capacity);
static void degrade(void * p, size_t capacity);
static void * stretch(size_t capacity);
/* My global variables */
static void * table;
static void * start;
static void * end;
/* memlib interface: 
 *   void *mem_sbrk(int incr);
 *   void *mem_heap_lo(void);
 *   void *mem_heap_hi(void);
 *   size_t mem_heapsize(void);
 *   size_t mem_pagesize(void);
 */

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void * cur;
    table = mem_sbrk(32 * DSIZE);
    start = mem_sbrk(WSIZE);
    void * index = table;
    for (int i = 0; i < RANGE; ++i) {
        int size = classToBlock(i) * INIT;
        cur = mem_sbrk(size);
        if (cur == (void *)-1) return -1;
        /*
        cur += WSIZE;
        createFree(cur, i, size - DSIZE, index, 0);
        index += WSIZE;

        To be modified, since RANGE defined as 0, no need for this at present.
        */
    }
    for (int i = RANGE; i < 32; ++i) {
        put(pre(index), index);
        put(next(index), index);
        index += DSIZE;
    }
    end = mem_sbrk(WSIZE);
    put(start, 1);
    put(end, 1);
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t class = memToClass(size);
    size_t alignedSize = ALIGN(size);
    void * p = find(class, alignedSize);
    if( (p != NULL) || (p = stretch(alignedSize)) != NULL) {
        place(p, alignedSize);
        return p;
    }
    //void * last_foot = end - WSIZE;
    
    p = extend(class);
    if (p == NULL) return NULL;
    place(p, alignedSize);
    return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void * p) {
    createBlock(p, getClass(p), getCapacity(header(p)), 0);
    coalesce(p);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return;
    }
    size_t original = getCapacity(header(ptr));
    size_t alignedSize = ALIGN(size);
    size_t class = memToClass(alignedSize);
    if (original >= alignedSize) return ptr;
    else if (original > alignedSize) {
        size_t remains = original - alignedSize;
        if (remains <= 32) { /* NOTE!STRATEGY BE MODIFIED HERE! */ 
            return ptr;
        } else {
            void * new = ptr + alignedSize + DSIZE;
            put(header(ptr), alignedSize + 8 | classInHead(class) | 1);
            put(footer(ptr), alignedSize + 8 | classInFoot(class));
            upgrade(new, remains - DSIZE);
            return ptr;
        }
    }
    size_t remains = alignedSize - original;
    void * after = afterHead(ptr);
    

    if (after == end) {
        void * p = mem_sbrk(remains);
        end = p + remains - WSIZE;
        put(end, 1);
        put(header(ptr), alignedSize + 8 | classInHead(class) | 1);
        put(footer(ptr), alignedSize + 8 | classInFoot(class));
        return ptr;

    }
    if (!isAllocated(after) && getCapacity(after) >= remains) {
        void * new = ptr + alignedSize + DSIZE;
        void * after_body = after + WSIZE;
        size_t cap = getCapacity(after) - remains;
        size_t c = memToClass(alignedSize);
        delete(after_body);
        if (cap >102400) {
            upgrade(new, cap);
            put(header(ptr), alignedSize + 8 | classInHead(c) | 1);
            put(footer(ptr), alignedSize + 8 | classInFoot(c));
        } else {
            c = memToClass(original + getBlock(after));
            put(header(ptr), original + getBlock(after) + 8 | classInHead(c) | 1);
            put(footer(ptr), original + getBlock(after) + 8 | classInFoot(c));
        }
        return ptr;
    }
    
    void * new = mm_malloc(size);
    memcpy(new, ptr, original);
    mm_free(ptr);
    return new;
}

#ifdef DEBUG
int main() {
    mem_init();
    mm_init();
    void * p = mm_malloc(64);
    void * q = mm_malloc(448);
    void * r = mm_malloc(64);
    void * s = mm_malloc(448);
    void * t = mm_malloc(64);
    void * u = mm_malloc(448);
    mm_free(p);
    mm_free(q);
    mm_free(r);
    mm_free(s);
    mm_free(t);
    mm_free(u);
    /*
a 0 2040
a 1 4010
a 2 48
a 3 4072
a 4 4072
a 5 4072
    */
    printf("done.");
    return 0;
}
#endif

static void * stretch(size_t cap) {
    void * last_foot = end - WSIZE;
    size_t lastAllocated = last_foot == start ? 1 : isAllocated(footToHead(last_foot));
    if (lastAllocated) return NULL;
    void * last = footToBody(last_foot);
    delete(last);
    size_t last_cap = getCapacity(header(last)), diff = cap - last_cap;
    void * p = mem_sbrk(diff);
    end += diff;
    put(end, 1);
    createBlock(last, memToClass(cap), cap, 0);
    addFirst(last);
    return last;
}

static void * coalesce(void * p) {
    void * before_foot = beforeFoot(p), * after_head = afterHead(p);
    size_t beforeAllocated = before_foot == start ? 1 : isAllocated(footToHead(before_foot));
    size_t afterAllocated = isAllocated(after_head);
    size_t cap = getCapacity(header(p));
    if (beforeAllocated && afterAllocated) {
        addFirst(p);
        /* STATEGY HERE, TO BE MODIFIED!! */
    } else if (!beforeAllocated && afterAllocated) {
        p = footToBody(before_foot);
        delete(p);
        upgrade(p, getBlock(header(p)) + cap);
        /* NO MOVE TO FRONT !! COULD BE MODIFIED !! */
    } else if (beforeAllocated && !afterAllocated) {
        void * after_body = after_head + WSIZE;
        delete(after_body);
        upgrade(p, getBlock(header(after_body)) + cap);
    } else {
        void * after_body = after_head + WSIZE;
        p = footToBody(before_foot);
        delete(p);
        delete(after_body);
        upgrade(p, getBlock(header(p)) + cap + getBlock(header(after_body)));
    }
    return p;

};
static void * find(size_t class, size_t size) {
    for (size_t t = class; t < 32; ++t) {
        void * sentinel = table + t * DSIZE, * p = get(next(sentinel));
        void * ans = 0;
        unsigned int best = 1 << 31;
        while (p != sentinel) {
        
            size_t cur = getCapacity(header(p)); 
             
            if (cur == size) return p;
            if (cur > size && cur < best) {
                ans = p;
                best = cur;
            }
            
            //if (cur >= size) return p;
            p = get(next(p));
        }
        if (ans != 0) return ans;
    }
    return NULL;
}

static size_t split(void * p, size_t size) {
    size_t remains = getCapacity(header(p)) - size;
    delete(p);
    if (remains <= max(8,getCapacity(header(p))/8)) { /* NOTE! STRATEGY MAY BE MODIFIED HERE! */
        return remains + size;
    } else {
        degrade(p + size + DSIZE, remains - DSIZE);
        return size;
    }
}

static void delete(void * p) {
    connect(get(pre(p)), get(next(p)));
}

static void degrade(void * p, size_t capacity) {
    size_t class = memToClass(capacity);
    createBlock(p, class, capacity, 0);
    addLast(p);
}

static void upgrade(void * p, size_t capacity) {
    size_t class = memToClass(capacity);
    createBlock(p, class, capacity, 0);
    addFirst(p);
}

static void connect(void * father, void * son) {
    put(pre(son), father);
    put(next(father), son);
};

static void place(void * p, size_t size) {
    size_t capacity = split(p, size);
    createBlock(p, memToClass(capacity), capacity, 1);
};

static void createBlock(void * p, size_t class, size_t capacity, int allocated) {
    size_t size = capacity + DSIZE;
    put(header(p), size | classInHead(class) | allocated);
    put(footer(p), size | classInFoot(class));
};

static void * extend(size_t class) {
    size_t block = classToBlock(class);
    void * p = mem_sbrk(block);
    if (p == (void *)-1) return NULL;
    end = p + block - WSIZE;
    put(end, 1);
    createBlock(p, class, block - DSIZE, 0);
    return coalesce(p);
};

static void addFirst(void * p) {
    size_t class = getClass(p);
    void * sentinel = table + class * DSIZE, * front = get(next(sentinel));
    connect(sentinel, p);
    connect(p, front);
};

static void addLast(void * p) {
    size_t class = getClass(p);
    void * sentinel = table + class * DSIZE, * tail = get(pre(sentinel));
    connect(tail, p);
    connect(p, sentinel);
}

static size_t memToClass(size_t size) {
    if (size-- <= 4) return 0;
    size_t ans = 0;
    while (size != 1) {
        size >>= 1;
        ++ans;
    }
    return ans - 1;
}

