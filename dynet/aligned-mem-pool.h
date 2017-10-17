#ifndef DYNET_ALIGNED_MEM_POOL_H
#define DYNET_ALIGNED_MEM_POOL_H

#include <iostream>
#include "dynet/mem.h"
#include "dynet/globals.h"
#include "dynet/except.h"

namespace dynet {

class BaseMemoryPool {
 public:
  BaseMemoryPool(const std::string & name, MemAllocator* a) : name(name), a(a) {}
  virtual ~BaseMemoryPool() {}
  virtual void* allocate(size_t n) = 0; 

  virtual void free() = 0;
  // zeros out the amount of allocations
  virtual void zero_allocated_memory() = 0;

  size_t used;
  MemAllocator* a;
 protected:
  virtual void sys_alloc(size_t cap) {}
  virtual void zero_all() {}

  std::string name;
  size_t capacity;
  void* mem;
};

class DynamicCPUMemoryPool : public BaseMemoryPool {
 private:
  std::vector<void*> ptrs;
  std::vector<size_t> sizes;

 public:
  explicit DynamicCPUMemoryPool(const std::string & name, size_t cap, MemAllocator* a) : BaseMemoryPool(name, a) {
    sys_alloc(cap);
    zero_all();
    this->a = new CPUAllocator();
  }

  ~DynamicCPUMemoryPool() {
      free();
      delete a;
  }

  void* allocate(size_t n); 
  void zero(void* p, size_t n); 

  void free() {
    for (auto p : ptrs)
      a->free(p);
    ptrs.clear();
    sizes.clear();
  }
  // zeros out the amount of allocations
  void zero_allocated_memory() {
    for (unsigned i = 0; i < ptrs.size(); i++)
      zero(ptrs[i], sizes[i]);
  }

 private:
  void sys_alloc(size_t cap);
  void zero_all() {}
};

class InternalMemoryPool : public BaseMemoryPool {
 public:
  explicit InternalMemoryPool(const std::string & name, size_t cap, MemAllocator* a) : BaseMemoryPool(name, a) {
    sys_alloc(cap);
    zero_all();
  }

  ~InternalMemoryPool() {
      a->free(mem);
  }

  void* allocate(size_t n); 

  void free() {
    //std::cerr << "freeing " << used << " bytes\n";
    used = 0;
  }
  // zeros out the amount of allocations
  void zero_allocated_memory() {
    if (used == 0) return;
    a->zero(mem, used);
  }

  size_t used;
 private:
  void sys_alloc(size_t cap);

  void zero_all() {
    a->zero(mem, capacity);
  }
};

class AlignedMemoryPool {
  public:
    explicit AlignedMemoryPool(const std::string &name, size_t initial_cap, MemAllocator *a, size_t expanding_unit = 1<<24, bool dynamic = false);
    ~AlignedMemoryPool();

    void* allocate(size_t n);

    void free();

    void zero_allocated_memory();

    size_t used();
    void set_used(size_t s);

    std::vector<BaseMemoryPool *> pools;
    MemAllocator* a;

    bool is_dynamic() { return dynamic; }

  private:
    std::string name;
    size_t cap;
    int current;
    size_t expanding_unit;
    bool dynamic;
};

} // namespace dynet

#endif
