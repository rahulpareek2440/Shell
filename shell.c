#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
void minish();
bool executeInBack(char**, int*);
char* updateArgumentsArray(char**, int*, const char*, char **,  int *);
char* parseForPipe(char**, int*, const char*, char **,  int *);
char* checkInFile(char**, int*, char **, int *);
char* checkOutFile(char**, int* , char **, int *);
char* checkForPipeline(char**, int*, char **, int * );
char** parse(int*);
void printToStdout();
void printToFile();
bool execute(char**, const int, bool, char*,char*, char*,char **, int);
void makespace(const int ,char ** , char* ,char * );
//char ** checkTokenLength(int* , int , char*  );
void sigint_handler(int);
void sigtstp_handler(int);

struct procs{
    pid_t pid;
    int status;
};
struct procs jobs[10];
//fg -1
//bg -2
//st -3

char* checkForkill(char** arguments, int* argLen, char ** commands,int * comLen ) {
    return updateArgumentsArray(arguments, argLen, "%", commands,comLen);
}
char* checkForPipeline(char** arguments, int* argLen, char ** commands,int * comLen ) {
    return parseForPipe(arguments, argLen, "|", commands,comLen);
}

char* checkInFile(char** arguments, int* argLen, char ** commands,int * comLen) {
    return updateArgumentsArray(arguments, argLen, "<", commands,comLen);
}

char* checkOutFile(char** arguments, int* argLen, char ** commands,int * comLen) {
    return updateArgumentsArray(arguments, argLen, ">", commands,comLen);
}

