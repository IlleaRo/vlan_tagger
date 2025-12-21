#ifndef pthread_init_h
#define pthread_init_h

#include "vlan_tagger.h"

#define MAX_NUM_OF_RUL 64

void pthread_init(const char *);

void logging_programm_completion(struct thread_data*);

#endif //pthread_init_h
