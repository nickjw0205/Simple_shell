#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#define MAX_LINE 80 /* The maximum length command */

int main(void){

	// user input array
	char input[MAX_LINE];
	// array that tokenized command will be stored
	char *args[MAX_LINE/2 + 1];
	//for while condition
	int should_run = 1;
	// 0: background 1: frountground
	int wait_num = 1;
	// 0 : not use pipe 1: use pipe
	int usingpipe = 0;

	pid_t pid;

	while (should_run){
		// reset the arrays
		for(int i = 0; i < (MAX_LINE/2 + 1); i++) args[i] = NULL;
		for(int i = 0; i < MAX_LINE; i++) input[i] = '\0';

		printf("osh>");
		fflush(stdout);

		// get input and erase "\n"
		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1] = '\0';

		// loc : length of args
		int loc = 0;

		// tokenize input
		char *ptr = strtok(input, " ");

		while(ptr != NULL){
			args[loc] = ptr;
			loc++;
			ptr = strtok(NULL, " ");
		}

		// if input is "exit" exit the shell
		if(strncmp(args[0], "exit", strlen("exit"))==0){
			should_run = 0;
			break;
		}

		// if last character of array is "&" use background
		if(strcmp(args[loc - 1], "&")==0){
			wait_num = 0;
			args[loc - 1] = NULL;
			loc--;
		}

		pid = fork();

		//fork error
		if(pid < 0){
			printf("Fork Failed.\n");
			exit(1);
		}

		//child process
		else if(pid == 0){

			//for redirection..
			int in, out;

			for(int i = 0; i < loc; i++){
				// when use redirection ">"
				if(strcmp(args[i], ">")==0){
					out = open(args[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					dup2(out, 1);
					close(out);
					args[i] = NULL;
					args[i+1] = NULL;
					break;
				}

				// when use redirection "<"
				else if(strcmp(args[i], "<")==0){
					in = open(args[i+1], O_RDONLY);
					dup2(in, 0);
					close(in);
					args[i] = NULL;
					args[i+1] = NULL;
					break;
				}

				// when use pipe
				else if(strcmp(args[i], "|")==0){
					// for communicate child and parents
					int fd[2];

					// pipe error
					if(pipe(fd) == -1){
						fprintf(stderr, "Pipe Failed\n");
						return 1;
					}

					// left side of "|"
					char *leftcommand[i+1];

					// right side of "|"
					char *rightcommand[loc - i + 1];

					// set use pipe
					usingpipe = 1;

					// store left side
					for(int j = 0; j < i; j++){
						leftcommand[j] = args[j];
					}
					leftcommand[i] = NULL;

					// store right side
					for(int j = 0; j < loc - i + 1; j++){
						rightcommand[j] = args[j + i + 1];
					}
					rightcommand[loc - i - 1] = NULL;

					// fork
					int pipe_pid = fork();

					// pipe error
					if(pipe_pid < 0){
						printf("pipe fork failed");
						return 1;
					}

					// child process
					else if(pipe_pid == 0){
						close(fd[0]);
						dup2(fd[1], STDOUT_FILENO);
						close(fd[1]);

						// execute left side command
						if(execvp(leftcommand[0], leftcommand) == -1){
							printf("Invaild Command\n");
							return 1;
						}
						exit(1);
					}

					// parents process
					else{
						// wait to finsh child process
						wait(NULL);
						close(fd[1]);
						dup2(fd[0], STDIN_FILENO);
						close(fd[0]);

						// execute right side
						if(execvp(rightcommand[0], rightcommand) == -1){
							printf("Invaild Command\n");
							return 1;
						}
					}

					// close pipe
					close(fd[0]);
					close(fd[1]);
					break;
				}
			}

			// if we didn't use pipe
			if(usingpipe == 0){

				// execute commands inputed
				if(execvp(args[0], args) == -1) printf("Invaild Command\n");
			}
			exit(1);
		}

		// parents process
		else{
			if(wait_num == 1) wait(NULL);
		}
	}

	return 0;
}
