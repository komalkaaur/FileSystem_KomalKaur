/**************************************************************
 * Class:  CSC-415-01 Fall 2023
 * Names: Collins Gichohi, Louis Houston, Komaldeep Kaur,
 * 			Raymond Liu, Aleia Natividad
 * Student IDs: 922440815, 922379442, 920198887, 916624142,
 *				922439437
 * GitHub Name: gsnilloC, LouisHouston, komalkaaur, Airray117
 *				leileigoose
 * Group Name: The Strugglers
 * Project: Basic File System
 *
 * File: directories.c
 *
 * Description:
 * This file contains the initDirectory() function which
 * intializes a directory and the parsePath() function which
 * is used to turn pathnames into actual Directory Entries.
 * It also contains other directory functions and helper 
 * functions used to handle directories.
 **************************************************************/

#include "directories.h"
#include <stdlib.h>

DirEntry *rootDir;
DirEntry *cwd;

// Function: initDirectory
// Description: Sets initial metadata for each entry and handles the "." and ".." structure for file systems.
// The directory is then written to disk.
// Parameters:
//   - int initialDirEntries: The initial number of directory entries.
//   - uint64_t blockSize: The block size for the file system.
//   - DirEntry *parent: Pointer to the parent directory. If NULL, the function handles the root directory case.
// Returns:
//   - int: Start block of where directory entries start on disk (or -1 in case of an error).
//   Note: The function dynamically allocates memory for directory entries and should be freed by the caller
//   if the parent is NULL (root directory case).
int initDirectory(int initialDirEntries, uint64_t blockSize, DirEntry *parent)
{
    // there has to be at least 1 directory entry
    if (initialDirEntries <= 0)
    {
        printf("Error: Invalid number of initial directory entries.\n");
        return -1;
    }

    // calculations needed for allocation and writing
    int sizeOfDE = sizeof(DirEntry);
    int rootDirSizeBytes = initialDirEntries * sizeOfDE;
    int rootDirSizeBlocks = (initialDirEntries * sizeOfDE + blockSize - 1) / blockSize;
    int actualDirEntries = rootDirSizeBytes / sizeof(DirEntry);

    // allocate memory for the directory entries
    DirEntry *directoryEntries = malloc(actualDirEntries * sizeof(DirEntry));

    // check if malloc succeeded
    if (directoryEntries == NULL)
    {
        printf("Error: Memory allocation failed for directory entries.\n");
        return -1;
    }

    // Initialize directory entries with default values
    for (int i = 0; i < actualDirEntries; ++i)
    {

        directoryEntries[i].fileName[0] = '\0'; // Ensure the string is initially empty
        strcpy(directoryEntries[i].fileName, "-1");
        directoryEntries[i].extentTable = NULL;
        directoryEntries[i].lastModified = 0;
        directoryEntries[i].lastAccessed = 0;
        directoryEntries[i].timeCreated = 0;
        directoryEntries[i].isDirectory = 0;
        directoryEntries[i].directoryStartBlock = -1;
        directoryEntries[i].directoryBlockCount = -1;
    }

    time_t t = time(NULL);

    // create new directory and set its metadata
    strcpy(directoryEntries[0].fileName, ".");
    directoryEntries[0].size = actualDirEntries * sizeof(DirEntry);
    directoryEntries[0].isDirectory = 1;
    directoryEntries[0].timeCreated = t;
    directoryEntries[0].lastAccessed = t;
    directoryEntries[0].lastModified = t;
    directoryEntries[0].extentTable = allocateBlocks(rootDirSizeBlocks, rootDirSizeBlocks);
    directoryEntries[0].directoryStartBlock = directoryEntries[0].extentTable->start;
    directoryEntries[0].directoryBlockCount = rootDirSizeBlocks;

    // return start block of where directory entries start on disk
    int startBlock = directoryEntries[0].extentTable->start;

    DirEntry *firstEntryPtr;

    // handle root directory case
    if (parent != NULL)
    {
        firstEntryPtr = parent;

        // Copy details for the ".." directory entry (parent directory)
        strcpy(directoryEntries[1].fileName, "..");
        directoryEntries[1].size = firstEntryPtr->size;
        directoryEntries[1].isDirectory = firstEntryPtr->isDirectory;
        directoryEntries[1].timeCreated = firstEntryPtr->timeCreated;
        directoryEntries[1].lastAccessed = firstEntryPtr->lastAccessed;
        directoryEntries[1].lastModified = firstEntryPtr->lastModified;
        directoryEntries[1].extentTable = firstEntryPtr->extentTable;
        directoryEntries[1].directoryStartBlock = firstEntryPtr->directoryStartBlock;
        directoryEntries[1].directoryBlockCount = firstEntryPtr->directoryBlockCount;
    }
    else if (parent == NULL)
    {
        firstEntryPtr = &directoryEntries[0];

        // Copy details for the ".." directory entry (parent directory)
        strcpy(directoryEntries[1].fileName, "..");
        directoryEntries[1].size = firstEntryPtr->size;
        directoryEntries[1].isDirectory = firstEntryPtr->isDirectory;
        directoryEntries[1].timeCreated = firstEntryPtr->timeCreated;
        directoryEntries[1].lastAccessed = firstEntryPtr->lastAccessed;
        directoryEntries[1].lastModified = firstEntryPtr->lastModified;
        directoryEntries[1].extentTable = firstEntryPtr->extentTable;
        directoryEntries[1].directoryStartBlock = firstEntryPtr->directoryStartBlock;
        directoryEntries[1].directoryBlockCount = firstEntryPtr->directoryBlockCount;
    }

    // write it to disk
    LBAwrite(directoryEntries, rootDirSizeBlocks, startBlock);

    // free the directory entries the first time around
    if (parent == NULL)
    {
        free(directoryEntries);
    }

    return startBlock; // Return start block of directory entries
}

