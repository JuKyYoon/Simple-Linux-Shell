#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>


typedef enum boolean{false, true} boolean;
#define MAX_LEN 1024
struct passwd *lpwd;

char *history[1000]; // history command
int progress_number = 0; // history index
int files[100]; // pipeline
int file_flag = O_CREAT|O_WRONLY|O_TRUNC; // Default File Flag 
int history_flag = 1; // If the value is 0, history is not written
int block_break = 0;

char *get_command(char *command);

int default_exec(int argc, char *args[], char filename[], char MODE[], int back_flag, char *block, int noclobber);

int history_exec(char *history[], int progress_number);
int cd_command(int argc, char *args[]);
int history_command(int argc, char *args[]);
int set_noclobber(int argc, char *args[], int *noclobber);

void redirect_output(int fd);
void redirect_input(int fd);
int my_exec(char *args[], int argc);
void return_stdin(char *args);

void error_exit(char *error_msg);
void prompt(struct passwd *lpwd, int main_pid);

int main(){
    //init
    
    char *buffers = (char*)malloc(MAX_LEN*sizeof(char)); // Command Buffer
    char *block = (char*)malloc(MAX_LEN*sizeof(char)); // Command Group Buffer
    char *args[MAX_LEN];
    char *state;
    char file_name[100]; 
    char MODE[20];
    int status = 0;
    int backflag = 0; // background FLAG ( 1 : Background )
    int main_pid = getpid(); // Main process ID
    int noclobber = 1; // Clobber FLAG ( 1 : Overwrite ) 
    history[progress_number] = (char*)malloc(100*sizeof(char));
    boolean no_exit = true; 
    int argc = 0; 
    lpwd = getpwuid(getuid());
    prompt(lpwd, main_pid);
    while(no_exit){
        state = get_command(buffers);
        
        if(!strcmp(state,"EOF")){ break;} 

        if( !strcmp(state,"COMMAND")){ 
            args[argc] = (char*)malloc(MAX_LEN*sizeof(char)); 
            strcpy(args[argc++], buffers);
            if( history_flag){ 
                strcat(history[progress_number], buffers); 
                strcat(history[progress_number], " "); // Insert a space
            }
        }

        else if( !strcmp(state,"BLOCK")){ 
            args[argc] = (char*)malloc(MAX_LEN*sizeof(char)); 
            strcpy(block, buffers);
            strcat(block, ";");
            strcpy(args[argc++], "BLOCK command"); 
            if(history_flag){
                strcat(history[progress_number], "("); 
                strcat(history[progress_number], buffers); 
                strcat(history[progress_number], ")"); 
            }

        }

        else if (!strcmp(state,"EXIT_REDIRECT_PLUS") || !strcmp(state, "EXIT_REDIRECT") || !strcmp(state,"EXIT_REDIRECT_PLUS2") || !strcmp(state,"EXIT_REDIRECT_STDIN")){
            if( !strcmp(state,"EXIT_REDIRECT_PLUS") ){
                strcpy(MODE, ">>");
            }
            else if( !strcmp(state,"EXIT_REDIRECT_PLUS2") ){
                strcpy(MODE, ">|");
            }
            else if( !strcmp(state,"EXIT_REDIRECT") ){
                strcpy(MODE, ">");
            }
            else if( !strcmp(state,"EXIT_REDIRECT_STDIN") ){
                strcpy(MODE,"<");
            }
            else{
                strcpy(MODE, "NOMODE");
            }
            args[argc] = 0; // NULL
            
            get_command(file_name);

            if(history_flag){
                strcat(history[progress_number], MODE); 
                strcat(history[progress_number], " "); 
                strcat(history[progress_number], file_name); 
            }
            

        }

        else if( !strcmp(state,"EXIT_C") || !strcmp(state,"EXIT_CENTER") || !strcmp(state, "EXIT_BACKGROUND") || !strcmp(state, "EXIT_PIPE") ){      
            if( argc==0 ){ 
                if( !strcmp(state,"EXIT_CENTER") && getpid() == main_pid ){
                    prompt(lpwd, main_pid);
                }
                continue;
            } 

            args[argc] = 0; // NULL
            if(history_flag){
                progress_number++;
                history[progress_number] = (char*)malloc(100*sizeof(char) );
            }
            

            if( !strcmp(args[0], "exit") ){ 
                 no_exit = false; 
                 break; 
            }

            if( status == 22 && main_pid != getpid() ){ 
                no_exit = false;
            } 
            
            if( !strcmp(state, "EXIT_BACKGROUND")){
                strcat(history[progress_number-1], "&");
                backflag = 1;
            }
            if(!strcmp(state, "EXIT_PIPE")){ 
                strcpy(MODE, "|"); 
            }


            if(!strcmp(args[0], "set") && backflag == 0){ 
                status = set_noclobber(argc, args, &noclobber); 
            
            }
            else if (!strcmp(args[0], "cd")){ 
                status = cd_command(argc, args); 
            }
            else if( args[0][0] == '!'){ 
                char hctmp[100]; 
                args[0]+=1; 
                strcpy(hctmp, args[0]); 
                if( atoi(hctmp) > progress_number - 1) { 
                    printf("not valid number\n");
                    free(history[progress_number]); 
                    progress_number--;
                }
                else{ // Valid progress_number
                    if(history_flag){ 
                        // strcat(history[progress_number-1], history[atoi(hctmp) - 1]); // 
                    }
                    args[0]-=1; // ! Restore !
                    status = default_exec(argc, args, file_name, MODE, backflag, block, noclobber);  
                }
            }
            else{
                status = default_exec(argc, args, file_name, MODE, backflag, block, noclobber); 
            }
            
            
            
            

            strcpy(MODE, "NOMODE"); 
            argc = 0; 
            if(block_break == 0 ){ 
                backflag = 0; 
            }
            

            if(status == 22 && no_exit == false){
                no_exit = true;
            }
            
            if( !strcmp(state,"EXIT_CENTER") && status != 11 && status != 22 && status != 10 && getpid() == main_pid){
                prompt(lpwd, main_pid);
            }
            
        }
        
    }
    free(buffers);
    free(block);

    for(int i=0; i<= progress_number; i++){
        free(history[i]);
    }
    for(int i=0; i<= argc; i++){
        free(args[i]);
    }

    if( getpid() == main_pid){
        printf("Logout\n");
    }
    return 0;
}

