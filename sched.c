#include "sched.h"

void FCFS(int number_of_jobs, const int job_submitted_time[], 
            const int job_required_time[], int job_sched_start[]) {
    int i, time = 0;
    for (i = 0; i < number_of_jobs; i++) {
        if (job_submitted_time[i] > time) {
            time = job_submitted_time[i];
            // update current time;
        }
        job_sched_start[i] = time;
        time += job_required_time[i];
    }
}

static int is_deleted[2003];

// ensures "the worst condition is ", "n * (n + n) = 2*n**2", "O(n**2)"
void SJF(int number_of_jobs, const int job_submitted_time[], 
            const int job_required_time[], int job_sched_start[]) {
    int i, j, min_reqtime, min_pos, time = 0;
    // 1. required_time is least (in case job_submitted_time <= time)
    // 2. submitted_time is least
    // 3. id is least
    i = 0;
    while(i < number_of_jobs) {
        min_reqtime = 1000006;
        min_pos = -1;
        for (j = 0; j < number_of_jobs && job_submitted_time[j] <= time; j++) {
            if (is_deleted[j]) continue;
            if (min_reqtime > job_required_time[j]) {
                min_reqtime = job_required_time[j];
                min_pos = j;
            }
        }
        if (min_pos != -1) {
            // min_pos is the shortest job
            is_deleted[min_pos] = 1;
            job_sched_start[min_pos] = time;
            time += job_required_time[min_pos];
            i++; // indicates that a job has assigned.
        }
        else {
            time = job_submitted_time[j];
            // assert j != num_of_jobs
            // assert (\forall int i; 0 <= i && i < j; is_deleted[i] == 1)
            // i don't increase, since a job has not been assigned.
        }
    }
}