// Function: loadRootDir
// Description: Load the root directory off disk to be used by the program.
// Returns:
//   - int: 0 on success
//         -1 on failure.
int loadRootDir()
{
    // This is hardcoded but should not be; had trouble using variables
    int rootDirSize = 6 * 512;

    // Malloc memory for the directory and check if the malloc succeeded
    rootDir = (DirEntry *)malloc(rootDirSize);

    if (rootDir == NULL)
    {
        return -1;
    }

    // Read the rootDir back from disk
    LBAread(rootDir, 6, 6);
    return 0;
}

// Function: parsePath
// Description: Used to retrieve actual directory entries when given a path name.
// Parameters:
//   - char *pathname: The path name to parse.
//   - ppInfo *ppi: Pointer to a structure (ppInfo) that will hold information about the parsed path.
// Returns:
//   -  0: Success
//   - -1: Invalid parameters (pathname or ppi is NULL)
//   - -2: Path error (directory not found or not a directory)
//   Note: The function dynamically allocates memory for the last element in the path (ppi->lastElement),
//   so it's the caller's responsibility to free this memory when it's no longer needed.

int parsePath(char *pathname, ppInfo *ppi)
{
    // check if parameters are valid
    if (pathname == NULL || ppi == NULL)
    {
        return -1;
    }

    DirEntry *startPath = malloc(sizeof(DirEntry));
    DirEntry *parent = malloc(sizeof(DirEntry));

    // check if path is absolute or relative
    if (pathname[0] == '/')
    {
        startPath = rootDir;
    }
    else
    {
        startPath = cwd;
    }

    parent = startPath;

    char *token1;
    char *saveptr = NULL;

    // tokenize path using "/" as a delimiter
    token1 = strtok_r(pathname, "/", &saveptr);

    // check if it path is root if there is no token after "/"
    if (token1 == NULL)
    {
        if (strcmp(pathname, "/") == 0)
        {
            ppi->parent = parent;
            ppi->index = -1;
            ppi->lastElement = NULL;
            return 0;
        }

        return -1;
    }

    char *token2;
    while (token1 != NULL)
    {
        // look for name in directory entries one by one
        int index = findEntryInDir(parent, token1);
        token2 = strtok_r(NULL, "/", &saveptr);

        // If this is the last token, store the information in ppInfo
        if (token2 == NULL)
        {
            ppi->parent = parent;
            ppi->lastElement = strdup(token1);
            ppi->index = index;

            return 0;
        }

        // directory was not found in the parent
        if (index == -1)
        {
            return -2;
        }

        // check if dir is a directory
        if (!isDir(&parent[index]))
        {
            return -2;
        }

        // load next directory level
        DirEntry *temp = LoadDir(&(parent[index]));

        // free memory if parent is not the starting path
        if (parent != startPath)
        {
            free(parent);
        }

        parent = temp;
        token1 = token2;
    }

    // free memory allocated for starting path
    free(startPath);
    return 0;
}

void testLoadDir(char *pathname)
{
    ppInfo *pathInfo = (ppInfo *)malloc(sizeof(ppInfo));

    int retVal = parsePath(pathname, pathInfo);

    if (retVal != 0)
    {
        printf("Parse path failed");
        free(pathInfo);
        return;
    }

    DirEntry *parent = pathInfo->parent;

    int index = findEntryInDir(parent, pathInfo->lastElement);

    int startBlock = parent[index].directoryStartBlock;
    int count = parent[index].directoryBlockCount;

    printf("start Block: %d\n", startBlock);
    printf("directoryBlockCount: %d\n", count);

    DirEntry *temp = LoadDir(&(parent[index]));

    if (temp != NULL)
    {
        // Access and print information about each entry in the loaded directory
        for (int i = 0; i < 2; ++i)
        {
            printf("Entry %d: File Name: %s, Size: %ld\n", i, temp[i].fileName, temp[i].size);
        }

        free(temp);
    }
    else
    {
        printf("Failed to load directory structure.\n");
    }

    free(pathInfo);
}

