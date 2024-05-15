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


// malloc a new Memory Usage struct
MemoryUsage *newMemoryUsage(){

    MemoryUsage *mem = malloc(sizeof(MemoryUsage));
    struct sysinfo s; // memory usage info
    sysinfo(&s); // fetch memory info

    float free_ram; // free ram (GB) in a sample
    float total_swap; // total virtual memory (GB) in a sample
    float free_swap; // free virtual memory (GB) in a sample
    float swap_used; // swap space used

    free_ram = (float)s.freeram/s.mem_unit/1000000000;  // get free_ram
    total_swap =(float)s.totalswap/s.mem_unit/1000000000;  // get total_swap
    free_swap =(float)s.freeswap/s.mem_unit/1000000000;  // get free_swap
    swap_used = total_swap - free_swap;  // get swap_used (total_swap - free_swap = swap_used)

    mem->total_ram = (float)s.totalram/s.mem_unit/1000000000; //total ram (GB) in a sample
    mem->total_memory = mem->total_ram + total_swap; // total_ram + total_virtual
    mem->phys_mem_used = mem->total_ram - free_ram; // physical memory used (GB)
    mem->vir_mem_used = mem->phys_mem_used + swap_used; // virtual memory used (GB)
    sprintf(mem->output,"%.2f GB / %.2f GB  -- %.2f GB / %.2f GB", mem->phys_mem_used, mem->total_ram,
         mem->vir_mem_used, mem->total_memory);
    return mem;
}

// malloc a new CPU Usage struct
CpuUsage *newCpuUsage(){
    CpuUsage *cpu = NULL;
    FILE *stat = fopen("/proc/stat", "r"); // open /proc/stat
    if(stat == NULL){
        printf("Error opening /proc/stat");   // Error handling, error open file
        return NULL;
    }
    char cpuState[1024];
    fgets(cpuState,1024,stat);  // fetch the cpu stat
    fclose(stat); // close /proc/stat
    cpu = malloc(sizeof(CpuUsage));
    int user, nice, system, idle, iowait, irq, softirq; // cpu stat

    char *token = strtok(cpuState, " ");
    token = strtok(NULL, " "); // move to user
    user = atoi(token);
    token = strtok(NULL, " "); // move to nice
    nice = atoi(token);
    token = strtok(NULL, " "); // move to system
    system = atoi(token);
    token = strtok(NULL, " "); // move to idle
    idle = atoi(token);
    token = strtok(NULL, " "); // move to iowait
    iowait = atoi(token);
    token = strtok(NULL, " "); // move to irq
    irq = atoi(token);
    token = strtok(NULL, " "); // move to softirq
    softirq = atoi(token);


    cpu->totalCpu = user + nice + system + idle + iowait + irq + softirq; // total time stored into CpuUsage
    cpu->idle = cpu->totalCpu - idle; // idle stored into CpuUsage
    cpu->graphic[0] = '\0';
    return cpu;
}

// Calculate the CPU 
float calculateCPUUsage(CpuUsage *cpu1, CpuUsage *cpu2){
    if(cpu1 == NULL) return (cpu2->idle * 100.0)/(cpu2->totalCpu);  // meaningless number
    return ((cpu2->idle - cpu1->idle) * 100.0) / (cpu2->totalCpu - cpu1->totalCpu);  // calculate the CPU Usage
}

// display CPU Usage or graphically or sequentially
char* displayCPUUsage(CpuUsage **cpuArray, int i, int sample, bool graphflag, bool sequentialflag, bool display, CpuUsage *cpu){
    if(!display) return NULL;
    // CpuUsage *cpu = newCpuUsage();
    // cpuArray[i+1] = cpu;
    float cpuUse = calculateCPUUsage(cpuArray[i], cpu); // calculate the cpu usage
    char *result;
    result = malloc(1024 * sizeof(char));
    sprintf(result,"Number of cores: %ld    \ntotal cpu use = %.2f%%   \n", sysconf(_SC_NPROCESSORS_ONLN),cpuUse);
    //printf("total cpu use = %.2f%%   \n", cpuUse);  // print out the cpu Use in %
    if(graphflag){
        char graphCpu[1024];
        graphCpu[0] = '\t'; // \t preserve the structure of graphic
        graphCpu[1] = '\0'; // manually make sure it is a string end with '\0'
        int counter = 2; // default value for the number of '|' to represent the graph
        char graph[1024];
        graph[0] = '\0';  // manually make sure it is a string end with '\0';
        int baseNum = (int)cpuUse;
        if((cpuUse - baseNum) < 0.5 && cpuUse != 0.00){     // check the value of the decimal part
            counter += 1;
        }else if (cpuUse - baseNum >= 0.50){           // check the value of the decimal part
            counter += 2;
        }
        for(int j = 0; j < (counter + 2 * baseNum); j++){
            strcat(graph, "|");                    // draw graphics
        }
        char graphdata[1024];
        sprintf(graphdata, " %.2f", cpuUse);
        strcat(graph,graphdata);
        strcat(graphCpu, graph);  // complete the grapgic
        strcpy(cpuArray[i+1]->graphic, graphCpu);
        if(!sequentialflag){
            for(int g = 0; g < i + 1; g++){
            strcat(result, cpuArray[g+1]->graphic);    // display the graphics cpu
            strcat(result, "\n");
            }
        }else if(sequentialflag){
            for(int g = 0; g < sample + 1; g++){
                if(g == i ){
                    strcat(result, cpuArray[g+1]->graphic);    // display the graphics cpu
                    strcat(result, "\n");
                }else if ( g != sample){
                    strcat(result, "\n");
                }
            }
        }
    }
    return result;
}

