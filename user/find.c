#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

// One shared path buffer for the whole walk. xv6 user stacks are a single
// 4 KB page, so a 512-byte buffer per recursion level would overflow on
// deep trees.
static char path[512];
static char *pattern;
static int want;          // 0 = both, 'f' = files/devices, 'd' = directories

// Does pattern p match the whole string s? '*' matches any sequence
// (including empty). Everything else must match exactly.
int
match(char *p, char *s)
{
  if(*p == 0)
    return *s == 0;
  if(*p == '*'){
    for(;; s++){
      if(match(p + 1, s))
        return 1;
      if(*s == 0)
        return 0;
    }
  }
  if(*s == 0 || *p != *s)
    return 0;
  return match(p + 1, s + 1);
}

// path[] holds the directory to scan, NUL-terminated at 'end'.
void
walk(char *end)
{
  struct stat st, cst;
  struct dirent *de, *buf;
  char name[DIRSIZ + 1];
  char *p;
  int fd, n, i;

  if((fd = open(path, 0)) < 0)
    return;
  if(fstat(fd, &st) < 0 || st.type != T_DIR || st.size == 0){
    close(fd);
    return;
  }

  // Read the whole directory, then close it BEFORE recursing, so deep
  // trees don't run out of file descriptors (NOFILE is 16).
  buf = malloc(st.size);
  if(buf == 0){
    close(fd);
    return;
  }
  n = read(fd, buf, st.size) / sizeof(struct dirent);
  close(fd);

  for(i = 0; i < n; i++){
    de = &buf[i];
    if(de->inum == 0)
      continue;
    memmove(name, de->name, DIRSIZ);
    name[DIRSIZ] = 0;
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

p = end;
int need_slash = (p > path && p[-1] != '/');
int namelen = strlen(name);

if((p - path) + need_slash + namelen + 1 > sizeof(path))
  continue;

if(need_slash)
  *p++ = '/';

strcpy(p, name);



    if(stat(path, &cst) < 0)
      continue;

int is_dir = (cst.type == T_DIR);
int type_ok = (want == 0) ||
              (want == 'd' && cst.type == T_DIR) ||
              (want == 'f' &&
               (cst.type == T_FILE || cst.type == T_DEVICE));

    if(type_ok && match(pattern, name))
      printf("%s\n", path);

    if(is_dir)
      walk(p + strlen(name));
  }
  *end = 0;                           // restore the directory path
  free(buf);
}

int
main(int argc, char *argv[])
{
  int fd;
  struct stat st;

  if(argc == 3){
    want = 0;
  } else if(argc == 5 && strcmp(argv[3], "-type") == 0 &&
            (strcmp(argv[4], "f") == 0 || strcmp(argv[4], "d") == 0)){
    want = argv[4][0];
  } else {
    fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
    exit(1);
  }
  pattern = argv[2];

  if(strlen(argv[1]) >= sizeof(path) - DIRSIZ - 2 ||
     (fd = open(argv[1], 0)) < 0){
    fprintf(2, "find: cannot open %s\n", argv[1]);
    exit(1);
  }
  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot open %s\n", argv[1]);
    close(fd);
    exit(1);
  }
  close(fd);

  strcpy(path, argv[1]);
  walk(path + strlen(path));        // <dir> itself is never printed
  exit(0);
}