// Function: isDir
// Description: Checks whether a given directory entry represents a directory.
// Parameters:
//   - DirEntry *entry: Pointer to the directory entry to be checked.
// Returns:
//   - int: 1 if the entry represents a directory, 0 if not, -1 for invalid input.
int isDir(DirEntry *entry)
{
    // Check for invalid input
    if (entry == NULL)
    {
        return -1; // Error code for invalid input
    }

    // Return directory status (1 for directory, 0 for not a directory)
    return entry->isDirectory;
}

// Function: findEntryInDir
// Description: Finds the index of a directory entry with a given name in a directory.
// Parameters:
//   - DirEntry *directory: Pointer to the directory in which to search.
//   - char *entryName: Name of the directory entry to find.
// Returns:
//   - int: Index of the found directory entry, or -1 if not found or for invalid inputs.
int findEntryInDir(DirEntry *directory, char *entryName)
{
    if (directory == NULL || entryName == NULL)
    {
        printf("l INPUT");
        return -1; // Error code for invalid inputs
    }

    int numEntries = directory->size / sizeof(DirEntry);

    for (int i = 0; i < numEntries; i++)
    {
        if (strcmp(directory[i].fileName, entryName) == 0)
        {
            return i; // Return the index of the found directory entry
        }
    }

    return -1; // Return -1 if the entry is not found in the directory
}

// Function: findNextAvailableEntryInDir
// Description: Finds the index of the next available entry in a directory.
// Parameters:
//   - DirEntry *directory: Pointer to the directory in which to search.
// Returns:
//   - int: Index of the next available entry, or -1 if not found or for invalid inputs.
int findNextAvailableEntryInDir(DirEntry *directory)
{

    ppInfo *pathInfo = malloc(sizeof(ppInfo));

    int returnVal = parsePath("/", pathInfo);

    if (returnVal != 0)
    {
        free(pathInfo);
        printf("HERE");
        return -1;
    }

    DirEntry *entry = pathInfo->parent;

    if (entry == NULL)
    {
        printf("NO HERE");
        return -1; // Error code for invalid input
    }

    int numEntries = entry->size / sizeof(DirEntry);

    for (int i = 0; i < numEntries; i++)
    {
        if (strcmp(entry[i].fileName, "-1") == 0)
        {
            return i; // Return the index of the first available entry
        }
    }

    return -1; // Return -1 if no available entry is found in the directory
}

// Function: LoadDir
// Description: Loads a directory structure from disk based on the given directory entry.
// Parameters:
//   - DirEntry *entry: Pointer to the directory entry to load.
// Returns:
//   - DirEntry *: Pointer to the loaded directory structure, or NULL for invalid inputs or failures.
DirEntry *LoadDir(DirEntry *entry)
{
    if (entry == NULL || entry->isDirectory == 0)
    {
        return NULL; // Return NULL if the entry is not a valid directory
    }

    // Calculate the number of blocks to read for the directory based on its extent information
    int blockSize = 512;
    int totalBlocksToRead = entry->directoryBlockCount;
    int startBlock = entry->directoryStartBlock;

    printf("Start Block in load: %d", totalBlocksToRead);
    // Calculate the size in bytes to read
    uint64_t sizeToRead = totalBlocksToRead * blockSize;

    // Allocate memory to store the directory structure
    DirEntry *directoryStructure = (DirEntry *)malloc(sizeToRead);

    // Read the directory structure from disk using LBAread
    uint64_t blocksRead = LBAread(directoryStructure, totalBlocksToRead, 13);

    // Perform error handling and return NULL in case of failure
    if (blocksRead != totalBlocksToRead)
    {
        printf("RETURNING NULL");
        free(directoryStructure); // Free allocated memory in case of failure
        return NULL;
    }

    if (directoryStructure != NULL)
    {
        // Access and print information about each entry in the loaded directory
        for (int i = 0; i < 3; ++i)
        {
            printf("Entry %d: File Name: %s, Size: %ld\n", i, directoryStructure[i].fileName, directoryStructure[i].size);
        }
    }
    else
    {
        printf("Failed to load directory structure.\n");
    }

    printf("LoadDir Success\n");
    // Returning the directory structure
    return directoryStructure;
}


/// @brief Deletes the dirEntry by setting all of its values to initial values
/// @param directory
/// @return Returns 0 if successful returns 1 if unsuccessful
int deleteDirEntry(DirEntry *directory)
{
    if (directory == NULL)
    {
        return -1;
    }

    strcpy(directory[0].fileName, "-1");
    directory->size = 0;
    directory->extentTable = NULL;
    directory->lastModified = 0;
    directory->lastAccessed = 0;
    directory->timeCreated = 0;
    directory->isDirectory = 0;

    // series of checks to make sure the dir is now gone?
    return 0;
}