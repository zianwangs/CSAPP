#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct node * Node;
typedef struct deque * Deque;
typedef unsigned long ul;

int hit = 0, miss = 0, evict = 0;

typedef struct node {
    ul tag;
    Node next;
    Node pre;
} node;

typedef struct deque {
    node sentinel;
    int size;
    int max;
} deque;
// Input Parsing:
void loadArgs(int argc, char * argv[], int * s, int * e, int * b, int * len);
int readline(char * buf, int * op, ul * address);
// Deque Interface:
void addFirst(Deque d, Node n); 
void removeLast(Deque d); // evict++;
void delete(Deque d, Node n);
void moveToFront(Deque d, Node n);
Node search(Deque d, ul tag); // if NULL miss++ else hit++ then move n to front;
// Cache Interface:
void read(Deque cache, int b, int s, ul address);
// void write(Deque cache, int b, int s, ul address); <- redundant

int main(int argc, char * argv[]) {
    FILE * file;
    int s = 0, e = 0, b = 0, len = 0; // len = 2 ^ s;
    ul address = 0;
    int op = 0;
    char buf[64];

    loadArgs(argc, argv, &s, &e, &b, &len);
    file = fopen(argv[argc - 1], "r");
    
    Deque cache = (Deque)malloc(len * sizeof(deque));
    
    for (int i = 0; i < len; ++i) cache[i].max = e;
    
    while (fgets(buf, 64, file) != NULL) {
        if (readline(buf, &op, &address) == 0) continue;
        if (op == 'L') {
            read(cache, b, s, address);
        } else if (op == 'S') {
            read(cache, b, s, address);
        } else {
            read(cache, b, s, address);
            read(cache, b, s, address);
        }
    }
    
    fclose(file);
    printSummary(hit, miss, evict);
    return 0;
}

// Input Parsing:
void loadArgs(int argc, char * argv[], int * s, int * e, int * b, int * len) {
    int i = 1;
    for ( ; i < argc; ++i) {
        char * cur = argv[i];
        if (strcmp(cur, "-s") == 0) *s = atoi(argv[++i]);
        else if (strcmp(cur, "-E") == 0) *e = atoi(argv[++i]);
        else if (strcmp(cur, "-b") == 0) *b = atoi(argv[++i]);
    }
    *len = 1 << *s;
    return;
};

int readline(char * buf, int * op, ul * address) {
    if (buf[0] != ' ') return 0;
    
    *op = buf[1];
    
    char * cur = &buf[3], * stop;
    int i = -1;
    while (cur[++i] != ',');
    cur[i] = '\0';
    *address = strtoul(cur, &stop, 16);
    return 1;
};

// Deque Interface:
void addFirst(Deque d, Node n) {
    if (d->size == 0) {
        d->sentinel.next = n;
        n->pre = &(d->sentinel);
        n->next = &(d->sentinel);
        d->sentinel.pre = n;
    } else {
        Node front = d->sentinel.next;
        d->sentinel.next = n;
        n->pre = &(d->sentinel);
        n->next = front;
        front->pre = n;
    }
    ++d->size;
    return;
}; 

void removeLast(Deque d) {
    Node last = d->sentinel.pre;
    delete(d, last);
    free(last);
    return;
}; 

void delete(Deque d, Node n) {
    Node father = n->pre;
    Node son = n->next;
    father->next = son;
    son->pre = father;
    --d->size;
    return;
}

void moveToFront(Deque d, Node n) {
    delete(d, n);
    addFirst(d, n);
    return;
}

Node search(Deque d, ul tag) {
    if (d->size == 0) return NULL;
    Node start = d->sentinel.next;
    Node end = &(d->sentinel);
    while (start != end) {
        if (start->tag == tag) return start;
        start = start->next;
    }
    return NULL;
};

// Cache Simulator:
void read(Deque cache, int b, int s, ul address) {
    int num = (address >> b) & ((1 << s) - 1);
    ul tag = address >> (b + s);
    
    Deque d = &cache[num];
    Node target = search(d, tag);
    
    if (target != NULL) {
        ++hit;
        moveToFront(d, target);
    } else {
        ++miss;
        Node n = (Node)malloc(sizeof(node));
        n->tag = tag;
        if (d->size == d->max) {
            ++evict;
            removeLast(d);
        }
        addFirst(d, n);
    }
    return;
};

