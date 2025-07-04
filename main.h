#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#define LOG_RESOURCES 1
extern int resource_count;


#if LOG_RESOURCES
#   define LOGR(msg, inc) {\
      resource_count += inc;\
      printf("%s | TOTAL RESOURCES: %d\n",msg,resource_count);\
    }
#else
#   define LOGR(msg, inc) ;
#endif

/* INCLUDED_MAIN_H */
#endif
