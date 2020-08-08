#ifndef consumerapp_h
#define consumerapp_h

struct ConsumerApp {
   int readFd;
};

int consumerAppRun(ConsumerApp& app);

#endif
