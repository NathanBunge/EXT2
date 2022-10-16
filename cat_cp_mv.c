
int myCat(char *pathname)
{
    printf("in cat with: %s", pathname);
    fd = myOpen(pathname, 0|O_DIRECTORY);

    fd = myLseek(fd, 0, SEEK_SET);
    if(fd<0)
    {
        printf("lseek failed\n");
        return -1;
    }
    char buf[MAXBUF];
    myRead(fd, buf, MAXBUF);

    printf("myCat result:\n\n%s", buf);

}
int myCp(char* source, char* destination)
{
    char buf[MAXBUF];
    printf("source: %s\n Destination: %s\n", source, destination);
    fd = myOpen(source, 0);
    myRead(fd, buf, MAXBUF);

    myClose(fd);
    fd = myOpen(destination, 1);
    printf("write fd: %d\n", fd);
    int count = myWrite(fd, buf, MAXBUF);

    printf("count written: %d\n", count);

    myClose(fd);
}

int myMv(char *source, char* destination)
{
    myCp(source, destination);

    char buf[MAXBUF];
    buf[0] = '\0';
    fd = myOpen(source, 1);
    myWrite(fd, buf, MAXBUF);
    myClose(fd);

}