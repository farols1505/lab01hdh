#include "kernel/types.h"
#include "user/user.h"

void sieve(int left_pipe_read_fd) __attribute__((noreturn));

void sieve(int left_pipe_read_fd)
{
    int my_prime;
    int num;
    int right_pipe[2];
    int child_pid = -1; 

    if (read(left_pipe_read_fd, &my_prime, sizeof(my_prime)) <= 0)
    {
        close(left_pipe_read_fd);
        exit(0);
    }

    printf("prime %d\n", my_prime);

    while (read(left_pipe_read_fd, &num, sizeof(num)) > 0)
    {
        if (num % my_prime != 0)
        {
            if (child_pid == -1)
            {
                pipe(right_pipe);
                child_pid = fork();

                if (child_pid == 0)
                {
                    close(right_pipe[1]);
                    close(left_pipe_read_fd);
                    sieve(right_pipe[0]);
                }
                else
                {
                    close(right_pipe[0]);
                }
            }
            write(right_pipe[1], &num, sizeof(num));
        }
    }

    close(left_pipe_read_fd);

    if (child_pid != -1)
    {
        close(right_pipe[1]);
        wait(0);
    }

    exit(0);
}

int main(int argc, char *argv[])
{
    int p[2];

    pipe(p);

    int pid = fork();

    if (pid == 0)
    {
        close(p[1]);
        sieve(p[0]);
    }
    else
    {
        close(p[0]);

        for (int i = 2; i <= 280; i++)
        {
            write(p[1], &i, sizeof(i));
        }

        close(p[1]);
        wait(0);
    }

    exit(0);
}