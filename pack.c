#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef uint64_t Term;

/* old */

typedef uint8_t  Tag;  //  8 bits
typedef uint32_t Lab;  // 24 bits
typedef uint32_t Loc;  // 32 bits

Term term_new(Tag tag, Lab lab, Loc loc) {
  return ((Term)loc << 32) | ((Term)lab << 8) | tag;
}

Lab term_lab(Term term) { return (term >> 8) & 0xFFFFFF; }

Loc term_loc(Term term) { return term >> 32; }

Tag term_tag(Term term) { return term & 0xFF; }

/* new  */


typedef uint64_t Loc64;
typedef uint8_t Lab64;
typedef uint8_t Tag64;

#define USE_HVM_LAYOUT 0

#if USE_HVM_LAYOUT
  // [Loc:54 | Lab:8 | Tag:4]
  #define TAG_BITS 4
  #define LAB_BITS 8
  #define LOC_BITS 54
#else
  // [Loc:48 | Lab:8 | Tag:8]
  #define TAG_BITS 8
  #define LAB_BITS 8
  #define LOC_BITS 48
#endif

#define TAG_MASK ((1ULL << TAG_BITS) - 1)   // 0xFF
#define LAB_MASK ((1ULL << LAB_BITS) - 1)   // 0xFF
#define LOC_MASK ((1ULL << LOC_BITS) - 1)   // 0xFFFFFFFFFFFF

Term term_new64(Tag64 tag, Lab64 lab, Loc64 loc) {
    return ((Term)loc << (LAB_BITS + TAG_BITS)) |
           ((Term)lab << TAG_BITS) |
           ((Term)tag);
}

Term term_with_loc(Term term, Loc64 loc) {
    return (term & ~((Term)LOC_MASK << (LAB_BITS + TAG_BITS))) | 
           (((Term)loc & LOC_MASK) << (LAB_BITS + TAG_BITS));
}

Loc64 term_loc64(Term term) {
    return term >> (LAB_BITS + TAG_BITS);
}

Lab64 term_lab64(Term term) {
    return (term >> TAG_BITS) & LAB_MASK;
}

Tag64 term_tag64(Term term) {
    return term & TAG_MASK;
}

float
bench_diff_sec(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

void sink(Term v) { __asm__ volatile("" : : "r"(v)); }

int main() {
    const size_t ITER = 1000000000ULL;

    volatile Term v = 0;

    Term acc = 0;
    struct timespec start, end;

    float diff;
    float diff_old = 0.0;
    float diff_new = 0.0;

    // TERM_NEW

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
        acc ^= term_new(i % 256, i % 256, i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);

    diff = bench_diff_sec(start, end);
    diff_old += diff;
    printf("term_new:   %.6f sec\n", diff);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
        acc ^= term_new64(i % 256, i % 256, i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);

    diff = bench_diff_sec(start, end);
    diff_new += diff;
    printf("term_new64: %.6f sec\n", diff);

    // TERM_TAG

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
      acc++;
      Term t = acc ^ i;
      acc ^= term_tag(t);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);
    v += acc;

    diff = bench_diff_sec(start, end);
    diff_old += diff;
    printf("term_tag:   %.6f sec\n", diff);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
      acc++;
      Term t = acc ^ i;
      acc ^= term_tag64(t);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);
    v += acc;

    diff = bench_diff_sec(start, end);
    diff_new += diff;
    printf("term_tag64: %.6f sec\n", diff);

    // TERM_LAB

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
        Term t = acc ^ i;
        acc ^= term_lab(t);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);

    diff = bench_diff_sec(start, end);
    diff_old += diff;
    printf("term_lab:   %.6f sec\n", diff);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
        Term t = acc ^ i;
        acc ^= term_lab64(t);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);

    diff = bench_diff_sec(start, end);
    diff_new += diff;
    printf("term_lab64: %.6f sec\n", diff);

    // TERM_LOC
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
        Term t = acc ^ i;
        acc ^= term_loc(t);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);

    diff = bench_diff_sec(start, end);
    diff_old += diff;
    printf("term_loc:   %.6f sec\n", diff);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < ITER; ++i) {
        Term t = acc ^ i;
        acc ^= term_loc64(t);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    sink(acc);

    diff = bench_diff_sec(start, end);
    diff_new += diff;
    printf("term_loc64: %.6f sec\n", diff);

    printf("old total: %.2f sec\nnew total: %2.f sec\n", diff_old,
        diff_new);

    return v;
}

