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
static void moveToFront(size_t class, size_t size);
static void connect(void * father, void * son);
static void createFree(void * p, size_t class, size_t capacity, void * father, void * son);
static void createAllocated(void * p, size_t class, size_t capacity);
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
    table = mem_sbrk(32 * WSIZE);
    start = mem_sbrk(WSIZE);
    void * index = table;
    for (int i = 0; i < RANGE; ++i) {
        int size = classToBlock(i) * INIT;
        cur = mem_sbrk(size);
        if (cur == (void *)-1) return -1;
        cur += WSIZE;
        createFree(cur, i, size - DSIZE, index, 0);
        index += WSIZE;
    }
    for (int i = RANGE; i < 32; ++i) {
        put(index, 0);
        index += WSIZE;
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
    if (p != NULL) {
        place(p, alignedSize);
        return p;
    } 
    p = extend(class);
    if (p == NULL) return NULL;
    place(p, alignedSize);
    return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void * p) {
    coalesce(p);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t original = getCapacity(header(ptr));
    size_t alignedSize = ALIGN(size);
    size_t class = getClass(header(ptr));
    if (original == alignedSize) return ptr;
    else if (original > alignedSize) {
        size_t remains = original - alignedSize;
        if (remains <= 8) { /* NOTE! STRATEGY MAY BE MODIFIED HERE! */
            return ptr;
        } else {
            void * new = ptr + alignedSize + DSIZE;
            put(header(new), remains | classInHead(class) | 0);
            put(footer(new), remains | classInFoot(class));
            coalesce(new);
            return ptr;
        }
    }
    size_t remains = alignedSize - original;
    void * after = afterHead(ptr);

    if (after == end) {
        void * p = mem_sbrk(remains);
        end = p + remains - WSIZE;
        class = memToClass(alignedSize);
        put(end, 1);
        put(header(ptr), alignedSize + 8 | classInHead(class) | 1);
        put(footer(ptr), alignedSize + 8 | classInFoot(class));
        return ptr;

    }
    
    if (!isAllocated(after) && getCapacity(after) >= remains) {
        void * new = ptr + alignedSize + DSIZE;
        size_t cap = getCapacity(after) - remains, new_class = memToClass(alignedSize);
        size_t after_class = getClass(after + WSIZE);
        if (cap >= 8) {
            size_t c = memToClass(cap);
            
            void * father = get(pre(after + WSIZE)), *son = get(next(after + WSIZE));
            /*
            connect(father, son);
            */
            void * t = table + c * WSIZE, *ts = get(t);
           
            createFree(new, after_class, cap, father, son);
      
            put(header(ptr), alignedSize + 8 | classInHead(new_class) | 1);
            put(footer(ptr), alignedSize + 8 | classInFoot(new_class));
        } else {
            new_class = memToClass(original + getBlock(after) + 8);
            put(header(ptr), original + getBlock(after) + 8 | classInHead(new_class) | 1);
            put(footer(ptr), original + getBlock(after) + 8 | classInFoot(new_class));
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
    for (int i = 0; i < 10000; ++i) {
        mm_malloc(64);
        mm_malloc(448);
        }
    //for (int i)
    return 0;
}
#endif


static void * coalesce(void * p) {
    void * before_foot = beforeFoot(p);
    void * after_head = afterHead(p);
    size_t beforeAllocated = before_foot == start ? 1 : isAllocated(footToHead(before_foot));
    size_t afterAllocated = isAllocated(after_head);
    size_t beforeClass = 32, afterClass = 32, class = getClass(p), cap = getCapacity(header(p));
    void * father = table + class * WSIZE;
    void * son = (void *)get(father);
    if (!beforeAllocated) beforeClass = getClass(footToBody(before_foot));
    if (!afterAllocated) afterClass = getClass(after_head + WSIZE);
    if (beforeClass != class && afterClass != class) {
        createFree(p, class, cap, father, son);
    } else if (beforeClass == class && afterClass != class) {
        p = footToBody(before_foot);
        createFree(p, class, getBlock(header(p)) + cap, get(pre(p)), get(next(p))); 
        /* NO MOVE TO FRONT !! COULD BE MODIFIED !! */
    } else if (beforeClass != class && afterClass == class) {
        void * after_body = after_head + WSIZE;
        createFree(p, class, getBlock(after_head) + cap, get(pre(after_body)), get(next(after_body)));
    } else {
        void * after_body = after_head + WSIZE;
        p = footToBody(before_foot);
        void * before_pre = get(pre(p)), * before_next = get(next(p));
        void * after_pre = get(pre(after_body)), * after_next = get(next(after_body));
        if (before_pre == after_body) {
            createFree(p, class, getBlock(after_head) + getBlock(header(p)) + cap, after_pre, before_next);
        } else if (after_pre == p) {
            createFree(p, class, getBlock(after_head) + getBlock(header(p)) + cap, before_pre, after_next);
        } else {
            createFree(p, class, getBlock(after_head) + getBlock(header(p)) + cap, before_pre, before_next);
            connect(after_pre, after_next);
        }
    }
    return p;

};
static void * find(size_t class, size_t size) {
    void * p; 
    for (size_t t = class; t < 32; ++t) {
        p = (void *)get(table + t * WSIZE);
        while (p != 0) {
            if (getCapacity(header(p)) >= size) return p;
            p = get(next(p));
        }
    }
    return NULL;
}

static size_t split(void * p, size_t size) {
    int maxSize = getCapacity(header(p));
    void * father = get(pre(p));
    void * son = get(next(p));
    if (maxSize - size <= max(8, 0)) { /* NOTE! STRATEGY MAY BE MODIFIED HERE! */
        connect(father, son);
        return maxSize;
    } else {
        createFree(p + size + DSIZE, getClass(p), maxSize - size - DSIZE, father, son);
        return size;
    }
}

static void connect(void * father, void * son) {
    if (father < start) {
        put(father, son);
    } else {
        put(next(father), son);
    }
    if (son != 0) put(pre(son), father);
};

static void place(void * p, size_t size) {
    size_t capacity = split(p, size);
    createAllocated(p, getClass(p), capacity);
};

static void createFree(void * p, size_t class, size_t capacity, void * father, void * son) {
    size_t size = capacity + DSIZE;
    put(header(p), size | classInHead(class) | 0);
    put(footer(p), size | classInFoot(class));
    connect(father, p);
    connect(p, son);
};

static void createAllocated(void * p, size_t class, size_t capacity) {
    size_t size = capacity + DSIZE;
    put(header(p), size | classInHead(class) | 1);
    put(footer(p), size | classInFoot(class));
};

static void * extend(size_t class) {
    void * father = table + class * WSIZE;
    void * son = (void *)get(father);
    size_t block = classToBlock(class) * 8;
    void * p = mem_sbrk(block);
    if (p == (void *)-1) return NULL;
    end = p + block - WSIZE;
    put(end, 1);
    
    void * f = beforeFoot(p);
    if (f != start && !isAllocated(footToHead(f)) && getClass(footToBody(f)) == class) {
        p = footToBody(f);
        createFree(p, class, getCapacity(header(p)) + block, get(pre(p)), get(next(p)));
        return p;
    }
    
    createFree(p, class, block - DSIZE, father, son);
    return p; /* STRATEGY TO MODIFY HERE, COULD RETURN COALESCE(P)*/

};
static void moveToFront(size_t class, size_t size) {
};



static size_t memToClass(size_t size) {
    if (size-- <= 4) return 0;
    size_t ans = 0;
    while (size != 1) {
        size >>= 1;
        ++ans;
    }
    return ans - 1;
}

