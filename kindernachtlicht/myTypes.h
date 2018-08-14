#ifndef myTypes_h
#define myTypes_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  int valid;                        // 0=no configuration, 1=valid configuration
  int sleep_h;                      
  int sleep_m;
  int wake_h;
  int wake_m;
  int before_wake;
} configData_t;

#endif
