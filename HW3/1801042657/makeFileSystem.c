#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include "diskParts.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Wrong input..\n");
        return -1;
    }

    uint16_t blockSize = atoi(argv[1]);
    char *datFile = argv[2];

    size_t totalSize = 4 * MB;
    totalSize -= (INODE_NUM * sizeof(struct Inode) + sizeof(struct SuperBlock));

    uint8_t fileSystem[4 * MB] = {0};
    struct SuperBlock *superB = (struct SuperBlock *)fileSystem;
    superB->blockSize = blockSize;
    superB->blocksNum = totalSize / (blockSize * KB);
    superB->inodesNum = INODE_NUM;
    superB->blockOffset = 4 * MB - (superB->blocksNum * superB->blockSize * KB);
    superB->inodeOffset = superB->blockOffset - (INODE_NUM * sizeof(struct Inode));
    superB->lastUsedInodeAddress = superB->inodeOffset;
    superB->lastUsedInodeIndex = 0;
    for (size_t i = 0; i < 1024; i++)   // fill bitmap with 0
        superB->bitMap[i] = 0;

    struct Inode *root = (struct Inode *)(fileSystem + superB->inodeOffset);
    root->size = 2 * sizeof(struct File);  // for current and parent
    root->type = DIRECTORY_TYPE;  
    root->creationTime = (int32_t)time(NULL);
    root->connected = 1;

    struct File *file = (struct File *)(fileSystem + superB->blockOffset);
    file->iNode = 0;   // current inode
    strcpy(file->name, ".");
    root->direct[0] = file;
    file++;   // shifting to the right (1 file size)
    file->iNode = 0;   // parent inode
    strcpy(file->name, "..");
    root->direct[1] = file; 
    superB->bitMap[0] = 1;
    superB->bitMap[1] = 1;

    FILE *fileP;
    fileP = fopen(datFile, "wb+");
    if (fileP == NULL){
        printf("File could not be opened..");
        return -1;
    }

    /* Writiing to the file */
    if (1 != fwrite(fileSystem, 4 * MB, 1, fileP)) {
        printf("Could not write to file..\n");
        return -1;
    }
    fclose(fileP);

    return 0;
}