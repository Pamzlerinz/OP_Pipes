#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/**
 * Executes the command "cat scores | grep <argument> | sort".
 *
 *   P1 (parent)        : cat scores        -> writes into pipe1
 *   P2 (child)         : grep <argument>   -> reads pipe1, writes into pipe2
 *   P3 (child's child) : sort              -> reads pipe2, writes to stdout
 *
 * In this quick-and-dirty implementation the parent doesn't wait for the
 * children to finish and so the command prompt may reappear before they
 * terminate.
 */

int main(int argc, char **argv)
{
  int pipe1[2];   // cat  -> grep
  int pipe2[2];   // grep -> sort
  pid_t pid;

  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s <grep argument>\n", argv[0]);
      return 1;
    }

  char *cat_args[] = {"cat", "scores", NULL};
  char *grep_args[] = {"grep", argv[1], NULL};
  char *sort_args[] = {"sort", NULL};

  // make the first pipe (fds go in pipe1[0] and pipe1[1])

  if (pipe(pipe1) == -1)
    {
      perror("pipe");
      return 1;
    }

  pid = fork();

  if (pid < 0)
    {
      perror("fork");
      return 1;
    }

  if (pid == 0)
    {
      // child (P2) gets here and handles "grep <argument>"

      // make the second pipe, between grep and sort

      if (pipe(pipe2) == -1)
        {
          perror("pipe");
          exit(1);
        }

      pid = fork();

      if (pid < 0)
        {
          perror("fork");
          exit(1);
        }

      if (pid == 0)
        {
          // child's child (P3) gets here and handles "sort"

          // replace standard input with input part of pipe2

          dup2(pipe2[0], 0);

          // close all pipe ends this process doesn't need

          close(pipe1[0]);
          close(pipe1[1]);
          close(pipe2[0]);
          close(pipe2[1]);

          // execute sort

          execvp("sort", sort_args);
          perror("execvp sort");
          exit(1);
        }
      else
        {
          // P2 continues here and handles "grep <argument>"

          // replace standard input with input part of pipe1

          dup2(pipe1[0], 0);

          // replace standard output with output part of pipe2

          dup2(pipe2[1], 1);

          // close all original pipe ends (the copies on 0 and 1 remain)

          close(pipe1[0]);
          close(pipe1[1]);
          close(pipe2[0]);
          close(pipe2[1]);

          // execute grep

          execvp("grep", grep_args);
          perror("execvp grep");
          exit(1);
        }
    }
  else
    {
      // parent (P1) gets here and handles "cat scores"

      // replace standard output with output part of pipe1

      dup2(pipe1[1], 1);

      // close unused input half of pipe and the original output fd

      close(pipe1[0]);
      close(pipe1[1]);

      // execute cat

      execvp("cat", cat_args);
      perror("execvp cat");
      exit(1);
    }
}