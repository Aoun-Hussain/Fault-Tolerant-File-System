#ifndef GTFS
#define GTFS

#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <map>

using namespace std;

#define PASS "\033[32;1m PASS \033[0m\n"
#define FAIL "\033[31;1m FAIL \033[0m\n"

// GTFileSystem basic data structures

#define MAX_FILENAME_LEN 255
#define MAX_NUM_FILES_PER_DIR 1024

extern int do_verbose;

typedef struct gtfs {
    std::string dirname;
    // TODO: Add any additional fields if necessary

    map <string, void*>* file_add_dict;
    static gtfs* gtfs_metadata;

} gtfs_t;

typedef struct file {
    std::string filename;
    int file_length;
    // TODO: Add any additional fields if necessary

    void* addr;
} file_t;

typedef struct write {
    std::string filename;
    int offset;
    int length;
    char *data;
    // TODO: Add any additional fields if necessary

    char *org_data;
    void* addr;
} write_t;

// GTFileSystem basic API calls

gtfs_t* gtfs_init(std::string directory, int verbose_flag);
int gtfs_clean(gtfs_t *gtfs);

file_t* gtfs_open_file(gtfs_t* gtfs, std::string filename, int file_length);
int gtfs_close_file(gtfs_t* gtfs, file_t* fl);
int gtfs_remove_file(gtfs_t* gtfs, file_t* fl);

char* gtfs_read_file(gtfs_t* gtfs, file_t* fl, int offset, int length);
write_t* gtfs_write_file(gtfs_t* gtfs, file_t* fl, int offset, int length, const char* data);
int gtfs_sync_write_file(write_t* write_id);
int gtfs_abort_write_file(write_t* write_id);

int gtfs_clean_n_bytes(gtfs_t *gtfs, int bytes);
int gtfs_sync_write_file_n_bytes(write_t* write_id, int bytes);

#endif
