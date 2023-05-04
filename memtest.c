#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include "rdtsc.h"
#ifdef _MSC_VER
#include <intrin.h>
#endif

#define bzero(p, l) memset(p, 0, l)
#pragma warning(disable:4244)

//#define SERIAL_CONNECT

#if !defined(CLS) || CLS==0
#undef CLS
#define CLS 64			// Cache Line Size
#endif
#define NPAD ((CLS-sizeof(struct l*))/sizeof(uint64_t))

struct l {
    struct l *next;
    uint64_t pad[NPAD];
};
unsigned int count;
typedef int (*generator_t)(struct l *, uint64_t);
typedef uint64_t (*tester_t)(struct l *, uint64_t, unsigned);
long long __divti3 (long long a, long long b);
int serial_connect(struct l *field, uint64_t n)
{
    uint64_t i;

    for (i=0; i < n-1; i++)
    {
        field[i].next = field+i+1;
    }
    field[n-1].next = field;
    return 1;
}

int compare(const void* a, const void*b)
{
    struct l*a1 = *(struct l **)a;
    struct l*a2 = *(struct l **)b;
    return (int)(a1->pad[2]-a2->pad[2]);
}

int random_connect(struct l *field, uint64_t n)
{
    uint64_t i;
    struct l **f=NULL;

    bzero(field, n*(uint64_t)sizeof(struct l));
    f = (struct l **)malloc((uint64_t)sizeof(struct l *)*n);
    if(f == NULL)
    {
        fprintf(stderr, "No enougth memory]\n");
        return 0;
    }
    for (i=0; i<n; i++)
    {
        f[i]=field+i;
        f[i]->pad[2] = rand();
    }
    qsort(f, n, sizeof(struct l*), compare);
    for (i=1; i<n; i++)
    {
        f[i-1]->next = f[i];
    }
    f[n-1]->next = f[0];
    free(f);
    return 1;
}

inline uint32_t div64(uint64_t a, uint64_t b)
{
#if defined (__i386__) || defined(__arm__)
    return __divti3(a, b);
#else
    return a/b;
#endif
}

uint64_t do_read_test(struct l *field, uint64_t n, unsigned cycles)
{
    struct l *a;
    uint64_t end, beg;
    register unsigned i;

    a=field;
    do
    {
        count++;
        a = a->next;
    } while (a != field);
    beg=__rdtsc();
    for(i=0; i<cycles; i++)
    {
        count = 0;
        a=field;
        do
        {
            count++;
            a = a->next;
        } while (a != field);
    }
    end=__rdtsc();
    //  printf("%lld %lld %lld\n", beg, end, end-beg);
    return end-beg;
}

uint64_t do_write_test(struct l *field, uint64_t n, unsigned cycles)
{
    struct l *a;
    uint64_t end, beg;
    register unsigned i;

    beg = __rdtsc();
    for (i = 0; i<cycles; i++)
    {
        count = 0;
        a = field;
        do
        {
            count++;
            a = a->next;
        } while (a != field);
    }
    end = __rdtsc();
    //  printf("%lld %lld %lld\n", beg, end, end-beg);
    return end - beg;
}

uint64_t run_test(unsigned fsize, unsigned cycles, generator_t generator, tester_t tester)
{
    void *mem;
    struct l *field;
    uint64_t nelem;
    uint64_t testtime;
    uint32_t memory;
    char mult;

    nelem = (fsize*1024)/sizeof(struct l);
    memory=fsize*1024;
    if (memory < 1024)
        mult='b';
    else
    {
        memory /= 1024;
        if (memory < 1024)
            mult = 'K';
        else
        {
            memory /= 1024;
            if (memory < 1024)
                mult = 'M';
            else
            {
                memory /= 1024;
                mult='G';
            }
        }
    }
    if (nelem == 0)
    {
        printf("Work field too small (%d)...\n", fsize);
        return 0;
    }
    mem = malloc((nelem+1)*sizeof(struct l));
    if (mem == NULL)
    {
        fprintf(stderr, "Not enouht memory\n");
        return 0;
    }
    field = (struct l *)mem;
    field = (struct l *)(((uint64_t)(mem) + 0x3F) & 0xFFFFFFFFFFFFFFC0);
    if (!generator(field, nelem))
    {
        fprintf(stderr, "Can't generate work field\n");
        return 0;
    }
    //  printf("Executing...\n");
    testtime=tester(field, nelem, cycles);
    printf("%u%c,%u,%u,%u\n", memory, mult, (uint32_t)nelem, (uint32_t)testtime, div64(testtime, nelem*cycles));
    free(mem);
    return testtime;
}

uint32_t sizes[]={8, 16, 24, 32, 40, 48, 64, 128, 256, 384, 512, 768, 1024, 1536, 2048, 3*1024, 4*1024, 5*1024, 6*1024, 7*1024, 8*1024, 9*1024, 10*1024, 20*1024, 30*1024, 0};

int main(void)
{
    uint64_t i;

    srand(time(NULL));
    i=0;
    printf("Element size:%ld\n", sizeof(struct l));
    init_perfcounters(1, 0);
    while (sizes[i] != 0)
    {
#if defined (SERIAL_CONNECT)
        run_test(sizes[i], 100, serial_connect, do_read_test);
#else
        run_test(sizes[i], 100, random_connect, do_read_test);
#endif
        i++;
    }
    return 0;
}

