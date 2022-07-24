
#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

// #define GRAPHICSMODE

#define CAPACITY 20
#define FALSE 0
#define TRUE 1

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

void printf(char *str)
{
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;

    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char *foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;

public:
    MouseToConsole()
    {
        uint16_t *VideoMemory = (uint16_t *)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80)
            x = 79;
        if (x < 0)
            x = 0;
        y += yoffset;
        if (y >= 25)
            y = 24;
        if (y < 0)
            y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

/*--------------PETERSON’S ALGORITHM------------------*/
int turn;
int interested[2];

void enterRegion(int process)
{
    int other;
    other = 1 - process;
    interested[process] = TRUE;
    turn = process;
    while (turn == process && interested[other] == TRUE)
        ;
}

void leaveRegion(int process)
{
    interested[process] = FALSE;
}

/*--------------PRODUCER-CONSUMER------------------*/
int productNum;

void producer()
{
    while (true)
    {
        if (productNum < CAPACITY)
        {
            // enterRegion(0);   // Entering critical region 
            productNum++;
            printf("Producer: ");
            printfHex(productNum);
            printf("    ");
            // leaveRegion(0);   // Leaving critical region 
        }
    }
}

void consumer()
{
    while (true)
    {
        if (productNum > 0)
        {
            // enterRegion(1);   // Entering critical region
            productNum--;
            printf("Consumer: ");
            printfHex(productNum);
            printf("    ");
            // leaveRegion(1);   // Leaving critical region
        }
    }
}

/*---------------TASKS--------------------*/
void taskA_f1()
{
    while (true)
    {
        printf("A");
        for (uint32_t i = 0; i < 100000000; ++i)
        {
        }
    }
}
void taskA_f2()
{
    while (true)
    {
        printf("B");
        for (uint32_t i = 0; i < 100000000; ++i)
        {
        }
    }
}
void taskB_f1()
{
    while (true)
    {
        printf("C");
        for (uint32_t i = 0; i < 100000000; ++i)
        {
        }
    }
}
void taskB_f2()
{
    while (true)
    {
        printf("D");
        for (uint32_t i = 0; i < 100000000; ++i)
        {
        }
    }
}

/*---------Multithreading Functions------------*/
void threadCreate(Thread *thread, GlobalDescriptorTable *gdt, void enrtrypoint())
{
    thread->bind(gdt, enrtrypoint);
}

void threadYield(Thread *t1)
{

    int8_t newP = t1->getPriority() - 1; // Decrease priority of current thread

    if (t1->getPriority() > 0)
        t1->setPriority(newP);

    t1->setYieldVal(1); // Scheduler checks yield value. If it is 1, scheduler
                        // arranges execution according to priorities of threads.
}

void threadTerminate(Thread *t1)
{
    t1->setTerminated(1);
}

void threadJoin(Thread *t1, Thread *t2)
{
    if (t1->getTerminated() == 0) // If t1 is not terminated, t2 cannot execute.
        t2->setTerminated(1);
}

extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- http://www.AlgorithMan.de\n");

    GlobalDescriptorTable gdt;
    TaskManager taskManager;
    ThreadManager threadManagerA, threadManagerB, threadManagerC;

    /*These are objects, actually not thread.*/
    Thread thread0(0, 3); // id, priority
    Thread thread1(1, 3);
    Thread thread2(0, 3);
    Thread thread3(1, 3);
    Thread threadProd(0, 3);
    Thread threadCons(1, 3);

    /*Now, objects are real thread.*/
    threadCreate(&thread0, &gdt, taskA_f1);
    threadCreate(&thread1, &gdt, taskA_f2);
    threadCreate(&thread2, &gdt, taskB_f1);
    threadCreate(&thread3, &gdt, taskB_f2);
    threadCreate(&threadProd, &gdt, producer);
    threadCreate(&threadCons, &gdt, consumer);

    /*Adding threads to thread managers.*/
    threadManagerA.AddThread(&thread0);
    threadManagerA.AddThread(&thread1);
    threadManagerB.AddThread(&thread2);
    threadManagerB.AddThread(&thread3);
    threadManagerC.AddThread(&threadProd);
    threadManagerC.AddThread(&threadCons);

    // threadTerminate(&thread3);
    /*
    //threadTerminate(&thread2);
    threadJoin(&thread2, &thread1);
    */

    //threadYield(&thread2);
    /*A task holds one thread manager.*/
    Task task1(&threadManagerA);
    Task task2(&threadManagerB);
    Task task3(&threadManagerC);

    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    //taskManager.AddTask(&task3); //-> producer-consumer

    InterruptManager interrupts(0x20, &gdt, &taskManager);

    printf("Initializing Hardware, Stage 1\n");

#ifdef GRAPHICSMODE
    Desktop desktop(320, 200, 0x00, 0x00, 0xA8);
#endif

    DriverManager drvManager;

#ifdef GRAPHICSMODE
    KeyboardDriver keyboard(&interrupts, &desktop);
#else
    PrintfKeyboardEventHandler kbhandler;
    KeyboardDriver keyboard(&interrupts, &kbhandler);
#endif
    drvManager.AddDriver(&keyboard);

#ifdef GRAPHICSMODE
    MouseDriver mouse(&interrupts, &desktop);
#else
    MouseToConsole mousehandler;
    MouseDriver mouse(&interrupts, &mousehandler);
#endif
    drvManager.AddDriver(&mouse);

    PeripheralComponentInterconnectController PCIController;
    PCIController.SelectDrivers(&drvManager, &interrupts);

    VideoGraphicsArray vga;

    printf("Initializing Hardware, Stage 2\n");
    drvManager.ActivateAll();

    printf("Initializing Hardware, Stage 3\n");

#ifdef GRAPHICSMODE
    vga.SetMode(320, 200, 8);
    Window win1(&desktop, 10, 10, 20, 20, 0xA8, 0x00, 0x00);
    desktop.AddChild(&win1);
    Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);
    desktop.AddChild(&win2);
#endif

    interrupts.Activate();

    while (1)
    {
#ifdef GRAPHICSMODE
        desktop.Draw(&vga);
#endif
    }
}
