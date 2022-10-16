
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

//Prints file content - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.3
int ls_file(MINODE *mip, char *pathname)
{
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
  char ftime[64];

  INODE *ip = &mip->INODE;
  u16 mode = ip->i_mode;
  u16 links = ip->i_links_count;
  u16 uid = ip->i_uid;
  u16 gid = ip->i_gid;
  u32 size = ip->i_size;

  if ((mode & 0xF000) == 0x8000) // if (S_ISREG())
    printf("%c", '-');
  if ((mode & 0xF000) == 0x4000) // if (S_ISDIR())
    printf("%c", 'd');
  if ((mode & 0xF000) == 0xA000) // if (S_ISLNK())
    printf("%c", 'l');
  
  for (int i = 8; i >= 0; i--) //Prints the permissions
  {
    if (mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]); // or print -
  }
  
  strcpy(ftime, ctime((const long int *)&mip->INODE.i_mtime)); //Print time in calendar form
  ftime[strlen(ftime) - 1] = 0; //Kill \n at end
 
 printf("    %d %4d %4d %8u %26s %s", links, gid, uid, size, ftime, pathname); //Prints all others

  if ((mode & 0xF000) == 0xA000) //If symbolic file
  {
    printf(" -> %s", (char*)(mip->INODE.i_block)); //Prints the directory the symbolic file is linked to
  }
  printf("\n");

  return 0;
}

//Gets information from a directory and prints each thing inside it - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.3
int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  char *cp;
  DIR *dp;
  MINODE *mip2;

  // Assume DIR has only one data block i_block[0]
  get_block(dev, mip->INODE.i_block[0], buf);

  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE)
  {
    strncpy(temp, dp->name, dp->name_len); //Saves directory name into temp
    temp[dp->name_len] = 0;

    mip2 = iget(dev, dp->inode); //Gets the block content
    ls_file(mip2, temp);
    iput(mip2); //Frees mip

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("\n");

  return 0;
}

//Prints the directories and files in a given pathname
int myLs(char *pathname)
{
  printf("ls %s\n", pathname);
  if (strcmp(pathname, "") == 0) //No given pathname so print current directory
    ls_dir(running->cwd);

  else
  {
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    ls_dir(mip);
    iput(mip);
  }
}

//Recursively gets working directory - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.3
void rmyPwd(MINODE *wd)
{
  char pName[256];

  if(wd == root){
    return;
  } 

  int my_ino = wd->ino;
  int parent_ino = search(wd, "..");

  MINODE *pip = iget(dev, parent_ino);
  findmyname(pip, my_ino, pName);

  rmyPwd(pip);
  printf("/%s", pName);
}

//Prints working directory 
void *myPwd(MINODE *wd)
{
  if (wd == root){
    printf("/");
  }
  else
  {
    rmyPwd(running->cwd);
  }
  printf("\n");
}

//Changes Directory - Adapted from Professor Wang's algorithm in his book, Chapter 11.7.3
void myChdir(char *pathname)
{
  printf("in myChdir\n");
  int ino = getino(pathname);
  MINODE *mip = iget(dev, ino);

  //verify mip is dir
  if((mip->INODE.i_mode / 10000) !=1 )
  {
    printf("Not a directory type\n");
    return;
  }

  iput(running->cwd);
  running->cwd = mip;
}
