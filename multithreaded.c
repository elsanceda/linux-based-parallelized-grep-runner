#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct Node {
    char value[250];
    struct Node *next;
}   node;

typedef struct Queue {
    node *head;
    node *tail;
    sem_t head_lock, tail_lock;
}   queue;

typedef struct Counter {
    int value;
    sem_t lock;
}   counter;

typedef struct Thread_Data {
    int id;             // Worker ID
    int n;              // Number of workers
    char *string;       // Search string
    queue *queue;       // Global queue
    counter *counter;   // Global counter
}   thread_data;

void grep_runner(thread_data *argt);
void init_queue(queue *q);
void enqueue(queue *q, char *new_task);
int dequeue(queue *q, char *value); 
void init_counter(counter *c);
void increment(counter *c);
void decrement(counter *c);
int get(counter *c);

sem_t lock[3];

int main(int argc, char *argv[]) {
    int n = atoi(argv[1]);
    pthread_t tid[n];
    thread_data tdata_arr[n];
    queue *task_queue = malloc(sizeof(queue));
    counter *t_sleep_counter = malloc(sizeof(counter));

    // Initialize global queue and counter 
    init_queue(task_queue);
    init_counter(t_sleep_counter);

    // Enqueue rootpath
    char *rootpath = (char *) malloc(250*sizeof(char));
    realpath(argv[2], rootpath);
    enqueue(task_queue, rootpath);

    // Initialize 2 of the semaphores as locks
    for (int i = 0; i < 2; i++) {
        sem_init(&lock[i], 0, 1);
    }

    // Initialize last semaphore to enforce execution order
    sem_init(&lock[2], 0, 0);

    for (int i = 0; i < n; i++) {
        // Set thread arguments
        tdata_arr[i].id = i;
        tdata_arr[i].n = n;
        tdata_arr[i].string = argv[3];
        tdata_arr[i].queue = task_queue;
        tdata_arr[i].counter = t_sleep_counter;

        pthread_create(&tid[i], NULL, (void *)grep_runner, &tdata_arr[i]);
    }

    sem_post(&lock[2]);

    for (int i = 0; i < n; i++) {
        pthread_join(tid[i], NULL);
    }
    
    free(rootpath);
    free(t_sleep_counter);
    if (task_queue->head != NULL) free(task_queue->head);
    free(task_queue);
    return 0;
}

void grep_runner(thread_data *argt) {
    sem_wait(&lock[2]);
    sem_post(&lock[2]);

    int t_id = argt->id;
    int t_n = argt->n;
    char *t_string = argt->string;
    queue *t_queue = argt->queue;
    counter *t_counter = argt->counter;

    int not_empty;
    int count;
    while (1) {
        sem_wait(&lock[0]);
        // Dequeue task queue and get count = no. of workers on standby
        char *task = (char *) malloc(250*sizeof(char));
        not_empty = dequeue(t_queue, task);
        count = get(t_counter);
        sem_post(&lock[0]);
        
        if (not_empty != 0) {
            if (count < t_n-1) {
                // Queue empty, standby and wait
                increment(t_counter);
                sem_wait(&lock[2]);

                // Once awake, go back to start to check if queue has tasks
                decrement(t_counter);
                free(task);
                continue;
            } else {
                // No more tasks to do, wake sleeping threads and terminate
                increment(t_counter);
                sem_post(&lock[2]);
                free(task);
                break;
            }
        }

        printf("[%d] DIR %s\n", t_id, task);

        sem_wait(&lock[1]);
        DIR *directory = opendir(task);
        
        struct dirent *child_object;
        child_object = readdir(directory);
        sem_post(&lock[1]);
        
        while (child_object != NULL) {
            sem_wait(&lock[0]);
            // Ignore . and .. file objects
            if ((child_object->d_type == DT_DIR && strcmp(child_object->d_name, ".") == 0) 
                || (child_object->d_type == DT_DIR && strcmp(child_object->d_name, "..") == 0)) {
                child_object = readdir(directory);
                sem_post(&lock[0]);
                continue;
            } 

            // Form the absolute path of child object
            char *path = (char *) malloc(250*sizeof(char));
            realpath(task, path);
            if (strcmp(task, "/") != 0) strcat(path, "/");
            strcat(path, child_object->d_name);

            if (child_object->d_type == DT_DIR) {
                // Enqueue directory to task queue and wake any sleeping threads
                enqueue(t_queue, path);
                printf("[%d] ENQUEUE %s\n", t_id, path);
                sem_post(&lock[2]);
            } else if (child_object->d_type == DT_REG) {
                // Form the grep command string to be invoked by system()
                int cmd_length = strlen(t_string) + strlen(path) + 19;
                char *command = (char *) malloc(cmd_length*sizeof(char));
                strcpy(command, "grep ");
                strcat(command, t_string);
                strcat(command, " ");
                strcat(command, path);
                strcat(command, " > /dev/null");

                // Perform grep on file
                int ret_val = system(command);
                if (ret_val == 0) printf("[%d] PRESENT %s\n", t_id, path);
                else printf("[%d] ABSENT %s\n", t_id, path);

                free(command);
            }
            
            free(path);
            child_object = readdir(directory);
            sem_post(&lock[0]);
        }

        sem_wait(&lock[1]);
        closedir(directory);
        sem_post(&lock[1]);

        free(task);
    }
}

void init_queue(queue *q) {
    node *temp = malloc(sizeof(node));
    temp->next = NULL;
    q->head = q->tail = temp;
    sem_init(&q->head_lock, 0, 1);
    sem_init(&q->tail_lock, 0, 1);
}

void enqueue(queue *q, char *new_task) {
    node *temp = malloc(sizeof(node));
    assert(temp != NULL);
    strcpy(temp->value, new_task);
    temp->next = NULL;

    sem_wait(&q->tail_lock);
    q->tail->next = temp;
    q->tail = temp;
    sem_post(&q->tail_lock);
}

int dequeue(queue *q, char *value) {
    sem_wait(&q->head_lock);
    node *temp = q->head;
    node *new_head = temp->next;
    if (new_head == NULL) {
        sem_post(&q->head_lock);
        return -1;
    }
    strcpy(value, new_head->value);
    q->head = new_head;
    sem_post(&q->head_lock);
    free(temp);
    return 0;
}

void init_counter(counter *c) {
    c->value = 0;
    sem_init(&c->lock, 0, 1);
}

void increment(counter *c) {
    sem_wait(&c->lock);
    c->value++;
    sem_post(&c->lock);
}

void decrement(counter *c) {
    sem_wait(&c->lock);
    c->value--;
    sem_post(&c->lock);
}

int get(counter *c) {
    sem_wait(&c->lock);
    int rc = c->value;
    sem_post(&c->lock);
    return rc;
}