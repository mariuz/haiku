/*
  This file contains a simple file system "shell" that lets you
  manipulate a file system.  There is a simple command table that
  contains the available functions.  It is very easy to extend.
  
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/

#include "compat.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>

#include "additional_commands.h"
#include "argv.h"
#include "external_commands.h"
#include "kprotos.h"
#include "tracker.h"

#include <fs_attr.h>
#include <fs_query.h>

static bool sInteractiveMode = true;


static int do_lat_fs(int argc, char **argv);
static void do_fsh(void);

static void
print_usage(const char *program)
{
   	printf("----------------------------------------------------------------------\n");
   	printf("Ultra neat-o userland filesystem testing shell thingy\n");
   	printf("----------------------------------------------------------------------\n");
   	printf("usage: %s [-n] [%%s:DISK_IMAGE=big_file|%%d:RANDOM_SEED]\n",
   		program);
   	printf("\n");
}


int
main(int argc, char **argv)
{
    int        seed;
    char      *disk_name = "big_file";
    myfs_info  *myfs;
    char *arg;
    int argi = 1;
    
    if (argv[1] && strcmp(argv[1], "--help") == 0) {
    	print_usage(argv[0]);
    	exit(0);
    }

	// eat options
	while (argi < argc && argv[argi][0] == '-') {
		arg = argv[argi++];
		if (strcmp(arg, "-n") == 0) {
			sInteractiveMode = false;
		} else {
	    	print_usage(argv[0]);
			exit(1);
		}
	}

	if (argi >= argc) {
    	print_usage(argv[0]);
		exit(1);
	}
	arg = argv[argi];

    if (arg != NULL && !isdigit(arg[0]))
        disk_name = arg;
    else if (arg && isdigit(arg[0]))
        seed = strtoul(arg, NULL, 0);
    else
        seed = getpid() * time(NULL) | 1;
    printf("random seed == 0x%x\n", seed);

    srand(seed);

    myfs = init_fs(disk_name);

    do_fsh();

    if (sys_unmount(1, -1, "/myfs") != 0) {
        printf("could not un-mount /myfs\n");
        return 5;
    }

    shutdown_block_cache();

    return 0;   
}


static void
make_random_name(char *buf, int len)
{
    int i, max = (rand() % (len - 5 - 3)) + 2;

    strcpy(buf, "/myfs/");
    for(i=0; i < max; i++) {
        buf[i+6] = 'a' + (rand() % 26);
    }

    buf[i] = '\0';
}


static void
SubTime(struct timeval *a, struct timeval *b, struct timeval *c)
{
  if ((long)(a->tv_usec - b->tv_usec) < 0)
   {
     a->tv_sec--;
     a->tv_usec += 1000000;
   }

  c->tv_sec  = a->tv_sec  - b->tv_sec;
  c->tv_usec = a->tv_usec - b->tv_usec;
}



int cur_fd = -1;


static int
do_close(int argc, char **argv)
{
    int err;

    err = sys_close(1, cur_fd);
/*  printf("close of fd %d returned: %d\n", cur_fd, err); */
    cur_fd = -1;

	return err;
}

static int
do_open(int argc, char **argv)
{
    char name[64];

    if (cur_fd >= 0)
        do_close(0, NULL);

    if (argc < 2)
        make_random_name(name, sizeof(name));
    else
        sprintf(name, "/myfs/%s", &argv[1][0]);

    cur_fd = sys_open(1, -1, name, O_RDWR, MY_S_IFREG, 0);
    if (cur_fd < 0) {
        printf("error opening %s : %s (%d)\n", name, fs_strerror(cur_fd), cur_fd);
    	return cur_fd;
    } else {
        printf("opened: %s\n", name);
		return 0;
    }
}

static int
do_make(int argc, char **argv)
{
    char name[64];

    if (cur_fd >= 0)
        do_close(0, NULL);

    if (argc < 2) 
        make_random_name(name, sizeof(name));
    else if (argv[1][0] != '/')
        sprintf(name, "/myfs/%s", &argv[1][0]);
    else
        strcpy(name, &argv[1][0]);

    cur_fd = sys_open(1, -1, name, O_RDWR|O_CREAT, 0666, 0);
    if (cur_fd < 0) {
        printf("error creating: %s: %s\n", name, fs_strerror(cur_fd));
        return cur_fd;
    }

/*  printf("created: %s (fd %d)\n", name, cur_fd); */

	return 0;
}



static int
do_mkdir(int argc, char **argv)
{
    int err;
    char name[64];

    if (argc < 2) 
        make_random_name(name, sizeof(name));
    else
        sprintf(name, "/myfs/%s", &argv[1][0]);

    err = sys_mkdir(1, -1, name, MY_S_IRWXU);
    if (err)
        printf("mkdir of %s returned: %s (%d)\n", name, fs_strerror(err), err);

	return err;
}


static int
do_read_test(int argc, char **argv)
{
    int    i, err;
    char   *buff;
    size_t len = 256;
    
    
    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

    if (argv[1] && isdigit(argv[1][0]))
        len = strtoul(&argv[1][0], NULL, 0);

    buff = (char*)malloc(len);
    if (buff == NULL) {
        printf("no memory for write buffer of %lu bytes\n", (unsigned long)len);
        return FS_ENOMEM;
    }

    for (i = 0; (size_t)i < len; i++)
        buff[i] = (char)0xff;

    err = sys_read(1, cur_fd, buff, len);

	if (len <= 1024)
		hexdump(buff, len);
	else {
		printf("Hex dump shows only the first 1024 bytes:\n");
		hexdump(buff, 1024);
	}

	free(buff);
	printf("read read %lu bytes and returned %d\n", (unsigned long)len, err);

	return (err < 0 ? err : 0);
}


