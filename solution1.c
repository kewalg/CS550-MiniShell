#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100
size_t MAX_LINE_LEN = 10000;


// builtin commands
#define EXIT_STR "exit"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99


FILE *fp, *fp2, *fp3; // file struct for stdin and stdout
char **tokens;
char *line, *input_array[100], file_path[100], string_input[100];
int status,token_count,p_arr[50],counter=0,f_ground=0, pipe_cmd = 0;
pid_t pid;
int k_counter = 0, pipe_no = 0;
static char cmd[1000];


void handler()
{
	int r = waitpid(pid,&status,WNOHANG);
}



void initialize()
{
	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// open stdin as a file pointer 
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);
}

// add null character at end and remove whitespaces
char* trim_char(char *string_input)
{
	while(isspace(*string_input)){
		string_input++;
	} 
	string_input[strlen(string_input)-1] = '\0';
	return string_input;
}

void tokenize (char * string)
{
	token_count = 0;
	int size = MAX_TOKENS;
	char *this_token, *pipe_token;
	pipe_cmd = 0;
	pipe_no = 0;

	char* get_position = strchr(string, '|');
	if (get_position!=NULL)
	{
		pipe_cmd = 1;  // flag is set
		char *first_pipe[50];
		while((pipe_token = strsep(&string, "|" ))!= NULL) {
			//trim charadcters and setting current token to first_pipe
			tokens[token_count] = trim_char(pipe_token);
			*first_pipe = tokens[token_count];
			token_count++;
			pipe_no++;
		}
	}

	while ((this_token = strsep( &string, " \t\v\f\n\r")) != NULL)
	{

		if (*this_token == '&')
		{
			//flag for background process
			f_ground = 0;
			break;
		}
			//flag for pipes process
		else if(*this_token == '|'){
			pipe_cmd = 1;
		}
		else
		{
			//flag for foreground process
			f_ground = 1;
		} 

		if (*this_token == '\0') continue;
		tokens[token_count] = this_token;
			//printf("Token %d: %s\n", token_count, tokens[token_count]);
		token_count++;
			//if there are more tokens than space ,reallocate more space
		if(token_count >= size)
		{
			size*=2;
			assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
		}
	}
	tokens[token_count] = NULL;
}



void read_command() 
{
	//getline will reallocate if input exceeds max length
	assert( getline(&line, &MAX_LINE_LEN, fp) > -1); 
	//printf("Shell read this line: %s\n", line);
	tokenize(line);
}
 

int kill(pid_t pp,int sig);

void sighandler(){
	printf("ntrl C has been pressed!\n");
}

