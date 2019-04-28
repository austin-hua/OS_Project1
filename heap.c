#include <stdlib.h>
#include "scheduler.h"

void heap_init(Heap* h, int max_size) {
     h->pq = (ProcessInfo**)malloc(sizeof(ProcessInfo*) * max_size);
     h->heap_size = 0;
}

static bool heap_element_lt(Heap *h, int lhsIdx, int rhsIdx)
{
    const ProcessInfo *lhs = h->pq[lhsIdx];
    const ProcessInfo *rhs = h->pq[rhsIdx];
    if (lhs->remaining_time == rhs->remaining_time){
        return lhs->pid < rhs->pid;
    }
    return lhs->remaining_time < rhs->remaining_time;
}

static int lchild(int parentIdx)
{
    return parentIdx * 2 + 1;
}

static int rchild(int parentIdx)
{
    return parentIdx * 2 + 2;
}

static int parent(int childIdx)
{
    return (childIdx - 1)/2;
}

static void heap_swap(Heap *h, int lhs, int rhs) {
     ProcessInfo *temp = h->pq[lhs];
     h->pq[lhs] = h->pq[rhs];
     h->pq[rhs] = temp;
}

static void upheap(Heap *h, int childIdx) {
     int parentIdx = (childIdx - 1) / 2;
     while (childIdx > 0 && heap_element_lt(h, childIdx, parentIdx)) {
          heap_swap(h, parentIdx, childIdx);
          childIdx = parentIdx;
          parentIdx = (childIdx - 1) / 2;
     }
}

void heap_insert(Heap *h, ProcessInfo *p) {
     int childIdx = h->heap_size;
     h->pq[childIdx] = p;
     h->heap_size++;
     upheap(h,childIdx);
}

ProcessInfo *heap_top(Heap *h) {
     return h->pq[0];
}

static void downheap(Heap *h, int parentIdx) {
     while(lchild(parentIdx) < h->heap_size) {
         int minIdx = parentIdx;
         if (heap_element_lt(h, lchild(parentIdx), minIdx)){ 
             minIdx = lchild(parentIdx);
         }
         if (rchild(parentIdx) < h->heap_size && heap_element_lt(h, rchild(parentIdx), minIdx)){
             minIdx = rchild(parentIdx);
         }
         if (parentIdx == minIdx){
             break;
         } 
         heap_swap(h, parentIdx, minIdx);
         parentIdx = minIdx;
     }
}

void heap_pop(Heap *h) {
     h->heap_size--;
     heap_swap(h, h->heap_size, 0);
     downheap(h, 0);
}

int heap_size(Heap *h) {
     return h->heap_size;
}

bool heap_empty(Heap *h){
    return h->heap_size == 0;
}
