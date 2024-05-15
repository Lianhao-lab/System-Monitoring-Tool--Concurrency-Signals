#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "stats.h"
#include <signal.h>
#include <sys/wait.h>
// main
int main(int argc, char **argv){
    
    // Ignore SIGTSTP (Ctrl-Z)
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, signal_handler);
    
    int sample = 10; // default value of sample
    int tdelay = 1; // default value of tdelay
    
    bool systemflag = false; // default boolean value if no input --system flag 
    bool userflag = false; // default boolean value if no input --user flag
    bool graphflag = false; // default boolean value if no input --graphic or -g
    bool sequentialflag = false; // default boolean value if no input --sequential flag
    bool sampleflag = false; // default boolean value if no input --samples=N flag
    bool tdelayflag = false; // default boolean value if no input --tdelay=T flag

    bool correctInput = checkInput(argc, argv, &systemflag,&userflag,&graphflag,
        &sequentialflag,&sampleflag,&tdelayflag,&sample,&tdelay);//check input arguments
    if(!correctInput){
        return 1;
    }


    CpuUsage *cpu[sample + 1]; // used to store cpu usage
    MemoryUsage *memory[sample]; // used to store memory usage
    cpu[0] = NULL;

    bool cpuflag = true;
    if(userflag && !systemflag){
        cpuflag = false;
    }
    if(!userflag && !systemflag){
        userflag = true;
        systemflag = true;
    }
    char *result = NULL;

    int i = 0;

    while (i < sample)
    {
        int pipeMem[2];
        int pipeUse[2];
        int pipeCpu[2];

    if(pipe(pipeMem) == -1){
        perror("Pipe Error");
        exit(EXIT_FAILURE);
    }
    if(pipe(pipeUse) == -1){
        perror("Pipe Error");
        exit(EXIT_FAILURE);
    }
    if(pipe(pipeCpu) == -1){
        perror("Pipe Error");
        exit(EXIT_FAILURE);
    }

        MemoryUsage *mem = newMemoryUsage();
        memory[i] = mem;
        CpuUsage *cpu2 = newCpuUsage();
        cpu[i+1] = cpu2;

        pid_t pidMem = fork();
        if(pidMem == -1){
            perror("fork fail\n");
            exit(EXIT_FAILURE);
        }else if(pidMem == 0){
            close(pipeMem[0]);
            result = displayMemoryUsage(memory, i,sample, graphflag, sequentialflag, systemflag);
            write(pipeMem[1],result,1024);
            free(result);
            close(pipeMem[1]);
            exit(EXIT_SUCCESS);
        }


        pid_t pidUse = fork();
        if(pidUse == -1){
            perror("fork fail\n");
            exit(EXIT_FAILURE);
        }else if(pidUse == 0){
            close(pipeUse[0]);
            result = displayUserUsage(userflag);
            write(pipeUse[1],result,1024);
            free(result);
            close(pipeUse[1]);
            exit(EXIT_SUCCESS);
        }

        pid_t pidCpu = fork();
        if(pidCpu == -1){
            perror("fork fail\n");
            exit(EXIT_FAILURE);
        }else if(pidCpu == 0){
            close(pipeCpu[0]);
            result = displayCPUUsage(cpu, i, sample, graphflag, sequentialflag,cpuflag, cpu2);
            write(pipeCpu[1],result,1024);
            free(result);
            close(pipeCpu[1]);
            exit(EXIT_SUCCESS);
        }

        for (int y = 0; y < 3; y++) {
            wait(NULL);
        }
            close(pipeMem[1]);
            close(pipeUse[1]);
            close(pipeCpu[1]); // close write end

            char *Memoryresult = malloc(1024);
            char *Userresult = malloc(1024);
            char *Cpuresult = malloc(1024);

            int byteM = read(pipeMem[0], Memoryresult, 1024 - 1);
            close(pipeMem[0]);
            if(byteM == -1){
                perror("Read fail");
                return 1;
            }else{
                Memoryresult[byteM] = '\0';
            }

            int byteU = read(pipeUse[0], Userresult, 1023);
            close(pipeUse[0]);
            if(byteU == -1){
                perror("Read fail");
                return 1;
            }else{
                Userresult[byteU] = '\0';
                
            }

            int byteC = read(pipeCpu[0], Cpuresult, 1023);
            close(pipeCpu[0]); // close read end
            if(byteC == -1){
                perror("Read fail");
                return 1;
            }else{
                Cpuresult[byteC] = '\0';
            }


            if(!sequentialflag){
            printf("\x1b[s"); // save position
            }
            displayRunningParameters(sample,tdelay, i, sequentialflag);
            if(Memoryresult!=NULL){
                printf("%s", Memoryresult);
            }

            if(Userresult!=NULL){
                printf("%s", Userresult);
            }
            if(Cpuresult!=NULL){
                printf("%s", Cpuresult);
            }
            free(Memoryresult);
            free(Userresult);
            free(Cpuresult);
    //         // printf("%s", Memoryresult);
    //         // printf("%s", Userresult);
    //         // printf("%s", Cpuresult);
            if( i < sample - 1){
                sleep(tdelay);
                if(!sequentialflag){
                    printf("\x1b[u"); // go back to save position
            }
        }
        i++;
    }

    displaySystemInfo();  // display System Info
    if(!(userflag && !systemflag && !graphflag && !sequentialflag)){     // if call --user, don't free
        for(int m = 0; m < sample; m++){
            free(memory[m]);                              // free malloc memoryUsage
        }
        for(int c = 0; c < sample + 1; c++){
            free(cpu[c]);                                 // free malloc cpu Usage
        }
    }
    return 0;
}
