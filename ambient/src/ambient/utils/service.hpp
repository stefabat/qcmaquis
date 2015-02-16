/*
 * Copyright Institute for Theoretical Physics, ETH Zurich 2014.
 * Distributed under the Boost Software License, Version 1.0.
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

#ifndef AMBIENT_UTILS_SERVICE
#define AMBIENT_UTILS_SERVICE

#include <dlfcn.h>

extern "C" {
    void MKL_Set_Num_Threads(int nth);
}

namespace ambient {

    class mkl_parallel {
    public:
        typedef void (*fptr_t)(int);
        mkl_parallel(int nt = 0){
            if(nt || ambient::isset("AMBIENT_MKL_NUM_THREADS")){
                if(!nt) nt = ambient::getint("AMBIENT_MKL_NUM_THREADS");
                if(fptr == NULL) import();
                fptr(nt);
            }
            manual = (nt != 0);
        }
       ~mkl_parallel(){
            if(manual) fptr(1);
        }
    private:
        void import(){
            void* handle = dlopen("libmkl_intel_lp64.so", RTLD_LAZY); 
            if(!handle) throw std::runtime_error("Error: cannot open libmkl_intel_lp64.so!");
            dlerror(); // reset errors
            fptr = (fptr_t) dlsym(handle, "MKL_Set_Num_Threads");
            if(dlerror()) throw std::runtime_error("Error: cannot load symbol 'MKL_Set_Num_Threads'");
            dlclose(handle);
        }
    private:
        bool manual;
        static fptr_t fptr;
    };

    #ifdef AMBIENT_GLOBALS
    mkl_parallel::fptr_t mkl_parallel::fptr = NULL;
    #endif

}

#endif
