#include <stdlib.h>
#include "scheduler.h"

void heap_init(Heap* h) {
     h->pq = (ProcessInfo**)malloc(sizeof(ProcessInfo*) * num_process);
     h->heap_size = 0;
}

static void heap_swap(Heap *h, int lhs, int rhs) {
     ProcessInfo *temp = h->pq[lhs];
     h->pq[lhs] = h->pq[rhs];
     h->pq[rhs] = temp;
}

static void upheap(Heap *h, int childInd) {
     int parentInd = (childInd - 1) / 2;
     while (parentInd != 0 && h->pq[parentInd]->remaining_time < h->pq[childInd]->remaining_time) {
          heap_swap(h, parentInd, childInd);
          childInd = parentInd;
          parentInd = (childInd - 1) / 2;
     }
}

void heap_insert(Heap *h, ProcessInfo *p) {
     int childInd = h->heap_size;
     h->pq[childInd] = p;
     h->heap_size++;
     upheap(h,childInd);
}

ProcessInfo *heap_top(Heap *h) {
     return h->pq[0];
}

static int min_child(Heap *h, int parentInd) {
     // assuming left child id < heapSize
     int child1Ind = parentInd * 2;
     int child2Ind = parentInd * 2 + 1;
     if (child2Ind >= h->heap_size){
          return child1Ind;
     }
     if (h->pq[child1Ind]->remaining_time > h->pq[child2Ind]->remaining_time){
         return child2Ind;
     }
     return child1Ind;
}

static void downheap(Heap *h, int parentInd) {
     while(parentInd * 2 < h->heap_size) {
         int minChildInd = min_child(h,parentInd);
         if (h->pq[parentInd]->remaining_time <= h->pq[minChildInd]->remaining_time){
             break;
         }
         heap_swap(h, parentInd, minChildInd);
         parentInd = minChildInd;
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
