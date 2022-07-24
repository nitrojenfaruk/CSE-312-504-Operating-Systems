
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));

    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;

    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */

    // cpustate -> error = 0;

    // cpustate -> esp = ;
    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate->eflags = 0x202;
}

Task::Task(ThreadManager *threadManager)
{
    this->threadManager = threadManager;
    cpustate = threadManager->getThread(0)->getCPUState();
}

Task::~Task()
{
}

TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task *task)
{
    if (numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    return true;
}

CPUState *TaskManager::Schedule(CPUState *cpustate)
{
    CPUState *threadState = tasks[currentTask]->threadManager->Schedule(cpustate);

    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
        tasks[currentTask]->cpustate = threadState;

    if (++currentTask >= numTasks)
        currentTask %= numTasks;
    return tasks[currentTask]->cpustate;
}

/*------------------THREAD - THREAD MANAGER---------------------*/



void Thread::bind(GlobalDescriptorTable *gdt, void entrypoint())
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));

    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;

    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */

    // cpustate -> error = 0;

    // cpustate -> esp = ;
    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate->eflags = 0x202;
}

Thread::Thread(int id, int priority)
{
    this->id = id;
    this->priority = priority;
}

int Thread::getId()
{
    return id;
}

int Thread::getPriority()
{
    return priority;
}

int Thread::getYieldVal()
{
    return yieldVal;
}
int Thread::getTerminated()
{
    return terminated;
}

void Thread::setId(int id)
{
    this->id = id;
}

void Thread::setPriority(int priority)
{
    this->priority = priority;
}

void Thread::setYieldVal(int yieldVal)
{
    this->yieldVal = yieldVal;
}

void Thread::setTerminated(int terminated)
{
    this->terminated = terminated;
}

CPUState *Thread::getCPUState()
{
    return cpustate;
}

Thread::~Thread()
{
}

/*-----------------------------------------*/
ThreadManager::ThreadManager()
{
    numThreads = 0;
    currentThread = 0;
}

Thread *ThreadManager::getThread(int index)
{
    return threads[index];
}

void ThreadManager::setThread(int dest, int source)
{
    threads[dest] = threads[source];
}

ThreadManager::~ThreadManager()
{
}

bool ThreadManager::AddThread(Thread *thread)
{
    if (numThreads >= 256)
        return false;
    threads[numThreads++] = thread;
    return true;
}

CPUState *ThreadManager::Schedule(CPUState *cpustate)
{

    if (numThreads <= 0)
        return cpustate;

    if (currentThread >= 0)
        threads[currentThread]->cpustate = cpustate;

    if (++currentThread >= numThreads)
        currentThread %= numThreads;

    /*--Terminated Thread Control--begin*/
    if (threads[currentThread]->terminated == 1){
        currentThread++;

    }

    if (currentThread >= numThreads)
        currentThread %= numThreads;
    /*--end*/

    /*--Yield Control--begin*/
    if (threads[currentThread]->yieldVal == 1)
    {
        int tmp = currentThread;
        threads[currentThread]->yieldVal = 0;
        if (++currentThread >= numThreads)
            currentThread %= numThreads;
        if (threads[currentThread]->priority < threads[tmp]->priority)
            currentThread = tmp;
    }
    /*--end*/


    return threads[currentThread]->cpustate;
}
