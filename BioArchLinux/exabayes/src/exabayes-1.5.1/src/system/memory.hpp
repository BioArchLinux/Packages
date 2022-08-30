#ifndef _MEMORY_HPP
#define _MEMORY_HPP

#ifdef _DEVEL

#include <stdio.h>
#include <proc/readproc.h>

uint64_t getCurrentMemory()
{
  struct proc_t usage;
  look_up_our_self(&usage);
  return usage.vsize; 
}


#endif
#endif
