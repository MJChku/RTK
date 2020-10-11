

#include <nautilus/fs.h>
#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/syscall_user.h>

#define ERROR(fmt, args...) ERROR_PRINT("sycall_test: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("syscall_test: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("syscall_test: " fmt, ##args)

#define EXPECT(x)                  \
  if (!(x)) {                      \
    ERROR("Expectation failed\n"); \
    total_tests++;                 \
  } else {                         \
    total_tests++;                 \
    passed_tests++;                \
  }

static int passed_tests;
static int total_tests;

static int handle_syscall_tests(char* buf, void* priv) {
  INFO("Shell command for testing file system system calls\n");
  INFO("%s\n", buf);

  /// Test setup
  passed_tests = 0;
  total_tests = 0;

  // Test read / wrtie on fd [0,2]
  {
    char msg_stdout[] = "This is a print to 'stdout'\n";
    char msg_stderr[] = "This is a print to 'stderr'\n";
    EXPECT(write(1, msg_stdout, sizeof(msg_stdout)) == sizeof(msg_stdout));
    EXPECT(write(2, msg_stderr, sizeof(msg_stderr)) == sizeof(msg_stderr));

    char read_buffer[8] = {0};
    printk("Please enter 7 chars to test read from stdin: ");
    EXPECT(read(0, read_buffer, 7) == 7);
    printk("\nYour input: ");
    EXPECT(write(1, read_buffer, strlen(read_buffer)) == strlen(read_buffer));
    printk("\n");
  }

  // Test open / read / write / close on files
  {
    // TODO: Don't print ERROR here once debug print works
    int fn = open("fs:/new_file2", 3 | 8); // RW/Create
    EXPECT(fn != -1);
    char wr_buf[] = "this used to not exist";

    int wr_bytes = write(fn, wr_buf, sizeof(wr_buf));

    EXPECT(wr_bytes == sizeof(wr_buf));

    char read_buf[32] = {0};
    EXPECT(read(fn, read_buf, 32) == sizeof(wr_buf));

    EXPECT(strcmp(read_buf, wr_buf) == 0);

    /* // This will crash if the fs is not loaded; TODO: enable
    struct nk_fs_stat statbuf;
    memset(&statbuf, 0, sizeof(struct nk_fs_stat));
    ERROR("Stat result: %d\n", fstat(fn, &statbuf));
    ERROR("Stat size: %d\n", statbuf.st_size);

    EXPECT(close(fn) == 0);
    */
  }

  // Test fork / exit
  {
    // TODO: Don't print ERROR here once debug print works
    int f_val = fork();
    int tid = getpid();
    DEBUG("Return value of fork()/getpid(): %d/%d\n", f_val, tid);
    if (f_val == 0) {
      DEBUG("In forked thread\n");
      exit(0);
    }
  }

  /// get/set timeofday; nanosleep
  {
    /// TODO: include from wherever this is defined correctly
    struct timeval {
      int tv_sec;  /* seconds */
      int tv_usec; /* microseconds */
    };
    struct timeval time;

    EXPECT(gettimeofday(&time, NULL) == 0);
    printk("Initial time of day in s: %d\n", time.tv_sec);

    const int time_to_set = 100000;
    time.tv_sec = time_to_set;
    EXPECT(settimeofday(&time, NULL) == 0);

    time.tv_sec = 0; // just to be sure gettimeofday updates it
    EXPECT(gettimeofday(&time, NULL) == 0);
    EXPECT(time.tv_sec >= time_to_set);
    printk("Modified time of day in s: %d\nSleeping...\n", time.tv_sec);

    /// TODO: include from wherever this is defined correctly
    struct timespec {
      uint64_t tv_sec;  /* seconds */
      uint64_t tv_nsec; /* nanoseconds */
    };

    struct timespec sleep_time;
    sleep_time.tv_sec = 10;
    sleep_time.tv_nsec = 0;

    nanosleep(&sleep_time, NULL);
    gettimeofday(&time, NULL);
    printk("Time of day after sleeping: %d\n", time.tv_sec);
  }

  /// Test teardown
  printk("Passed %d of %d tests.\n", passed_tests, total_tests);

  return 0;
}

static struct shell_cmd_impl syscalltest_impl = {

    .cmd = "syscalltest",

    .help_str = "syscalltest (run system call tests)",

    .handler = handle_syscall_tests,

};

nk_register_shell_cmd(syscalltest_impl);
