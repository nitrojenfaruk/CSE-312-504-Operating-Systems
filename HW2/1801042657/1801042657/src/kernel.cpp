#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <queue.h>
// #define GRAPHICSMODE

// #define PAGE_SIZE 2  ->   in queue_sc.h
#define PAGE_NUM 4  // #page in virtual memory and page table 
#define PM_SIZE 2   // #page in physical memory 

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
void printf(char *str);
void printfHex(uint8_t key);


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

void taskA()
{
    while (true)
        printf("A");
}
void taskB()
{
    while (true)
        printf("B");
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}


class Solution
{
    int arrSize;   
    int hit;
    int miss;
    int writeBack;
    int pageLoaded;

    public:

    char *algorithms[3] = {"bubble", "bubble", "quick"};
    char *arrOrderType[2] = {"random", "sorted"};
    int arr[8] = {6, 1, 4, 7, 14, 7, 34, 2};
    
    PageEntry virtualMem[PAGE_NUM]; // size -> 4 page * 2 = 8 array value
    PageEntry physicalMem[PM_SIZE]; // size -> 2 page
    int pageTable[PAGE_NUM];        // holds page's memory locations      

    int getHit(){ return hit;}
    int getMiss(){ return miss; }
    int getWriteBack(){ return writeBack; }
    int getPageLoaded(){ return pageLoaded; }

    Solution()
    {
        for (int i = 0; i < PAGE_NUM; i++)
            pageTable[i] = -1; // pages are in disk 

        int j = 0, k = 0;
        /* Fill virtual memory with pages */
        for (int i = 0; i < PAGE_NUM; i++)
        {
            virtualMem[i].virtualAdress = i;
            while (j < PAGE_SIZE)
            {
                virtualMem[i].data[j % PAGE_SIZE] = arr[k++];
                virtualMem[i].counter = 0;
                j++;
            }
            j = 0;
        }

        int returnVal = sortProgram(2, "insertion", "random");   // !!! sizeRespect and algorithm
        if(returnVal != -1)   // returnVal = arrSize
            printf("Sorted array: ");
            for (int k = 0; k < returnVal; k++)   
            {
                printfHex(arr[k]);
                printf(" ");
            }
            printf("\n");

    }

    int compare(char str1[], char str2[])
    {
        int flag = 0, i = 0;
        while (str1[i] != '\0' && str2[i] != '\0')
        {
            if (str1[i] != str2[i])
            {
                flag = 1;
                break;
            }
            i++;
        }
        return flag;
    }

    int sortProgram(double sizeRespectToPM, char *algorithm, char *arrOrderType)
    {

        arrSize = sizeRespectToPM * PM_SIZE * PAGE_SIZE;
        if (arrSize / PAGE_SIZE > PAGE_NUM)
        {
            printf("Array size is too big. Virtual memory could not keep the array!");
            return -1;
        }
        printf("Array size: ");
        printfHex(arrSize);
        printf("\n");

        int compResult, select;
        for (int i = 0; i < 3; i++)
        {
            int compResult = compare(algorithm, algorithms[i]);
            if (compResult == 0) // matched
                select = i;
        }

        switch (select)
        {
            case 0:
                bubbleSort(*this, arrSize);
                break;
            case 1:
                insertionSort(*this, arrSize);
                break;
            case 2:
                quickSort(arr, *this, 0, arrSize - 1);
                break;
            default:
                printf("Invalid choice!");
                break;
        }
        return arrSize;
    }

  
    void swap(int *a, int *b)
    {
        int t = *a;
        *a = *b;
        *b = t;
    }

    int partition(Solution &obj, int low, int high)
    {

        int pivot = arr[high];
        int i = (low - 1);

        for (int j = low; j < high; j++)
        {
            if (arr[j] <= pivot)
            {
                i++;
                swap(&arr[i], &arr[j]);
                swap(obj[i], obj[j]);

                /* Change modified bit */
                int tmp = i / PAGE_SIZE;
                int tmp2 = j / PAGE_SIZE;
                int address = lookUpPageTable(tmp);
                int address2 = lookUpPageTable(tmp2);
                if (address != -1)
                {
                    physicalMem[address].modified = 1;
                }
                if (address2 != -1)
                {
                    physicalMem[address2].modified = 1;
                }
                /* USED IN LRU - COUNTER IS INCREMENTED */
                for (int i = 0; i < PM_SIZE; i++)
                {
                    physicalMem[i].counter += physicalMem[i].referenced;
                }
            }
        }

        swap(&arr[i + 1], &arr[high]);
        swap(&obj[i + 1], &obj[high]);

        return (i + 1);
    }