static int
do_write_test(int argc, char **argv)
{
    int    i, err;
    char   *buff;
    size_t len = 256;
    
    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

    if (argv[1] && isdigit(argv[1][0]))
        len = strtoul(&argv[1][0], NULL, 0);

    buff = (char*)malloc(len);
    if (buff == NULL) {
        printf("no memory for write buffer of %lu bytes\n", (unsigned long)len);
        return FS_ENOMEM;
    }

    for (i = 0; (size_t)i < len; i++)
        buff[i] = i;

    err = sys_write(1, cur_fd, buff, len);
    free(buff);
    
    printf("write wrote %lu bytes and returned %d\n", (unsigned long)len, err);

	return (err < 0 ? err : 0);
}


static int
do_write_stream(int argc, char **argv)
{
    size_t amount = 100000;
	char buffer[4096];
	int length = sizeof(buffer);
    int i, err;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

    if (argv[1] && isdigit(argv[1][0]))
        amount = strtoul(&argv[1][0], NULL, 0);

    for(i = 0; (size_t)i < sizeof(buffer); i++)
        buffer[i] = i;

	for (i = 0; (size_t)i < amount; i++) {
		err = sys_write(1, cur_fd, buffer, length);
		if (err < FS_OK)
			break;
	}

    printf("write wrote %d bytes and returned %d\n", length, err);

	return (err < 0 ? err : 0);
}


static int
do_read_attr(int argc, char **argv)
{
    char *attribute,*buffer;
    size_t len = 256;
    int i, err;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

	if (argc < 2) {
        printf("usage: rdattr <attribute name> [bytes to write]\n");
		return FS_EINVAL;
	}

	attribute = argv[1];
    if (argv[2] && isdigit(*argv[2]))
        len = strtoul(argv[2], NULL, 0);
	if (len < 0) {
		printf("invalid length for write attribute!\n");
		return FS_EINVAL;
	}

    buffer = (char*)malloc(len);
    if (buffer == NULL) {
        printf("no memory for write buffer of %lu bytes\n", (unsigned long)len);
        return FS_ENOMEM;
    }

	for (i = 0; (size_t)i < len; i++)
		buffer[i] = (char)0xff;

    err = sys_read_attr(1, cur_fd, attribute, 'CSTR', buffer, len, 0);

    if (err >= 0)
        hexdump(buffer, err < 512 ? err : 512);

    free(buffer);
    printf("read read %lu bytes and returned %d (%s)\n", (unsigned long)len, err, fs_strerror(err));

	return (err < 0 ? err : 0);
}


static int
do_write_attr(int argc, char **argv)
{
    char *attribute,*buffer;
    size_t len = 256;
    int i, err;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

	if (argc < 2) {
        printf("usage: wrattr <attribute name> [bytes to write]\n");
		return FS_EINVAL;
	}

	attribute = argv[1];
    if (argv[2] && isdigit(*argv[2]))
        len = strtoul(argv[2], NULL, 0);
	if (len < 0) {
		printf("invalid length for write attribute!\n");
		return FS_EINVAL;
	}

    buffer = (char*)malloc(len);
    if (buffer == NULL) {
        printf("no memory for write buffer of %lu bytes\n", (unsigned long)len);
        return FS_ENOMEM;
    }

    for (i = 0; (size_t)i < len; i++)
        buffer[i] = i;

    err = sys_write_attr(1, cur_fd, attribute, 'CSTR', buffer, len, 0);
    free(buffer);

    printf("write wrote %lu bytes and returned %d\n", (unsigned long)len, err);

	return (err < 0 ? err : 0);
}


static int
do_remove_attr(int argc, char **argv)
{
    int err;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

	if (argc < 2) {
        printf("usage: rmattr <attribute name>\n");
		return FS_EINVAL;
	}

    err = sys_remove_attr(1, cur_fd, argv[1]);

    printf("remove_attr returned %d\n", err);

	return err;
}


static void
mode_bits_to_str(int mode, char *str)
{
    int i;
    
    strcpy(str, "----------");

    if (MY_S_ISDIR(mode))
        str[0] = 'd';
    else if (MY_S_ISLNK(mode))
        str[0] = 'l';
    else if (MY_S_ISBLK(mode))
        str[0] = 'b';
    else if (MY_S_ISCHR(mode))
        str[0] = 'c';

    for(i=7; i > 0; i-=3, mode >>= 3) {
        if (mode & MY_S_IROTH)
            str[i]   = 'r';
        if (mode & MY_S_IWOTH)
            str[i+1] = 'w';
        if (mode & MY_S_IXOTH)
            str[i+2] = 'x';
    }
}


