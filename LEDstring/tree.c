#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#define STRAND_LEN 160 // Number of LEDs on strand
#define MAX 10
#define DELAY 1000000

static FILE *rgb_fp;
// static FILE *ain_fp;
int running=1;

void clear() {
  int i;
  for (i=0; i<STRAND_LEN; i++) {
    fprintf(rgb_fp, "0 0 0 %d", i);
  }
}

void display() {
  fprintf(rgb_fp, "0 0 0 -1\n");
}

void rgb(int red, int green, int blue, int index, int us) {
  fprintf(rgb_fp, "%d %d %d %d", red, green, blue, index);
  if(us) {
    display();
//    printf("sending %d %d %d %d for %d\n", red, green, blue, index, us);
  }
  usleep(us);
}

// pattern4 matches the static LEDs on the tree.
void pattern4(int skip) {
  int i;
  int period=5;
  int phase;
  for(phase=0; phase<skip*period; phase++) {
    clear();
    for(i=phase-skip*period; i<STRAND_LEN; i+=skip*period) {
      rgb(MAX,   0,   0, i+0*skip, 0);
      rgb(  0, MAX,   0, i+1*skip, 0);  
      rgb(  0,   0, MAX, i+2*skip, 0);  
      rgb(MAX, MAX/4, 0, i+3*skip, 0);  
      rgb(MAX, MAX,   0, i+4*skip, 0); 
    }
  display();
  usleep(DELAY);
  }
}

// Pattern 5 is a single LED running smoothly along.running smoothly along.
void pattern5(int timeUp, int timeBack) {
  int i, j;
  int smooth=10;

  for(i=0; i<STRAND_LEN-1; i++) {
    for(j=1; j<=smooth; j++) {
      rgb(0, (smooth-j), 0, i,   0);
      rgb(0,        (j), 0, i+1, timeUp/(smooth*1.5));
    }
  }
  for(i=STRAND_LEN-1; i>=0; i--) {
    for(j=1; j<=smooth; j++) {
      rgb(0,  0, (smooth-j), i+1, 0);
      rgb(0,  0,        (j), i  , timeBack/(smooth*1.5));
    }
  }
}

// Pattern 3 is a single LED running backward and forward.
void pattern3(int timeUp, int timeBack) {
  int i;

  // Climbing up
  for(i=0; i<STRAND_LEN-1; i++) {
    rgb(   0, 0, 0, i,   0);
    rgb((i*20/STRAND_LEN)+1, 0, 0, i+1, timeUp);
  }
  // Sledding down
  for(i=STRAND_LEN-1; i>=0; i--) {
    rgb(0,  0,  0, i+1, 0);
    rgb(0,  0, (STRAND_LEN-i+4)*20/STRAND_LEN, i  , timeBack);
  }
}

// Pattern 6 is a single LED falling and bouncing
void pattern6(int timeUp, int timeBack) {
  int i, top;
#ifdef HACK
  for(top=STRAND_LEN; top>0; top-=10) {
    // Falling down
    for(i=top; i>=0; i--) {
      rgb(0,  0, 0, i+1, 0);
      rgb(0, 15, 0, i  , timeBack-(pow(top-i,2)/pow(top,2)*timeBack*19/20));
    }
    // Bouncing up
    for(i=0; i<top-10; i++) {
      rgb(0,  0, 0, i,   0);
      rgb(0, 15, 0, i+1, timeUp-(pow(top-i,2)/pow(top,2)*timeUp*19/20));
    }
  }
  #endif
}

// Pattern 7 reads the analog in and positions the LED.
void pattern7(int timeUp, int timeBack) {
#ifdef HACK
  int value, oldIndex;
  static int index = 0;
  oldIndex = index;
  fseek(ain_fp, 0L,  SEEK_SET);
  fscanf(ain_fp, "%d", &value);
  index = (value-4072)*159/(1416-4072);
  if(index != oldIndex) {
//    printf("ain: %d, %d, %d (old)\n", value, index, oldIndex);
    rgb(0, 0, 0, oldIndex, 0);
    rgb(10, 10, 10, index, 10000);
  }
#endif
}

// Pattern 0 outputs all the same color
void pattern0(int r, int g, int b) {
  int i;
  for (i=0; i<STRAND_LEN; i++) {
    rgb(r, g, b, i, 0);
  }
  display();
}

// Pattern 1 outputs a string of increasing brightness
void pattern1() {
  int i;
  for (i=0; i<STRAND_LEN; i++) {
    rgb(0, i%127, 0, i, 20000);
  }
}

void pattern2() {
  int i, j;
  unsigned char g, r, b;
  srand(time(NULL));
  
  g = rand() % 0x7F;
  r = rand() % 0x7F;
  b = rand() % 0x7F;
 
  for (j = 0; j < STRAND_LEN * 3; j += 3) {
    rgb(r, b, g, j/3, 0);
  }
  display();
  usleep(200000);
}

//signal handler that breaks program loop and cleans up
void signal_handler(int signo){
  if (signo == SIGINT) {
    printf("\n^C pressed, cleaning up and exiting..\n");
    running=0;
    // fflush(stdout);
    // fclose(rgb_fp);
    // exit(0);
  }
}

int main(int argc, char *argv[]) { 
  rgb_fp = fopen("/sys/firmware/lpd8806/device/rgb", "w");
//  ain_fp = fopen("/sys/devices/platform/omap/tsc/ain6", "r");
  setbuf(rgb_fp, NULL);
//  if (rgb_fp == NULL || ain_fp == NULL) {
  if (rgb_fp == NULL) {
    printf("Failed to open device\n");
    return 1;
  }
 
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    printf("Error with SIGINT handler\n");
    return 1;
  }
  
  int pattern=3;
  int arg=4;
  int arg2 = 10000;
  if(argc > 1) {
    pattern=atoi(argv[1]);
  }
  if(argc > 2) {
    arg=atoi(argv[2]);
    arg2 = arg;
  }
  if(argc > 3) {
    arg2=atoi(argv[3]);
  }
  printf("Running pattern%d(%d)\n", pattern, arg);

  clear();
  while (running) {
    switch(pattern) {
      case 0:
	pattern0(0, 0, 0);
	break;
      case 1:
	pattern1();
	break;
      case 2:
	pattern2();
	break;
      case 3:
        pattern3(arg, arg2);
	break;
      case 4:
	pattern4(arg);
	break;
      case 5:
	pattern5(arg, arg2);
	break;
      case 6:
	pattern6(arg, arg2);
	break;
      case 7:
	pattern7(arg, arg2);
	break;
    }
  }

//  fflush(stdout);
  fclose(rgb_fp);
//  fclose(ain_fp);
}