char* updateArgumentsArray(char** arguments, int* argLen, const char* findchar, char ** commands, int * comLen) {
    if(argLen == 0) return NULL;
    char* filename = NULL;
    int i,j;
    for( i = 1; i < *argLen; i++) {
        if(strcmp(arguments[i], findchar) == 0) {
            free(arguments[i]);
            filename = arguments[i + 1];
            *argLen -= 2;
            for(j = i; j <= *argLen; j++) {
                arguments[j] = arguments[j + 2];
            }
            break;
        }
    }
    return filename;
}
char* parseForPipe(char** arguments, int* argLen, const char* findchar, char ** commands,int* comLen){
    if(argLen == 0) return NULL;
    char* filename = NULL;
    
    commands=(char **)malloc((sizeof(char*)*10));
    int i,j,k;
    for(i = 1; i < *argLen; i++) {
        if(strcmp(arguments[i], findchar) == 0) {
            filename = "Pipeline";
            free(arguments[i]);
            //arguments[i]=NULL;
            
            for (k=i+1,j=0; k <= *argLen; k++,j++) {
                commands[j]=(char*)malloc(sizeof(char)*50);
                commands[j]=arguments[k];
            }
            comLen=k;
            break;
        }
    }
    for (i = 0;commands[i]!=NULL; ++i)
    {
        printf("commands %s\n",commands[i]);
    }
    
    return filename;
}
bool execute(char** arguments, const int argLen, bool bg, char* inputFile,char* pipeline, char* outputFile,char ** commands,const int comLen) {
    FILE * fp;
    if(argLen == 0 || arguments[0] == NULL)
        return true;
    // exit command
    if(*arguments[0] == EOF || strcmp(arguments[0], "exit") == 0)
        return false;
    //cd command
    if(strcmp(arguments[0], "cd") == 0) {
        if(chdir(arguments[1])!=0){
            perror("cd : error found");
        }
        return true;
    }
    //pwd command
    if(strcmp(arguments[0], "pwd") == 0){
        char * buffer;
        char * cwd;
        buffer = (char *)malloc(sizeof(char) * 1024);
        
        if((cwd = getcwd(buffer, 1024)) != NULL)
            printf("%s\n", cwd);
        else
            perror("error found");
        free(buffer);
        cwd=NULL;
        free(cwd);
        return true;
    }
    int myPipe[2];
    
    if(pipeline != NULL)
    {
        if(pipe(myPipe)<0)                    //create pipe
        {
            fprintf(stderr, "Pipe failed!");
            exit(-1);
        }
      
    }
    
    signal(SIGINT,sigint_handler);
    pid_t pid = fork();
    if(pid == -1) {
        perror(NULL);
        return false;
    }
    if(pid == 0) {
        
         signal(SIGTSTP,sigtstp_handler);
        
        addprocs(jobs,pid,0);
        if(outputFile != NULL) {
            addprocs(jobs,pid,1);
            fp = fopen(outputFile, "a");
            if(dup2(fileno(fp), 1) == -1) {
                perror(NULL);
                return false;
            }
        }
        
        if(inputFile != NULL) {
            addprocs(jobs,pid,1);
            fp = fopen(inputFile, "r");
            if(dup2(fileno(fp), 0) == -1) {
                perror(NULL);
                return false;
            }
        }
        if (pipeline!=NULL) {
                 printf("pipe %s\n",pipeline);
                close(myPipe[1]);
                dup2(myPipe[0], 0);
                close(myPipe[0]);
               if(execvp(commands[0], commands) == -1) { perror("Failed to execute command"); return false; }
               
            }
        if(bg != NULL) {
            if(dup2(open("/dev/null", 0), 1) == -1) {
                perror(NULL);
                return false;
            }
            if(dup2(open("/dev/null", 0), 2) == -1) {
                perror(NULL);
                return false;
            }
            
        }
        if(execvp(arguments[0], arguments) == -1) { perror("Failed to execute command"); return false; }
        } else if(!bg) {
             int status;
            int r;
            //printf("pipeparent %s\n",pipeline);
            if (pipeline!=NULL) {
                //printf("in  pipel");
                pid_t pid2 = fork();
                if(pid2 < 0)
                {
                    printf("error in forking");
                    exit(-1);
                }
                if(pid2 == 0)
                {
                    printf("forking again");
                    close(myPipe[0]);
                    dup2(myPipe[1], 1);
                    close(myPipe[1]);
                    r = execvp(arguments[0], arguments);
                    close(myPipe[0]);
                    
                }
                else
                {
                    close(myPipe[0]);
                    close(myPipe[1]);
                     while (wait(&status) != pid2);
                    
                }
                addprocs(jobs,pid,1);
                setpgid(pid,pid);
                if (bg=NULL) {
                    printf("[%d]\n",pid);
                }
                
                if (pipeline=NULL){
                    //waitpid(pid, NULL, 0);
                    setpgid(pid,pid);
                }
            }else {
                
                waitpid(pid, NULL, 0);
                addprocs(jobs,pid,0);
                setpgid(pid,pid);
                
            }
                
        }
    
        return true;
        
}
    bool executeInBack(char** arguments, int* argLen) {
        if(!(*argLen == 0) && strcmp(arguments[*argLen - 1], "&") == 0) {
            (*argLen)--;
            free(arguments[*argLen]);
            arguments[*argLen] = NULL;
            return true;
        } else {
            return false;
        }
    }
    char** parse(int* argLen) {
        char** arguments = (char**)malloc(4 * sizeof(char*));
        int tokensMaxLength = 4;
        *argLen = 0;
        
        char tok;
        
        do {
            char* token = (char*)malloc(8);
            int tokenMaxLength = 8;
            int tokenLength = 0;
            
            if(*argLen == tokensMaxLength - 1) {
                tokensMaxLength *= 2;
                arguments = (char**)realloc(arguments, tokensMaxLength * sizeof(char*));
            }
            
            for(tok = getchar(); tok != ' ' && tok != '\n'; tok = getchar()) {
                if(tok == -1) return arguments;
                if(tokenLength == tokenMaxLength - 1) {
                    tokenMaxLength *= 2;
                    token = (char*)realloc(token, tokenMaxLength);
                }
                token[tokenLength++] = tok;
            }
            token[tokenLength] = '\0';
            
            if(tokenLength != 0) arguments[(*argLen)++] = token;
        } while(tok != '\n');
        
        arguments[*argLen] = NULL;
        
        
        return arguments;
    }
    void makespace(const int argLen ,char ** arguments, char* inputFile,char * outputFile){
        free(arguments);
        free(inputFile);
        free(outputFile);
    }
    void minish() {
        char** arguments = NULL;
        char** commands = NULL;
        int argLen = 0;
        int comLen = 0;
        bool bg;
        bool kil;
        char* inputFile = NULL;
        char* pipeline = NULL;
        char* outputFile = NULL;
        char dir[100];
        int i=0;
        
        do {
            
            printf("minish> ");
            for(i = 0; i < argLen; i++) free(arguments[i]);
            makespace(&argLen ,arguments, inputFile,outputFile);
            arguments = parse(&argLen);
            bg = executeInBack(arguments, &argLen);
            kil = checkForkill(arguments, &argLen,commands,&comLen);
            inputFile = checkInFile(arguments, &argLen,commands,&comLen);
            pipeline  = checkForPipeline(arguments, &argLen,commands,&comLen);
            outputFile = checkOutFile(arguments, &argLen,commands,&comLen);
            
        } while(execute(arguments, argLen, bg, inputFile, pipeline, outputFile,commands,comLen));
        
        for(i = 0; i < argLen; i++) free(arguments[i]);
        makespace(&argLen ,arguments, inputFile,outputFile);
        
        
    }
    
    void sigint_handler(int sig) {
        signal(SIGINT,sigint_handler);
        putchar('\n');
    }
    
    void sigtstp_handler(int sig) {
        signal(SIGTSTP,sigtstp_handler);
        putchar('\n');
    }
    
    int fgpid(struct procs *jobs) {
        int i;
        
        for (i = 0; i < 10; i++)
            if (jobs[i].status == 1)
                return jobs[i].pid;
        return 0;
    }
    int addprocs(struct procs *jobs,int pid, int state ){
        int i=0;
        if (pid<1) {
            return 0;
        }
        for (i; i<10; i++) {
            if (jobs[i].pid==0) {
                jobs[i].pid=pid;
                jobs[i].status=state;
            }
        }
    }
    int main() {
        minish();
        return 0;
    }