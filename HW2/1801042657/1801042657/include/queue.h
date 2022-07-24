using namespace myos;
#define PAGE_SIZE 2

namespace myos
{
    typedef struct PageEntry
    {
        int referenced;
        int counter;
        int modified;
        int valid; 
        int data[PAGE_SIZE];
        int virtualAdress;
    } PageEntry;

    class Queue {

        public:
            int size, capacity, front, rear;
            PageEntry* array;
           
        Queue(int cap)
        {
            capacity = cap;
            front = size = 0;

            rear = capacity - 1;
            array = new PageEntry[capacity];
        }

        int isFull()
        {
            return (size == capacity);
        }
        
        void add(PageEntry item)
        {
            if (isFull()){
                return;
            }
            rear = (rear + 1) % capacity;
            array[rear] = item;
            size = size + 1;
        }
        
        PageEntry remove()
        {

            PageEntry item = array[front];
            front = (front + 1) % capacity;
            size = size - 1;
            return item;
        }
        
        PageEntry front_sc()
        {
            return array[front];
        }
        
        PageEntry rear_sc()
        {
            return array[rear];
        }
    };

      
}