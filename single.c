#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>

typedef struct Node {
    char value[250];
    struct Node *next;
}   node;

typedef struct Queue {
    node *head;
    node *tail;
}   queue;

void init_queue(queue *q);
void enqueue(queue *q, char *new_task);
int dequeue(queue *q, char *value);

int main(int argc, char *argv[]) {
    // Initialize queue
    queue *task_queue = malloc(sizeof(queue));
    init_queue(task_queue);

    // Enqueue rootpath
    char *rootpath = (char *) malloc(250*sizeof(char));
    realpath(argv[2], rootpath);
    enqueue(task_queue, rootpath);

    int i;
    char *task = (char *) malloc(250*sizeof(char));
    while ((i = dequeue(task_queue, task)) == 0) {
        printf("[0] DIR %s\n", task);

        DIR *directory = opendir(task);
        
        struct dirent *child_object;
        child_object = readdir(directory);
        while (child_object != NULL) {
            // Ignore . and .. file objects
            if ((child_object->d_type == DT_DIR && strcmp(child_object->d_name, ".") == 0) 
                || (child_object->d_type == DT_DIR && strcmp(child_object->d_name, "..") == 0)) {
                child_object = readdir(directory);
                continue;
            } 

            // Form the absolute path of child object
            char *path = (char *) malloc(250*sizeof(char));
            realpath(task, path);
            if (strcmp(task, "/") != 0) strcat(path, "/");
            strcat(path, child_object->d_name);

            if (child_object->d_type == DT_DIR) {
                // Enqueue directory to task queue
                enqueue(task_queue, path);
                printf("[0] ENQUEUE %s\n", path);
            } else if (child_object->d_type == DT_REG) {
                // Form the grep command string to be invoked by system()
                int cmd_length = strlen(argv[3]) + strlen(path) + 19;
                char *command = (char *) malloc(cmd_length*sizeof(char));
                strcpy(command, "grep ");
                strcat(command, argv[3]);
                strcat(command, " ");
                strcat(command, path);
                strcat(command, " > /dev/null");

                // Perform grep on file
                int ret_val = system(command);
                if (ret_val == 0) printf("[0] PRESENT %s\n", path);
                else printf("[0] ABSENT %s\n", path);

                free(command);
            }
            
            free(path);
            child_object = readdir(directory);
        }

        closedir(directory);
    }

    free(task);
    free(rootpath);
    if (task_queue->head != NULL) free(task_queue->head);
    free(task_queue);
    return 0;
}

void init_queue(queue *q) {
    node *temp = malloc(sizeof(node));
    temp->next = NULL;
    q->head = q->tail = temp;
}

void enqueue(queue *q, char *new_task) {
    node *temp = malloc(sizeof(node));
    assert(temp != NULL);
    strcpy(temp->value, new_task);
    temp->next = NULL;
    q->tail->next = temp;
    q->tail = temp;
}

int dequeue(queue *q, char *value) {
    node *temp = q->head;
    node *new_head = temp->next;
    if (new_head == NULL) {
        return -1; // queue was empty
    }
    strcpy(value, new_head->value);
    q->head = new_head;
    free(temp);
    return 0; // dequeue success
}