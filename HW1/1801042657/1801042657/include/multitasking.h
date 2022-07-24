 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>


namespace myos
{

    class Task;
    class TaskManager;
    class Thread;
    class ThreadManager;

    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    
    
    class Task
    {
        friend class TaskManager;
        friend class ThreadManager;

        private:
            common::uint8_t stack[4096]; // 4 KiB
            CPUState* cpustate;
            ThreadManager* threadManager;

        public:
            Task(GlobalDescriptorTable *gdt, void entrypoint());
            Task(ThreadManager *threadManager);
            ~Task();
    };
    
    
    class TaskManager
    {
        private:
            Task *tasks[256];
            int numTasks;
            int currentTask;

        public:
            TaskManager();
            ~TaskManager();
            bool AddTask(Task *task);
            CPUState *Schedule(CPUState *cpustate);
    };


    class Thread
    {
        friend class ThreadManager;
        private:
            common::uint8_t stack[4096]; 
            CPUState *cpustate;
            int yieldVal;
            int priority;
            int id;
            int terminated;

        public : 
            Thread(GlobalDescriptorTable *gdt, void entrypoint());
            void bind(GlobalDescriptorTable *gdt, void entrypoint());
            Thread(int id, int priority);
            int getId();
            int getPriority();
            int getYieldVal();
            int getTerminated();
            void setId(int id);
            void setPriority(int priority);
            void setYieldVal(int yieldVal);
            void setTerminated(int terminated);
            CPUState* getCPUState();
            ~Thread();
    };


    class ThreadManager
    {
        private:
            Thread* threads[256];
            int numThreads;
            int currentThread;
        public:
            ThreadManager();
            ~ThreadManager();
            Thread *getThread(int index);
            void setThread(int dest, int source);
            bool AddThread(Thread* thread);
            CPUState* Schedule(CPUState* cpustate);
    };
    
    
    
}


#endif