static int
do_dir(int argc, char **argv)
{
	int dirfd, err, max_err = 10;
	char dirname[128], buff[512], time_buf[64] = { '\0', };
	size_t len, count = 0;
	struct my_dirent *dent;
	struct my_stat st;
	struct tm *tm;
	char mode_str[16];

	dent = (struct my_dirent *)buff;
	strcpy(dirname, "/myfs/");
	if (argc > 1)
		strcat(dirname, &argv[1][0]);

	if ((dirfd = sys_opendir(1, -1, dirname, 0)) < 0) {
		printf("dir: error opening: %s\n", dirname);
		return dirfd;
	}

	printf("Directory listing for: %s\n", dirname);
	printf("      inode#  mode bits     uid    gid        size    "
		"Date      Name\n");

	while (1) {
		len = 1;
		err = sys_readdir(1, dirfd, dent, sizeof(buff), len);
		if (err < 0) {
			printf("readdir failed for: %s\n", dent->d_name);
			if (max_err-- <= 0)
				break;
			
			continue;
		}

		if (err == 0)
			break;

		err = sys_rstat(1, dirfd, dent->d_name, &st, 1);
		if (err != 0) {
			// may happen for unresolvable links
			printf("stat failed for: %s (%Ld)\n", dent->d_name, dent->d_ino);
			continue;
		}

		tm = localtime(&st.mtime);
		strftime(time_buf, sizeof(time_buf), "%b %d %I:%M", tm);

		mode_bits_to_str(st.mode, mode_str);

		printf("%12Ld %s %6d %6d %12Ld %s %s\n", st.ino, mode_str,
			st.uid, st.gid, st.size, time_buf, dent->d_name);
		count++;
	}

	if (err != 0)
		printf("readdir failed on: %s\n", dent->d_name);

	printf("%ld files in directory!\n", (long)count);

printf("sys_closedir()...\n");
	sys_closedir(1, dirfd);
printf("sys_closedir() done\n");

	return 0;	// not really correct, but anyway...
}


static int
do_ioctl(int argc, char **argv)
{
	int fd;
	int err;

	if (argc < 2) {
		printf("ioctl: too few arguments\n");
		return FS_EINVAL;
	}

	if ((fd = sys_open(1, -1, "/myfs/.", O_RDONLY, S_IFREG, 0)) < 0) {
	    printf("ioctl: error opening '.'\n");
	    return fd;
	}

	err = sys_ioctl(1, fd, atoi(argv[1]), 0, 0);
	if (err < 0) {
	    printf("ioctl: error!\n");
	}

	sys_close(1, fd);

	return err;
}


static int
do_fcntl(int argc, char **argv)
{
	int err;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

	if (argc < 2) {
		printf("fcntl: too few arguments\n");
		return FS_EINVAL;
	}

	err = sys_ioctl(1, cur_fd, atoi(argv[1]), 0, 0);
	if (err < 0) {
	    printf("fcntl: error!\n");
	}

	return err;
}


static int
do_rmall(int argc, char **argv)
{
    int               dirfd, err, max_err = 10;
    char              dirname[128], fname[512], buff[512];
    size_t            len,count = 0;
    struct my_dirent *dent;
    
    dent = (struct my_dirent *)buff;

    strcpy(dirname, "/myfs/");
    if (argc > 1)
        strcat(dirname, &argv[1][0]);

    if ((dirfd = sys_opendir(1, -1, dirname, 0)) < 0) {
        printf("dir: error opening: %s\n", dirname);
        return dirfd;
    }

    while(1) {
        len = 1;
        err = sys_readdir(1, dirfd, dent, sizeof(buff), len);
        if (err < 0) {
            printf("readdir failed for: %s\n", dent->d_name);
            if (max_err-- <= 0)
                break;
            
            continue;
        }
        
        if (err == 0)
            break;

        if (strcmp(dent->d_name, "..") == 0 || strcmp(dent->d_name, ".") == 0)
            continue;

        sprintf(fname, "%s/%s", dirname, dent->d_name);
        err = sys_unlink(1, -1, fname);
        if (err != 0) {
            printf("unlink failed for: %s (%Ld)\n", fname, dent->d_ino);
        } else
	        count++;
    }

    if (err != 0) {
        printf("readdir failed on: %s\n", dent->d_name);
    }
	printf("%ld files removed!\n", (long)count);

    sys_closedir(1, dirfd);

	return 0;	// not really correct, but anyway...
}


static int
do_trunc(int argc, char **argv)
{
    int             err, new_size;
    char            fname[256];
    struct my_stat  st;

    strcpy(fname, "/myfs/");
    if (argc < 3) {
        printf("usage: trunc fname newsize\n");
        return FS_EINVAL;
    }
    strcat(fname, argv[1]);
    new_size = strtoul(argv[2], NULL, 0);

    st.size = new_size;
    err = sys_wstat(1, -1, fname, &st, WSTAT_SIZE, 0);
    if (err != 0) {
        printf("truncate to %d bytes failed for %s\n", new_size, fname);
    }

    return err;
}


static int
do_seek(int argc, char **argv)
{
    fs_off_t err;
    fs_off_t pos;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }

    
    if (argc < 2) {
        printf("usage: seek pos\n");
        return FS_EINVAL;
    }
    pos = strtoul(&argv[1][0], NULL, 0);

    err = sys_lseek(1, cur_fd, pos, SEEK_SET);
    if (err != pos) {
        printf("seek to %Ld failed (%Ld)\n", pos, err);
    }

	return err;
}


static int
do_rm(int argc, char **argv)
{
    int err;
    char name[256];

    if (cur_fd >= 0)
        do_close(0, NULL);

    if (argc < 2) {
        printf("rm: need a file name to remove\n");
        return FS_EINVAL;
    }

    sprintf(name, "/myfs/%s", &argv[1][0]);
    
    err = sys_unlink(1, -1, name);
    if (err != 0) {
        printf("error removing: %s: %s\n", name, fs_strerror(err));
    }

	return err;
}


