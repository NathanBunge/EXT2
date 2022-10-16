
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], dname[256], bname[256];

/*********** util.c file ****************/

//Reads a block - Adapted from Professor Wang's algorithm in his book, Chapter 11
int get_block(int dev, int blk, char *buf)
{
  lseek(dev, (long)blk * BLKSIZE, 0);
  int n = read(dev, buf, BLKSIZE);
  if (n < 0)
    printf("get_block [%d %d] error\n", dev, blk);
}

//Writes to a block - Adapted from Professor Wang's algorithm in his book, Chapter 11
int put_block(int dev, int blk, char *buf)
{
  lseek(dev, (long)blk * BLKSIZE, 0);
  int n = write(dev, buf, BLKSIZE);
  if (n != BLKSIZE)
    printf("put_block [%d %d] error\n", dev, blk);
}

//Copy pathname into gpath[]; tokenize it into name[0] to name[n-1] - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.2
int tokenize(char *pathname)
{
  char *s;
  strcpy(gpath, pathname);
  n = 0;
  s = strtok(gpath, "/");
  while (s)
  {
    name[n++] = s;
    s = strtok(0, "/");
  }
}

//Splits a pathname into directory name and base name
void dbname(char *pathname)
{
  char temp[256];
  strcpy(temp, pathname);       // make temporary cpy of pathname
  strcpy(dname, dirname(temp)); // store directory name in dname
  strcpy(temp, pathname);
  strcpy(bname, basename(temp)); // store basename in bname var
}

//Return minode pointer of loaded INODE=(dev, ino) - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.2
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino)
    {
      mip->refCount++;
      return mip;
    }
  }

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    if (mip->refCount == 0)
    {
      mip->refCount = 1;
      mip->dev = dev;
      mip->ino = ino;

      //Get INODE of ino to buf
      blk = (ino - 1) / 8 + inode_start;
      disp = (ino - 1) % 8;

      get_block(dev, blk, buf);
      ip = (INODE *)buf + disp;

      //Copy INODE to mp->INODE
      mip->INODE = *ip;
      return mip;
    }
  }
  printf("There are no more free minodes\n");
  return 0;
}

//Dispose of minode pointed by mip - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.2
void iput(MINODE *mip)
{
  int i, block, offset;
  char buf[BLKSIZE];
  INODE *ip;

  if (mip == 0)
    return;

  mip->refCount--;       //dec refCount by 1
  if (mip->refCount > 0) //Still has user
    return;
  if (!mip->dirty) //Do not need to write back
    return;

  //printf("iput: dev = %d   ino = %d\n", mip->dev, mip->ino);
  //Write inode back to the disk
  block = ((mip->ino - 1) / 8) + inode_start;
  offset = (mip->ino - 1) % 8;

  //Get the block containing this inode
  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset;      //ip points to INODE
  *ip = mip->INODE;                //copy INODE to inode in block
  put_block(mip->dev, block, buf); //writes back to disk
  malloc(mip);                     //mip->refCount = 0;
}

