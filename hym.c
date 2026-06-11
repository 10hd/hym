#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

// volatile means the value can change unexpectedly. 1 is skip and 0 is play
volatile int skip = 0;

void *input(void *arg) {
  int inp;
  while ((inp = getchar()) != EOF) {
    skip = 1;
  }
  return NULL;
}

void play(int count, char *name[]) {
  srand(time(NULL));

  // fisher-yates shuffle or whatever
  for (int i = count - 1; i > 1; i--) {
    int j = 1 + (rand() % i);

    // * just points at where the text lives
    char *temp = name[i];
    name[i] = name[j];
    name[j] = temp;
  }

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, input, NULL);
  pthread_detach(thread_id);

  for (int i = 1; i < count; i++) {
    drmp3 mp3;
    // &mp3 gives the computer a map coordinate to the mp3 box
    // it means dont make a clone of mp3 instead go to the mp3 and use that
    if (!drmp3_init_file(&mp3, name[i], NULL)) {
      if (errno == 0) {
        errno = EILSEQ;
      }

      printf("Failed to open file '%s': %s\n", name[i], strerror(errno));

      continue;
    }

    char cmd[128];

    snprintf(cmd, sizeof(cmd), "aplay -t raw -f S16_LE -r %d -c %d -q",
             mp3.sampleRate, mp3.channels);

    FILE *aud_o = popen(cmd, "w");
    if (!aud_o) {
      printf("Failed to open audio pipe!\n");
      drmp3_uninit(&mp3);

      return;
    }

    // buffer is a spoon because the computer cant eat all the music at once
    short buffer[2048];
    drmp3_uint64 samples_read;

    // reset the skip variable
    skip = 0;

    // find the last "/" in the name
    char *filename = strrchr(name[i], '/');

    // if filename isnt empty
    if (filename != NULL) {
      filename++; // move 1 chracacter past the "/" to get just the name
    } else {
      filename = name[i]; // if no "/" then the path is already just the name
    }

    printf("Playing: %s\n", filename);

    while (1) {
      if (skip) {
        printf("Skipped\n\n");
        break;
      }

      // one spoon
      samples_read =
          drmp3_read_pcm_frames_s16(&mp3, 2048 / mp3.channels, buffer);
      // if the spoon is empty the song is done/ eaten
      if (samples_read <= 0) {
        break;
      }
      // feed the spoon to the speakers or something
      fwrite(buffer, sizeof(short), samples_read * mp3.channels, aud_o);
    }

    pclose(aud_o);
    drmp3_uninit(&mp3);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage:\nhym songName.mp3\n");

    return 1;
  }

  play(argc, argv);

  printf("Closing...\n");

  _exit(0);
}