static int
do_rmdir(int argc, char **argv)
{
    int err;
    char name[256];

    if (cur_fd >= 0)
        do_close(0, NULL);

    if (argc < 2) {
        printf("rm: need a file name to remove\n");
        return FS_EINVAL;
    }

    sprintf(name, "/myfs/%s", &argv[1][0]);
    
    err = sys_rmdir(1, -1, name);
    if (err != 0) {
        printf("rmdir: error removing: %s: %s\n", name, fs_strerror(err));
    }

	return err;
}


static int
do_copy_to_myfs(char *host_file, char *bfile)
{
    int     bfd, err = 0;
    char    myfs_name[1024];
    FILE   *fp;
    size_t  amt;
    static char buff[4 * 1024];

    sprintf(myfs_name, "/myfs/%s", bfile);

    fp = fopen(host_file, "rb");
    if (fp == NULL) {
        printf("can't open host file: %s\n", host_file);
        return from_platform_error(errno);
    }
        
    if ((bfd = sys_open(1, -1, myfs_name, O_RDWR|O_CREAT,
                        MY_S_IFREG|MY_S_IRWXU, 0)) < 0) {
        fclose(fp);
        printf("error opening: %s\n", myfs_name);
        return bfd;
    }
    
    while((amt = fread(buff, 1, sizeof(buff), fp)) == sizeof(buff)) {
        err = sys_write(1, bfd, buff, amt);
        if (err < 0)
            break;
    }

    if (amt && err >= 0) {
        err = sys_write(1, bfd, buff, amt);
    }

    if (err < 0) {
        printf("err == %d, amt == %ld\n", err, (long)amt);
        perror("write error");
    }

    sys_close(1, bfd);
    fclose(fp);

	return (err < 0 ? err : 0);
}


static int
do_copy_from_myfs(char *bfile, char *host_file)
{
    int     bfd,err = 0;
    char    myfs_name[1024];
    FILE   *fp;
    size_t  amt;
    static char buff[4 * 1024];

    sprintf(myfs_name, "/myfs/%s", bfile);

    fp = fopen(host_file, "wb");
    if (fp == NULL) {
        printf("can't open host file: %s\n", host_file);
        return from_platform_error(errno);
    }
        
    if ((bfd = sys_open(1, -1, myfs_name, O_RDONLY, MY_S_IFREG, 0)) < 0) {
        fclose(fp);
        printf("error opening: %s\n", myfs_name);
        return bfd;
    }
    
    while(1) {
        amt = sizeof(buff);
        err = sys_read(1, bfd, buff, amt);
        if (err < 0)
            break;

        if (fwrite(buff, 1, err, fp) != amt)
            break;
    }


    if (err < 0)
        perror("read error");

    sys_close(1, bfd);
    fclose(fp);

	return (err < 0 ? err : 0);
}


static int
do_copy(int argc, char **argv)
{
    if (cur_fd >= 0)
        do_close(0, NULL);

    if (argc < 3) {
        printf("copy needs two arguments!\n");
        return FS_EINVAL;
    }
    
    if (argv[1][0] == ':' && argv[2][0] != ':') {
        return do_copy_to_myfs(&argv[1][1], &argv[2][0]);
    }

    if (argv[2][0] == ':' && argv[1][0] != ':') {
        return do_copy_from_myfs(&argv[1][0], &argv[2][1]);
    }

    printf("can't copy around inside of the file system (only in and out)\n");

	return FS_EINVAL;
}


static int
copydir(char *fromPath,char *toPath)
{
    int     bfd, err = 0;
    char    *myfs_name; //[1024];
    char	*from_name; //[1024];
	int		fd;
    size_t  amt;
    char	*buff; //[4 * 1024];
    size_t  bufferSize = 4 * 1024;
	DIR		*from;
	dirent_t	*dirent;

	from = opendir(fromPath);
	if (from == NULL) {
		printf("could not open %s\n", fromPath);
		return FS_ENTRY_NOT_FOUND;
	}

	myfs_name = (char*)malloc(1024);
	from_name = (char*)malloc(1024);
	buff = (char*)malloc(bufferSize);
	if (myfs_name == NULL || from_name == NULL || buff == NULL) {
		printf("out of memory\n");
		return FS_NO_MEMORY;
	}

	while ((dirent = readdir(from)) != NULL) {
		DIR *attrDirectory;
		dirent_t *attr;
		struct stat st;

		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;

		strcpy(from_name, fromPath);
		if (from_name[strlen(from_name) - 1] != '/')
			strcat(from_name, "/");
		strcat(from_name, dirent->d_name);

		if (stat(from_name, &st) != 0)
			continue;
		
		if (st.st_mode & S_IFDIR) {
			char path[1024];
			strcpy(path, toPath);
			strcat(path, "/");
			strcat(path, dirent->d_name);

			if ((err = sys_mkdir(1, -1, path, MY_S_IRWXU)) == FS_OK)
				copydir(from_name, path);
			else
				printf("Could not create directory %s: (%s)\n", path, fs_strerror(err));
		} else {
			fd = open(from_name, O_RDONLY);
			if (fd < 0) {
				printf("can't open host file: %s\n", from_name);
				return fd;
			}

			sprintf(myfs_name, "%s/%s", toPath, dirent->d_name);
			if ((bfd = sys_open(1, -1, myfs_name, O_RDWR|O_CREAT,
								MY_S_IFREG|MY_S_IRWXU, 0)) < 0) {
		        close(fd);
		        printf("error opening: %s\n", myfs_name);
		        return bfd;
		    }

			// copy attributes first!
			if ((attrDirectory = fs_fopen_attr_dir(fd)) != NULL) {
				while ((attr = fs_read_attr_dir(attrDirectory)) != NULL) {
					struct attr_info attrInfo;
					int32 size = bufferSize, bytesRead;

					if (fs_stat_attr(fd, attr->d_name, &attrInfo) != 0)
						continue;

					if (attrInfo.size <= size)
						size = attrInfo.size - 1;
					else
						printf("truncating attribute: %s\n", attr->d_name);

					bytesRead = fs_read_attr(fd, attr->d_name, attrInfo.type, 0, buff, size);
					if (bytesRead < size) {
						printf("could not read attribute %s: %s\n", attr->d_name, fs_strerror(bytesRead));
						continue;
					}
					buff[size] = '\0';

				    err = sys_write_attr(1, bfd, attr->d_name, attrInfo.type, buff, size, 0);
					if (err < FS_OK) {
						printf("write attr (\"%s\", type = %p, %p, %ld bytes) failed: %s\n",
							attr->d_name, (void *)attrInfo.type, buff, size, fs_strerror(err));
						continue;
					}
				}
				fs_close_attr_dir(attrDirectory);
			} else
				puts("could not open attr-dir");

			while ((amt = read(fd, buff, bufferSize)) == bufferSize) {
				err = sys_write(1, bfd, buff, amt);
				if (err < 0)
					break;
			}

			if (amt && err >= 0) {
				err = sys_write(1, bfd, buff, amt);
			}

			if (err < 0) {
				printf("write error: err == %d, amt == %ld\n", err, (long)amt);
			}

			sys_close(1, bfd);
			close(fd);
		}
	}
	closedir(from);

	free(myfs_name);  free(from_name);  free(buff);
	return FS_OK;
}