// Add Graphics to Memory Usage
void MemoryUsageGraphic(MemoryUsage *memory, float last_phys_mem_used, int i){
    char graph[1024]; // store graph symbol only (except with delta == 0.00)
    char graphdata[1024]; // store graph data
    int roundupgraph; // round up integer to indicate # of graph symbol
    if(i == 0){
        memory->deltaMemory = 0.00;        // the first one, no change, set to 0
        sprintf(graph,"\t|o %.2f (%.2f) ",memory->deltaMemory, memory->phys_mem_used);
        strcat(memory->output, graph);
    }else{
        memory->deltaMemory = memory->phys_mem_used - last_phys_mem_used; // memory viration = this phys_mem_used - last phts_mem_used
        graph[0] = '\t';        // the first element of graph is '\t'
        graph[1] = '|';         // the second element of graph is '|'
        roundupgraph = (int)(memory->deltaMemory * 100.0);    // indicate how many graph symbol it should have
        int g = 2; // the indice of the graph symbol
        if(memory->deltaMemory >= 0.01){
            for (g = 2; g < roundupgraph + 2; g++){
                graph[g] = '#';  // graphics with #
            }
            graph[g] = '*'; // indicate it is total relative positive change
        }else if(memory->deltaMemory < 0.00){
            for (g = 2; g < (((-1) * roundupgraph) + 2); g++){
                graph[g] = ':';  // graphics with :
            }
            graph[g] = '@'; // indicate it is total relative negative change or zero -
        }else if(memory->deltaMemory >= 0.00 && memory->deltaMemory < 0.01){
            graph[g] = 'o'; // zero +
        }
        graph[g+1] = '\0'; // manually make sure graph is a string end with '\0'
            sprintf(graphdata," %.2f (%.2f) ",memory->deltaMemory, memory->phys_mem_used); // Graphics showing data
            strcat(graph,graphdata);  // Complete the Graphics
            strcat(memory->output,graph); // Add Graphics to Memory Usage
    }
}

// display Memory Usage or graphically showing Memory Usage or Sequentially showing Memory Usage
char* displayMemoryUsage(MemoryUsage **memory, int i, int sample, bool graphflag, bool sequentialflag, bool display){
    if(!display) return NULL;
    // MemoryUsage *mem = newMemoryUsage();
    // memory[i] = mem;
    char *result = (char *)malloc(1024 * sizeof(char));
    result[0] = '\0';
    if(graphflag){
        if( i != 0){
            MemoryUsageGraphic(memory[i], memory[i-1]->phys_mem_used, i); // make sure memory[i-1] valid
        }else{
            MemoryUsageGraphic(memory[i], 0.00, i); // if i = 0, it is the first memory usage, not last_phys_mem_used
        }
    }
    strcat(result,"-----------------------------------------------\n");
    strcat(result,"### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    int j;
    if(!sequentialflag){
        for(j = 0; j < i + 1 ; j++){
            char buffer[1024];
            sprintf(buffer,"%s", memory[j]->output);          // display the Phys.Used/Tot -- Virtual Used/Tot
            strcat(result,buffer);
            strcat(result,"\n");
        }
        for(int space = j; space < sample; space++){     // dynamic showing
            strcat(result,"\n"); // leave space
        }
    }else if(sequentialflag){
        for(j = 0; j < sample ; j++){
            if( j == i){
                char bufffer[1024];
                sprintf(bufffer,"%s", memory[j]->output);    // display the Phys.Used/Tot -- Virtual Used/Tot
                strcat(result,bufffer);
                strcat(result,"\n");
            }
            if( j != sample - 1){
                strcat(result,"\n");                         // sequentially showing
            }
        }
    }
    return result;
}

// check the input string can be convert to integer or not
bool validInteger(char *s){
    int i = 0;
    while (s[i] != '\0') {
        if (isdigit(s[i]) == 0) {
            return false;  // string s is not a valid integer
        }
        i++;
    }
    return true; // true if the string is a valid integer
}

// return the position of the first integer argument
int positionSampleTdelay(int argc, char **argv){
    if(argc > 2){
        for (int i = 0; i < argc - 2; i++){
            if(validInteger(argv[i+1]) && validInteger(argv[i+2])){
               return (i+1);         // return the position of the first integer argument
            }
        }
    }
    return 0; // return 0 if we don't input a shortcut of sample and tdelay
}

int positionSampleNoTdelay(int argc, char **argv){
    if(argc > 1){
        for (int i = 0; i < argc - 1; i++){
            if(validInteger(argv[i+1])){
               return (i+1);         // return the position of the first integer argument
            }
        }
    }
    return 0; // return 0 if we don't input a shortcut of sample and tdelay
}

// Display running parameter
void displayRunningParameters(int samples, int tdelay, int iteration, bool sequentialflag){
    struct rusage usage;
    if(!sequentialflag){
        printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay); // display samples and tdelay
    }else{
        printf(">>> iteration %d\n", iteration);
    }
    if(getrusage(RUSAGE_SELF, &usage) == 0){
        printf(" Memory usage: %ld kilobytes\n", usage.ru_maxrss); // display memory usage
    }else{
        printf( "Error");
    }
}