int set_noclobber(int argc, char *args[], int *noclobber){
    if(argc != 3 || strcmp(args[2], "noclobber") ){
        printf("cannot execute the command\n");
        return -1;
    }
    if( !strcmp(args[1], "-o") ){
        file_flag = O_CREAT|O_EXCL|O_WRONLY; 
        *noclobber = 0;
    }
    else if ( !strcmp(args[1], "+o") ){  
        file_flag = O_CREAT|O_WRONLY|O_TRUNC;
        *noclobber = 1;
    }
    else {
        printf("cannot execute the command\n");
        return -1;
    }
    return 1;
}

int history_exec(char *history[], int progress_number){
    for(int i=0; i< progress_number; i++){
        printf("%6d  %s\n", i+1, history[i]);
    }
    return 0;
}

void redirect_output(int fd){
    close(1);
    dup(fd); 
    close(fd); 
}

void redirect_input(int fd){
    close(0); 
    dup(fd);
    close(fd); 
}


void return_stdin(char *args){
    int len = strlen(args);
    for(int i=len-1; i >= 0 ; i--){
        ungetc(args[i], stdin);
    }
}

int history_command(int argc, char *args[]){
    char hctmp[100]; 
    args[0]+=1; 
    strcpy(hctmp, args[0]); 

    return_stdin("; exit;"); 
    return_stdin(history[atoi(hctmp) - 1]); 
    return 11; 
}

