

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

//Returns the ideal length - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
int ideal_length(int name_len)
{
    return 4 * ((8 + name_len + 3) / 4); 
}

//Reads to last entry in the block, allocates memory for new block, and writes entry inside existing block - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
void enter_name(MINODE *pip, int myino, char *myname)
{
    int myNameLen = strlen(myname);
    int needLen = ideal_length(myNameLen); //Get the ideal length

    INODE ip = pip->INODE;

    for (int i = 0; i < 12; i++) //Traverses through data block of parent DIR, assume there are only 12 direct blocks
    {
        char buf[BLKSIZE];
        DIR *dp = (DIR *)buf;
        char *cp = buf;

        if (ip.i_block[i] == 0) //Checks if there is no space left in existing block
        {
            //Allocates space for a new data block and increases parent size by BLKSIZE
            ip.i_block[i] = balloc(dev);                                            //Allocates space for a new block
            get_block(pip->dev, ip.i_block[i], buf);                                //Reads new block into buf
            *dp = (DIR){.inode = myino, .rec_len = BLKSIZE, .name_len = myNameLen}; //Enter new entry as the first entry in the new data block with rec_len = BLKSIZE.
            strncpy(dp->name, myname, dp->name_len);
            return;
        }

        get_block(pip->dev, ip.i_block[i], buf); //Get parent’s data block into a buf
        while (cp + dp->rec_len < buf + BLKSIZE) //Traverses to the last entry in the block until dp points to the last entry in the block
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        int remain = (dp->rec_len - ideal_length(dp->name_len)); //Remain holds the remaining space (directory length - need length)
        if (remain >= needLen)                                   //If there is remaining space
        {
            //Puts the new entry in the last entry position and trims the remain to the needed length
            dp->rec_len = ideal_length(dp->name_len); //Set rec_len as the ideal length of dp's name length
            cp += dp->rec_len;                        //Add the rec_len to cp
            dp = (DIR *)cp;
            *dp = (DIR){.inode = myino, .rec_len = remain, .name_len = myNameLen}; //Update dp attributes
            strncpy(dp->name, myname, dp->name_len);
            put_block(pip->dev, ip.i_block[i], buf); //Writes to disk
            pip->dirty = 1;                          //Mark as modified (dirty = 1)
            return;
        }
    }
}

//Makes either a file (dir = 0) or directory (dir = 1) - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
int mkdir_creat(int isDir, char *name)
{
    dbname(name); //Gets dirname and basename

    int parent = getino(dname); // Gets the inode number of the parent directory
    if (!parent)                //Checks if the parent path does not exists
    {
        printf("Error: parent path does not exist\n");
        return -1; //Error, so exits function
    }

    MINODE *pip = iget(dev, parent);               //Gets pointer to the in-memory minode

    if (!((pip->INODE.i_mode) & 0xF000) == 0x4000) // if (S_ISDIR())
    {
        printf("Error: parent path is not a directory\n");
        return -1; //Error, so exits function
    }

    if (search(pip, bname)) //Checks if the entry already exists
    {
        printf("Error: entry %s already exists\n", bname);
        return -1; //Error, so exits function
    }

    int ino = ialloc(dev), bno = isDir ? balloc(dev) : 0; //Allocate an inode and a disk block

    MINODE *mip = iget(dev, ino); //Loads INODE into minode
    INODE *ip = &(mip->INODE);    //Initialize mip->INODE as a DIR INODE;
    time_t t = time(0L);          //Found time(0L) in K.C. Wang's textbook

    //Update INODE attributes
    *ip = (INODE){.i_mode = (isDir ? 0x41ED : 0x81A4), // DIR type and permissions
                  .i_uid = running->uid,               //Owner ID
                  .i_gid = running->gid,               //Group ID
                  .i_size = (isDir ? BLKSIZE : 0),     //Size in bytes
                  .i_links_count = (isDir ? 2 : 1),    //Links count 2 for dir because of . and ..
                  .i_atime = t,                        //Time last access
                  .i_ctime = t,                        //Time last change
                  .i_mtime = t,                        //Time last modification
                  .i_blocks = (isDir ? 2 : 0),         //Blocks count in 512 byte chunks
                  .i_block = {bno}};                   //New dir has one data block
    mip->dirty = 1;                                    //Mark minode modified (dirty);

    if (isDir) //Checks if its a directory
    {
        pip->INODE.i_links_count++; //Increment parent INODE’s links_count by 1

        char buf[BLKSIZE] = {0};
        char *cp = buf;

        //Make . entry
        DIR *dp = (DIR *)buf;
        *dp = (DIR){.inode = ino, .rec_len = 12, .name_len = 1, .name = "."};
        cp += dp->rec_len;

        //Make .. entry
        dp = (DIR *)cp;
        *dp = (DIR){.inode = pip->ino, .rec_len = BLKSIZE - 12, .name_len = 2, .name = ".."};
        put_block(dev, bno, buf); //Writes to block on disk
    }

    enter_name(pip, ino, bname); //Reads to last entry in the block, allocates memory for new block, and writes entry inside existing block

    iput(mip); 
    iput(pip); 
    return ino;
}

//My mkdir function - creates a directory
void myMkdir(char *pathname)
{
    mkdir_creat(1, pathname); //Creates a directory (1 = dir)
}

//My create function - creates a file
void myCreat(char *pathname)
{
    mkdir_creat(0, pathname); //Creates a file (0 = file)
}