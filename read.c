
int myRead(int fd, char *buf, int nbytes)
{
    char kbuf[BLKSIZE];
    int lbk;
    int blk;
    int start;
    int remain;
    int count = 0;
    char *cp;
    int offset = running->fd[fd]->offset;
    int avil = running->fd[fd]->mptr->INODE.i_size - offset;
    printf("offest: %d  avil: %d\n", offset, avil);

    //2
    while(nbytes && avil){
        lbk = offset / BLKSIZE;
        start = offset % BLKSIZE;

        //3convert logical block to physical block -_-
        blk = map(running->fd[fd]->mptr->INODE, lbk);

        //4
        get_block(dev, blk, kbuf);
        //printf("kbuf: %s start: %d\n", kbuf, start);
        cp = kbuf + start;
        remain = BLKSIZE - start;
        //printf("cp: %s\n", cp);
        while(remain){
            //printf("added\n");
            *buf++ = *cp++;
            offset++;
            count++;
            remain--;
            avil--;
            nbytes--;
            if(nbytes == 0 || avil ==0){
                break;
            }
        }
    }
    *buf++ = '\0';
    //printf("final read: %s\n", buf);
    
    return count;
    
}
