#ifndef PAGE_H
#define PAGE_H

// Page.h
// header file for page class

#include "FlashConfiguration.h"
#include <stdint.h> 

namespace NVDSim{
  class Page{
    
  public:
    Page();
    Page(uint page);

    void* read(uint word_num);
    void* read();

    void write(uint word_num);
    void write();

  private:
    uint page_num;
    void* page_data;
    std::unordered_map<uint, void*> word_data;
    
  };
}

#endif
