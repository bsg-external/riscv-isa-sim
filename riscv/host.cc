#include <thread>
#include "devices.h"
#include "processor.h"

host_t::host_t(std::vector<processor_t*>& procs)
  : procs(procs), getchar_queue(), finish_signals(procs.size(), false)
{
  std::thread t(&host_t::monitor, this);
  t.detach();
}

#define PUTCHAR_BASE	0x0
#define GETCHAR_BASE	0x1000
#define FINISH_BASE     0x2000

void host_t::monitor()
{
  int c = -1;
  while(1) {
      c = getchar();
      getchar_queue.push(c);
  }
}

bool host_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  int c = -1;
  if(addr == GETCHAR_BASE && !getchar_queue.empty()) {
    c = getchar_queue.front();
    getchar_queue.pop();
  }
  memcpy(bytes, &c, (size_t)len);
  return true;
}

bool host_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  if(addr == PUTCHAR_BASE) {
    char c = bytes[0];
    printf("%c", c);
    fflush(stdout);
  }
  else if(addr >= FINISH_BASE) {
    int hartid = (addr - FINISH_BASE) >> 3;
    if(hartid >= procs.size())
      return false;
    finish_signals[hartid] = true;
    printf("[CORE%d FSH]: %x\n", hartid, bytes[0]);
    
    bool done = true;
    for(int i=0; i<procs.size(); i++) {
      if(finish_signals[i] == false) {
        done = false;
        break;
      }
    }
    if(done) {
      exit(0);
    }
  }
  else {
    return false;
  }
  return true;
}
