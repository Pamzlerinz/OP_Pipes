// C program to demonstrate use of fork() and pipe() with a 2-way pipe
//
// P1 (parent) reads a string and sends it to P2 (child) over pipe fd1.
// P2 concatenates "howard.edu", prints it, reads a second string,
// appends it, and sends the result back to P1 over pipe fd2.
// P1 concatenates "gobison.org" and prints the final string.
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>

#define BUF_SIZE 256   // every message sent through a pipe is exactly BUF_SIZE bytes

// Write exactly BUF_SIZE bytes to fd. Returns 0 on success, -1 on error.
static int write_msg(int fd, const char *buf)
{
    size_t total = 0;
    while (total < BUF_SIZE)
    {
        ssize_t n = write(fd, buf + total, BUF_SIZE - total);
        if (n <= 0)
            return -1;
        total += (size_t)n;
    }
    return 0;
}

// Read exactly BUF_SIZE bytes from fd and guarantee null-termination.
// Returns 0 on success, -1 on error or early end of file.
static int read_msg(int fd, char *buf)
{
    size_t total = 0;
    while (total < BUF_SIZE)
    {
        ssize_t n = read(fd, buf + total, BUF_SIZE - total);
        if (n <= 0)
            return -1;
        total += (size_t)n;
    }
    buf[BUF_SIZE - 1] = '\0';
    return 0;
}

int main()
{
    // We use two pipes
    // First pipe (fd1): P1 (parent) -> P2 (child)
    // Second pipe (fd2): P2 (child) -> P1 (parent)

    int fd1[2];  // Used to store two ends of first pipe
    int fd2[2];  // Used to store two ends of second pipe

    char fixed_str[] = "howard.edu";
    char fixed_str2[] = "gobison.org";
    char input_str[BUF_SIZE];
    pid_t p;

    if (pipe(fd1) == -1)
    {
        fprintf(stderr, "Pipe Failed\n");
        return 1;
    }
    if (pipe(fd2) == -1)
    {
        fprintf(stderr, "Pipe Failed\n");
        return 1;
    }

    memset(input_str, 0, sizeof(input_str));
    printf("Input : ");
    fflush(stdout);
    if (scanf("%255s", input_str) != 1)
    {
        fprintf(stderr, "No input\n");
        return 1;
    }
    fflush(stdout);   // avoid duplicated buffered output after fork

    p = fork();

    if (p < 0)
    {
        fprintf(stderr, "fork Failed\n");
        return 1;
    }

    // Parent process (P1)
    else if (p > 0)
    {
        // Close unused ends immediately after fork
        close(fd1[0]);  // P1 only writes to fd1
        close(fd2[1]);  // P1 only reads from fd2

        // Send input string to P2, then close write end
        if (write_msg(fd1[1], input_str) == -1)
        {
            fprintf(stderr, "Write Failed\n");
            close(fd1[1]);
            close(fd2[0]);
            wait(NULL);
            return 1;
        }
        close(fd1[1]);

        // Wait for P2 to finish its work
        wait(NULL);

        // Read the string sent back by P2, then close read end
        char result_str[BUF_SIZE];
        memset(result_str, 0, sizeof(result_str));
        if (read_msg(fd2[0], result_str) == -1)
        {
            fprintf(stderr, "Read Failed\n");
            close(fd2[0]);
            return 1;
        }
        close(fd2[0]);

        // Concatenate "gobison.org" and print
        strncat(result_str, fixed_str2, sizeof(result_str) - strlen(result_str) - 1);
        printf("Output : %s\n", result_str);
    }

    // Child process (P2)
    else
    {
        // Close unused ends immediately after fork
        close(fd1[1]);  // P2 only reads from fd1
        close(fd2[0]);  // P2 only writes to fd2

        // Read a string using first pipe, then close read end
        char concat_str[BUF_SIZE];
        memset(concat_str, 0, sizeof(concat_str));
        if (read_msg(fd1[0], concat_str) == -1)
        {
            fprintf(stderr, "Read Failed\n");
            close(fd1[0]);
            close(fd2[1]);
            exit(1);
        }
        close(fd1[0]);

        // Concatenate "howard.edu" and print
        strncat(concat_str, fixed_str, sizeof(concat_str) - strlen(concat_str) - 1);
        printf("Output : %s\n", concat_str);

        // Prompt for a second input and append it
        char second_str[BUF_SIZE];
        memset(second_str, 0, sizeof(second_str));
        printf("Input : ");
        fflush(stdout);
        if (scanf("%255s", second_str) != 1)
            second_str[0] = '\0';   // treat missing input as empty string
        strncat(concat_str, second_str, sizeof(concat_str) - strlen(concat_str) - 1);

        // Send the string back to P1, then close write end
        if (write_msg(fd2[1], concat_str) == -1)
        {
            fprintf(stderr, "Write Failed\n");
            close(fd2[1]);
            exit(1);
        }
        close(fd2[1]);

        exit(0);
    }

    return 0;
}