static int
do_copytest(int argc, char **argv)
{
    char *fromPath = "/boot/apps/internet/mozilla";

	if (argc > 2) {
		printf("usage: copytest <path>");
		return FS_EINVAL;
	} else if (argc == 2)
		fromPath = argv[1];
	else
		printf("copying from: %s\n", fromPath);

	return copydir(fromPath, "/myfs");
}


static int32
copydirthread(void *data)
{
	char *args[] = {"cptest", (char *)data, NULL};

	do_copytest(2, args);
	return 0;
}


static int
do_threadtest(int argc, char **argv)
{
	char *paths[] = {"/boot/home/mail/pinc - axeld/in", "/boot/home/mail/pinc - axeld/out", NULL};
	int32 i;
	
	for (i = 0; paths[i] != NULL; i++) {
		thread_id thread = spawn_thread(copydirthread, "copythread", B_NORMAL_PRIORITY, paths[i]);
		resume_thread(thread);
	}

	//do_lat_fs(0,NULL);

	return 0;
}


int32 gCopyNum;


static int32
copyfilethread(void *data)
{
	char *args[] = {"cp", (char *)data, NULL, NULL};

	char name[32];
	sprintf(name, "target%ld", gCopyNum++);
	args[2] = name;

	do_copy(3, args);
	return 0;
}


static int
do_threadfiletest(int argc, char **argv)
{
	char *paths[] = {":/video/Filme/Wallace & Gromit - A Close Shave.mpg",":/video/Filme/Wallace & Gromit - The Wrong Trousers.mpg",":/stuff/Simpsons/The Simpsons - 13.01 - Treehouse Of Horror XII.mpg",NULL};
	int32 i;

	for (i = 0;paths[i] != NULL;i++) {
		thread_id thread = spawn_thread(copyfilethread,"copythread",B_NORMAL_PRIORITY,paths[i]);
		resume_thread(thread);
	}

	do_lat_fs(0,NULL);

	return 0;
}


static int
do_tracker(int argc, char **argv)
{
	static thread_id thread = -1;

	if (thread < FS_OK) {
		puts("starting tracker...");

		thread = spawn_thread(tracker_loop, "tracker", B_NORMAL_PRIORITY, NULL);
		resume_thread(thread);
	} else {
		status_t status;
		puts("stopping tracker.");

		write_port(gTrackerPort, FSH_KILL_TRACKER, NULL, 0);
		wait_for_thread(thread, &status);

		thread = -1;
	}

	return 0;
}


//	#pragma mark -


static bool sRunStatTest;
static const char *sStatTestFile = "/myfs/stattest-file";


static int32
stat_thread(void *data)
{
	(void)data;

	while (sRunStatTest) {
		struct my_stat st;
		sys_rstat(1, -1, sStatTestFile, &st, 0);
	}
	return 0;
}


static int32
create_remove_thread(void *data)
{
	(void)data;

	while (sRunStatTest) {
		int fd;
		if ((fd = sys_open(1, -1, sStatTestFile, O_RDWR | O_CREAT, S_IFREG | S_IRWXU, 0)) >= 0)
			sys_close(1, fd);

		snooze(10000);
		sys_unlink(1, -1, sStatTestFile);
	}
	return 0;
}


static int
do_stattest(int argc, char **argv)
{
	thread_id a = spawn_thread(stat_thread, "stat_thread", B_NORMAL_PRIORITY, NULL);
	thread_id b = spawn_thread(create_remove_thread, "create_remove_thread", B_NORMAL_PRIORITY, NULL);
	char buffer[1];
	status_t status;

	sRunStatTest = true;
	
	resume_thread(a);
	resume_thread(b);

	printf("This test will stop when you press <Enter>: ");
	fflush(stdout);

	fgets(buffer, 1, stdin);

	sRunStatTest = false;
	
	wait_for_thread(a, &status);
	wait_for_thread(b, &status);

	return 0;
}


//	#pragma mark -


