


int myWrite(int fd, char* buf, int nbytes)
{
    char kbuf[BLKSIZE];
    int lbk;
    int blk;
    int start;
    int remain;
    int count = 0;
    char *cp;
    int offset = running->fd[fd]->offset;
    

    while(nbytes)
    {
        lbk = offset / BLKSIZE;
        start = offset % BLKSIZE;
        blk = map(running->fd[fd]->mptr->INODE, lbk);
        get_block(dev, blk, kbuf); //?
        cp = kbuf + start;
        remain = BLKSIZE - start;
        while(remain)
        {
            *cp++ = *buf++;
            offset++;
            count++;
            remain--;
            nbytes--;
            if(offset > running->fd[fd]->mptr->INODE.i_size)
                running->fd[fd]->mptr->INODE.i_size++;
            if(nbytes <=0)
                break;
        }
        put_block(dev, blk, kbuf);
    }
    running->fd[fd]->mptr->dirty = 1;
    return count;
}