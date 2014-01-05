/* Masstree
 * Eddie Kohler, Yandong Mao, Robert Morris
 * Copyright (c) 2012-2013 President and Fellows of Harvard College
 * Copyright (c) 2012-2013 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Masstree LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Masstree LICENSE file; the license in that file
 * is legally binding.
 */
#ifndef STRINGBAG_HH
#define STRINGBAG_HH 1
#include "compiler.hh"
#include "string_slice.hh"
#include <iostream>
#include <stdint.h>

/** */
template <typename L>
class stringbag {
    typedef L info_type;
    static constexpr int max_halfinfo = (1 << (4 * sizeof(info_type))) - 1;
  //static int id_count; //h

  public:

    stringbag(int width, size_t allocated_size) {
	size_t firstpos = overhead(width);
	assert(allocated_size >= firstpos
	       && allocated_size <= (size_t) max_halfinfo);
	main_ = make_info((int) firstpos, (int) allocated_size);
	memset(info_, 0, sizeof(info_type) * width);
        //id_ = id_count; //h
        //id_count++; //h
    }
  //huanchen
  /*
    stringbag(int width, size_t allocated_size, int id) {
	size_t firstpos = overhead(width);
	assert(allocated_size >= firstpos
	       && allocated_size <= (size_t) max_halfinfo);
	main_ = make_info((int) firstpos, (int) allocated_size);
	memset(info_, 0, sizeof(info_type) * width);
        id_ = id; //h
    }
  */
  //huanchen
  /*
  int id() const {
    return id_;
  }
  */
    static size_t overhead(int width) {
	return sizeof(stringbag<L>) + width * sizeof(info_type);
    }

    int size() const {
	return info_pos(main_);
    }
    int allocated_size() const {
	return info_len(main_);
    }

    lcdf::Str get(int p) const {
	info_type info = info_[p];
	return lcdf::Str(s_ + info_pos(info), info_len(info));
    }

    bool equals_sloppy(int p, const char *s, int len) const {
	info_type info = info_[p];
	if (info_len(info) != len)
	    return false;
	else
	    return string_slice<uintptr_t>::equals_sloppy(s, s_ + info_pos(info), len);
    }
    bool equals_sloppy(int p, lcdf::Str s) const {
	return equals_sloppy(p, s.s, s.len);
    }
    bool equals(int p, const char *s, int len) const {
	info_type info = info_[p];
	return info_len(info) == len
	    && memcmp(s_ + info_pos(info), s, len) == 0;
    }
    bool equals(int p, lcdf::Str s) const {
	return equals(p, s.s, s.len);
    }

    int compare(int p, const char *s, int len) const {
	info_type info = info_[p];
	int minlen = std::min(len, info_len(info));
	int cmp = memcmp(s_ + info_pos(info), s, minlen);
	return cmp ? cmp : ::compare(info_len(info), len);
    }
    int compare(int p, lcdf::Str s) const {
	return compare(p, s.s, s.len);
    }

    bool assign(int p, const char *s, int len) {
	int pos, mylen = info_len(info_[p]);
	if (mylen >= len)
	    pos = info_pos(info_[p]);
	else if (size() + len <= allocated_size()) {
	    pos = size();
	    main_ = make_info(pos + len, allocated_size());
	} else {
          //std::cout << "p = " << p << ", len = " << len << ", size = " << size() << ", alloc =" << allocated_size() << "\n";
          return false;
        }
        //return false;
	memcpy(s_ + pos, s, len);
	info_[p] = make_info(pos, len);
        //std::cout << id_ << " " << allocated_size() << "\n"; //h
	return true;
    }

    bool assign(int p, lcdf::Str s) {
	return assign(p, s.s, s.len);
    }

  //huanchen
  
  void compact(int suf[], int width) {
    int sp = overhead(width);
    int position[width];
    int len[width];
    int perm[width];
    int perm_value[width];
    int i, j, k;
    int p, pos, l;
    int perm_end = 0;
    for (i = 0; i < width; i++) {
      position[i] = 0;
      len[i] = 0;
      perm[i] = 0;
      perm_value[i] = 0;
    }
    //std::cout << "before stringbag\n";
    for (i = 0; i < width; i++) {
      if (suf[i]) {
        position[i] = info_pos(info_[i]);
        len[i] = info_len(info_[i]);
      }
      //std::cout << "pos = " << position[i] << ", len = " << len[i] << "\n";
    }
    for (i = 0; i < width; i++) {
      if (position[i] != 0) {
        j = 0;
        if (perm_end == 0) {
          perm_value[0] = position[i];
          perm[0] = i;
        }
        while (j < perm_end) {
          if (position[i] < perm_value[j]) {
            for (k = perm_end; k > j; k--) {
              perm_value[k] = perm_value[k-1];
              perm[k] = perm[k-1];
            }
            perm_value[j] = position[i];
            perm[j] = i;
            //std::cout << "i = " << i << "\n";
            j = perm_end;
          }
          j++;
        }
        if (j == perm_end) {
          perm_value[perm_end] = position[i];
          perm[perm_end] = i;
        }
        perm_end++;
      }
    }
    //std::cout << "stringbag\n";
    for (i = 0; i < perm_end; i++) {
      p = perm[i];
      pos = position[p];
      l = len[p];
      memcpy(s_ + sp, s_ + pos, l);
      info_[p] = make_info(sp, l);
      sp += l;
      //std::cout << "p = " << p << ", l = " << l << "\n";
    }

    for (j = 0; j < width; j++) {
      if (suf[j] == 0)
        info_[j] = make_info(0, 0);
    }

    main_ = make_info(sp, allocated_size());
  }
  

  void shrink () {
    int size = info_pos(main_);
    int alloc_size = info_len(main_);
    while (size * 2 < alloc_size) {
      alloc_size = alloc_size / 2;
    }
    //std::cout << "size = " << size << ", info_len = "<< info_len(main_) <<", alloc_size = " << alloc_size << "\n";
    main_ = make_info(size, alloc_size);
  }

    void print(int width, FILE *f, const char *prefix, int indent) {
	fprintf(f, "%s%*s%p (%d:)%d:%d [%d]...\n", prefix, indent, "",
		this, (int) overhead(width), size(), allocated_size(), max_halfinfo + 1);
	for (int i = 0; i < width; ++i)
	    if (info_len(info_[i]))
		fprintf(f, "%s%*s  #%x %d:%d %.*s\n", prefix, indent, "",
			i, info_pos(info_[i]), info_len(info_[i]), std::min(info_len(info_[i]), 40), s_ + info_pos(info_[i]));
    }

  private:
  //uint32_t id_; //h
    union {
	struct {
	    info_type main_;
	    info_type info_[0];
	};
	char s_[0];
    };

    static info_type make_info(int pos, int len) {
	return (info_type(pos) << (4 * sizeof(info_type))) | len;
    }
    static int info_pos(info_type info) {
	return info >> (4 * sizeof(info));
    }
    static int info_len(info_type info) {
	return info & ((info_type(1) << (4 * sizeof(info))) - 1);
    }

};

//huanchen
/*
template <typename L>
int stringbag<L>::id_count = 0;
*/


#endif
