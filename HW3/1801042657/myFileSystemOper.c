#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "diskParts.h"

#define PATH_LENGTH (30)

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Wrong input..\n");
        return -1;
    }
    argc--;
    argv++;

    char *fileName = *argv;

    uint8_t filesystem[4 * MB] = {0};

    FILE *fileP;
    fileP = fopen(fileName, "rb");
    if (NULL == fileP)
    {
        printf("File could not be opened..");
        return -1;
    }
    if (1 != fread(filesystem, 4 * MB , 1, fileP))
    {
        printf("Could not read from file..\n");
        return -1;
    }
    fclose(fileP);

    argc--;  // 2
    argv++;  // 0 -> mkdir 1 -> \user
    
    char *path = argv[1];

    struct Inode *root;
    struct SuperBlock *superB = (struct SuperBlock *)filesystem;
    root = (struct Inode *)(filesystem + superB->inodeOffset);

    if (strcmp(*argv, "mkdir") == 0)
        mkdir(filesystem, path, root);

    FILE *fp = fopen(fileName, "wb+");
    if (fp == NULL)
    {
        printf("File could not be opened..");
        return -1;
    }
    if (fwrite(filesystem, 4 * MB, 1, fp) != 1){
        printf("Could not write to file..\n");
        return -1;
    }
    fclose(fp);
    return 0;
}


uint8_t addDirectory(char *path, struct Inode *rootInode, char *realPath, struct SuperBlock *s)
{

    uint8_t length = 0;  // path bound, \user\ysa -> 5
    uint8_t lastAddress;
    char *dirName = path + 1; //  \user -> user

    
    for (uint8_t i = 1; *(path + i) != '\\' && *(path + i) != '\0'; ++i)
        length++;

    *(dirName + length) = '\0';

    uint8_t count = 0;
    for (uint8_t i = 1; *(path + i) != '\0'; ++i)
    {
        if (*(path + i) == '\\')
            count++;
    }

    uint8_t countReal = 0;
    for (uint8_t i = 1; *(realPath + i) != '\0'; ++i)
    {
        if (*(realPath + i) == '\\')
            countReal++;
    }

    uint8_t i = 0;
    while (i < 8)
    {

        struct File *file = (struct File *)rootInode->direct[i];
        
        /* Empty space is found */
        if (file == NULL)
        {   //
            lastAddress = (s->lastUsedInodeIndex == 0) ? 1 : s->lastUsedInodeIndex;
            struct Inode *newInode = (struct Inode *)(s->inodeOffset + lastAddress * sizeof(struct Inode));
            newInode->creationTime = (int32_t)time(NULL);
            newInode->size = 2 * sizeof(struct File);
            newInode->type = DIRECTORY_TYPE;
            newInode->connected = 1;
            //
            struct File *newFile = (struct File *)(s->blockOffset + i * s->blockSize);  
            s->lastUsedInodeAddress += sizeof (struct Inode);
            newFile->iNode = (++s->lastUsedInodeIndex);   
            strcpy(newFile->name, ".");
            newInode->direct[0] = newFile;
            newFile++;
            newFile->iNode = (newFile - s->blockSize * 2)->iNode;
            strcpy(newFile->name, "..");
            newInode->direct[1] = newFile;    
            //
            rootInode->direct[i] = newFile;
            rootInode->size += sizeof (struct File);
            //
            return 1;
        }
        /* File already exists */
        if (strcmp(file->name, dirName) == 0 && count == countReal) return -1;

        i++;

        // recursive call
        addDirectory(path + length, (struct Inode *)(s->inodeOffset + file->iNode * sizeof(struct Inode)) , realPath, s);
    }
}

void mkdir(uint8_t *fileSystem,  char* path, struct Inode* root)
{
    
    char tmpPath[PATH_LENGTH];
    struct SuperBlock *superB = (struct SuperBlock *)fileSystem;

    for (uint8_t i = 0; *(path + i) != '\0'; ++i)
        tmpPath[i] = *(path + i);

    uint8_t result = addDirectory(tmpPath, root, path, superB);

    if (result == 1) 
        printf("Directory added!");
    else if(result == 0)
        printf("Cannot create directory: No such file or directory\n");
    else if(result == -1)
        printf("Cannot create directory: File exists\n");
        
}
