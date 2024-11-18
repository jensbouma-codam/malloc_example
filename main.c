/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   main.c                                             :+:    :+:            */
/*                                                     +:+                    */
/*   By: jensbouma <jensbouma@student.codam.nl>       +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/11/18 09:36:14 by jensbouma     #+#    #+#                 */
/*   Updated: 2024/11/18 12:15:33 by jensbouma     ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>

bool 	g_running = true;
char	*g_str = NULL;
const unsigned long long	gigabyte = 1ULL << 30;
const unsigned long long	megabyte = 1ULL << 20;
const char					indicator[4] = "|/-\\";
int							x = 0;

// Function to set terminal to raw mode
void	enablerawmode(void)
{
	struct termios term;

    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Function to reset terminal to normal mode
void	disablerawmode(void)
{
	struct termios	term;

	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag |= (ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void	signal_handler(int signum)
{
	g_running = false;
}

void	allocate(int allocated, int zeroed, int overallocate, bool cal)
{
	if (g_str)
		free(g_str);

	printf("\033[9;1H");
	if (cal)
	{
		printf("Allocating %.2f GB with calloc ", (float) allocated * gigabyte / 1024 / 1024 / 1024);
		g_str = calloc(sizeof(char), (gigabyte * allocated));
	}
	else
	{
		printf("Allocating %.2f GB with malloc ", (float) allocated * gigabyte / 1024 / 1024 / 1024);
		g_str = malloc(sizeof(char) * (gigabyte * allocated));
	}

	if (!g_str)
	{
		printf("failed\n");
		exit(EXIT_FAILURE);
	}
	printf("successful\n");

	if (zeroed > 0)
	{
		printf("\033[11;1H");
		if (overallocate > 0)
			printf("Overallocating %.2f GB with %d byte\n", (float) allocated * gigabyte / 1024 / 1024 / 1024, overallocate);
		else
			printf("Zeroing %.2f GB                                                               ", (float) gigabyte * allocated / 1024 / 1024 / 1024);
		
		unsigned long long i = 0;
		while (i < (sizeof(char) * gigabyte * allocated) + (sizeof(char) * overallocate))
		{
			if ((i) % (1ULL << 20) == 0)
			{
				printf("\033[6;1H");
				printf("%c  ", indicator[x % 4]); // Display spinner
				x++;
				
				printf("\033[12;1H");
				printf("%.2f GB zeroed\n", (float)(i) / 1024 / 1024 / 1024);
			}
			if (overallocate && allocated > 0 && i >= sizeof(char) * gigabyte * allocated)
			{
				printf("\033[12;1H");
				printf("%.2f GB over allocated with %llu byte\n", (float)(i) / 1024 / 1024 / 1024, (i - (allocated * gigabyte)) + 1);
				usleep(100000);
			}
			g_str[i] = 0;
			i++;
		}
	}
	else
	{
		printf("\033[11;1H");
		printf("Memory not zeroed                                                             ");
		printf("\033[12;1H");
		printf("                                                                              ");
	}
}

int	main()
{
	int							allocated;
	int							overallocate;
	bool						zeroed;
	char						c;
	char						buffer[3];

	allocated = 0;
	overallocate = 0;
	zeroed = false;
	signal(SIGINT, signal_handler);

	printf("\033[H\033[J");

	allocate(allocated, zeroed, overallocate, false);

	printf("\033[1;10H");
	printf("Press 'q' to quit\n");
	printf("\033[2;10H");
	printf("Press 'up' to allocate more memory\n");
	printf("\033[3;10H");
	printf("Press 'down' to deallocate memory\n");
	printf("\033[4;10H");
	printf("Press 'z' to enable zeroing memory\n");
	printf("\033[5;10H");
	printf("Press 'a' to disable zeroing\n");
	printf("\033[6;10H");
	printf("Press 'x' to overallocate memory with 1MB\n");
	printf("\033[7;10H");
	printf("Press 'c' to use calloc instead of malloc\n");

	enablerawmode();

	while (g_running) {
        printf("\033[6;1H");
        printf("%c  ", indicator[x % 4]); // Display spinner
        x++;

		fflush(stdout);

        struct timeval tv;
        fd_set readfds;

        tv.tv_sec = 0;
        tv.tv_usec = 50000; // 100 milliseconds

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select()");
            break;
        }
		else if (retval > 0) {
			ssize_t n = read(STDIN_FILENO, buffer, 3);

            if (n > 0) {
                if (buffer[0] == '\033' && buffer[1] == '[') {
                    switch (buffer[2]) {
                        case 'A': // Up Arrow
                            if (allocated < 1024 * 1024) allocated++;
                            break;
                        case 'B': // Down Arrow
                            if (allocated > 0) allocated--;
                            break;
                        case 'C': // Right Arrow
                            if (allocated < 1024 * 1024) allocated += 1024;
                            break;
                        case 'D': // Left Arrow
                            if (allocated >= 1024) allocated -= 1024;
                            break;
					}
				allocate(allocated, zeroed, overallocate, false);
				}
				else if (buffer[0] == 'z')
				{
					zeroed = true;
					allocate(allocated, zeroed, overallocate, false);
				}
				else if (buffer[0] == 'a')
				{
					zeroed = false;
					allocate(allocated, zeroed, overallocate, false);
				}
				else if (buffer[0] == 'q')
				{
					g_running = false; // Quit on 'q'
				}
				else if (buffer[0] == 'x' && allocated > 0)
				{
					overallocate = 1;
					allocate(allocated, zeroed, overallocate, false);
				}
				else if (buffer[0] == 'c')
				{
					allocate(allocated, zeroed, overallocate, true);
				}
			}
		}
        printf("\033[7;1H");
    }

	disablerawmode();
	printf("Freeing memory\n");
	free(g_str);
	return (EXIT_SUCCESS);
}
