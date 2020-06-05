#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <string.h>


typedef struct menu  //struct with all flags
{
    char *from;
    char *to;
    char useMem;
    char printHelp;
}menu, *pmenu;

void initialMenu(pmenu b);
void getArguments(int argc, char **argv, pmenu b);
void printHelpMessage(void);
void copy_mmap(int fd_from, int fd_to);
void copy_read_write(int fd_from, int fd_to);

int main(int argc, char **argv)
{
    menu option;
    int fd_from, fd_to;
    initialMenu(&option);
    getArguments(argc, argv, &option);
    
    if(option.printHelp)
    {
        printHelpMessage();
        return 1;
    }
    
    if(strcmp(option.from, option.to) == 0)
    {
        errno = EINVAL; //invalid argument
        err(1, "Source file and destination file cannot be the same");
    }
    
    if((fd_from = open(option.from, O_RDONLY)) == -1) // only read
    {
        err(1, "Open source");
    }
    if((fd_to = open(option.to, O_RDWR|O_CREAT, 0666)) == -1) //Open the file for reading and writing if open file not exist create it at the moment.
    {
        err(1, "Open destination");
    }
    
    if(option.useMem)
    {
        copy_mmap(fd_from, fd_to);
        printf("Copy by using mmap() and memcpy() function.\n");
    }
    else
    {
        copy_read_write(fd_from, fd_to);
        printf("Copy by using read() and write() funtion.\n");
    }
    
    return 0;
}

void initialMenu(pmenu b)
{
    b->from = NULL;
    b->to = NULL;
    b->useMem = 0;
    b->printHelp = 0;
}

void getArguments(int argc, char **argv, pmenu b)
{
    int temp;
    while((temp = getopt(argc, argv, ":mh")) != -1)
        //"hm:" which is optstring:the string consist of option character which is declared
    {
        switch (temp)
        {
            case 'h':
                b->printHelp = 1;
                break;
            case 'm':
                b->useMem = 1;
                break;
            default:
                b->printHelp = 1;
                return;
        }
    }
    if(optind + 1 >= argc) //index of next option is >= argc then show help message
    {
        b->printHelp = 1;
        return;
    }
    
    b->from = argv[optind]; //from take the 1st parameter we will analysis  optind initail =1
    b->to = argv[optind+1]; //to take the argv[optind+1] so then one next parameter we will analysis
                            //optind scan if not option argument will skip,and optind+1
}

void copy_mmap(int fd_from, int fd_to)
{
    char *mapfrom, *mapto;
    struct stat fdata;
    //fstat () is used to copy the file state of the parameter stat to the struct fdata in the structure
    if (fstat(fd_from, &fdata) == -1)
    {
        err(1, "fstatr source file information failed");
    }
    //The lseek system call is used to move control the location of the read and write pointers
    if (lseek (fd_to, fdata.st_size - 1, SEEK_SET) == -1)
    {
        err (1, "lseek on destination file");
    }
    //The parameter fd_to is an open file descriptor, and the fdata.st_size - 1 is the number of displacements that move the read and write position according to the parameter SEEK_SET:Calculate the offset from the beginning of the file
    if (write (fd_to, "", 1) != 1)
    {
        err (1, "writing to destination");
    }
    
    //mmap() Used to map a file content to the memory, access to the memory area that is directly on the contents of the file read and write.
    /*void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);
    start: start address
    length:the size of file part which will map to the memory
    prot: Mapping areaprotection type, exec, read write, none
    flags: PROT_READ： The mapping area can be read; PROT_WRITE：The mapping area can be written;
    MAP_SHARED:The write data for the mapped area is copied back into the file, and other processes that map the file are allowed to be shared.
    fd_from:The file descriptor returned by open (from) represents the file to be mapped to memory.
    fd_to:    The file descriptor returned by open (to) represents the file to be mapped to memory.
    offsize: The offset of the file map is usually set to 0, stands for align from the beginning of the file.*/
    mapfrom = mmap(NULL, fdata.st_size, PROT_READ , MAP_SHARED, fd_from, 0);
    mapto   = mmap(NULL, fdata.st_size, PROT_WRITE, MAP_SHARED, fd_to, 0);
    //MAP_FAILED represents the error return
    if (mapfrom == MAP_FAILED)
    {
        err(1, "Error in mapping source file");
        
    }
    if (mapto   == MAP_FAILED)
    {
        err(1, "Error in mapping destination file");
    }
    //From the mapfrom to the address of the starting address of the continuous fdata.st_size(n) bytes of data copied to mapto point to the address as the starting address of the space
    //void *memcpy(void*dest, const void *src, size_t n);
    //Copy the contents of the memory by mapping address
        memcpy(mapto, mapfrom, fdata.st_size);
}

void copy_read_write(int fd_from, int fd_to)
{
    ssize_t ret;
    char buf[1024*1024];
    while((ret = read(fd_from, buf, sizeof(buf))) != 0)
    {
        if(ret == -1)
        {
            err(1, "Reading source file");
        }
        if((write(fd_to, buf, ret)) == -1)
        {
            err(1, "Writting to destination file");
        }
        
        if((close(fd_from)) == -1)
        {
            err(1, "Close the source file");
        }
        if((close(fd_to)) == -1)
        {
            err(1, "Close the destination file");
        }
    }
}

void printHelpMessage(void)
{
        printf ("Usage:\n\tcopy [-m] <file_name> <new_file_name>\n\
    \tcopy [-h]\n\n\
    \
    Without option -m read() and write() functions will be used to copy file contents. If \
    the option -m is given, do not use neither read() nor write() but map files \
    to memory regions with mmap() and copy the file with memcpy() instead.\n");
}

