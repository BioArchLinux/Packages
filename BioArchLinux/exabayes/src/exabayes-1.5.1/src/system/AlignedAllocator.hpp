#ifndef ALIGNED_ALLOCATOR_HPP
#define ALIGNED_ALLOCATOR_HPP

// modified version from from: 
// http://stackoverflow.com/questions/12942548/making-stdvector-allocate-aligned-memory


#include <stddef.h>
#include "common.h"

#include "VectAlign.hpp"


template <typename T, VectAlign Align = VectAlign::AVX>
class AlignedAllocator;


template <VectAlign Align>
class AlignedAllocator<void, Align>
{
public:
  typedef void*             pointer;
  typedef const void*       const_pointer;
  typedef void              value_type;

  template <class U> struct rebind { typedef AlignedAllocator<U, Align> other; };
};


template <typename T, VectAlign Align>
class AlignedAllocator
{
public:
  typedef T         value_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U>
  struct rebind { typedef AlignedAllocator<U, Align> other; };

public:
  AlignedAllocator() noexcept
  {}

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept
  {}

  size_type
  max_size() const noexcept
  { return (size_type(~0) - size_type(Align)) / sizeof(T); }

  pointer
  address(reference x) const noexcept
  { return std::addressof(x); }

  const_pointer
  address(const_reference x) const noexcept
  { return std::addressof(x); }

  pointer
  allocate(size_type n, typename AlignedAllocator<void, Align>::const_pointer = 0)
  {
    void *ptr = aligned_malloc<T, size_t(Align)>(n);

    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void
  deallocate(pointer p, size_type) noexcept
  { return free(p); }

  template <class U, class ...Args>
  void
  construct(U* p, Args&&... args)
  { ::new(reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...); }

  void
  destroy(pointer p)
  { p->~T(); }
};


template <typename T, VectAlign Align>
class AlignedAllocator<const T, Align>
{
public:
  typedef T         value_type;
  typedef const T*  pointer;
  typedef const T*  const_pointer;
  typedef const T&  reference;
  typedef const T&  const_reference;
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U>
  struct rebind { typedef AlignedAllocator<U, Align> other; };

public:
  AlignedAllocator() noexcept
  {}

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept
  {}

  size_type
  max_size() const noexcept
  { return (size_type(~0) - size_type(Align)) / sizeof(T); }

  const_pointer
  address(const_reference x) const noexcept
  { return std::addressof(x); }

  pointer
  allocate(size_type n, typename AlignedAllocator<void, Align>::const_pointer = 0)
  {
    void *ptr = aligned_malloc<T, size_t(Align)>(); 
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void
  deallocate(pointer p, size_type) noexcept
  {return free(p); }

  template <class U, class ...Args>
  void
  construct(U* p, Args&&... args)
  { ::new(reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...); }

  void
  destroy(pointer p)
  { p->~T(); }
};

template <typename T, VectAlign TAlign, typename U, VectAlign UAlign>
inline
bool
operator== (const AlignedAllocator<T,TAlign>&, const AlignedAllocator<U, UAlign>&) noexcept
{ return TAlign == UAlign; }

template <typename T, VectAlign TAlign, typename U, VectAlign UAlign>
inline
bool
operator!= (const AlignedAllocator<T,TAlign>&, const AlignedAllocator<U, UAlign>&) noexcept
{ return TAlign != UAlign; }


#endif
