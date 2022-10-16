/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[128];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, nINODEs, bmap, imap, INODE_start;
extern char line[256], cmd[32], pathname[256];
int MAXBUF = 10000;

int myOpen(char *filename, int flags)
{
    //----1. get file's minnode----
    int ino = getino(filename);
    if(ino==0){
        printf("file does not exist\n");
        return -1;
    }
    MINODE* mip = malloc(sizeof(MINODE));
    mip = iget(dev, ino);

    //----2. allocate openTable entry OFT----
    OFT *newOFT = malloc(sizeof(OFT));

    newOFT->mode = flags;
    newOFT->mptr = mip;
    newOFT->refCount = 1;
    newOFT->offset = 0;

    //----3. search for first FRER
        //find first empty spot?
    int i = 0;
    while(running->fd[i] != NULL){
        printf("i: %d full\n", i);
        i++;
    }

    running->fd[i] = newOFT;
    
    printf("size of file: %d\n",running->fd[i]->mptr->INODE.i_size);
    //do we need to set global fd???


    //----4. return index as fd----
    return i;


}
int map(INODE n, int lbk)
{
    int blk;
    char ibuf[256];
    if(lbk<12){
        blk = n.i_block[lbk];
    }
    else if( 12<=lbk<12+256){
        read(n.i_block[12], ibuf, 256);
        blk = ibuf[lbk-12];
    }
    else{
        printf("TODO: double indirect block\n");
        
    }
    return blk;
}


int myLseek(int fd, int position, int wehence)
{
    int s = running->fd[fd]->mptr->INODE.i_size;
    if(position>=s)
    {
        return -1;
    }
    return fd;
}


int myClose(int fd)
{
    //check if fd is valid
    if(running->fd[fd] != 0)
    {
        running->fd[fd]->refCount--;
        if(running->fd[fd]->refCount == 0)
            iput(running->fd[fd]->mptr);
    }
    running->fd[fd] = 0;
}