    void quickSort(int arr[], Solution &obj, int low, int high)
    {
        for (size_t i = 0; i < PM_SIZE; i++) 
        {
            physicalMem[i].counter = 0;
        }
        if (low < high)
        {

            int pi = partition(obj, low, high);
            quickSort(arr, obj, low, pi - 1);
            quickSort(arr, obj, pi + 1, high);
        }
    }

    void insertionSort(Solution &obj, int size)
    {
        for (size_t i = 0; i < PM_SIZE; i++) 
        {
            physicalMem[i].counter = 0;
        }

        int key, j;
        for (int i = 1; i < size; i++)
        {
            j = i;
            while (j > 0 && arr[j] < arr[j - 1])
            {
                swap(obj[j], obj[j - 1]);
                int temp = arr[j];
                arr[j] = arr[j - 1];
                arr[j - 1] = temp;
                j--;

                /* Change modified bit */
                int tmp = j / PAGE_SIZE;
                int tmp2 = (j - 1) / PAGE_SIZE;
                int address = lookUpPageTable(tmp);
                int address2 = lookUpPageTable(tmp2);
                if (address != -1)
                {
                    physicalMem[address].modified = 1;
                }
                if (address2 != -1)
                {
                    physicalMem[address2].modified = 1;
                }
                /* USED IN LRU - COUNTER IS INCREMENTED */
                for (int i = 0; i < PM_SIZE; i++)
                {
                    physicalMem[i].counter += physicalMem[i].referenced;
                }
            }

            if ((i % 10 == 0 && i != 0) || i == size - 1)
                printStatistics(i);
        }
    }

    void bubbleSort(Solution &obj, int size)
    {
        int i, j, flag = 0;

        for (size_t i = 0; i < PM_SIZE; i++)
        {
            physicalMem[i].counter = 0;
        }

        for (i = 0; i < size; i++)
        {
            // Last i elements are already in place  -> buyugu sona atar
            for (j = 0; j < size - i - 1; j++)
            {
                if (arr[j] > arr[j + 1])
                {
                   
                    swap(obj[j], obj[j + 1]);
                    // //swap(arr[j], arr[j + 1]);
                    int temp = arr[j];
                    arr[j] = arr[j + 1];
                    arr[j + 1] = temp;

                    /* Change modified bit */
                    int tmp = j / PAGE_SIZE;
                    int tmp2 = (j + 1) / PAGE_SIZE;
                    int address = lookUpPageTable(tmp);
                    int address2 = lookUpPageTable(tmp2);
                    if (address != -1)
                    {
                        physicalMem[address].modified = 1;
                    }
                    if (address2 != -1)
                    {
                        physicalMem[address2].modified = 1;
                    }
                    flag = 1;

                    /* USED IN LRU - COUNTER IS INCREMENTED */
                    for (int i = 0; i < PM_SIZE; i++)
                    {
                        physicalMem[i].counter += physicalMem[i].referenced;
                    }
                }
            }

            if ((i % 10 == 0 && i != 0) || i == size - 1)
                printStatistics(i);

            if (flag == 0)
                break;
        }
    }

    void swap(int &i, int &j)
    {
        int temp = i;
        j = i;
        i = temp;
    }

    int &operator[](int i)
    {
        int pageIndex = i / PAGE_SIZE;
        int offset = i % PAGE_SIZE;
        int frameNum = lookUpPageTable(pageIndex);
        if (frameNum != -1)
        {
            physicalMem[frameNum].referenced = 1; 
            hit++;
        }
        else
            makePageReplacement(pageIndex);

        return physicalMem[pageIndex].data[offset];
    }

    /* Return position of page in memory or -1(disk) */
    int lookUpPageTable(int pageIndex)
    {
        return pageTable[pageIndex];
    }

