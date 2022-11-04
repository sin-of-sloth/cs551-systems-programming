/********************************************************************
*   THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING     *
*   A TUTOR OR CODE WRITTEN BY OTHER STUDENTS                       *
*                               - ARJUN LAL                         *
********************************************************************/


#include <ar.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <utime.h>
#include <time.h>

#define ARMAG "!<arch>\n"
#define ARMAG_SIZE 8
#define AR_STR_HDR_SIZE sizeof(struct ar_hdr)
#define DEFAULT_MODE 0666
#define CURRENT_DIR "./"

struct meta {
    char name[16];
    int mode;
    int size;
    time_t mtime;
};


int fill_ar_hdr(char *filename, struct ar_hdr *hdr) {
    struct stat statbuf;
    if(stat(filename, &statbuf) == -1) {
        perror("stat");
        exit(1);
    }

    strcpy(hdr->ar_name, filename);
    int i;
    // if file name does not end with a '/', add it
    for(i = 0; hdr->ar_name[i] != '\0'; i++);
    if(hdr->ar_name[i - 1] != '/') {
        hdr->ar_name[i] = '/';
        hdr->ar_name[i + 1] = '\0';
    }
    sprintf(hdr->ar_date, "%ld", statbuf.st_mtim.tv_sec);
    sprintf(hdr->ar_uid, "%d", statbuf.st_uid);
    sprintf(hdr->ar_gid, "%d", statbuf.st_gid);
    sprintf(hdr->ar_mode, "%o", statbuf.st_mode);
    sprintf(hdr->ar_size, "%ld", statbuf.st_size);
    strcpy(hdr->ar_fmag, "`\n");
    return 1;
}


int fill_meta( struct ar_hdr hdr, struct meta *meta) {
    strcpy(meta->name, hdr.ar_name);
    int i;
    // if file name ends with a '/', remove it
    for(i = 0; meta->name[i] != '\0'; i++);
    if(meta->name[i - 1] == '/') {
        meta->name[i - 1] = '\0';
    }
    sscanf(hdr.ar_mode + 2, "%o", (unsigned *) &(meta->mode));
    meta->size = atoi(hdr.ar_size);
    meta->mtime = strtol(hdr.ar_date, NULL, 10);
}


int archive_files(char *archive_name, char *argv[], int input_start_index, int input_end_index) {
    struct ar_hdr hdr;
    int fd, i;
    char begin_str[ARMAG_SIZE] = ARMAG;
    char hdr_string[AR_STR_HDR_SIZE];
    if((fd = creat(archive_name, DEFAULT_MODE)) == -1) {
        perror("open1");
        exit(1);
    }

    // write the string that begins the archive file
    write(fd, begin_str, sizeof(begin_str));

    // loop through each file to be archived
    for(i = input_start_index; i < input_end_index; i++) {
        // fill the ar_hdr
        fill_ar_hdr(argv[i], &hdr);

        // open the file to be archived as read only
        int fd2;
        if((fd2 = open(argv[i], O_RDONLY)) == -1) {
            perror("open2");
            exit(1);
        }

        // create the ar_hdr string
        snprintf(
            hdr_string,
            AR_STR_HDR_SIZE + 1,
            "%-16s%-12s%-6s%-6s%-8s%-10s%-2s",
            hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag
        );
        // write the ar_hdr to archive
        write(fd, hdr_string, AR_STR_HDR_SIZE);
        // read file contents from the file to be archived
        int content_size = atoi(hdr.ar_size);
        int req_size = content_size;
        if(content_size % 2 != 0) {
            req_size += 1;
        }
        // allocate required buffer
        char *buff = (char *) malloc(sizeof(char) * req_size);
        read(fd2, buff, content_size);
        // if file size is odd, add a padding byte
        if(content_size % 2 != 0) {
            buff[content_size] = '\n';
        }
        // write the file contents
        write(fd, buff, req_size);
        // free buffer
        free(buff);

        // close the file to be archived
        close(fd2);
    }

    close(fd);

    return 1;
}


