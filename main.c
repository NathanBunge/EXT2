/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/

#include "type.h"
#include "util.c"
#include "cd_ls_pwd.c"
#include "link.c"
#include "stats_utime_chmod.c"
#include "open_close_lseek.c"
#include "read.c"
#include "write.c"
#include "cat_cp_mv.c"

// globals
MINODE minode[NMINODE];
MINODE *root;

PROC proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 32 components in pathname
int n;           // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;

char line[256], cmd[32], pathname[256], dname[256], bname[256];

MINODE *iget();

int init()
{
  int i, j;
  MINODE *mip;
  PROC *p;

  printf("init()\n");

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j = 0; j < NFD; j++)
      p->fd[j] = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  printf("mount_root()\n");
  root = iget(dev, 2);
}


int quit()
{
  int i;
  MINODE *mip;
  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}


char *disk = "diskimage";
int main(int argc, char *argv[])
{
  int ino;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname[128], pathname2[128];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0)
  {
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = fd;

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53)
  {
    printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
    exit(1);
  }
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  while (1)
  {
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readlink|stats|chmod|utime|open|read|write|cat|cp|mv|close|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;

    if (line[0] == 0)
      continue;
    pathname[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, pathname2);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if (strcmp(cmd, "ls") == 0)
      myLs(pathname);
    if (strcmp(cmd, "cd") == 0)
      myChdir(pathname);
    if (strcmp(cmd, "pwd") == 0)
      myPwd(running->cwd);
    if (strcmp(cmd, "mkdir") == 0)
      myMkdir(pathname);
    if (strcmp(cmd, "creat") == 0)
      myCreat(pathname);
    if (strcmp(cmd, "rmdir") == 0)
      myRmdir(pathname);
    if (strcmp(cmd, "link") == 0)
      myLink(pathname, pathname2);
    if (strcmp(cmd, "unlink") == 0)
      myUnlink(pathname);
    if (strcmp(cmd, "symlink") == 0)
      mySymlink(pathname, pathname2);
    if (strcmp(cmd, "readlink") == 0)
      myReadLink(pathname);
    if (strcmp(cmd, "stats") == 0)
      myStat(pathname);
    if (strcmp(cmd, "chmod") == 0)
      myChmod(pathname);
    if (strcmp(cmd, "utime") == 0)
      myUtime(pathname);
    if(strcmp(cmd, "open") == 0)
    {
      fd = myOpen(pathname, atoi(pathname2));

      //test open for debugging
      //int test = open("test", O_RDONLY, 0664);
      //printf("file des: %d\n", test);
      //printf("fd: %d\n", fd);
      //printf("count: %d", myRead(fd, buf, 10));
      //printf("final buf: %s\n", buf);
    }
    if(strcmp(cmd, "read")==0)
      myRead(fd, buf, 1024);
    if(strcmp(cmd, "write")==0)
      myWrite(fd, buf, 1024);
    if(strcmp(cmd, "cat")==0)
      myCat(pathname);
    if(strcmp(cmd, "cp")==0)
      myCp(pathname, pathname2);
    if(strcmp(cmd, "mv")==0)
      myMv(pathname, pathname2);
    if(strcmp(cmd, "close")==0)
      myClose(fd);
    if (strcmp(cmd, "quit") == 0)
      quit();
  }
}
