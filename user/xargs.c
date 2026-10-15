
#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
}

static int
parse_n(char *s, int *value)
{
  int n = 0;
  int i;

  if (s[0] == '\0')
    return -1;

  for (i = 0; s[i] != '\0'; i++) {
    int digit;

    if (s[i] < '0' || s[i] > '9')
      return -1;

    digit = s[i] - '0';

    // Saturate at MAXARG + 1 to avoid integer overflow.
    if (n < MAXARG + 1) {
      n = n * 10 + digit;
      if (n > MAXARG + 1)
        n = MAXARG + 1;
    }
  }

  if (n < 1)
    return -1;

  *value = n;
  return 0;
}

static int
run_command(char **cmd_argv, int base_count,
            char **words, int nwords)
{
  char *args[MAXARG + 1];
  int i;
  int pid;
  int status = 0;

  if (nwords == 0)
    return 0;

  if (base_count + nwords > MAXARG)
    return -1;

  // Preserve the command and its original arguments.
  for (i = 0; i < base_count; i++)
    args[i] = cmd_argv[i];

  // Append the words read from standard input.
  for (i = 0; i < nwords; i++)
    args[base_count + i] = words[i];

  args[base_count + nwords] = 0;

  pid = fork();

  if (pid < 0) {
    fprintf(2, "xargs: fork failed\n");
    return -1;
  }

  if (pid == 0) {
    exec(cmd_argv[0], args);
    fprintf(2, "xargs: exec %s failed\n", cmd_argv[0]);
    exit(1);
  }

  // Wait for this command before starting the next one.
  if (wait(&status) < 0) {
    fprintf(2, "xargs: wait failed\n");
    return -1;
  }

  if (status != 0)
    return -1;

  return 0;
}

static void
run_pending(char **cmd_argv, int base_count,
            char **words, int *nwords, int *failed)
{
  int i;

  if (*nwords == 0)
    return;

  if (run_command(cmd_argv, base_count, words, *nwords) < 0)
    *failed = 1;

  // The parent no longer needs these argument strings.
  for (i = 0; i < *nwords; i++)
    free(words[i]);

  *nwords = 0;
}

static int
add_word(char *buffer, int length, char **words,
         int *nwords, int room)
{
  char *word;

  if (room <= 0 || *nwords >= room) {
    fprintf(2, "xargs: too many arguments\n");
    return -1;
  }

  word = malloc(length + 1);

  if (word == 0) {
    fprintf(2, "xargs: out of memory\n");
    return -1;
  }

  memmove(word, buffer, length);
  word[length] = '\0';

  words[*nwords] = word;
  (*nwords)++;

  return 0;
}

int
main(int argc, char *argv[])
{
  int n_mode = 0;
  int requested_n = 0;
  int command_index = 1;
  int base_count;
  int room;
  int batch_limit = 0;
  int nwords = 0;
  int failed = 0;
  int read_result;
  int word_length = 0;
  int word_capacity = 64;
  int i;
  char ch;
  char *word_buffer;
  char *cmd_argv[MAXARG + 1];
  char *words[MAXARG];

  if (argc < 2) {
    usage();
    exit(1);
  }

  // Parse the optional -n N arguments.
  if (strcmp(argv[1], "-n") == 0) {
    if (argc < 4 || parse_n(argv[2], &requested_n) < 0) {
      usage();
      exit(1);
    }

    n_mode = 1;
    command_index = 3;
  }

  base_count = argc - command_index;

  if (base_count < 1 || base_count > MAXARG) {
    usage();
    exit(1);
  }

  for (i = 0; i < base_count; i++)
    cmd_argv[i] = argv[command_index + i];

  cmd_argv[base_count] = 0;

  // Leave enough argument slots for the input words.
  room = MAXARG - base_count;

  if (n_mode) {
    batch_limit = requested_n;

    if (batch_limit > room)
      batch_limit = room;
  }

  word_buffer = malloc(word_capacity);

  if (word_buffer == 0) {
    fprintf(2, "xargs: out of memory\n");
    exit(1);
  }

  // Read standard input one character at a time.
  while ((read_result = read(0, &ch, 1)) == 1) {
    if (ch == ' ' || ch == '\t' || ch == '\n') {
      // Finish a word when a separator is encountered.
      if (word_length > 0) {
        if (add_word(word_buffer, word_length, words,
                     &nwords, room) < 0)
          exit(1);

        word_length = 0;

        // In -n mode, run whenever the batch is full.
        if (n_mode && nwords >= batch_limit)
          run_pending(cmd_argv, base_count, words,
                      &nwords, &failed);
      }

      // Without -n, each non-empty line is one batch.
      if (!n_mode && ch == '\n')
        run_pending(cmd_argv, base_count, words,
                    &nwords, &failed);
    } else {
      // Grow the current word buffer when necessary.
      if (word_length + 1 >= word_capacity) {
        int new_capacity = word_capacity * 2;
        char *new_buffer = malloc(new_capacity);

        if (new_buffer == 0) {
          fprintf(2, "xargs: out of memory\n");
          exit(1);
        }

        memmove(new_buffer, word_buffer, word_length);
        free(word_buffer);

        word_buffer = new_buffer;
        word_capacity = new_capacity;
      }

      word_buffer[word_length++] = ch;
    }
  }

  if (read_result < 0) {
    fprintf(2, "xargs: read failed\n");
    exit(1);
  }

  // Process the last word if EOF did not end with a separator.
  if (word_length > 0) {
    if (add_word(word_buffer, word_length, words,
                 &nwords, room) < 0)
      exit(1);

    if (n_mode && nwords >= batch_limit)
      run_pending(cmd_argv, base_count, words,
                  &nwords, &failed);
  }

  // Process any remaining words at EOF.
  run_pending(cmd_argv, base_count, words, &nwords, &failed);

  free(word_buffer);

  exit(failed ? 1 : 0);
}
