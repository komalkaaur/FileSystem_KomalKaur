#include "fsLow.h"
#include "directories.h"
#include "freespace.h"
DirEntry * rootDir;
DirEntry * CWD;
typedef struct {
    DirEntry * parent;
    int index;
    char * lastElement;
    int pathValid;
}  ppinfo;

// helper function to load rootdir info from memory
void initializeRootDir(){
    rootDir = (struct DirEntry *)malloc(sizeof(DirEntry)); 
    LBAread(rootDir, 1, 0); // initialize rootdir directly from memory
    // easy to tell because it is hardcoded in memory as 0? NOTE: idk if this is right yet
}

int main (char * path, ppinfo * ppi) {
    initializeRootDir();

    struct DirEntry *startPath = (struct DirEntry *)malloc(sizeof(DirEntry)); 
    /* made startPath a pointer to a struct DirEntry so that we can check for null attributes might need might not*/
    if (path == NULL || ppi == NULL){ // checking if User is an idiot
        return -1; // bad call
    }

    // temp path initialized
    char *pathCopy = strdup(path);

    char *token1, *saveptr;
    token1 = strtok_r(pathCopy, "/", &saveptr); // tokenizing the path from user 
    //NOTE: pathCopy just a temp to hold the OG path

    // initializing ppi struct for later population
    ppi->parent =NULL;
    ppi->index = -1;
    ppi->lastElement = NULL;
    ppi->pathValid = 1; // risky but we are going to assume path is valid 


    if(path[0] == '/'){ // the reason why its '/' because pathParse is iterative so its going to start from ROOT and work its way out
    //if(strcmp(ppi->parent->fileName, "..")){ // i think this is a better way to check root because you are going to be passing ppi to the function anyway
        startPath = rootDir;
    }else{ 
        startPath = CWD;
    }
    ppi->parent = startPath;

    if(token1 == NULL){
        if(strcmp(path, "/") == 0){ // valid path but its rootdir
            ppi->index = -1; // there is no last element
            ppi->lastElement = NULL;
            ppi->pathValid = 0;
            return ppi->pathValid;
        }
        return -1; // if its an empty string than return -1 idiot user
    }
    while(token1 != NULL){
        
    }








    return 0;
}