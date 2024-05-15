// struct that store Cpu Usage
typedef struct CpuUsage{
    int totalCpu;
    int idle;
    char graphic[1024];
}CpuUsage;

// struct that store Memory Usage
typedef struct MemoryUsage{
    float phys_mem_used;
    float total_ram;
    float vir_mem_used;
    float total_memory;
    char output[1024];
    float deltaMemory;
}MemoryUsage;

MemoryUsage *newMemoryUsage();
CpuUsage *newCpuUsage();
float calculateCPUUsage(CpuUsage *cpu1, CpuUsage *cpu2);

void MemoryUsageGraphic(MemoryUsage *memory, float last_phys_mem_used, int i);

bool validInteger(char *s);
int positionSampleTdelay(int argc, char **argv);
void displayRunningParameters(int samples, int tdelay, int iteration, bool sequentialflag);

double rebootTime();
void uptimeFormat(double totalSeconds);
void displaySystemInfo();
bool checkInput(int argc, char **argv,bool *systemflag, bool *userflag, bool *graphflag,
    bool *sequentialflag, bool *sampleflag, bool *tdelayflag,int *sample, int *tdelay);
void printSystemStat(int sample, int tdelay, bool systemflag, bool userflag, bool graphflag, bool sequentialflag);
void signal_handler(int sig);

char* displayCPUUsage(CpuUsage **cpuArray, int i, int sample, bool graphflag, bool sequentialflag, bool display, CpuUsage *cpu);
char* displayMemoryUsage(MemoryUsage **memory, int i, int sample, bool graphflag, bool sequentialflag, bool display);
char* displayUserUsage(bool display);