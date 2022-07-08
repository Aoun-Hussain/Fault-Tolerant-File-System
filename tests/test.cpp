#include <gtfs.hpp>
#include <constants.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <cstring>

using namespace std;

// Assumes files are located within the current directory
string directory;
int verbose {0};

// **Test 1**: Testing that data written by one process is then successfully read by another process.
void writer() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test1.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());
    gtfs_sync_write_file(wrt);

    gtfs_close_file(gtfs, fl);
}

void reader() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test1.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    char *data = gtfs_read_file(gtfs, fl, 10, str.length());
    if (data != NULL) {
        str.compare(string(data)) == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

void test_write_read() {
    int pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        writer();
        exit(0);
    }
    waitpid(pid, NULL, 0);
    reader();
}

// **Test 2**: Testing that aborting a write returns the file to its original contents.

void test_abort_write() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test2.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Testing string.\n";
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 0, str.length(), str.c_str());
    gtfs_sync_write_file(wrt1);

    write_t *wrt2 = gtfs_write_file(gtfs, fl, 20, str.length(), str.c_str());
    gtfs_abort_write_file(wrt2);

    char *data1 = gtfs_read_file(gtfs, fl, 0, str.length());
    if (data1 != NULL) {
        // First write was synced so reading should be successfull
        if (str.compare(string(data1)) != 0) {
            cout << FAIL;
        }
        // Second write was aborted and there was no string written in that offset
        char *data2 = gtfs_read_file(gtfs, fl, 20, str.length());
        if (data2 == NULL) {
            cout << FAIL;
        } else if (string(data2).compare("") == 0) {
            cout << PASS;
        }
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl);
}

// **Test 3**: Testing that the logs are truncated.

void test_truncate_log() {

    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test3.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Testing string.\n";
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 0, str.length(), str.c_str());
    gtfs_sync_write_file(wrt1);

    write_t *wrt2 = gtfs_write_file(gtfs, fl, 20, str.length(), str.c_str());
    gtfs_sync_write_file(wrt2);

    cout << "Before GTFS cleanup\n";
    system("ls -l " TEST_FS_DIR);

    gtfs_clean(gtfs);

    cout << "After GTFS cleanup\n";
    system("ls -l " TEST_FS_DIR);

    cout << "If log is truncated: " << PASS << "If exactly same output:" << FAIL;

    gtfs_close_file(gtfs, fl);

}

string str0 = "Hi, I'm the writer 0.\n";
string str1 = "Hi, I'm the writer 1.\n";
string str2 = "Hi, I'm the writer 2.\n";

// **Test 4**: Testing read from memory(without sync)
void test_read_directly(){
    gtfs_t *gtfs = gtfs_init(directory, verbose);

    string filename = "test4.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    write_t *wrt0 = gtfs_write_file(gtfs, fl, 10, str0.length(), str0.c_str());

    char *data0 = gtfs_read_file(gtfs, fl, 10, str0.length());

    if (data0 != NULL) {
        str0.compare(string(data0)) == 0 ? cout << PASS : cout << "Read is not the same: "<< FAIL;
    } else {
        cout << "Read is NULL: " << FAIL;
    }

    gtfs_abort_write_file(wrt0);
    gtfs_close_file(gtfs, fl);
}

// **Test 5**: Testing that multiple data written by one process is then successfully read by another process.
// sync write 0 and write 2, and abort write 1
// check if undo log is functional

void writes(string filename) {
    gtfs_t *gtfs = gtfs_init(directory, verbose);

    file_t *fl = gtfs_open_file(gtfs, filename, 500);

    write_t *wrt0 = gtfs_write_file(gtfs, fl, 10, str0.length(), str0.c_str());
    gtfs_sync_write_file(wrt0);

    write_t *wrt1 = gtfs_write_file(gtfs, fl, 100, str1.length(), str1.c_str());
    gtfs_abort_write_file(wrt1);

    write_t *wrt2 = gtfs_write_file(gtfs, fl, 200, str2.length(), str2.c_str());
    gtfs_sync_write_file(wrt2);

    gtfs_close_file(gtfs, fl);
}

