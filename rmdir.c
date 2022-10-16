

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

//Removes a child - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.4
int rmChild(MINODE *pip, char *pathname)
{
    char *cp, buf[BLKSIZE];
    DIR *dp, *prev;
    int current;
    INODE ip = pip->INODE;

    for (int i = 0; i < 12 && ip.i_block[i]; i++) //Traverses through data block of parent DIR, assume there are only 12 direct blocks
    {
        current = 0;

        get_block(dev, ip.i_block[i], buf); //Reads block
        cp = buf;
        dp = (DIR *)buf;
        prev = 0;

        while (cp < buf + BLKSIZE)
        {
            if (strncmp(dp->name, pathname, dp->name_len) == 0)
            {
                int idealLen = ideal_length(dp->name_len);
                if (idealLen != dp->rec_len) //Check if it is the last entry
                    prev->rec_len += dp->rec_len; //Expand previous entry to take up deleted space
            
                else if (prev == NULL && cp + dp->rec_len == buf + 1024) //Check if its the first and only one entry
                {
                    //Deallocate the data block and changes parent's file size accordingly 
                    char empty[BLKSIZE] = {0};
                    put_block(dev, ip.i_block[i], empty); //Writes empty to the block
                    bdalloc(dev, ip.i_block[i]); //Deallocate the entire block Case where it is not (first and only) or last
                    ip.i_size -= BLKSIZE;        //Decrement the block by the entire size of a block
                    for (int j = i; j < 11; j++) //Shifts the blocks left
                    {
                        ip.i_block[j] = ip.i_block[j + 1];
                    }
                    ip.i_block[11] = 0; //Sets the last block to nothing
                }

                else //Not the first or the last block
                {
                    //Shifts all blocks left
                    int removed = dp->rec_len;
                    char *temp = buf;
                    DIR *last = (DIR *)temp;

                    while (temp + last->rec_len < buf + BLKSIZE) //Traverse to the block that is deleted
                    {
                        temp += last->rec_len;
                        last = (DIR *)temp;
                    }
                    last->rec_len = last->rec_len + removed;

                    //Move everything after the removed entry to the left by the length of the entry
                    memcpy(cp, cp + removed, BLKSIZE - current - removed + 1);
                }
                put_block(dev, ip.i_block[i], buf);
                pip->dirty = 1; //Mark as modified (dirty = 1)
                return 1;
            }
            cp += dp->rec_len; //Move to next
            current += dp->rec_len;
            prev = dp;
            dp = (DIR *)cp;
        }
        return 0;
    }

    printf("Error: %s does not exist.\n", bname);
    return 1;
}

//Removes a directory - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.4
void myRmdir(char* pathname)
{
    dbname(pathname); //Splits pathname into directory name and basename

    char *temp = strdup(pathname);
    int ino = getino(temp); //Get inode number from pathname
    free(temp);

    if (!ino) //Checks if path does not exists
    {
        printf("Error: %s does not exist.\n", pathname);
        return;
    }

    MINODE *mip = iget(dev, ino); //Get inode  
    INODE ip = mip->INODE;
                                
    if ((running->uid != ip.i_uid) && (running->uid != 0)) //Check if busy
    {
        printf("Error: you cannot remove %s.\n", pathname);
        iput(mip); //Release mip
        return;
    }

    if (!((ip.i_mode) & 0xF000) == 0x4000) // if (S_ISDIR())
    {
        printf("Error: %s is not a directory", pathname);
        iput(mip); //Release mip
        return;
    }

    if (ip.i_links_count > 2) //Checks if directory is not empty
    {
        printf("Error: directory %s is not empty.\n", pathname);
        iput(mip); //Release mip
        return;
    }

    if (ip.i_links_count == 2) //Checks if the directory still has regular files
    {
        char buf[BLKSIZE], *cp;
        DIR *dp;
        get_block(dev, ip.i_block[0], buf); //Reads block
        cp = buf;
        dp = (DIR *)buf;
        cp += dp->rec_len;
        dp = (DIR *)cp; // Goes to the parent directory ".."
        if (dp->rec_len != 1012) //Check if directory is empty
        {
            printf("Error: directory %s is not empty.\n", pathname);
            iput(mip); //Release mip
            return;
        }
    }

    for (int i = 0; i < 12; i++) //Traverses through data block of parent DIR, assume there are only 12 direct blocks
    {
        if (ip.i_block[i] == 0) //If block is already empty, skip
            continue;
        bdalloc(mip->dev, ip.i_block[i]); //Deallocate disk block
    }
    idalloc(mip->dev, mip->ino); //Deallocates an inode (number): clears ino's bit to 0 and adds free inode count in SUPER and GD
    iput(mip);                   //Release mip

    MINODE *pip = iget(dev, getino(dname));
    rmChild(pip, bname); //Removes child directory

    pip->INODE.i_links_count--; //Decrement inode's link_count
    pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
    pip->dirty = 1; //Mark as modified (dirty = 1)
    iput(pip);      //Release pip
}