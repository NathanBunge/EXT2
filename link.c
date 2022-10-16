
#include "mkdir_creat.c"
#include "rmdir.c"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[128];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, nINODEs, bmap, imap, INODE_start;
extern char line[256], cmd[32], pathname[256], dname[256], bname[256];


//Creates a hard link from newFile to oldFile - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.6
void myLink(char *pathname, char *pathname2)
{
    int ino = getino(pathname); //Get inode number from pathname
    if (!ino)                   //Checks if file exists
    {
        printf("Source file does not exist.\n");
        return;
    }

    MINODE *oldFileNode = iget(dev, ino);               //Get inode
    if ((oldFileNode->INODE.i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    {
        printf("Cannot have a link to directory.\n");
        return;
    }

    dbname(pathname2); //Gets directory and base name

    int dirino = getino(dname);
    if (!dirino) //Checks if destination directory exists
    {
        printf("Destination directory does not exist.\n");
        return;
    }

    MINODE *newFileNode = iget(dev, dirino);
    if (!((newFileNode->INODE.i_mode) & 0xF000) == 0x4000) // if (S_ISDIR())
    {
        printf("Destination is not a directory.\n");
        return;
    }

    char *base = strdup(bname);
    if (search(newFileNode, base)) //Checks if destination file already exists
    {
        printf("Destination file already exists\n");
        return;
    }

    free(base);                      //Free memory
    enter_name(newFileNode, ino, bname); //Enter ino, bname as new dir_entry into a parent directory

    oldFileNode->INODE.i_links_count++;
    oldFileNode->dirty = 1; //Mark as modified (dirty = 1)

    iput(oldFileNode);
    iput(newFileNode);
}

//Unlinks an existing link - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.7
void myUnlink(char *pathname)
{
    int ino = getino(pathname); //Gets the inode number of pathname
    if (!ino)                   //Checks if path is not there
    {
        printf("Could not find path.\n");
        return;
    }

    MINODE *mip = iget(dev, ino);                 //Gets inode
    if (((mip->INODE.i_mode) & 0xF000) == 0x4000) //Check if it is a directory
    {
        printf("Specified path is a directory\n");
        return;
    }

    mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count == 0) //If link count = 0, then remove the filename
    {
        for (int i = 0; i < 12 && mip->INODE.i_block[i]; i++) //Assume that there is only need to allocate the 12 direct blocks
            bdalloc(mip->dev, mip->INODE.i_block[i]);           //Deallocate a block disk

        idalloc(mip->dev, mip->ino); //Deallocate INODE
    }

    else
    {
        mip->dirty = 1; //Mark as modified (dirty = 1)
    }

    dbname(pathname); //Gets the directory and base name
    MINODE *parent = iget(dev, getino(dname));
    rmChild(parent, bname); //Removes the name entry from the parent directory

    iput(parent);
    iput(mip);
}

//Handles symbolic links - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.8
void mySymlink(char *pathname, char *pathname2) 
{
    char *pathnameDup = strdup(pathname); //Duplicates pathname 
    if (!getino(pathnameDup))             //Check if source path exists
    {
        printf("Specified source path does not exist\n");
        return;
    }

    free(pathnameDup);                 //No need for duplicated pathname
    int ino = mkdir_creat(0, pathname2); //Makes a symbolic link

    MINODE *mip = iget(dev, ino);
    mip->INODE.i_mode = 0xA1A4; //Makes it a symbolic link
    
    strcpy((char *)(mip->INODE.i_block), pathname);

    iput(mip);
}

//Reads the content of symbolic link - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.9
void myReadLink(char *pathname) 
{
    int ino = getino(pathname); //Get inode
    if (!ino)                   //Check if path exists
    {
        printf("Specified path does not exist.\n");
        return;
    }

    MINODE *mip = iget(dev, ino);
    if (!(mip->INODE.i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    {
        printf("Specified path is not a link file.\n");
        return;
    }
    printf("%s\n", (char *)(mip->INODE.i_block));

    iput(mip);
}