void reads(string filename) {
    gtfs_t *gtfs = gtfs_init(directory, verbose);

    file_t *fl = gtfs_open_file(gtfs, filename, 500);

    char *data0 = gtfs_read_file(gtfs, fl, 10, str0.length());
    char *data1 = gtfs_read_file(gtfs, fl, 100, str1.length());
    char *data2 = gtfs_read_file(gtfs, fl, 200, str2.length());

    if (data0 != NULL) {
        str0.compare(string(data0)) == 0 ? cout << PASS : cout << "First read is not the same: "<< FAIL;
    } else {
        cout << "First read is NULL: " << FAIL;
    }

    if (data1 != NULL){
        string(data1).empty() ? cout << PASS : cout << "Second read is not empty: "<< FAIL;
    } else {
        cout << "Second read is NULL: " << FAIL;
    }

    if (data2 != NULL) {
        str2.compare(string(data2)) == 0 ? cout << PASS : cout << "Third read is not the same: "<< FAIL;
    } else {
        cout << "Third read is NULL: " << FAIL;
    }

    gtfs_close_file(gtfs, fl);
}

void test_multi_write_read() {

    int pid;
    string filename = "test5.txt";

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        writes(filename);
        exit(0);
    }
    waitpid(pid, NULL, 0);
    reads(filename);
}

// **Test 6**: Write at same place, but abort
// the operation is the same as test 4, but different offset of write 1
// check if undo log is functional

void write_overlap(string filename) {
    gtfs_t *gtfs = gtfs_init(directory, verbose);

    file_t *fl = gtfs_open_file(gtfs, filename, 500);

    write_t *wrt0 = gtfs_write_file(gtfs, fl, 10, str0.length(), str0.c_str());
    gtfs_sync_write_file(wrt0);

    write_t *wrt1 = gtfs_write_file(gtfs, fl, 10, str1.length(), str1.c_str());
    gtfs_abort_write_file(wrt1);

    write_t *wrt2 = gtfs_write_file(gtfs, fl, 200, str2.length(), str2.c_str());
    gtfs_sync_write_file(wrt2);

    gtfs_close_file(gtfs, fl);
}


void test_write_at_same_place() {

    int pid;
    string filename = "test6.txt";

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        write_overlap(filename);
        exit(0);
    }
    waitpid(pid, NULL, 0);
    reads(filename);
}

/* Test 9 */
void writer_crash() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test_crash.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());
    gtfs_sync_write_file(wrt);

    string str2 = "Hi, I'm the reader.\n";
    write_t *wrt2 = gtfs_write_file(gtfs, fl, 10, str2.length(), str2.c_str());

    abort();

    gtfs_sync_write_file(wrt2);

    gtfs_close_file(gtfs, fl);
}

void reader_crash() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test_crash.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer.\n";
    char *data = gtfs_read_file(gtfs, fl, 10, str.length());
    if (data != NULL) {
        str.compare(string(data)) == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << FAIL;
    }
    free(data);
    gtfs_close_file(gtfs, fl);
}

void test_write_read_crash() {
    int pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        writer_crash();
        exit(0);
    }
    waitpid(pid, NULL, 0);
    reader_crash();
}

/* Test 10 */
void writer_multi(int i) {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test_multi.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "Hi, I'm the writer" + to_string(i) + ".\n";
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());
    gtfs_sync_write_file(wrt);

    gtfs_close_file(gtfs, fl);
}

void reader_multi() {
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "test_multi.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    char *data = gtfs_read_file(gtfs, fl, 10, 30);
    printf("@@@reader_multi::%s\n", data);
    free(data);

    gtfs_close_file(gtfs, fl);
}

void test_multi() {
    int N = 5;
    int pids[N];
    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(-1);
        }
        if (pids[i] == 0) {
            writer_multi(i);
            exit(0);
        }
    }
    reader_multi();
    for (int i = 0; i < N; i++) {
        waitpid(pids[i], NULL, 0);
    }
}