int run_command()
{

	char *this_token;
	if(token_count==0){
		return UNKNOWN_CMD;	
	}
	if (strcmp(tokens[0], EXIT_STR ) == 0)
	{
		return EXIT_CMD;
	}

	//For "pipe" process
	else if(pipe_cmd == 1){

		//check once
		int pipe_file_desc[pipe_no],j=0,status,final_length_cmd = MAX_TOKENS;
		pid_t pid;
		char **cmd;
		assert( ( cmd = malloc( sizeof( char* )*MAX_TOKENS ) ) != NULL );
		for (int i = 0; i<pipe_no; i++)
		{
			if (pipe(pipe_file_desc + i*2)< 0)
			{
				printf("Error\n");
				exit(1);
			}
		}

		for (int i = 0; i < pipe_no; i++)
		{
			int k=0;
			while( ( this_token = strsep( &tokens[i], " \t\v\f\n\r'&'" ) ) != NULL ) 
			{
				cmd[k] = this_token;
				k++;
				if( final_length_cmd < k ) 
				{
					final_length_cmd = final_length_cmd * 2;
				}
			}	

			pid = fork();
			if (pid == 0)
			{

				if (tokens[i+1])
				{
					if (dup2(pipe_file_desc[j+1], 1) < 0)
					{
						printf("error in dup2\n");
						exit(1);
					}
				}

				if(j!= 0)
				{
						if(dup2(pipe_file_desc[j-2], 0) < 0) 
						{
							printf("error in dup2\n");
							exit(1);
						}
				}

				for(i = 0; i < pipe_no; i++)
				{
					close(pipe_file_desc[i]);
				}

				if( execvp(cmd[0], cmd) < 0 )
				{
					perror(cmd[0]);
					exit(1);
				}
			}
			else if(pid<0)
			{
				printf("Error");
				exit(1);
			}
			j = j+2;
		}

		//closing pipes
		for(int i = 0; i < pipe_no; i++)
		{
			close(pipe_file_desc[i]);
		}

		//adding wait symbol
		for(int i = 0; i < pipe_no + 1; i++)
		{
			wait(&status);
		}
	}

	//For "fg" process
	else if(strcmp(tokens[0], "fg")== 0){
		if (tokens[1]==NULL)
		{
			printf("Cannot execute without any background tasks.\n");
		}
		else{
		//making sure pid is int 
		int fg_pid = atoi(tokens[1]);
	    int fg_status;
	    //make process mentioned by user to wait
	    pid_t fg_return = waitpid(fg_pid,&fg_status,0);
	    //continue signal for that particular pid
	    int fg_rtrn = kill(fg_return, SIGCONT);
    	}
	}

    //For "listjobs" process
	else if(strcmp(tokens[0], "listjobs")==0){
		int e;
	 	for (int i=0;i<=counter;i++)
	 	{
			e = kill(p_arr[i],0);
			if (e==-1)
			{
				printf("Process with PID = %d  is Finished\n", p_arr[i]);	
			}
			else if(e == 0)
			{	
				if (p_arr[i]==0)
				{
					//printf("ssh550 is Running\n");
				}else
					{
						printf("Process with PID = %d  is Running\n", p_arr[i]);
					}
			}
		}	
	}

	//For reading input from a file
	else if(tokens[1]!=NULL && strcmp(tokens[1], "<")==0){
		fp2 = fopen(tokens[2], "r");

		if (fp2 == NULL) {
			printf("Can't open input file");
			exit(1);
		} else {
			if( fgets( string_input, 30, fp2 ) != NULL ) {
				//adding the tokens to a seperate array and passing it as input to the execvp
				input_array[0] = tokens[0];
				input_array[1] = string_input;

				pid = fork();
				if( pid == 0 ) {
					//parent process
					int read_status = execvp( tokens[0], input_array );
					if( read_status < 0 ) {
						printf("Command not found!\n");
					}
					exit(1);
				} else if( pid > 0 ) {
					//child process
					signal( SIGCHLD, SIG_DFL );
					wait(NULL);
				} else if( pid < 0 ) {
					//fork failure
					printf("Fork error!\n");
				}
			}
			fclose(fp2); 
		}
	}
	
	//For writing output to a file
	else if(tokens[1]!=NULL && strcmp(tokens[1], ">")==0){
		//reading file for command
		fp2 = popen(tokens[0], "r");
		if (fp2 == NULL) {
			printf("Unable to run the command!" );
			exit(1);
		}
		//writing to a file
		fp3 = fopen (tokens[2], "w");
		while (fgets(file_path, sizeof(file_path), fp2) != NULL) {
			fprintf (fp3, "%s", file_path);
		}
		pclose(fp2);
		pclose(fp3);
	}

	//For foregorund processes
	else if(f_ground == 1){
		int exvp;
		pid=fork();
		if(pid == 0)
		{
			exvp = execvp(tokens[0],tokens);		
			if(exvp < 0)
			{
				printf("Exec failed\n");
			}
			exit(1);
		}
		else if(pid > 0)
		{
			signal(SIGINT, sighandler);

			p_arr[counter]=pid;
			counter++;
			exvp = wait(NULL);
			if(exvp == -1)
			{
				//printf("Wait failed\n");
			}
	
		}
		else if(pid < 0)
		{
			printf("Error in creating process\n");
		}
	}

	//For background processes
	else if(f_ground == 0){
		int exvp_bg;
		pid=fork();

		if (pid==0)
		{
			exvp_bg = execvp(tokens[0],tokens);
			if (exvp_bg<0)
			{
				printf("Execution failed for background process\n");
			}
			exit(1);
		}
		if(pid>0)
		{
			p_arr[counter]=pid;
			counter++;
				printf("Process %d running in background\n", pid);
				signal(SIGCHLD, SIG_IGN);
		}
			if(pid<0)
			{
				printf("Fork failed for background process\n");
			}
	}
	return UNKNOWN_CMD;
}



int main(){
	initialize();
	do 
	{
		printf("sh550> ");
		signal(SIGCHLD, handler);  
		read_command();
	}while(run_command() != EXIT_CMD);
	return 0;
}