char *get_command( char *command){
    int c,i=0; 
    int state = 0;
    char *tmp;
    tmp = command;
    // lpwd = getpwuid(getuid());
    while ((c = getchar()) != EOF){ 
        if(state == 0){ 
            if( c == ' ' ){ continue; } 
            if( c == '\n' || c == ';' || c == '&' || c == '|' || c == '>' || c == '<'){
                if(c == '&'){ 
                    return "EXIT_BACKGROUND";
                }
                else if(c== '|'){ 
                    return "EXIT_PIPE";
                }
                else if (c == '>'){  
                    state = 2; 
                }
                else if( c == '<'){ 
                    return "EXIT_REDIRECT_STDIN";
                }
                else if( c == '\n'){
                    return "EXIT_CENTER";
                }
                else{ 
                    return "EXIT_C";
                }
            }

            else if( c == '('){ 
                state = 3; 
            }
            else{  
                tmp[i++] = c;
                state = 1;
            }
        }

        else if (state == 1){ 
            if( c == '\n' || c == ';' || c == '&' || c == '|' || c == ' ' || c == '>' || c == '<'){
                ungetc(c, stdin);
                tmp[i] = 0;
                return "COMMAND";
            }
            else{ 
                tmp[i++] = c; 
            }
        }
 
        else if (state == 2){
            if(c == '>'){
                return "EXIT_REDIRECT_PLUS";
            }
            if(c == '|'){
                return "EXIT_REDIRECT_PLUS2";
            }
            return "EXIT_REDIRECT";
        }
        
        else if (state == 3){ 
            if( c == ')'){ 
                tmp[i] = 0;
                return "BLOCK";
            }
            tmp[i++] = c; 
        }
    }
    return "EOF"; 
}

int my_exec(char *args[], int argc){
    history_flag = 0;
    if(!strcmp(args[0], "history")){ 
        history_exec(history, progress_number); 
        exit(0);
    }
    
    
    else if(args[0][0] == '!'){ 
        history_command(argc, args);
        if(history_flag){
            free(history[progress_number--]); 
        }
        
        return 10;
    }
    else if(!strcmp(args[0], "set")){
        // set_noclobber(argc, args); // no effect in a BACKGROUND
        exit(0);
    }
    else{   
        execvp(args[0],args);
        error_exit("Exec failed");
    }
    exit(0);
}



int cd_command(int argc, char *args[]){

    if ( argc != 1){
        char dest[100];
        strcpy(dest,getcwd(NULL,0)); 
        strcat(dest,"/"); 
        for(int i=1; i< argc; i++){
            strcat(dest,args[i]);
            strcat(dest, " ");
        }
        dest[ strlen(dest) -1] = 0; 
        if(chdir(dest) < 0){
            printf("cd: %s: No such file or directory \n", dest); 
            return -1;
        }
        return 1;
    }
    else {
        char dest[100]; 
        strcpy(dest,"/"); 
        if(chdir(dest) < 0){
            printf("cd: %s: No such file or directory \n", args[1]); 
            return -1;
        }
        return 1;
    }
    return -1;
}