//display User Usage
char * displayUserUsage(bool display){
    if(!display) return NULL;
    char *result = (char *)malloc(1024 * sizeof(char));
    result[0] = '\0';
    strcat(result,"-----------------------------------------------\n");
    strcat(result,"### Sessions/users ###\n");
    struct utmp *utm;
    setutent(); // open utmp file
    while((utm = getutent()) != NULL){   // utm = getutent() -> go next
        if(utm->ut_type == USER_PROCESS){
            char buffer1[256];
            //sprintf(buffer," %s\t%s (%s)\n",utm->ut_user, utm->ut_line, utm->ut_host); // display user usage
            sprintf(buffer1, "%s",utm->ut_user);
            strcat(result, buffer1);
            strcat(result, "\t");
            char buffer2[256];
            sprintf(buffer2, "%s",utm->ut_line);
            strcat(result, buffer2);
            strcat(result, " (");

            char buffer3[256];
            sprintf(buffer3, "%s",utm->ut_host);
            strcat(result, buffer3);
            strcat(result, ")");
            strcat(result, "\n");
        }
    }
    endutent(); // close utmp file
    strcat(result,"-----------------------------------------------\n");
    return result;
}

// return the reboot Time in seconds
double rebootTime(){
    FILE *uptime = fopen("/proc/uptime", "r"); // open /proc/uptime file
    if(uptime == NULL){
        printf("Error opening /proc/uptime");
        return 1;
    }
    double total_seconds;
    fscanf(uptime, "%lf", &total_seconds); // Store the time that the system has
                                          // been running since its last reboot to time_seconds
    fclose(uptime);  // close the file been running since its last reboot to time_seconds
    return total_seconds;
}

// printf the time that the system has been running since its last reboot
void uptimeFormat(double totalSeconds){
    int day = totalSeconds / (24 * 60 * 60);  // days
    totalSeconds = totalSeconds - (day * 24 * 60 * 60);
    int hour = totalSeconds / (60 * 60); // hours
    totalSeconds -= hour * 60 * 60;
    int minutes = totalSeconds / 60; // minutes
    double remainSecond = totalSeconds - minutes * 60; // seconds
    remainSecond = (int)remainSecond; // cast it to int
    printf(" System running since last reboot: %d days %d:%d:%.2lf (%d:%d:%.2lf)\n", day, hour,
     minutes, remainSecond, (day*24+hour),minutes,remainSecond); // format similar to "3 days 19:27:30 (91:27:30)"
}

// printf System information
void displaySystemInfo(){
    struct utsname uts;
    uname(&uts);
    double totalSeconds = rebootTime(); // get time of System running since last reboot in seconds
    printf("-----------------------------------------------\n");
    printf("### System Information ###\n");
    printf(" System Name = %s\n", uts.sysname); // display System Name
    printf(" Machine Name = %s\n", uts.nodename); // display Machine Name
    printf(" Version = %s\n", uts.version); // display Version
    printf(" Release = %s\n", uts.release); // display Release
    printf(" Architecture = %s\n", uts.machine); // display Architecture
    uptimeFormat(totalSeconds); // display System running since last reboot in format similar to "3 days 19:27:30 (91:27:30)"
    printf("-----------------------------------------------\n");
}

