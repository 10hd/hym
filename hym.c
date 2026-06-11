#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

// volatile means the value can change unexpectedly. 1 is skip and 0 is play
volatile int skip = 0;
volatile float volume = 1.0f;

void *input(void *arg) {
  int inp;       // variable for input
  int typed = 0; // variable for if you type something before hitting enter to
                 // prevent skipping song when changing volume
  while ((inp = getchar()) != EOF) {
    if (inp == '+') {
      // increase volume by 0.1 or 10%
      volume += 0.1f;
      if (volume > 3.0f) {
        volume = 3.0f; // cap volume at 3f or 300%
      }
      // print the volume and turn it into an int. also add 0.1f to round up
      // instead of down
      printf("Volume: %i\n", (int)(volume * 100 + 0.1f));
      typed = 1;
    } else if (inp == '-') {
      volume -= 0.1f; // decrease by 0.1 or 10%
      if (volume < 0.0f) {
        volume = 0.0f; // no negative volume
      }
      // same as before
      printf("Volume: %i\n", (int)(volume * 100 + 0.1f));
      typed = 1;
    } else if (inp == '\n' && typed == 0) {
      skip = 1; // skip if enter is pressed and typed = 0
    } else {
      typed = 0; // otherwise set typed to 0 (reset typed basically)
    }
  }
  return NULL;
}

void play(int count, char *name[], int shuffle) {
  srand(time(NULL));

  int start;
  if (shuffle == 0) {
    start = 1;
  } else {
    start = 2;
  }
  // test if shuffle int works
  printf("%i\n", shuffle);

  if (shuffle == 1) {
    // I HATE ALGORITHMS
    // fisher-yates shuffle or whatever
    for (int i = count - 1; i > start; i--) {
      int j = start + (rand() % (i - start + 1));

      // * just points at where the text lives
      char *temp = name[i];
      name[i] = name[j];
      name[j] = temp;
    }
  } else if (shuffle == 2) {
    // naive shuffle algorithm
    for (int i = start; i < count; i++) {
      int j = start + (rand() % (count - start));

      char *temp = name[i];
      name[i] = name[j];
      name[j] = temp;
    }
  } else if (shuffle == 3) {
    // I looked it up and it's very complicated and I will probably not
    // implement it for a while.
    printf("I don't understand the gilber-shannon-reeds model.\nNo shuffle.\n");
  } else if (shuffle == 4) {
    // fisher yates multiple times
    int total_rounds = 5 + (rand() % 100);
    for (int round = 0; round < total_rounds; round++) {
      for (int i = count - 1; i > start; i--) {
        int j = start + (rand() % (i - start + 1));

        char *temp = name[i];
        name[i] = name[j];
        name[j] = temp;
      }
    }
  } else {
    printf("No shuffle.\n");
  }

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, input, NULL);
  pthread_detach(thread_id);

  for (int i = start; i < count; i++) {
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

      int total_samples = samples_read * mp3.channels;

      for (int s = 0; s < total_samples; s++) {
        buffer[s] = (short)(buffer[s] * volume);
      }

      // feed the spoon to the speakers or something
      fwrite(buffer, sizeof(short), samples_read * mp3.channels, aud_o);
    }

    pclose(aud_o);
    drmp3_uninit(&mp3);
  }
}

int main(int argc, char *argv[]) {
  int shuffle = 0;

  if (argc < 2) {
    printf("Usage:\nhym songName.mp3\n");

    return 1;
  }

  if (!strcmp(argv[1], "-fy")) {
    shuffle = 1;
  } else if (!strcmp(argv[1], "-n")) {
    shuffle = 2;
  } else if (!strcmp(argv[1], "-gsr")) {
    shuffle = 3;
  } else if (!strcmp(argv[1], "-random")) {
    shuffle = 4;
  } else {
    shuffle = 0;
  }

  play(argc, argv, shuffle);

  printf("Closing...\n");

  _exit(0);
}