int default_exec(int argc, char *args[], char filename[], char MODE[], int back_flag, char *block, int noclobber){

    if (!strcmp(MODE, ">") || !strcmp(MODE, ">>") || !strcmp(MODE, ">|")){

        int openfile; 

        if(!strcmp(MODE, ">")){ 
            if( (openfile = open(filename, file_flag, 0777)) < 0 ){ 
                if( noclobber == 0) { 
                    printf("cannot overwrite existing file\n"); 
                }
                else{
                    perror("Can not open the file "); 
                }
                return -1;
            }
        }
        else if(!strcmp(MODE, ">|")){  
            if( (openfile = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0777)) < 0 ){ 
                perror("Can not open the file ");
                return -1;
            }
        }
        else if(!strcmp(MODE, ">>")){
            if( (openfile = open(filename, O_CREAT|O_WRONLY, 0777)) < 0 ){ 
                perror("Can not open the file ");
                return -1;
            }
            lseek(openfile,0,SEEK_END);
        }


        int pid = fork();

        if(pid < 0){ 
	    	fprintf(stderr, "Fork failed");
	    	return -1;
	    }
	    else if(pid == 0){ 
            redirect_output(openfile); 
            if( !strcmp(args[0],"BLOCK command")){ 
                strcat(block,"exit;"); 
                return_stdin(block); 
                block_break = 1;
                return 2;
            }
            else{ 
                if( my_exec(args,argc) == 10 ){
                    return 10;
                }
                error_exit("Exec Failed ");
            }
	    }
        else{
            if(!back_flag){ 
                waitpid(pid, NULL, 0); 
            }
            return 1;
        }
    }
    
    else if(!strcmp(MODE, "<")){
        int openfile; 
        if( (openfile = open(filename, O_RDONLY)) < 0 ){ 
            perror("Can not open the file ");
            return -1;
        }
        
        int pid = fork(); 

        if( pid < 0){ 
            fprintf(stderr, "Fork failed"); 
	    	return -1;
        }
        else if (pid == 0){ 
            redirect_input(openfile); 
            if( !strcmp(args[0],"BLOCK command")){ 
                strcat(block,"exit;"); 
                return_stdin(block); 
                block_break = 1;
                return 2;
            }
            else{ 
                if( my_exec(args,argc) == 10 ){
                    return 10;
                }
                error_exit("Exec failed"); 
            }
        }
        else{
            if(!back_flag){
                waitpid(pid, NULL, 0); 
            }
            return 1;
        }
    }
    
    else if(!strcmp(MODE, "|")){

        int pid = fork(); 

        if( pid < 0){ 
            fprintf(stderr, "Fork failed");
	    	return -1;
        }
        else if (pid == 0){ 
            pipe(files); 
            int ppid = fork(); 
            if ( ppid < 0){ 
                fprintf(stderr, "Fork failed");
                return -1;
            }
            else if (ppid == 0){ 
                close(1); 
                dup(files[1]);
                close(files[1]);
                close(files[0]); 
                if( my_exec(args,argc) == 10 ){
                    return 10;
                }
                error_exit("Exec failed");
            }
            else{ 
                close(0);
                dup(files[0]);
                close(files[0]);
                close(files[1]);
                return 22; 
            }
        }

        else{
            strcat(history[progress_number-1], "|"); 
            char tmp[100]; char c; int idx=0;

            while(1){
                if ( (c=getchar()) == EOF){ 
                    error_exit("EOF");
                }
                
                if(c == ';' || c == '&' ){ break; }
                else if( c == '\n' ){ ungetc(c, stdin); break;}
                tmp[idx++] = c; 
            }
            tmp[idx] = 0; 
            strcat(history[progress_number-1], tmp);
            if(!back_flag){
                waitpid(pid, NULL, 0);
            }
            return 1;
        }
    }

    else{


        int pid = fork();
        if(pid < 0){
	    	fprintf(stderr, "Fork failed");
	    	return -1;
	    }
	    else if(pid == 0){
            if( !strcmp(args[0],"BLOCK command")){ 
                strcat(block,"exit;"); 
                return_stdin(block);
                block_break = 1;
                return 2;
            }
            else{
                
                if( my_exec(args,argc) == 10 ){
                    return 10;
                }
	    	    error_exit("Exec failed");
            }
	    }
        else{
            if(!back_flag){
                waitpid(pid, NULL, 0); 
            }
            return 1;
        }
    }
    return -1;
}

void prompt(struct passwd *lpwd, int main_pid){
    if(getpid() != main_pid){
        return ;
    }
    printf("\033[1;36m");
    printf("%s:%s$ ", lpwd->pw_name,getcwd(NULL,0));
    printf("\033[0m");
}

void error_exit(char *error_msg){
    perror(error_msg);
    exit(1);
}