static int
do_attrtest(int argc, char **argv)
{
	int iterations = 10240;
	int maxSize = 1024;
	char *buffer;
	char name[2];
	int i;

    if (cur_fd < 0) {
        printf("no file open! (open or create one with open or make)\n");
        return FS_EINVAL;
    }
    if (argc > 1 && isdigit(*argv[1]))
    	iterations = atol(argv[1]);
    if (argc > 2 && isdigit(*argv[2]))
    	maxSize = atol(argv[2]);
    if ((argc > 1 && !isdigit(*argv[1])) || (argc > 2 && !isdigit(*argv[2]))) {
    	printf("usage: attrs [number of iterations] [max attr size]\n");
    	return FS_EINVAL;
    }

	buffer = (char*)malloc(1024);
	for (i = 0; i < 1024; i++)
		buffer[i] = i;

	name[1] = '\0';

	printf("%d iterations...\n", iterations);

	for (i = 0; i < iterations; i++) {
		int length = rand() % maxSize;
		int err;

		name[0] = rand() % 26 + 'a';

		err = sys_write_attr(1, cur_fd, name, 'CSTR', buffer, length, 0);
		if (err < length)
			printf("error writing attribute: %s\n", fs_strerror(err));
	}

	free(buffer);

	return 0;
}


static int
do_rename(int argc, char **argv)
{
    int  err;
    char oldname[128], newname[128];

    if (cur_fd >= 0)
        do_close(0, NULL);

    if (argc < 3) {
        printf("rename needs two arguments!\n");
        return FS_EINVAL;
    }
    
    strcpy(oldname, "/myfs/");
    strcpy(newname, "/myfs/");

    strcat(oldname, &argv[1][0]);
    strcat(newname, &argv[2][0]);

    err = sys_rename(1, -1, oldname, -1, newname);
    if (err)
        printf("rename failed with err: %s\n", fs_strerror(err));

	return err;
}


static int
do_symlink(int argc, char **argv)
{
	char target[B_PATH_NAME_LENGTH];
	char *source;
	status_t status;

	if (argc < 3) {
		printf("usage: symlink <source> <target>\n");
		return FS_EINVAL;
	}

	source = argv[1];

	strcpy(target, "/myfs/");
	strcpy(target + 6, argv[2]);

	status = sys_symlink(1, source, -1, target);
	if (status != FS_OK)
		printf("symlink failed with error: %s\n", fs_strerror(status));

	return status;
}


static int
do_sync(int argc, char **argv)
{
    int err;
    
    err = sys_sync();
    
    if (err)
        printf("sync failed with err %s\n", fs_strerror(err));

	return err;
}


#define MAX_LIVE_QUERIES	10
void *gQueryCookie[MAX_LIVE_QUERIES] = {NULL};
char *gQueryString[MAX_LIVE_QUERIES] = {NULL};


static int
do_listqueries(int argc, char **argv)
{
	int min, max;

	if (argc == 2)
		min = max = atol(argv[1]);
	else {
		min = 0;
		max = MAX_LIVE_QUERIES - 1;
	}

	// list all queries (or only the one asked for)

	for (; min <= max; min++) {
		if (gQueryCookie[min] == NULL)
			continue;

		printf("%d. (%p) %s\n", min, gQueryCookie[min], gQueryString[min]);
	}

	return 0;
}


static int
do_stopquery(int argc, char **argv)
{
	int err;
	int min, max;

	if (gQueryCookie == NULL) {
		printf("no query running (use the 'startquery' command to start a query).\n");
		return FS_EINVAL;
	}

	if (argc == 2)
		min = max = atol(argv[1]);
	else {
		min = 0;
		max = MAX_LIVE_QUERIES - 1;
	}

	// close all queries (or only the one asked for)

	for (; min <= max; min++) {
		if (gQueryCookie[min] == NULL)
			continue;

		err = sys_close_query(true, -1, "/myfs/.", gQueryCookie[min]);
		if (err < 0) {
			printf("could not close query: %s\n", fs_strerror(err));
			return err;
		}
		gQueryCookie[min] = NULL;
		free(gQueryString[min]);
	}

	return 0;
}


static int
do_startquery(int argc, char **argv)
{
	char *query;
	int freeSlot;
	int err;

	for (freeSlot = 0; freeSlot < MAX_LIVE_QUERIES; freeSlot++) {
		if (gQueryCookie[freeSlot] == NULL)
			break;
	}
	if (freeSlot == MAX_LIVE_QUERIES) {
		printf("All query slots are used, stop some\n");
		return FS_EINVAL;
	}

	if (argc != 2) {
		printf("query string expected\n");
		return FS_EINVAL;
	}
	query = argv[1];

	err = sys_open_query(true, -1, "/myfs/.", query, B_LIVE_QUERY, freeSlot, freeSlot, &gQueryCookie[freeSlot]);
	if (err < 0) {
		printf("could not open query: %s\n", fs_strerror(err));
		return err;
	}

	printf("query number %d started - use the 'stopquery' command to stop it.\n", freeSlot);
	gQueryString[freeSlot] = strdup(query);

	return 0;
}


