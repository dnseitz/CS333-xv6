// Skimmed down per-process state
struct uproc {
  int pid;
  uint uid;
  uint gid;
  int ppid;
  char state[9];
  uint sz;
  char name[16];
};
