#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_CHILDREN 20
#define ZOMBIE_TIMEOUT_MS 2000
#define POLL_INTERVAL_MS 5
#define WORKER_PATH "./proclab-worker"
#define WORKER_NAME "proclab-worker"

typedef struct {
  pid_t pid;
  int expected_exit;
} Child;

// Provided argument-handling code. Read and understand it before continuing.
static void print_usage(FILE *out, const char *program) {
  fprintf(out, "Usage: %s <N>\n", program);
}

static int parse_child_count(const char *text, int *count_out) {
  errno = 0;
  char *end = NULL;
  long value = strtol(text, &end, 10);

  if (errno != 0 || end == text || *end != '\0' || value < 0 ||
      value > MAX_CHILDREN) {
    return 0;
  }

  *count_out = (int)value;
  return 1;
}

int main(int argc, char **argv) {
  // Keep parent output in deterministic order, even when redirected to a file.
  setvbuf(stdout, NULL, _IONBF, 0);

  int child_count;
  if (argc != 2 || !parse_child_count(argv[1], &child_count)) {
    print_usage(stderr, argv[0]);
    return 1;
  }

  if (child_count == 0) {
    printf("DONE spawned=0 reaped=0\n");
    return 0;
  }

  /*
   * TODO: Implement the process lifecycle described in the project prompt.
   *
   * 1. Allocate storage for child records.
   * 2. Fork child_count children.
   * 3. In each child, exec WORKER_PATH with exit code 10 + index.
   * 4. In the parent, store and report each child in index order.
   * 5. Observe every child as a WORKER_NAME zombie through /proc.
   * 6. Reap every child with waitpid and report its decoded exit status.
   * 7. Print DONE and release all allocated memory.
   * 8. On any failure, report the error and reap every child already created.
   *
   * Choose your own helper functions and decomposition. The tutorial reference
   * in the prompt explains each required system interface.
   */

  Child *children = malloc((size_t)child_count * sizeof(*children));
  if(children == NULL) {
    perror("malloc");
    return 1;
  }
  int spawned=0;

  for(int i = 0; i < child_count; i++) {
    pid_t pid = fork();
    if (pid == -1) {
    perror("fork");
    break;
    }

    if (pid == 0) {
      char exit_code[16];

      snprintf(exit_code, sizeof(exit_code), "%d", 10 + i);
      execl( WORKER_PATH, WORKER_NAME, exit_code, (char *)NULL);

      perror("execl");
      _exit(127);
    }

    children[spawned].pid = pid;
    children[spawned].expected_exit = 10 + i;
    printf("SPAWN index=%d pid=%ld exit=%d\n", i, (long)pid, children[spawned].expected_exit);
    spawned++;
  }


  for(int i = 0; i < spawned; i++) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(;;) {
      FILE *file_stream;
      char path[64];
      long process_pid;
      char process_name[256];
      char process_state;
      
      snprintf(path, sizeof(path), "/proc/%ld/stat",  (long)children[i].pid);

      file_stream = fopen(path, "r");

      if (file_stream != NULL) {
        int fields = fscanf(file_stream, "%ld (%255[^)]) %c", &process_pid, process_name, &process_state);
        fclose(file_stream);

        if (fields == 3 &&
            process_pid == (long)children[i].pid &&
            strcmp(process_name, WORKER_NAME) == 0 &&
            process_state == 'Z') {
          printf("ZOMBIE pid=%ld name=%s state=%c\n", process_pid, process_name, process_state);
          break;
        }
      }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L + (now.tv_nsec - start.tv_nsec) / 1000000L;
    
    if (elapsed_ms > ZOMBIE_TIMEOUT_MS) {
      perror("Time exceeded looking for zombie");
      break;
    }

    struct timespec delay = {
      .tv_sec = 0,
      .tv_nsec = POLL_INTERVAL_MS * 1000000L
    };

    nanosleep(&delay, NULL);

    }  
  }
    
    
  int reaped = 0;
  for (int i = 0; i < spawned; i++) {
    int status = 0;
    pid_t reapedPID = waitpid(children[i].pid, &status, 0);

    if (reapedPID == -1) {
      perror("waitpid");
      continue;
    }

    int code = -1;
    if (reapedPID == children[i].pid && WIFEXITED(status)) {
      code = WEXITSTATUS(status);
    }

    printf("REAP index=%d pid=%ld exit=%d\n", i, (long)reapedPID, code);
    reaped++;
  }
  printf("DONE spawned=%d reaped=%d\n", spawned, reaped);
  free(children);
}
