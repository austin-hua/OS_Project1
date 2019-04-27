#include "scheduler.h"
#include <stdlib.h>

ProcessInfo **pq;
int heap_size;
ProcessInfo *activeProcess = NULL;

void heap_init()
{
     pq = (ProcessInfo**)malloc(sizeof(ProcessInfo*) * num_process);
     heap_size = 0;
}

void swap(int lhs, int rhs) {
     ProcessInfo *temp = pq[lhs];
     pq[lhs] = pq[rhs];
     pq[rhs] = temp;
}

void upheap(int childInd) {
     int parentInd = (childInd - 1) / 2;
     while (parentInd != 0 && pq[parentInd]->remaining_time < pq[childInd]->remaining_time) {
          swap(parentInd, childInd);    
          childInd = parentInd;
          parentInd = (childInd - 1) / 2;     
     }
}

void heap_insert(ProcessInfo *p)
{
     int childInd = heap_size;
     heap_size++;
     uphead(heap_size-1);
}

ProcessInfo *heap_top(){
     return pq[0];
}
int min_child(parentInd)
{
     // assuming left child id < heapSize
     int child1Ind = parentInd * 2;
     int child2Ind = parentInd * 2 + 1;
     if (child2Ind >= heapSize){
          return child1Ind;
     }
     if (pq[child1Ind]->remaining_time > pq[child2Ind]->remaining_time){
         return child2ind;
     }
     return child1Ind;
}

void downheap(int parentInd) {
     while(parentInd * 2 < heap_size) {
         int minChildInd = min_child(parentInd);
         if (pq[parentInd]->remaining_time <= pq[minChildInd]->remaining_time){
             break;
         }
         swap(parentInd, minChildInd);
         parentInd = minChildInd;
     }
}

void heap_pop() {
     swap(heap_size, 0);
     heap_size--;
     downheap(0);
}

int heap_size() {
     return heap_size;
}

void set_strategy_SJF() {
     heap_init();
}

void add_process_SJF(ProcessInfo *new_process) {
    pq[cur_num_process] = new_process;
    curr_num_process++;
}

void remove_curr_process_SJF(void) {
     pq[curr_num_process-1] = NULL;
     curr_num_process--;
}

int smallest_process(void) {
     if(curr_num_process <= 0) return -1;
     int smallest = 0;
     for(int i = 0; i < curr_num_process; i++) {
          if(pq[i]->remaining_time < pq[smallest]->remaining_time) smallest = i;
     }
     return smallest;
}

void run_process(int ind) {
     while(pq[ind]->remaining_time) {
          
     }
}

void context_switch_SJF(void) {
    if(scheduler_empty_SJF()) {
         int new_active_process_ind = smallest_process();
         run_process(new_active_process_ind);
    }
}

bool scheduler_empty_SJF(void) {
     return curr_num_process == 0;
}
