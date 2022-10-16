

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

//Prints the stats of the current directory. This includes size, blocks, directory, link, file, along with other INODE information
void myStat(char *pathname)
{
    int ino = getino(pathname); //Gets inode
    if (!ino)                   //Check to see if file doesn't exists
    {
        printf("File not found.\n");
        return;
    }

    MINODE *mip = iget(dev, ino);
    INODE ip = mip->INODE;

    dbname(pathname); //Gets the directory and base name

    printf("File: %s\n", bname);
    printf("Size: %d\n", ip.i_size);
    printf("Blocks: %d\n", ip.i_blocks);

    if ((ip.i_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("Type: Reg\n");
    if ((ip.i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("Type: Dir\n");
    if ((ip.i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("Type: Link\n");

    time_t a = (time_t)ip.i_atime, c = (time_t)ip.i_ctime, mod = (time_t)ip.i_atime; //Gets time information

    printf("Inode: %d\n", ino);
    printf("Links: %d\n", ip.i_links_count);
    printf("Access Time: %s\n", ctime(&a));
    printf("Modify Time: %s\n", ctime(&mod));
    printf("Change Time: %s\n", ctime(&c));
    printf("Device: %d\n", mip->dev);
    printf("UID: %d\n", ip.i_uid);
    printf("GID: %d\n", ip.i_gid);

    iput(mip);
}

//Change permissions of a given file - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.10
void myChmod(char *pathname)
{
    int ino = getino(pathname); //Gets Inode
    if (!ino)                   //Check if the file does not exist
    {
        printf("File does not exist.\n");
        return;
    }

    MINODE *mip = iget(dev, ino);
    int new;
    char pathname2[256];
    sscanf(pathname2, "%o", &new);

    int old = mip->INODE.i_mode; //Gets the current mode
    old >>= 9;                   //Changes the permission bits to octal value by shifting right - Looked up online
    new |= (old << 9);           //Bitwise inclusive or - Looked up online

    mip->INODE.i_mode = new; //Sets the mode to its new octual value
    mip->dirty = 1;          //Mark as modified (dirty = 1)

    iput(mip);
}

//Changes the timestamp of a given file or creates a file - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.10
void myUtime(char* pathname)
{
    int ino = getino(pathname); //Gets Inode
    if (ino == -1)              //Check if path does not exist
    {
        printf("Not a path.\n");
        return;
    }

    MINODE *mip = iget(dev, ino);

    time_t t = time(0L); //Found time(0L) in K.C. Wang's textbook
    mip->INODE.i_atime = t; //Updates access time
    mip->INODE.i_ctime = t; //Updates change time
    mip->INODE.i_mtime = t; //Updates modification time

    mip->dirty = 1; //Mark as modified (dirty = 1)

    iput(mip);
}