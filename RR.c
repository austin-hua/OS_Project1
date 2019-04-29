#include<stdio.h> 
#include "scheduler.h"

void waitingTime(int processes[], int n, int time_needed[], int wait_time[], int quantum) 
{
    int remaining_time[n]; 
    for (int i = 0 ; i < n ; i++) 
        remaining_time[i] = time_needed[i]; 
  
    int t = 0; // Current time 
  
    while (1) 
    { 
        int done = 1; 
  
        for (int i = 0 ; i < n; i++) 
        { 
            if (remaining_time[i] > 0) 
            { 
                done = 0; // There is a pending process
  
                if (remaining_time[i] > quantum) 
                { 
                    t += quantum; 
                    remaining_time[i] -= quantum; 
                }
                else
                { 
                    t = t + remaining_time[i]; 
                    wait_time[i] = t - time_needed[i]; 
                    remaining_time[i] = 0; 
                } 
            } 
        } 
  
        if (done == 1) 
          break; 
    } 
} 
  
// Function to calculate turn around time 
void TurnAroundTime(int processes[], int n, int time_needed[], int wait_time[], int tat[]) 
{ 
    // calculating turnaround time by adding time_needed[i] + wait_time[i] 
    for (int i = 0; i < n ; i++) 
        tat[i] = time_needed[i] + wait_time[i]; 
} 
  
// Function to calculate average time 
void findavgTime(int processes[], int n, int time_needed[], int quantum) 
{ 
    int wait_time[n], tat[n], total_wt = 0, total_tat = 0; 
  
    // Function to find waiting time of all processes 
    waitingTime(processes, n, time_needed, wait_time, quantum); 
  
    // Function to find turn around time for all processes 
    TurnAroundTime(processes, n, time_needed, wait_time, tat); 
  
    // Calculate total waiting time and total turn around time 
    for (int i=0; i<n; i++) 
    { 
        total_wt = total_wt + wait_time[i]; 
        total_tat = total_tat + tat[i]; 
        printf ("%d \t %d \t %d \t %d\n", i+1, time_needed[i], wait_time[i], tat[i]);
    } 
  
    printf ("Average waiting time = %f/%f\nAverage turn around time = %f/%f", total_wt, n, total_tat, n);
} 

// Please read comments in scheduler.h to see what functions to be implemented.