/* Additional test 1 */
// JGC Create dir1, gtfs_init there and a non-existent directory.
// Make sure dir1 succeeds and dir2 fails
void test_gtfs_init_clean() {
    string dir1 = TEST_FS_DIR"/dir1"; // exists
    string dir2 = TEST_FS_DIR"/dir2"; // not exists

    int stat1 = mkdir(dir1.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (stat1 == -1) {
        printf("test_gtfs_init_clean::%s\n", strerror(errno));
    }

    gtfs_t *gtfs1 = gtfs_init(dir1, verbose);
    if (gtfs1 == nullptr)
        cout << FAIL << endl;
    else
        cout << "existing dir...\t" << PASS << endl;

    gtfs_t *gtfs2 = gtfs_init(dir2, verbose);
    if (gtfs2 != nullptr)
        cout << FAIL << endl;
    else
        cout << "non-existing dir...\t" << PASS << endl;


    gtfs_t *gtfs3 = gtfs_init(dir1, verbose);
    if (gtfs3 != gtfs1) {
        cout << FAIL << endl;
    }

    // corner case

    string filename = "additional1.txt";
    file_t *fl = gtfs_open_file(gtfs1, filename, 100);

    string str = "Test init clean\n";
    write_t *wrt = gtfs_write_file(gtfs1, fl, 10, str.length(), str.c_str());

    gtfs_sync_write_file(wrt);
    gtfs_close_file(gtfs1, fl);

    cout << "Before GTFS cleanup\n";
    system("ls -l " TEST_FS_DIR"/dir1");
    //cout << "No log exists\n";

    int stat = gtfs_clean(gtfs1);
    if (stat == -1) { // todo: when?
        cout << FAIL;
    }

    cout << "\nAfter GTFS cleanup\n";
    system("ls -l " TEST_FS_DIR"/dir1");
    cout << "if logs are cleaned...\t" << PASS;


    stat = gtfs_remove_file(gtfs1, fl);
    stat1 = rmdir(dir1.c_str());
    if (stat1 == -1) {
        printf("test_gtfs_init_clean::%s\n", strerror(errno));
    }
}


/* Additional test 2 */
void test_gtfs_open_close_remove() {
    string dir1 = TEST_FS_DIR"/dir1"; // exists

    int stat1 = mkdir(dir1.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (stat1 == -1) {
        printf("test_gtfs_open_close_remove::%s\n", strerror(errno));
    }

    gtfs_t *gtfs1 = gtfs_init(dir1, verbose);
    if (gtfs1 == nullptr) {
        cout << FAIL;
    }

    string filename1 = "file1.txt";

    file_t* fl1 = gtfs_open_file(gtfs1, filename1, 100);
    if (fl1 == nullptr) {
        cout << FAIL;
    }

    int stat = gtfs_close_file(gtfs1, fl1);
    if (stat == -1) { // todo: when?
        cout << FAIL;
    }

    system("ls -l " TEST_FS_DIR"/dir1");
    cout << "Length 100\n";

    // JGC - opening with a smaller size should fail
    file_t* fl2 = gtfs_open_file(gtfs1, filename1, 10);
    if (fl2 != nullptr) {
        cout << FAIL;
    }

    // JGC - opening with a larger size should succeed
    fl2 = gtfs_open_file(gtfs1, filename1, 1000);
    if (fl2 == nullptr) {
        cout << FAIL;
    }

    // JGC - I added this check, files should be the same
    if (fl2 != fl1) { cout << FAIL; }

    system("ls -l " TEST_FS_DIR"/dir1");
    cout << "Length 1000\n";


    // JGC - remove should not be allowed on an open file
    stat = gtfs_remove_file(gtfs1, fl2);
    if (stat != -1) {
        cout << FAIL;
    }

    stat = gtfs_close_file(gtfs1, fl2);
    if (stat == -1) { // todo: when?
        cout << FAIL;
    }

    cout << "Before remove\n";
    system("ls -l " TEST_FS_DIR"/dir1");
    cout << "Logs exist\n";

    stat = gtfs_remove_file(gtfs1, fl1);
    if (stat == -1) {
        cout << FAIL;
    }

    // JGC - file1.txt has already been deleted, this should fail!
    stat = gtfs_remove_file(gtfs1, fl2);
    if (stat != -1) {
        cout << FAIL;
    }

    cout << "After remove\n";
    system("ls -l " TEST_FS_DIR"/dir1");
    cout << "Logs no longer exist\n";

    cout << "if no file...\t" << PASS;
    stat1 = rmdir(dir1.c_str());
    if (stat1 == -1) {
        printf("test_gtfs_open_close_remove::%s\n", strerror(errno));
    }
}



/* Additional test 3 */
void test_sync_write_n_bytes() {
    /*
     *  1. create a file
     *  2. write some
     *  3. sync & close
     *  4. open the file
     *  5. write to mem
     *  6. sync only n bytes // abort
     *  7. close & reopen the file
     */

    // 1. 2. 3. open and write and sync
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "testadditional1.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "This write synced in full\n";
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());

    cout << "[original process] log contents (fully sync): " << str;
    gtfs_sync_write_file(wrt);
    gtfs_close_file(gtfs, fl);

    // 4. 5. 6. write again but only sync part of the write to the redo log
    int pid = fork();
    if(pid < 0) {
        cout << "fork failed..." << endl;
        exit(-1);
    }
    if(pid == 0){
        string badstr = "But this write sync failed!\n";
        cout << "\t[forked process] log contents (5 bytes sync): " << badstr;
        gtfs_t *gtfs = gtfs_init(directory, verbose);
        file_t *fl = gtfs_open_file(gtfs, filename, 100);
        write_t *badwrt = gtfs_write_file(gtfs, fl, 10, badstr.length(), badstr.c_str());
        gtfs_sync_write_file_n_bytes(badwrt, 5);
        cout << "\t[forked process] sync 5 bytes and abort" << endl;
        abort();
    }
    waitpid(pid, NULL, 0);


    // 7. re-open file and read
    // This will clean the half-full redo log.
    // Applying the redo log should fail because redo log is incomplete
    gtfs_t *gtfs2 = gtfs_init(directory, verbose);
    file_t *fl2 = gtfs_open_file(gtfs2, filename, 100);
    char *data = gtfs_read_file(gtfs2, fl2, 10, str.length());

    cout << "[original process] re-open and read contents: " << data;
    // since sync "failed" file should contain first write
    if (data != NULL) {
        str.compare(string(data)) == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl2);
}

/* Additional test 4 */
void test_clean_n_bytes() {
    /*
     *  1. create a file
     *  2. write some
     *  3. sync & clean
     *  4. open the file
     *  5. write to mem & sync
     *  6. clean n bytes (commit redo log to file n bytes) & abort
     *  7. access to the file again, and the log must be redone.
     */

    // 1. 2. 3. open, write, sync, and close (clean)
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    string filename = "testadditional2.txt";
    file_t *fl = gtfs_open_file(gtfs, filename, 100);

    string str = "This write cleaned in full\n";
    cout << "[original process] previous contents (fully clean): " << str;
    write_t *wrt = gtfs_write_file(gtfs, fl, 10, str.length(), str.c_str());
    assert(gtfs_sync_write_file(wrt) == 0);
    assert(gtfs_clean(gtfs) == 0);

    // 4. 5. 6. only clean part of redo log

    string secondstr = "12345But this clean failed\n";
    int pid = fork();
    if(pid < 0) {
        cout << "fork failed..." << endl;
        exit(-1);
    }
    if(pid == 0){
        gtfs = gtfs_init(directory, verbose);
        cout << "\t[forked process] correct contents (5 bytes clean): " << secondstr;
        write_t *badwrt = gtfs_write_file(gtfs, fl, 10, secondstr.length(), secondstr.c_str());
        assert(gtfs_sync_write_file(badwrt) == 0);
        assert(gtfs_clean_n_bytes(gtfs, 5) == 0);

        cout << "\t[forked process] write, sync, clean 5 bytes, and abort" << endl;
        abort();
    }
    waitpid(pid, NULL, 0);


    // 7. verify faulty clean
    // since clean "failed" file should contain mixture of both writes
    ssize_t sz;
    char buff [100] = {0};
    auto abs_path = directory + "/" + filename;
    int fd = open(abs_path.c_str(), O_RDWR);
    assert(fd != -1);

    off_t currentPos = lseek(fd, (size_t)10, SEEK_SET);
    sz = read(fd, buff, str.length());
    if (sz != (ssize_t)str.length()) {
        printf("test failure::%s\n", strerror(errno));
        return;
    }
    auto mix = string("12345write cleaned in full\n");
    cout << "[original process] polluted contents: " << buff;
    mix.compare(string(buff)) == 0 ? cout << PASS : cout << FAIL;
    //cout << "the string was: '" << string(buff) + "'\n";

    // *crash*
    // intentionally leak all memory refs above ^

    // re-init fs, re-open file, and read
    // This will re-clean the redo log and re-apply the redo log,
    // overwriting the half written junk.
    gtfs_t *gtfs2 = gtfs_init(directory, verbose);
    file_t *fl2 = gtfs_open_file(gtfs2, filename, 100);
    char *data2 = gtfs_read_file(gtfs2, fl2, 10, secondstr.length());

    cout << "[original process] re-open and read contents: " << data2;
    // since clean has been re-applied, the file should contain the second write
    if (data2 != NULL) {
        secondstr.compare(string(data2)) == 0 ? cout << PASS : cout << FAIL;
    } else {
        cout << FAIL;
    }
    gtfs_close_file(gtfs, fl2);
}

/* Additional test 5 */
void test_overlapping_writes() {
    /*
     *  1. wrtie 1 and write 2
     *  2. write 1 to mem
     *  3. write 2 to mem
     *  4. write 1 abort
     *  5. write 2 sync and close
     *  6. reopen the file (clean)
     *  7. the file contents must be same to the writer 2's work
     */
    string filename = "testadditional3.txt";
    string str1 = "111111111111111111\n";
    string str2 = "22222222222\n";

    // 1, 2, 3, 4
    gtfs_t *gtfs = gtfs_init(directory, verbose);
    file_t *fl = gtfs_open_file(gtfs, filename, 100);
    cout << "W1 writes (about to abort): " << str1;
    cout << "W2 writes (should be preserved): " << str2;
    write_t *wrt1 = gtfs_write_file(gtfs, fl, 10, str1.length(), str1.c_str());
    write_t *wrt2 = gtfs_write_file(gtfs, fl, 10, str2.length(), str2.c_str());
    gtfs_abort_write_file(wrt1);
    cout << "gtfs_abort_write_file(wrt1) before sync wrt2" << endl;

    // 5, 6
    gtfs_sync_write_file(wrt2);
    gtfs_close_file(gtfs, fl);
    gtfs = gtfs_init(directory, verbose);
    fl = gtfs_open_file(gtfs, filename, 100);

    // 7
    char *data = gtfs_read_file(gtfs, fl, 10, str2.length());
    if(strlen(data) == str2.length()){
        cout << "file content after re-opening the file: " << data;
        cout << PASS;
    } else
        cout << FAIL;

    gtfs_close_file(gtfs, fl);
}


int main(int argc, char **argv) {
    if (argc < 2)
      printf("Usage: ./test verbose_flag\n");
    else
      verbose = strtol(argv[1], NULL, 10);

    directory = TEST_FS_DIR;

    // Call existing tests
    cout << "\n\n================== Test 1 ==================\n";
    cout << "Testing that data written by one process is then successfully read by another process.\n";
    test_write_read();

    cout << "directory" << directory << endl;
    cout << "\n\n================== Test 2 ==================\n";
    cout << "Testing that aborting a write returns the file to its original contents.\n";
    test_abort_write();

    cout << "\n\n================== Test 3 ==================\n";
    cout << "Testing that the logs are truncated.\n";
    test_truncate_log();

    cout << "================== Test 4 ==================\n";
    cout << "Testing read directly.\n";
    test_read_directly();

    cout << "================== Test 5 ==================\n";
    cout << "Testing multiple written and read.\n";
    test_multi_write_read();

    cout << "================== Test 6 ==================\n";
    cout << "Testing written at same location.\n";
    test_write_at_same_place();

    cout << "================== Test 7 ==================\n";
    cout << "Testing init_clean" << endl;
    test_gtfs_init_clean();

    cout << "================== Test 8 ==================\n";
    cout << "Testing open close remove" << endl;
    test_gtfs_open_close_remove();

    cout << "================== Test 9 ==================\n";
    cout << "Crash simulation on writing redo log" << endl;
    test_sync_write_n_bytes();

    cout << "================== Test 10 ==================\n";
    cout << "Crash simulation on clean" << endl;
    test_clean_n_bytes();

    cout << "================== Test 11 ==================\n";
    test_overlapping_writes();
	  cout << "=======================================================\n";
}