//Search for name in (DIRECT) data blocks of mip->INODE and returns its ino - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.2
int search(MINODE *mip, char *name)
{
  int i;
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  for (i = 0; i < 12; i++) // search DIR direct blocks only
  {
    if (mip->INODE.i_block[i] == 0)
      return 0;

    get_block(mip->dev, mip->INODE.i_block[i], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;

    while (cp < sbuf + BLKSIZE)
    {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

      if (strcmp(name, temp) == 0)
      {
        printf("found %s : inumber = %d\n", name, dp->inode);
        return dp->inode;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;
}

//Return ino of pathname - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.2
int getino(char *pathname)
{
  MINODE *mip;
  int i, ino;
  if (strcmp(pathname, "/") == 0)
  {
    return 2; // return root ino=2
  }
  if (pathname[0] == '/')
    mip = root; // if absolute pathname: start from root
  else
    mip = running->cwd; // if relative pathname: start from CWD
  mip->refCount++;      // in order to iput(mip) later
  tokenize(pathname);   // assume: name[ ], nname are globals
  for (i = 0; i < n; i++)
  { // search for each component string
    if (!S_ISDIR(mip->INODE.i_mode))
    { // check DIR type
      printf("%s is not a directory\n", name[i]);
      iput(mip);
      return 0;
    }
    ino = search(mip, name[i]);
    if (!ino)
    {
      printf("no such component name %s\n", name[i]);
      iput(mip);
      return 0;
    }
    iput(mip);            // release current minode
    mip = iget(dev, ino); // switch to new minode
  }
  iput(mip);
  return ino;
}

//search parent's data block for myino;
//Copy its name STRING to myname[ ]; - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.2
int findmyname(MINODE *parent, u32 myino, char *myname)
{
  int i;
  char *cp, sbuf[BLKSIZE];
  DIR *dp;
  for (i = 0; i < 12; i++) // search DIR direct blocks only
  {
    if (parent->INODE.i_block[i] == 0)
      return 0;

    get_block(parent->dev, parent->INODE.i_block[i], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;

    while (cp < sbuf + BLKSIZE)
    {
      if (dp->inode == myino)
      {
        strcpy(myname, dp->name);
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;
}

int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  // mip->a DIR minode. Write YOUR code to get mino=ino of .
  //                                         return ino of ..
}

//Checks if bit is a set - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
int tst_bit(char *buf, int bit)
{
  return buf[bit / 8] & (1 << (bit % 8));
}

//Sets the set bit - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
void set_bit(char *buf, int bit)
{
  buf[bit / 8] |= (1 << (bit % 8));
}

//Clears the bit - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.4
void clr_bit(char *buf, int bit)
{
  buf[bit / 8] &= ~(1 << (bit % 8));
}

//Decrements the free INODE count in SUPER and GD - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  get_block(dev, 1, buf);
  ((SUPER *)buf)->s_free_inodes_count--; //Decrement free inodes count in SUPER
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  ((GD *)buf)->bg_free_inodes_count--; //Decrement free inodes count in GD
  put_block(dev, 2, buf);
}

//Increments the free INODE count in SUPER and GD - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.4
int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  get_block(dev, 1, buf);
  ((SUPER *)buf)->s_free_inodes_count++; //Increment free inodes count in SUPER
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  ((GD *)buf)->bg_free_inodes_count++; //Increment free inodes count in GD
  put_block(dev, 2, buf);
}

// Allocate and Inode - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
int ialloc(int dev)
{
  char buf[BLKSIZE];
  get_block(dev, imap, buf); //Reads block from device and stores in buf
  for (int i = 0; i < ninodes; i++)
    if (tst_bit(buf, i) == 0) //Checks if bit is set
    {
      set_bit(buf, i);
      decFreeInodes(dev);
      put_block(dev, imap, buf); //Write to disk block
      return i + 1;              //Returns the new INODE count
    }
  return 0;
}

//Allocate a disk block - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.1
int balloc(int dev)
{
  char buf[BLKSIZE];

  get_block(dev, bmap, buf); //Read Group Descriptor 0 to get bmap, imap and iblock numbers
  for (int i = 0; i < nblocks; i++)
  {
    if (tst_bit(buf, i) == 0) //Checks if bit is set
    {
      set_bit(buf, i);
      decFreeInodes(dev);
      put_block(dev, bmap, buf); //Creates / Writes to a new block
      return i + 1;              //Returns FREE disk block number
    }
  }
  return 0;
}

//Deallocate an Inode - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.4
void idalloc(int dev, int ino)
{
  char buf[BLKSIZE];
  get_block(dev, imap, buf);
  if (ino > ninodes) //Checks to see if ino is out of range
  {
    printf("ERROR: inumber %d out of range.\n", ino);
    return;
  }
  get_block(dev, imap, buf); //Gets imap block from device and stores in buffer
  clr_bit(buf, ino - 1);
  put_block(dev, imap, buf); //Creates / Writes to a new block in device
  incFreeInodes(dev);
}

//Deallocate a disk block - Adapted from Professor Wang's algorithm in his book, Chapter 11.8.4
void bdalloc(int dev, int bno)
{
  char buf[BLKSIZE];
  if (bno > nblocks) //Checks to see if bno is out of range
  {
    printf("ERROR: bnumber %d out of range.\n", bno);
    return;
  }
  get_block(dev, bmap, buf); //Read Group Descriptor to get bmap, imap and iblock numbers
  clr_bit(buf, bno - 1);
  put_block(dev, bmap, buf); //Creates / Writes to a new block in device
  incFreeInodes(dev);
}
