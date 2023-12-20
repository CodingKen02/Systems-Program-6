/*
----------------------------------------------------------
Program: program6.c
Date: October 3, 2023
Student Name & NetID: Kennedy Keyes kfk38
Description: This program creates a parent-child process structure.
The parent process handles various signals, including SIGCHLD, SIGINT, 
SIGUSR1, and SIGUSR2. The child process reads data from the "angl.dat" 
file and sends signals to the parent based on certain conditions. 
----------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <math.h>
#include <sys/wait.h>

#define RANGE_LOWER -20.0
#define RANGE_UPPER 20.0

pid_t child_pid = 0;

// handles errors
int testError(int val, const char *msg)
{
    if (val == -1)
    {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return val;
}
void childSignalHandler(int signo)
{
    if (signo == SIGINT)
    {
        exit(EXIT_SUCCESS);
    }
    else if (signo == SIGTERM)
    {
        exit(EXIT_SUCCESS);
    }
}

void parentSignalHandler(int signo)
{
    if (signo == SIGCHLD)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        if (child_pid == 0)
        {
            write(STDOUT_FILENO, "Terminated all children. Exiting parent.\n", sizeof("Terminated all children. Exiting parent.\n"));
            exit(EXIT_SUCCESS);
        }
    }
    else if (signo == SIGINT)
    {
        char response;
        write(STDOUT_FILENO, "Exit: Are you sure (Y/n)? ", sizeof("Exit: Are you sure (Y/n)? "));
        ssize_t bytesRead = read(STDIN_FILENO, &response, 1);
        if (bytesRead > 0 && (response == 'Y' || response == 'y'))
        {
            kill(child_pid, SIGTERM);
            exit(EXIT_SUCCESS);
        }
    }
    else if (signo == SIGUSR1)
    {
        write(STDOUT_FILENO, "Warning! Roll outside of bounds\n", sizeof("Warning! Roll outside of bounds\n"));
    }
    else if (signo == SIGUSR2)
    {
        write(STDOUT_FILENO, "Warning! Pitch outside of bounds\n", sizeof("Warning! Pitch outside of bounds\n"));
    }
}

int main(int argc, char *argv[])
{
    struct sigaction sa_parent;
    sa_parent.sa_handler = parentSignalHandler;
    sigemptyset(&sa_parent.sa_mask);
    sa_parent.sa_flags = 0;
    sigaction(SIGCHLD, &sa_parent, NULL);
    sigaction(SIGINT, &sa_parent, NULL);
    sigaction(SIGUSR1, &sa_parent, NULL);
    sigaction(SIGUSR2, &sa_parent, NULL);

    // creates/forks a child proces and stores its PID
    child_pid = fork();

    if (child_pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0)
    {
        write(STDOUT_FILENO, "Child process started (PID: ", sizeof("Child process started (PID: "));
        char child_pid_str[10];
        snprintf(child_pid_str, sizeof(child_pid_str), "%d", getpid());
        write(STDOUT_FILENO, child_pid_str, sizeof(child_pid_str));
        write(STDOUT_FILENO, ")\n", sizeof(")\n"));

        struct sigaction sa_child;
        sa_child.sa_handler = childSignalHandler;
        sigemptyset(&sa_child.sa_mask);
        sa_child.sa_flags = 0;
        sigaction(SIGINT, &sa_child, NULL);
        sigaction(SIGTERM, &sa_child, NULL);

        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;

        int fd, rd;
        double roll, pitch, yaw;

        fd = testError(open("angl.dat", O_RDONLY), "open");

        while (1)
        {
            rd = testError(read(fd, &roll, sizeof(double)), "read");
            rd = testError(read(fd, &pitch, sizeof(double)), "read");
            rd = testError(read(fd, &yaw, sizeof(double)), "read");

            if (rd <= 0)
            {
                close(fd);
                exit(EXIT_SUCCESS);
            }

            if (roll < RANGE_LOWER || roll > RANGE_UPPER)
            {
                kill(getppid(), SIGUSR1);
            }

            if (pitch < RANGE_LOWER || pitch > RANGE_UPPER)
            {
                kill(getppid(), SIGUSR2);
            }

            nanosleep(&ts, NULL);
        }
    }
    else
    {
        while (1)
        {
            pause();
        }

        kill(child_pid, SIGTERM);
    }

    return EXIT_SUCCESS;
}
