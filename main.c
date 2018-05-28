#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "options.h"
#include "schedulers.h"
#include "print.h"

int scheduler_num = 10;
Scheduler schedulers[] = {first_come_first_served,
                          shortest_job_first_preemptive,
                          shortest_job_first_non_preemptive,
                          priority_preemptive,
                          priority_non_preemptive,
                          round_robin,
                          shortest_remaining_time_first_preemptive,
                          shortest_remaining_time_first_non_preemptive,
                          highest_response_ratio_next_preemptive,
                          highest_response_ratio_next_non_preemptive};
char *scheduler_names[] = {"first_come_first_served",
                           "shortest_job_first_preemptive",
                           "shortest_job_first_non_preemptive",
                           "priority_preemptive",
                           "priority_non_preemptive",
                           "round_robin",
                           "shortest_remaining_time_first_preemptive",
                           "shortest_remaining_time_first_non_preemptive",
                           "highest_response_ratio_next_preemptive",
                           "highest_response_ratio_next_non_preemptive"};

Process *processes;
ProcessQueue ready_queue;
ProcessQueue waiting_queue;
Schedule schedules;

int main() {
    srand((unsigned int) time(NULL));

    processes = create_processes(PROCESS_NUM);
    ready_queue = create_process_queue();
    waiting_queue = create_process_queue();

    print_processes(processes, PROCESS_NUM);

    int scheduler_index = 0;
    for (scheduler_index = 0; scheduler_index < scheduler_num; scheduler_index++) {
        reset_process_information(processes, PROCESS_NUM);
        Scheduler now_scheduler = schedulers[scheduler_index];
        Process previous_process = NULL;
        schedules = NULL;
        int terminated_process_num = 0;
        int now_time;

        for (now_time = 0; terminated_process_num != PROCESS_NUM; now_time++) {

            // Put arrived process to ready queue
            put_arrived_process_to_ready_queue(ready_queue, processes, PROCESS_NUM, now_time);

            // Choose process
            Process p = now_scheduler(ready_queue, previous_process);
            printf("8\n");
            // If chosen process is different from previous process,
            // reset continuous_cpu_burst_time of previous process
            if (p != previous_process && previous_process != NULL)
                previous_process->continuous_cpu_burst_time = 0;
            printf("9\n");
            // Run process (cpu and io)
            if (p != NULL) {
                printf("10\n");
                p->remaining_cpu_burst_time -= 1; // Decrease remaining_cpu_burst_time
                printf("11\n");
                p->continuous_cpu_burst_time++; // Increase continuous_cpu_burst_time
                printf("12\n");
            }
            printf("13\n");
            decrease_io_burst_time_in_waiting_queue(waiting_queue); // Decrease remaining_io_burst_time
            printf("14\n");
            increase_waiting_time(ready_queue, p);  // Increase waiting time of processes in ready queue
            printf("15\n");

            // Terminate process if cpu burst done
            if (p != NULL && p->remaining_cpu_burst_time == 0) {
                printf("16\n");
                remove_from_queue(ready_queue, p);
                printf("17\n");
                p->turnaround_time = (now_time + 1) - p->arrival_time;
                printf("18\n");
                terminated_process_num++;
                printf("19\n");
            }

            // Put process to ready queue when I/O done
            put_io_done_process_to_ready_queue(ready_queue, waiting_queue);
            printf("20\n");

            BOOL io_occurred = FALSE;
            printf("21\n");
            if (p != NULL && p->remaining_cpu_burst_time != 0) {
                printf("22\n");
                // Check I/O occurred
                io_occurred = random_double() < IO_OCCUR_PROBABILITY ? TRUE : FALSE;
                printf("23\n");

                // Put process if I/O occurred
                if (io_occurred) {
                    printf("24\n");
                    put_process_to_waiting_queue_from_ready_queue(ready_queue, waiting_queue, p);
                    printf("25\n");
                }
                printf("26\n");
            }

            // Write logs
            if (schedules == NULL)
                schedules = create_schedule(p, io_occurred);
            else
                add_schedule(schedules, p, io_occurred);

            previous_process = p;
        }

        printf("========================================================================\n");
        printf("%s\n\n", scheduler_names[scheduler_index]);
        print_schedule(schedules);
        __print_waiting_and_turnaround_time(processes, PROCESS_NUM);
        printf("\n");

        free_schedule(schedules);
    }

    return 0;
}

void put_arrived_process_to_ready_queue(ProcessQueue ready_queue, Process *processes, uint32_t size, int now_time) {
    int i;
    for (i = 0; i < size; i++) {
        Process p = processes[i];
        if (p->arrival_time == now_time) {
            add_to_queue(ready_queue, p);
        }
    }
}

void decrease_io_burst_time_in_waiting_queue(ProcessQueue waiting_queue) {
    Process *array = create_process_array_from_queue(waiting_queue);
    int size = size_of_queue(waiting_queue);
    int i;

    for (i = 0; i < size; i++) {
        array[i]->remaining_io_burst_time -= 1;
    }

    free(array);
}

void put_io_done_process_to_ready_queue(ProcessQueue ready_queue, ProcessQueue waiting_queue) {
    Process *array = create_process_array_from_queue(waiting_queue);
    int size = size_of_queue(waiting_queue);
    int i;

    for (i = 0; i < size; i++) {
        Process p = array[i];
        if (p->remaining_io_burst_time == 0 && p->is_in_io) {
            remove_from_queue(waiting_queue, p);
            add_to_queue(ready_queue, p);
            p->is_in_io = FALSE;
        }
    }

    free(array);
}

void put_process_to_waiting_queue_from_ready_queue(ProcessQueue ready_queue, ProcessQueue waiting_queue, Process p) {
    remove_from_queue(ready_queue, p);
    add_to_queue(waiting_queue, p);
    p->remaining_io_burst_time = p->io_burst_time;
    p->is_in_io = TRUE;
}

void reset_process_information(Process *processes, int size) {
    int i;
    for (i = 0; i < size; i++) {
        processes[i]->remaining_cpu_burst_time = processes[i]->cpu_burst_time;
        processes[i]->remaining_io_burst_time = 0;
        processes[i]->is_in_io = FALSE;
        processes[i]->continuous_cpu_burst_time = 0;
        processes[i]->waiting_time = 0;
        processes[i]->turnaround_time = 0;
    }
}

void increase_waiting_time(ProcessQueue ready_queue, Process now_running_process) {
    ProcessQueueNode now = *ready_queue;
    while (now != NULL) {
        if (now->process != now_running_process)
            now->process->waiting_time++;
        now = now->next;
    }
}