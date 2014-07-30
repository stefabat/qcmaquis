/*
 * Ambient Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
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

#ifndef AMBIENT_MEMORY_MMAP
#define AMBIENT_MEMORY_MMAP

#include <boost/thread.hpp>

#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <utility>
#include <atomic>
#include <tuple>

namespace ambient {
    void sync();
}

namespace ambient { namespace memory { 

    struct mmap {

        class pool {
            public:
            static pool& instance(){ 
                static pool singleton;
                return singleton;
            }
            pool() : active(false) {}
            void init(const std::string& path){
                this->active = true;
                this->path = path;
                this->sid = 0;
            }
            bool enabled(){
                return this->active;
            }
            std::string fp(size_t sid){
                return (path + std::to_string(sid) + std::string(".") + std::to_string(ambient::rank()));
            }
            size_t index(){
                if(vacants.size()){
                    size_t s = vacants.back();
                    vacants.pop_back();
                    return s;
                }
                return sid++;
            }
            void release(size_t s){
                vacants.push_back(s);
            }
            size_t sid;
            std::vector<size_t> vacants;
            std::string path;
            bool active;
        };


        class region {
        public:
            enum { core, store, uncore, prefetch, load } state;
            bool dumped;

            region(size_t length) : state(core), dumped(false), length(length) {
                sid = pool::instance().index();
                buffer = std::malloc(length);
                iterator = (char*)buffer;
            }
            void* malloc(size_t sz){
                return iterator.fetch_add(ambient::memory::aligned_64(sz));
            }

            class unmapx {
            public:
                unmapx(std::string fp, void* buffer, size_t length) :
                fp(fp), buffer(buffer), length(length) { }
                void operator()(){
                    std::ofstream ofs(fp.c_str(), std::ofstream::binary);
                    ofs.write((char*)buffer, length);
                    ofs.close();
                    std::free(buffer);
                }
            private:
                std::string fp;
                void* buffer;
                size_t length;
            };

            class remapx {
            public:
                remapx(std::string fp, void* buffer, size_t length) :
                fp(fp), buffer(buffer), length(length) { }
                void operator()(){
                    std::ifstream ifs(fp.c_str(), std::ifstream::binary);
                    ifs.read((char*)buffer, length);
                    ifs.close();
                }
            private:
                std::string fp;
                void* buffer;
                size_t length;
            };

            void unmap(){
                if(state == core){ /* ok */ }
                else if(state == store) return;
                else if(state == uncore) return;
                //else if(state == prefetch){ worker.join(); printf("WARNING: UNMAP OF PREFETCHED!\n"); }
                //else if(state == load) worker.join();

                pbuffer = buffer;
                if(!dumped){
                    state = store;
                    dumped = true;
                    worker = boost::thread(unmapx(pool::instance().fp(sid), buffer, length));
                }else{
                    state = uncore;
                    std::free(buffer);
                }
            }
            void fetch(){
                if(state == core) return;
                else if(state == store) worker.join();
                else if(state == uncore){ /* ok */ }
                else if(state == prefetch) return;
                //else if(state == load) return;

                state = prefetch;
                buffer = std::malloc(length);
                worker = boost::thread(remapx(pool::instance().fp(sid), buffer, length));
            }
            void remap(){
                if(state == core) return;
                //else if(state == store){ fetch(); worker.join(); printf("WARNING: UNPREFETCHED REMAP!\n"); }
                //else if(state == uncore){ fetch(); worker.join(); printf("WARNING: UNPREFETCHED REMAP!\n"); }
                else if(state == prefetch) worker.join();
                //else if(state == load) return;

                state = core;
            }
            void update(void*& p){
                p = (void*)((size_t)buffer + ((size_t)p-(size_t)pbuffer));
            }
            void drop(){
                if(state == core) std::free(buffer);
                //else if(state == store){ worker.join(); printf("WARNING: DROP OF ALREADY STORED DATA!\n"); }
                //else if(state == uncore){ /* ok */ printf("WARNING: DROP OF ALREADY STORED DATA!\n"); }
                //else if(state == prefetch){ worker.join(); std::free(buffer); printf("WARNING: DROP OF PREFETCHED DATA!\n"); }
                //else if(state == load) return;

                pool::instance().release(sid);
            }
        public: // private
            void* buffer;
            void* pbuffer;
            size_t sid;
            size_t length;
            std::atomic<char*> iterator;
            boost::thread worker;
        };

        class descriptor {
        public:
            descriptor() : loaded(true), buffer(NULL), size(0) { }
            void reserve(){ this->buffer = new region(size); }
            void* malloc(size_t sz){ return buffer->malloc(sz); }
            void drop() { if(buffer) buffer->drop();  } // buffer can be 0 here
            void update(void*& p){ buffer->update(p); }
            bool remap(){
                if(loaded) return false;
                loaded = true;  
                buffer->remap();
                return true;
            }
            void prefetch(){
                if(loaded) return;
                buffer->fetch();
            }
            void unmap(){ 
                if(loaded){ 
                    ambient::sync(); 
                    loaded = false; 
                    buffer->unmap();
                } 
            }
            region* buffer;
            bool loaded;
            size_t size;
        };

        static inline void init(const std::string& path){
            if(ambient::master()) printf("Temporary storage enabled in %s\n", path.c_str());
            pool::instance().init(path);
        }

        static inline bool enabled(){
            return pool::instance().enabled();
        }
    };

} }

#endif
