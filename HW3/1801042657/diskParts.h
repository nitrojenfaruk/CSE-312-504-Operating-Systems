#define FILE_TYPE (0)
#define DIRECTORY_TYPE (1)

#define DIRECT (10)
#define INDIRECT1 (1)
#define INDIRECT2 (1)
#define INDIRECT3 (1)

#define NAME_LENGTH (14)

#define KB (1024)
#define MB (1024 * 1024)

#define INODE_NUM (256)

struct Inode {
    uint32_t size;
    uint8_t  type;
    int32_t  creationTime;
    int32_t  modificationTime;
    uint8_t connected;
    uint16_t direct[DIRECT];        // 10
    uint16_t singleInd[INDIRECT1];  // 1
    uint16_t doubleInd[INDIRECT1];  // 1
    uint16_t tripleInd[INDIRECT1];  // 1
}__attribute__ ((__packed__));

struct SuperBlock {
    uint32_t lastUsedInodeIndex;
    uint32_t lastUsedInodeAddress; // initially inode[0]
    uint16_t blockSize;  // user-defined
    uint16_t blocksNum; 
    uint16_t inodesNum; // 256 - hardcoded
    uint32_t blockOffset; 
    uint32_t inodeOffset; // inception of root 
    uint8_t bitMap[1024];  
} __attribute__ ((__packed__));


struct File {
    uint16_t iNode;  // 2 byte
    char name[NAME_LENGTH];  // 14 byte
} __attribute__((__packed__));