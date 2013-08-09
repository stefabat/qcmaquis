/*
 * Ambient, License - Version 1.0 - May 3rd, 2012
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef AMBIENT
#define AMBIENT
// {{{ system includes
#include <mpi.h>
#include <complex>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <limits>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <memory.h>
#include <stdarg.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <algorithm>
#include <execinfo.h>
// }}}

#ifdef AMBIENT_CILK
    #include <cilk/cilk.h>
    #define AMBIENT_NUM_THREADS __cilkrts_get_total_workers()
    #define AMBIENT_THREAD_ID __cilkrts_get_worker_number()
    #define AMBIENT_THREAD cilk_spawn
    #define AMBIENT_SMP_ENABLE
    #define AMBIENT_SMP_DISABLE
#elif defined(AMBIENT_OMP)
    #include <omp.h>
    #define AMBIENT_THREAD_ID omp_get_thread_num()
    #define AMBIENT_PRAGMA(a) _Pragma( #a )
    #define AMBIENT_THREAD AMBIENT_PRAGMA(omp task untied)
    #define AMBIENT_SMP_ENABLE AMBIENT_PRAGMA(omp parallel) { AMBIENT_PRAGMA(omp single nowait)
    #define AMBIENT_SMP_DISABLE }
    #define AMBIENT_NUM_THREADS [&]()->int{ int n; AMBIENT_SMP_ENABLE \
                                { n = omp_get_num_threads(); } \
                                AMBIENT_SMP_DISABLE return n; }()
#else // Note: use for Cray machines
    #define AMBIENT_NUM_THREADS 1
    #define AMBIENT_THREAD_ID   0
    #define AMBIENT_THREAD
    #define AMBIENT_SMP_ENABLE
    #define AMBIENT_SMP_DISABLE
#endif

#define AMBIENT_MAX_NUM_PROCS         12
#define AMBIENT_DB_PROCS              0

//#define AMBIENT_EXPERIMENTAL
//#define AMBIENT_COMPUTATIONAL_TIMINGS
//#define AMBIENT_COMPUTATIONAL_DATAFLOW
#define AMBIENT_TRACE void* b[10]; backtrace_symbols_fd(b,backtrace(b,10),2);
//#define AMBIENT_CHECK_BOUNDARIES
//#define AMBIENT_LOOSE_FUTURE
//#define AMBIENT_TRACKING

#define AMBIENT_LARGE_BULK
#ifdef AMBIENT_LARGE_BULK
#define AMBIENT_BULK_CHUNK            4194304000
#else
#define AMBIENT_BULK_CHUNK            41943040
#endif

#ifdef AMBIENT_CRAY
#define AMBIENT_MAX_SID               4194304
#else
#define AMBIENT_MAX_SID               2147483647
#endif

#define AMBIENT_STACK_RESERVE         65536
#define AMBIENT_COLLECTOR_STR_RESERVE 65536
#define AMBIENT_COLLECTOR_RAW_RESERVE 1024
#define AMBIENT_SCOPE_SWITCH_FACTOR   20480
#define AMBIENT_FUTURE_SIZE           64
#define AMBIENT_IB                    512

#define PAGE_SIZE 4096
#define ALIGNMENT 16

#define AMBIENT_PERSISTENT_TRANSFERS

namespace ambient {
    inline int get_num_threads(){
        static int n = AMBIENT_NUM_THREADS; return n;
    }
    enum complexity { N, N2, N3 };
    enum locality   { remote, local, common };
    enum scope_t    { base, single, shared, dedicated };
    enum region_t   { rbulked, rstandard, rpersist, rdelegated };
}

#include "ambient/channels/mpi/groups/multirank.h"
#include "ambient/memory/mmap.hpp"
#include "ambient/memory/pool.hpp"
#include "ambient/memory/allocator.hpp"
#include "ambient/models/velvet/model.h"
#include "ambient/channels/mpi/channel.h"
#include "ambient/controllers/velvet/controller.h"
#include "ambient/utils/auxiliary.hpp"
#include "ambient/utils/io.hpp"
#include "ambient/utils/overseer.hpp"
#include "ambient/interface/typed.hpp"
#include "ambient/interface/kernel.hpp"
#include "ambient/memory/archive.hpp"
#include "ambient/interface/access.hpp"
#endif