static int
do_query(int argc, char **argv)
{
	char buffer[2048];
	struct my_dirent *dent = (struct my_dirent *)buffer;
	void *cookie;
	char *query;
	int max_err = 10;
	int err;

	if (argc != 2) {
		printf("query string expected");
		return FS_EINVAL;
	}
	query = argv[1];

	err = sys_open_query(true, -1, "/myfs/.", query, 0, 42, 42, &cookie);
	if (err < 0) {
		printf("could not open query: %s\n", fs_strerror(err));
		return FS_EINVAL;
	}
	
	while (true) {
		err = sys_read_query(true, -1, "/myfs/.", cookie, dent, sizeof(buffer), 1);
		if (err < 0) {
			printf("readdir failed for: %s\n", dent->d_name);
			if (max_err-- <= 0)
				break;

			continue;
		}
	    
		if (err == 0)
			break;

		printf("%s\n", dent->d_name);
	}

	err = sys_close_query(true, -1, "/myfs/.", cookie);
	if (err < 0) {
		printf("could not close query: %s\n", fs_strerror(err));
		return err;
	}

	return 0;
}


#define MAX_ITER  512
#define NUM_READS 16
#define READ_SIZE 4096

static int
do_cio(int argc, char **argv)
{
    int            i, j, fd;
    char           fname[64];
    fs_off_t       pos;
    size_t         len;
    static char    buff[READ_SIZE];
    struct timeval start, end, result;
    
    strcpy(fname, "/myfs/");
    if (argc == 1)
        strcpy(fname, "/myfs/fsh");
    else
        strcat(fname, &argv[1][0]);
    
    fd = sys_open(1, -1, fname, O_RDONLY, MY_S_IFREG, 0);
    if (fd < 0) {
        printf("can't open %s\n", fname);
        return fd;
    }

    gettimeofday(&start, NULL);

	for (i = 0; i < MAX_ITER; i++) {
		for (j = 0; j < NUM_READS; j++) {
			len = sizeof(buff);
			if (sys_read(1, fd, buff, len) != (ssize_t)len) {
				perror("cio read");
				break;
			}
		}

		pos = 0;
		if (sys_lseek(1, fd, pos, SEEK_SET) != pos) {
			perror("cio lseek");
			break;
		}
	}

    gettimeofday(&end, NULL);
    SubTime(&end, &start, &result);
        
    printf("read %lu bytes in %2ld.%.6ld seconds\n",
           (unsigned long)((MAX_ITER * NUM_READS * sizeof(buff)) / 1024),
           result.tv_sec, result.tv_usec);


    sys_close(1, fd);


	return 0;
}


static int
mkfile(char *s, int sz)
{
    int  fd, len;
    int err = 0;
	char *buffer;

    if ((fd = sys_open(1, -1, s, O_RDWR|O_CREAT,
                       MY_S_IFREG|MY_S_IRWXU, 0)) < 0) {
        printf("error creating: %s\n", s);
        return fd;
    }

	buffer = (char*)malloc(16 * 1024);
	if (buffer == NULL)
		return FS_ENOMEM;

    len = sz;
    if (sz) {
    	err = sys_write(1, fd, buffer, len);
        if (err != len)
            printf("error writing %d bytes to %s\n", sz, s);
    }
    if (sys_close(1, fd) != 0)
        printf("close failed?\n");

	free(buffer);

	return (err < 0 ? err : 0);
}

#define LAT_FS_ITER   1000


static int
do_lat_fs(int argc, char **argv)
{
    int  i, j, iter;
/*  int  sizes[] = { 0, 1024, 4096, 10*1024 }; */
    int  sizes[] = { 0, 1024 };
    char name[64];

    iter = LAT_FS_ITER;

    if (argc > 1) 
        iter = strtoul(&argv[1][0], NULL, 0);

    for (i = 0; (size_t)i < sizeof(sizes)/sizeof(int); i++) {
        printf("CREATING: %d files of %5d bytes each\n", iter, sizes[i]);
        for (j = 0; j < iter; ++j) {
            sprintf(name, "/myfs/%.5d", j);
            mkfile(name, sizes[i]);
        }

        printf("DELETING: %d files of %5d bytes each\n", iter, sizes[i]);
        for (j = 0; j < iter; ++j) {
            sprintf(name, "/myfs/%.5d", j);
            if (sys_unlink(1, -1, name) != 0)
                printf("lat_fs: failed to remove: %s\n", name);
        }
    }

	return 0;
}


static int
do_create(int argc, char **argv)
{
    int  j, iter = 100, err;
    int  size = 0;
    char name[64];

    sprintf(name, "/myfs/test");
    err = sys_mkdir(1, -1, name, MY_S_IRWXU);
    if (err && err != FS_EEXIST)
        printf("mkdir of %s returned: %s (%d)\n", name, fs_strerror(err), err);

    if (argc > 1)
        iter = strtoul(&argv[1][0], NULL, 0);
    if (argc > 2)
        size = strtoul(&argv[2][0], NULL, 0);

    printf("creating %d files (each %d bytes long)...\n", iter, size);

    for (j = 0; j < iter; ++j) {
        sprintf(name, "/myfs/test/%.5d", j);
        /* printf("CREATING: %s (%5d)\n", name, size); */
        mkfile(name, size);
    }

	return 0;
}


static int
do_delete(int argc, char **argv)
{
    int  j, iter = 100;
    char name[64];

    if (argc > 1)
        iter = strtoul(&argv[1][0], NULL, 0);

    for (j = 0; j < iter; ++j) {
        sprintf(name, "/myfs/test/%.5d", j);
        printf("DELETING: %s\n", name);
        if (sys_unlink(1, -1, name) != 0)
            printf("lat_fs: failed to remove: %s\n", name);
    }

	return 0;
}


static int do_help(int argc, char **argv);