// check the Input command line arguments, check flag
bool checkInput(int argc, char **argv,bool *systemflag, bool *userflag, bool *graphflag,
    bool *sequentialflag, bool *sampleflag, bool *tdelayflag,int *sample, int *tdelay){
        int positionflag = positionSampleTdelay(argc, argv);
        int positionflagS = positionSampleNoTdelay(argc, argv);
        for(int i = 1; i < argc; i ++){
            char *token = strtok(argv[i], "="); // split the input argument by "="
            if(strcmp(token, "--system") == 0){  // check --system
                *systemflag = true;              
            }else if(strcmp(token, "--user") == 0){       // check --user
                *userflag = true;
            }else if((strcmp(token,"--graphics") == 0) || (strcmp(token,"-g") == 0)){      //check --graphics
                *graphflag = true;
            }else if(strcmp(token,"--samples") == 0){        // check --samples=N
                *sample = atoi(strtok(NULL, ""));            // update sample value if sampleflag is presented
                *sampleflag = true;
            }else if(strcmp(token, "--tdelay") == 0){        // check --tdelay=T
                *tdelay = atoi(strtok(NULL, ""));            // update tdelay value if tdelayflag is presented
                *tdelayflag = true;
            }else if(strcmp(token, "--sequential") == 0){
                *sequentialflag = true;
            }else if(positionflag != 0){
                *sample = atoi(argv[positionflag]);
                *tdelay = atoi(argv[positionflag + 1]);
            }else if(positionflag != positionflagS && positionflagS != 0){
                *sample = atoi(argv[positionflagS]);
            }else{
                printf("Invalid Input, check your Arguments\n");
                return false;
            }
        }
        return true;
    }

// final printing output based on flag
// void printSystemStat(int sample, int tdelay, bool systemflag, bool userflag, bool graphflag, bool sequentialflag){
//     //displayRunningParameters(sample,tdelay);  showing the Running parameter including number of samples,
//                                                 // samples frequency and memory self-utilization 
//     CpuUsage *cpu[sample + 1]; // used to store cpu usage
//     MemoryUsage *memory[sample]; // used to store memory usage
//     int i = 0;
//     cpu[0] = NULL;

//     bool cpuflag = true;
//     if(userflag && !systemflag){
//         cpuflag = false;
//     }
//     if(!userflag && !systemflag){
//         userflag = true;
//         systemflag = true;
//     }

//     while (i < sample){
//         if(!sequentialflag){
//             printf("\x1b[s"); // save position
//         }
//         displayRunningParameters(sample,tdelay, i, sequentialflag);  /* showing the Running parameter including number of samples,
//                                                 samples frequency and memory self-utilization */
//         // if ((!systemflag && !userflag) || (systemflag && userflag)){   // no flag or systemflag and userflag both present
//         //     displayMemoryUsage(memory, i,sample, graphflag, sequentialflag);
//         //     displayUserUsage();
//         //     displayCPUUsage(cpu, i, sample, graphflag, sequentialflag);
//         // }else if(systemflag && !userflag){  // only --system flag or with graphic
//         //     displayMemoryUsage(memory, i, sample, graphflag, sequentialflag);
//         //     displayCPUUsage(cpu, i, sample, graphflag, sequentialflag);
//         // }else if(userflag && !systemflag){  // only --user flag
//         //     displayUserUsage();
//         // }

//         displayMemoryUsage(memory, i,sample, graphflag, sequentialflag, systemflag);
//         displayUserUsage(userflag);
//         displayCPUUsage(cpu, i, sample, graphflag, sequentialflag,cpuflag);

//         if( i < sample - 1){
//             sleep(tdelay);
//             if(!sequentialflag){
//             printf("\x1b[u"); // go back to save position
//             }
//         }
//         i++;
//     }
//     displaySystemInfo();  // display System Info
//     if(!(userflag && !systemflag && !graphflag && !sequentialflag)){     // if call --user, don't free
//         for(int m = 0; m < sample; m++){
//             free(memory[m]);                              // free malloc memoryUsage
//         }
//         for(int c = 0; c < sample + 1; c++){
//             free(cpu[c]);                                 // free malloc cpu Usage
//         }
//     }
// }



void signal_handler(int sig) {
    if (sig == SIGINT) {
        // Ask the user if they want to quit when Ctrl-C is pressed
        char response;
        printf("Do you want to quit the program? (y/n): ");
        scanf(" %c", &response);
        if (response == 'y' || response == 'Y') {
            exit(0);
        } else if(response == 'n' || response == 'N'){
            printf("Continuing the program...\n");
            signal(SIGINT, signal_handler); // Re-register the handler
        }else{
            exit(0);
        }
    }
}