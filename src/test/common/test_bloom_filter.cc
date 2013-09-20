// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2013 Inktank
 *
 * LGPL2
 */

#include <iostream>
#include <gtest/gtest.h>

#include "include/stringify.h"
#include "common/bloom_filter.hpp"

TEST(BloomFilter, Basic) {
  bloom_filter bf(10, .1, 1);
  bf.insert("foo");
  bf.insert("bar");

  ASSERT_TRUE(bf.contains("foo"));
  ASSERT_TRUE(bf.contains("bar"));
}

TEST(BloomFilter, Sweep) {
  std::cout << "# max\tfpp\tactual\tsize\tB/insert" << std::endl;
  for (int ex = 3; ex < 12; ex++) {
    for (float fpp = .001; fpp < .5; fpp *= 2.0) {
      int max = 2 << ex;
      bloom_filter bf(max, fpp, 1);
      bf.insert("foo");
      bf.insert("bar");

      ASSERT_TRUE(bf.contains("foo"));
      ASSERT_TRUE(bf.contains("bar"));

      for (int n = 0; n < max; n++)
	bf.insert("ok" + stringify(n));

      int test = max * 100;
      int hit = 0;
      for (int n = 0; n < test; n++)
	if (bf.contains("asdf" + stringify(n)))
	  hit++;

      ASSERT_TRUE(bf.contains("foo"));
      ASSERT_TRUE(bf.contains("bar"));

      double actual = (double)hit / (double)test;

      bufferlist bl;
      ::encode(bf, bl);

      double byte_per_insert = (double)bl.length() / (double)max;

      std::cout << max << "\t" << fpp << "\t" << actual << "\t" << bl.length() << "\t" << byte_per_insert << std::endl;
      ASSERT_TRUE(actual < fpp * 10);

    }
  }
}

TEST(BloomFilter, SweepInt) {
  std::cout << "# max\tfpp\tactual\tsize\tB/insert" << std::endl;
  for (int ex = 3; ex < 12; ex++) {
    for (float fpp = .001; fpp < .5; fpp *= 2.0) {
      int max = 2 << ex;
      bloom_filter bf(max, fpp, 1);
      bf.insert("foo");
      bf.insert("bar");

      ASSERT_TRUE(123);
      ASSERT_TRUE(456);

      for (int n = 0; n < max; n++)
	bf.insert(n);

      int test = max * 100;
      int hit = 0;
      for (int n = 0; n < test; n++)
	if (bf.contains(100000 + n))
	  hit++;

      ASSERT_TRUE(123);
      ASSERT_TRUE(456);

      double actual = (double)hit / (double)test;

      bufferlist bl;
      ::encode(bf, bl);

      double byte_per_insert = (double)bl.length() / (double)max;

      std::cout << max << "\t" << fpp << "\t" << actual << "\t" << bl.length() << "\t" << byte_per_insert << std::endl;
      ASSERT_TRUE(actual < fpp * 10);

    }
  }
}

// test the fpp over a sequence of bloom filters, each with unique
// items inserted into it.
//
// we expect:  actual_fpp = num_filters * per_filter_fpp
TEST(BloomFilter, Sequence) {

  int max = 1024;
  double fpp = .01;
  for (int seq = 2; seq <= 128; seq *= 2) {
    std::vector<bloom_filter*> ls;
    for (int i=0; i<seq; i++) {
      ls.push_back(new bloom_filter(max*2, fpp, i));
      for (int j=0; j<max; j++) {
	ls.back()->insert("ok" + stringify(j) + "_" + stringify(i));
	if (ls.size() > 1)
	  ls[ls.size() - 2]->insert("ok" + stringify(j) + "_" + stringify(i));
      }
    }

    int hit = 0;
    int test = max * 100;
    for (int i=0; i<test; ++i) {
      for (std::vector<bloom_filter*>::iterator j = ls.begin(); j != ls.end(); ++j) {
	if ((*j)->contains("bad" + stringify(i))) {
	  hit++;
	  break;
	}
      }
    }

    double actual = (double)hit / (double)test;
    std::cout << "seq " << seq << " max " << max << " fpp " << fpp << " actual " << actual << std::endl;
  }
}

// test the ffp over a sequence of bloom filters, where actual values
// are always inserted into a consecutive pair of filters.  in order
// to have a false positive, we need to falsely match two consecutive
// filters.
//
// we expect:  actual_fpp = num_filters * per_filter_fpp^2
TEST(BloomFilter, SequenceDouble) {
  int max = 1024;
  double fpp = .01;
  for (int seq = 2; seq <= 128; seq *= 2) {
    std::vector<bloom_filter*> ls;
    for (int i=0; i<seq; i++) {
      ls.push_back(new bloom_filter(max*2, fpp, i));
      for (int j=0; j<max; j++) {
	ls.back()->insert("ok" + stringify(j) + "_" + stringify(i));
	if (ls.size() > 1)
	  ls[ls.size() - 2]->insert("ok" + stringify(j) + "_" + stringify(i));
      }
    }

    int hit = 0;
    int test = max * 100;
    int run = 0;
    for (int i=0; i<test; ++i) {
      for (std::vector<bloom_filter*>::iterator j = ls.begin(); j != ls.end(); ++j) {
	if ((*j)->contains("bad" + stringify(i))) {
	  run++;
	  if (run >= 2) {
	    hit++;
	    break;
	  }
	} else {
	  run = 0;
	}
      }
    }

    double actual = (double)hit / (double)test;
    std::cout << "seq " << seq << " max " << max << " fpp " << fpp << " actual " << actual
	      << " expected " << (fpp*fpp*(double)seq) << std::endl;
  }
}