    void makePageReplacement(int pageIndex)
    {
        miss++;       
        pageLoaded++;
        int flag = 0;
        int cursor = 0;
        int address = 0;
        int select = 0;   // !!! choose algorithm 

        /* Fill physical memory - only enter at the beginning */
        for (int i = 0; i < PM_SIZE; i++)
        {
            if (physicalMem[i].valid == 0)
            {
                physicalMem[i] = virtualMem[pageIndex];
                physicalMem[i].valid = 1;
                physicalMem[i].referenced = 1;
                pageTable[pageIndex] = i; // update page table
                flag = 1;
                break;
            }
        }

        if (flag == 0)
        {

            if (select == 0)
            {
                // fifo
                address = physicalMem[cursor].virtualAdress;
                if (physicalMem[cursor].modified == 1)
                {
                    virtualMem[address] = physicalMem[cursor]; // write old page to the vm if it was modified
                    virtualMem[address].valid = 0;
                    writeBack++;
                }
                /* Updating page table */
                pageTable[pageIndex] = cursor; // new page location in memory
                pageTable[address] = -1;       // old page location in memory
                /* Page replacement */
                physicalMem[cursor] = virtualMem[pageIndex]; // new page is loaded
                physicalMem[cursor].valid = 1;

                cursor++;
                cursor %= PAGE_NUM;
            }
            else if (select == 1)
            {

                // second chance
                Queue queue(PM_SIZE);
                for (int i = 0; i < PM_SIZE; i++)
                {
                    PageEntry tmp = physicalMem[i];
                    queue.add(tmp);
                }

                for (int i = 0; i < PM_SIZE; i++)
                {
                    if (queue.front_sc().referenced == 0)
                    {
                        queue.front_sc() = virtualMem[pageIndex];
                        break;
                    }
                    else
                    {
                        PageEntry temp = queue.remove();
                        temp.referenced = 0;
                        queue.add(temp);
                    }
                }

                for (int i = 0; i < PM_SIZE; i++)
                {
                    address = physicalMem[i].virtualAdress;
                    if (physicalMem[i].modified == 1)
                    {
                        virtualMem[address] = physicalMem[i]; // write old page to the vm if it was modified
                        virtualMem[address].valid = 0;
                        writeBack++;
                    }

                    /* Page Replacement */
                    PageEntry tmp = queue.remove();
                    physicalMem[i] = tmp;
                    physicalMem[i].valid = 1;

                    /* Updating page table */
                    // !!!!
                    pageTable[pageIndex] = tmp.virtualAdress; // new page location in memory
                    pageTable[address] = -1;                  // old page location in memory
                }
            }
            else if(select == 2)
            {
                
                // least recently used
                int index = 0;
                int min = physicalMem[0].counter;
                for (int i = 0; i < PM_SIZE; i++)
                {
                    if (physicalMem[i].counter < min)
                    {
                        min = physicalMem[i].counter;
                        index = i;
                    }
                }

                address = physicalMem[index].virtualAdress;
                if (physicalMem[index].modified == 1)
                {
                    virtualMem[address] = physicalMem[index]; // write old page to the vm if it was modified
                    virtualMem[address].valid = 0;
                    writeBack++;
                }

                /* Updating page table */
                pageTable[pageIndex] = index; // new page location in memory
                pageTable[address] = -1;      // old page location in memory
                /* Page replacement */
                physicalMem[index] = virtualMem[pageIndex]; // new page is loaded
                physicalMem[index].valid = 1;
            }
        }
    }

    void printStatistics(int i)
    {
        printf("Time: ");
        printfHex(i);
        printf("\n");
        printf("hit: ");
        printfHex(hit);
        printf("\n");
        printf("miss: ");
        printfHex(miss);
        printf("\n");
        printf("write-back: ");
        printfHex(writeBack);
        printf("\n");
        printf("page loaded: ");
        printfHex(pageLoaded);
        printf("\n");
        printf("--------------");
        printf("\n");
    }
};



extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{

    GlobalDescriptorTable gdt;

    uint32_t *memupper = (uint32_t *)(((size_t)multiboot_structure) + 8);
    size_t heap = 10 * 1024 * 1024;
    MemoryManager memoryManager(heap, (*memupper) * 1024 - heap - 10 * 1024);

   

    Solution obj;
    printf("hit: ");
    printfHex(obj.getHit());
    printf("\n");
    printf("miss: ");
    printfHex(obj.getMiss());
    printf("\n");
    printf("write-back: ");
    printfHex(obj.getWriteBack());
    printf("\n");
    printf("page loaded: ");
    printfHex(obj.getPageLoaded());

    
    // Task task1(&gdt, taskA);
    // Task task2(&gdt, taskB);
    // taskManager.AddTask(&task1);
    // taskManager.AddTask(&task2);
    

    TaskManager taskManager;
    InterruptManager interrupts(0x20, &gdt, &taskManager);

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

    drvManager.ActivateAll();

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