int extract_files(char *archive_name, char *argv[], int input_start_index, int input_end_index, int original) {
    struct meta metadata;
    struct ar_hdr hdr;
	struct utimbuf newt;
    int fd;
    char buff[AR_STR_HDR_SIZE + 1];

    // open the archive to read
    if((fd = open(archive_name, O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    // read the beginning
    read(fd, buff, ARMAG_SIZE);
    // read each archive entry
    while(read(fd, buff, AR_STR_HDR_SIZE) > 0) {
        // read the header
        sscanf(
            buff,
            "%16s%12s%6s%6s%8s%10s%2s",
            hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag
        );
        fill_meta(hdr, &metadata);

        // check if the file has been asked to be extracted
        int i, extract = 0;
        for(i = input_start_index; i < input_end_index; i++) {
            if(strcmp(argv[i], metadata.name) == 0) {
                extract = 1;
                // we only need to extract this first match
                // change the filename so it doesn't match anymore
                argv[i][0] = '\0';
                break;
            }
        }

        // if not to be extracted, lseek to next header position
        if(!extract) {
            long seek_size = strtol(hdr.ar_size, NULL, 10);
            if(seek_size % 2 == 1) {
                seek_size += 1;
            }
            // move to next header
            lseek(fd, seek_size, SEEK_CUR);
            continue;
        }

        // open the file to be archived as read only
        int fd2;
        // if -o set, mode should be original mode, else 0666
        mode_t mode = original ? metadata.mode : 0666;
        if((fd2 = creat(metadata.name, mode)) == -1) {
            perror("open2");
            exit(1);
        }

        // read file contents from the archive
        int content_size = atoi(hdr.ar_size);
        int req_size = content_size;
        if(content_size % 2 != 0) {
            req_size += 1;
        }
        // allocate required buffer
        char *content_buff = (char *) malloc(sizeof(char) * req_size);
        read(fd, content_buff, req_size);
        // if file size is odd, remove the padding byte
        if(content_size % 2 != 0) {
            content_buff[content_size] = '\0';
        }
        // write the file contents
        write(fd2, content_buff, content_size);
        // free buffer
        free(content_buff);

        // if -o set, mtime and atime should be original time
        newt.actime = metadata.mtime;
        newt.modtime = metadata.mtime;
        if (utime(metadata.name, &newt) == -1) {
            perror("utime");
            exit(1);
		}

        // close the file created
        close(fd2);
    }

    close(fd);

    return 1;
}


void print_permissions(mode_t permissions) {
    printf((permissions & S_IRUSR) ? "r" : "-");
    printf((permissions & S_IWUSR) ? "w" : "-");
    printf((permissions & S_IXUSR) ? "x" : "-");
    printf((permissions & S_IRGRP) ? "r" : "-");
    printf((permissions & S_IWGRP) ? "w" : "-");
    printf((permissions & S_IXGRP) ? "x" : "-");
    printf((permissions & S_IROTH) ? "r" : "-");
    printf((permissions & S_IWOTH) ? "w" : "-");
    printf((permissions & S_IXOTH) ? "x" : "-");
}


int list_archive_contents(char *archive_name, int verbose) {
    struct meta metadata;
    struct ar_hdr hdr;
    int fd;
    char buff[AR_STR_HDR_SIZE + 1];

    // create the archive
    if((fd = open(archive_name, O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    // read the beginning
    read(fd, buff, ARMAG_SIZE);
    // read each archive entry
    while(read(fd, buff, AR_STR_HDR_SIZE) > 0) {
        // read the header
        sscanf(
            buff,
            "%16s%12s%6s%6s%8s%10s%2s",
            hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag
        );
        fill_meta(hdr, &metadata);
        long seek_size = strtol(hdr.ar_size, NULL, 10);
        if(seek_size % 2 == 1) {
            seek_size += 1;
        }

        // if not verbose, just print file name
        if(!verbose) {
            printf("%s", hdr.ar_name);
        }
        else {
            char date[18];
            strftime(date, sizeof(date), "%b %d %H:%M %Y", localtime(&metadata.mtime));
            print_permissions(metadata.mode);
            printf(" %s/%s %6d %s %s", hdr.ar_uid, hdr.ar_gid, metadata.size, date, metadata.name);
        }
        printf("\n");

        // move to next header
        lseek(fd, seek_size, SEEK_CUR);
    }

    return 1;
}


int delete_files(char *archive_name, char *argv[], int input_start_index, int input_end_index) {
    struct meta metadata;
    struct ar_hdr hdr;
    int fd, fd2;
    char buff[AR_STR_HDR_SIZE + 1];
    char hdr_string[AR_STR_HDR_SIZE];

    // open the archive to read
    if((fd = open(archive_name, O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    // unlink the file
    if(unlink(archive_name) == -1) {
        perror("unlink");
        exit(1);
    }


    // open new archive to write to
    if((fd2 = creat(archive_name, 0666)) == -1) {
        perror("open2");
        exit(1);
    }

    // read the beginning
    read(fd, buff, ARMAG_SIZE);
    // write the beginning
    write(fd2, buff, ARMAG_SIZE);

    // read each archive entry
    while(read(fd, buff, AR_STR_HDR_SIZE) > 0) {
        // read the header
        sscanf(
            buff,
            "%16s%12s%6s%6s%8s%10s%2s",
            hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag
        );
        hdr.ar_fmag[0] = '`';
        hdr.ar_fmag[1] = '\n';
        fill_meta(hdr, &metadata);

        // check if the file has been asked to be deleted
        int i, delete = 0;
        for(i = input_start_index; i < input_end_index; i++) {
            if(strcmp(argv[i], metadata.name) == 0) {
                delete = 1;
                // we only need to delete this first match
                // change the filename so it doesn't match anymore
                argv[i][0] = '\0';
                break;
            }
        }

        // if to be deleted, lseek to next header position
        if(delete) {
            long seek_size = strtol(hdr.ar_size, NULL, 10);
            if(seek_size % 2 == 1) {
                seek_size += 1;
            }
            // move to next header
            lseek(fd, seek_size, SEEK_CUR);
            continue;
        }

        // create the ar_hdr string
        snprintf(
            hdr_string,
            AR_STR_HDR_SIZE + 1,
            "%-16s%-12s%-6s%-6s%-8s%-10s%-2s",
            hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag
        );
        // write the ar_hdr to new archive
        write(fd2, hdr_string, AR_STR_HDR_SIZE);

        // read file contents from the archive
        int content_size = atoi(hdr.ar_size);
        int req_size = content_size;
        if(content_size % 2 != 0) {
            req_size += 1;
        }
        // allocate required buffer
        char *content_buff = (char *) malloc(sizeof(char) * req_size);
        // read from archive
        read(fd, content_buff, req_size);
        // write to new archive
        write(fd2, content_buff, req_size);
        // free buffer
        free(content_buff);
    }

    close(fd);
    close(fd2);

    return 1;

}


int append_files(char *archive_name, int num_of_days) {
    DIR *dirp;
    struct dirent *dp;
    int fd;
    struct ar_hdr hdr;
    struct stat statbuf;
    char begin_str[ARMAG_SIZE] = ARMAG;

    // try to open in append mode to see if file exists
    if((fd = open(archive_name, O_WRONLY | O_APPEND)) == -1) {
        // if it doesn't exist, create new one
        if((fd = creat(archive_name, DEFAULT_MODE)) == -1) {
            perror("open1");
            exit(1);
        }

        // write the string that begins the archive file
        write(fd, begin_str, sizeof(begin_str));
    }

    // open current directory
    if (!(dirp = opendir(CURRENT_DIR))) {
        perror("opendir:");
        exit(1);
    }

    // read through the files in directory
    while ((dp = readdir(dirp)) != NULL) {
        if(strcmp(dp->d_name, archive_name) == 0) {
            continue;
        }
        // stat the file
        if (stat(dp->d_name, &statbuf) == -1) {
            perror("stat");
            exit(1);
        }

        // find how many days old the file it is
        time_t file_time = statbuf.st_mtim.tv_sec;
        time_t current_time = time(NULL);
        int file_age = (current_time - file_time) / (24 * 60 * 60);

        // if it's a regular file and older than N days, append to archive
        if(S_ISREG(statbuf.st_mode) && (file_age >= num_of_days)) {
            char hdr_string[AR_STR_HDR_SIZE];

            // fill the ar_hdr
            fill_ar_hdr(dp->d_name, &hdr);

            // open the file to be archived as read only
            int fd2;
            if((fd2 = open(dp->d_name, O_RDONLY)) == -1) {
                perror("open2");
                exit(1);
            }

            // create the ar_hdr string
            snprintf(
                hdr_string,
                AR_STR_HDR_SIZE + 1,
                "%-16s%-12s%-6s%-6s%-8s%-10s%-2s",
                hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag
            );
            // write the ar_hdr to archive
            write(fd, hdr_string, AR_STR_HDR_SIZE);

            // read file contents from the file to be archived
            int content_size = atoi(hdr.ar_size);
            int req_size = content_size;
            if(content_size % 2 != 0) {
                req_size += 1;
            }
            // allocate required buffer
            char *buff = (char *) malloc(sizeof(char) * req_size);
            read(fd2, buff, content_size);
            // if file size is odd, add a padding byte
            if(content_size % 2 != 0) {
                buff[content_size] = '\n';
            }
            // write the file contents
            write(fd, buff, req_size);
            // free buffer
            free(buff);

            // close the file to be archived
            close(fd2);
        }
    }

    closedir(dirp);
    close(fd);

    return 1;
}


int main(int argc, char *argv[]) {
    // get options
    int opt, quick = 0, extract = 0, original = 0, list = 0, verbose = 0, delete = 0, append = 0, num_of_days, input_index;
    char *archive_name;

    while ((opt = getopt(argc, argv, "qxotvdA:")) != -1) {
        switch(opt) {
            case 'q':
                quick = 1;
                continue;
            case 'x':
                extract = 1;
                continue;
            case 'o':
                original = 1;
                continue;
            case 't':
                list = 1;
                continue;
            case 'v':
                verbose = 1;
                continue;
            case 'd':
                delete = 1;
                continue;
            case 'A':
                append = 1;
                num_of_days = (int) strtol(optarg, NULL, 10);
                continue;
            default:
                fprintf(stderr, "Usage: myar [qxotvdA:] archive-file [file1 ...]");
                exit(1);
        }
    }

    // optind will now have index of archive name in argv
    archive_name = argv[optind];

    if(quick) {
        archive_files(archive_name, argv, optind + 1, argc);
        return 0;
    }
    if(extract) {
        extract_files(archive_name, argv, optind + 1, argc, original);
        return 0;
    }
    if(list) {
        list_archive_contents(archive_name, verbose);
        return 0;
    }
    if(delete) {
        delete_files(archive_name, argv, optind + 1, argc);
        return 0;
    }
    if(append) {
        append_files(archive_name, num_of_days);
        return 0;
    }
    return 0;
}