static cmd_entry builtin_commands[] =
{
    { "ls",      do_dir, "print a directory listing" },
    { "dir",     do_dir, "print a directory listing (same as ls)" },
    { "open",    do_open, "open an existing file for read/write access" },
    { "make",    do_make, "create a file (optionally specifying a name)" },
    { "close",   do_close, "close the currently open file" },
    { "mkdir",   do_mkdir, "create a directory" },
    { "rdtest",  do_read_test, "read N bytes from the current file. default is 256" },
    { "wrtest",  do_write_test, "write N bytes to the current file. default is 256" },
    { "wrstream",  do_write_stream, "write N blocks of 4096 bytes to the current file. default is 100000" },
    { "rm",      do_rm, "remove the named file" },
    { "rmall",   do_rmall, "remove all the files. if no dirname, use '.'" },
    { "rmdir",   do_rmdir, "remove the named directory" },
    { "cp",      do_copy, "copy a file to/from myfs. prefix a ':' for host filenames" },
    { "copy",    do_copy, "same as cp" },
    { "trunc",   do_trunc, "truncate a file to the size specified" },
    { "seek",    do_seek, "seek to the position specified" },
    { "mv",      do_rename, "rename a file or directory" },
    { "sync",    do_sync, "call sync" },
    { "wrattr",  do_write_attr, "write attribute \"name\" to the current file (N bytes [256])." },
    { "rdattr",  do_read_attr, "read attribute \"name\" from the current file (N bytes [256])." },
    { "rmattr",  do_remove_attr, "remove attribute \"name\" from the current file." },
    { "attrs",   do_attrtest, "writes random attributes [a-z] up to 1023 bytes, N [10240] iterations." },
    { "lat_fs",  do_lat_fs, "simulate what the lmbench test lat_fs does" },
    { "create",  do_create, "create N files. default is 100" },
    { "delete",  do_delete, "delete N files. default is 100" },
    { "query",   do_query, "run a query on the file system" },
    { "startquery", do_startquery, "run a live query on the file system (up to 10)" },
    { "stopquery",  do_stopquery, "stops live query N, or all of them if none is specified" },
    { "lsquery",  do_listqueries, "list all live queries" },
    { "ioctl",   do_ioctl, "execute ioctl() without an inode (okay, with the root node)" },
    { "fcntl",   do_fcntl, "execute ioctl() with the active inode" },
    { "cptest",  do_copytest, "copies all files from the given path" },
    { "threads", do_threadtest, "copies several files, and does a lat_fs simulaneously" },
    { "mfile",	 do_threadfiletest, "copies several big files simulaneously" },
    { "tracker", do_tracker, "starts a Tracker like background thread that will listen to fs updates" },
    { "cio",	 do_cio, "does a I/O speed test" },
    { "stattest", do_stattest, "does an \"early\"/\"late\" stat test for files that are created or deleted" },
    { "symlink", do_symlink, "creates the specified symlink on the device" },

    { NULL, NULL }
};

static cmd_entry help_commands[] =
{
    { "help",    do_help, "print this help message" },
    { "?",       do_help, "print this help message" },
    { NULL, NULL }
};

static cmd_entry *fsh_commands[] =
{
	builtin_commands,
	additional_commands,
	help_commands,
	NULL
};

static int
do_help(int argc, char **argv)
{
    printf("commands fsh understands:\n");
    for (cmd_entry **commands = fsh_commands; *commands; commands++) {
	    cmd_entry *cmd = *commands;
	    for (; cmd->name != NULL; cmd++) {
	        printf("%8s - %s\n", cmd->name, cmd->help);
	    }
    }

	return 0;
}

static char *
getline(char *prompt, char *input, int len)
{
	if (sInteractiveMode) {
	    printf("%s", prompt); fflush(stdout);

    	return fgets(input, len, stdin);
	} else
		return get_external_command(prompt, input, len);
}


// This might have been defined by additional_commands.h - so
// we can't do it earlier.
#ifndef FS_SHELL_PROMPT
#	define FS_SHELL_PROMPT	"fsh"
#endif


static void
do_fsh(void)
{
#if 0
	char stack_filler[1024 * 1024 * 16 - 32 * 1024];
		// actually emulating 12kB stack size (BeOS has 16 MB - ~20kB for the main thread)
#endif
    int   argc, len;
    char *prompt = FS_SHELL_PROMPT ">> ";
    char  input[1024], **argv;
    cmd_entry *cmd = NULL;

    while(getline(prompt, input, sizeof(input)) != NULL) {
        argc = 0;
        argv = build_argv(input, &argc);
        if (argv == NULL || argc == 0) {
            continue;
        }

        len = strlen(&argv[0][0]);

	    bool done = false;
		for (cmd_entry **commands = fsh_commands;
			 !done && *commands;
			 commands++) {
		    cmd = *commands;
		    for (; cmd->name != NULL; cmd++) {
				if (strncmp(cmd->name, &argv[0][0], len) == 0) {
	                int result = cmd->func(argc, argv);
	
					if (!sInteractiveMode)
						reply_to_external_command(to_platform_error(result));
	
					done = true;
					break;
	            }
	        }
	    }
        
        if (strncmp(&argv[0][0], "quit", 4) == 0
            || strncmp(&argv[0][0], "exit", 4) == 0) {
			if (!sInteractiveMode)
				reply_to_external_command(0);
            break;
        }

        if ((cmd == NULL || cmd->name == NULL) && argv[0][0] != '\0') {
            printf("command `%s' not understood\n", &argv[0][0]);

			if (!sInteractiveMode)
				reply_to_external_command(EINVAL);
        }
        
        free(argv);
    }

    if (feof(stdin))
        printf("\n");

    if (cur_fd != -1)
        do_close(0, NULL);
}




