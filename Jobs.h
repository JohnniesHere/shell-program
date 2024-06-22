//
#ifndef UNTITLED6_JOBS_H
#define UNTITLED6_JOBS_H

#include <sys/types.h>
#include <stddef.h>
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "string.h"

#define MAX_CMD_LEN  1025

int glob_id =0;
typedef struct Job {
    int id;
    pid_t pid;
    char command[MAX_CMD_LEN];
    struct Job *next;
} Job;

Job *jobs = NULL;


int highest_job_id = 0;

void add_job(pid_t pid, const char *command) {
    Job *new_job = (Job *)malloc(sizeof(Job));
    if (!new_job) {
        perror("malloc");
        _exit(EXIT_FAILURE);
    }
    //if jobs list is empty, assign the id to 1
    if(jobs == NULL) {
        new_job->id = 1;
    } else {
        new_job->id = jobs->id + 1;
    }
    new_job->pid = pid;
    strncpy(new_job->command, command, MAX_CMD_LEN);
    //remove ampersand from the end of the command
    new_job->command[strlen(new_job->command) - 1] = '\0';
    //check if theres spaces at the end of the command and remove them
    while(new_job->command[strlen(new_job->command) - 1] == ' ') {
        new_job->command[strlen(new_job->command) - 1] = '\0';
    }


    new_job->command[MAX_CMD_LEN - 1] = '\0';  // Ensure null-termination
    new_job->next = jobs;
    jobs = new_job;
}

// Function to compare two jobs based on their PIDs for qsort
int compare_jobs(const void *a, const void *b) {
    Job *jobA = *(Job **)a;
    Job *jobB = *(Job **)b;
    return (jobA->pid - jobB->pid);
}

// Function to print the list of background jobs in ascending order of their PIDs
void print_jobs() {
    // First, count the number of jobs
    int count = 0;
    Job *current = jobs;
    while (current != NULL) {
        count++;
        current = current->next;
    }

    // Create an array of pointers to jobs
    Job **jobArray = malloc(count * sizeof(Job *));
    current = jobs;
    for (int i = 0; i < count; i++) {
        jobArray[i] = current;
        current = current->next;
    }

    // Sort the array based on PIDs
    qsort(jobArray, count, sizeof(Job *), compare_jobs);

    // Print the jobs from the sorted array
    for (int i = 0; i < count; i++) {
        printf("[%d]               %s &\n", jobArray[i]->id, jobArray[i]->command);
    }

    // Free the array
    free(jobArray);
}

// Function to remove completed jobs from the list---------------------------------------------------------------------
void remove_completed_jobs() {
    Job *current = jobs;
    Job *prev = NULL;
    while (current != NULL) {
        int status;
        pid_t result = waitpid(current->pid, &status, WNOHANG);
        if (result == 0) {  // Child still running
            prev = current;
            current = current->next;
        } else {  // Child finished
            if (prev == NULL) {  // Head of the list
                jobs = current->next;
            } else {
                prev->next = current->next;
            }
            Job *temp = current;
            current = current->next;
            free(temp);
        }
    }
}



#endif //UNTITLED6_JOBS_H
