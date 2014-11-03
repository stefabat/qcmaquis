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

#ifndef AMBIENT_CONTROLLERS_SSM_BACKBONE_HPP
#define AMBIENT_CONTROLLERS_SSM_BACKBONE_HPP

namespace ambient { 

        template<class Context>
        backbone<Context>::backbone() : sid(1) {
            this->init(&base);
            if(ambient::isset("AMBIENT_VERBOSE")){
                ambient::cout << "ambient: initialized ("                   << AMBIENT_THREADING_TAGLINE      << ")\n";
                if(ambient::isset("AMBIENT_MKL_NUM_THREADS")) ambient::cout << "ambient: selective threading (mkl)\n";
                ambient::cout << "ambient: size of instr bulk chunks: "     << AMBIENT_INSTR_BULK_CHUNK       << "\n";
                ambient::cout << "ambient: size of data bulk chunks: "      << AMBIENT_DATA_BULK_CHUNK        << "\n";
                if(ambient::isset("AMBIENT_BULK_LIMIT")) ambient::cout << "ambient: max share of data bulk: " << ambient::getint("AMBIENT_BULK_LIMIT") << "%\n";
                if(ambient::isset("AMBIENT_BULK_REUSE")) ambient::cout << "ambient: enabled bulk garbage collection\n";
                if(ambient::isset("AMBIENT_FORCE_BULK_DEALLOCATION")) ambient::cout << "ambient: enabled bulk deallocation\n";
                #ifdef MPI_VERSION
                ambient::cout << "ambient: maximum tag value: "             << ambient::get_tag_ub()          << "\n";
                ambient::cout << "ambient: number of procs: "               << ambient::num_procs()           << "\n";
                #endif
                ambient::cout << "ambient: number of threads: "             << ambient::num_threads()         << "\n";
                ambient::cout << "\n";
            }
            if(ambient::isset("AMBIENT_MKL_NUM_THREADS")) mkl_parallel();
            std::vector<int> procs; for(int i = 0; i < ambient::num_procs(); i++) procs.push_back(i);
            ambient::scope* global = new ambient::scope(procs.begin(), procs.end());
            tag_ub = ambient::get_tag_ub();
        }
        template<class Context>
        int backbone<Context>::generate_sid(){
            return (++sid %= tag_ub);
        }
        template<class Context>
        int backbone<Context>::get_sid(){
            return sid;
        }
        template<class Context>
        typename backbone<Context>::controller_type& backbone<Context>::get_controller(){
            return *get_actor().controller; // caution: != Context::get().controller
        }
        template<class Context>
        void backbone<Context>::revoke_controller(controller_type* c){
        }
        template<class Context>
        bool backbone<Context>::has_nested_actor(){
            return (&get_actor() != &this->base);
        }
        template<class Context>
        typename backbone<Context>::controller_type* backbone<Context>::provide_controller(){
            return &Context::get().controller;
        }
        template<class Context>
        void backbone<Context>::sync(){
            Context::sync();
            memory::data_bulk::drop();
        }
        template<class Context>
        actor& backbone<Context>::get_actor(){
            return *Context::get().actors.top();
        }
        template<class Context>
        actor_auto& backbone<Context>::get_base(){
            return this->base;
        }
        template<class Context>
        void backbone<Context>::pop_actor(){
            Context::get().actors.pop();
        }
        template<class Context>
        void backbone<Context>::push_actor(actor* s){
            Context::get().actors.push(s);
        }
        template<class Context>
        scope& backbone<Context>::get_scope(){
            return *Context::get().scopes.top();
        }
        template<class Context>
        void backbone<Context>::pop_scope(){
            Context::get().scopes.pop();
        }
        template<class Context>
        void backbone<Context>::push_scope(scope* s){
            Context::get().scopes.push(s);
        }
        template<class Context>
        bool backbone<Context>::tunable(){
            return (!get_controller().is_serial() && !has_nested_actor());
        }
        template<class Context>
        void backbone<Context>::intend_read(models::ssm::revision* r){
            base.intend_read(r); 
        }
        template<class Context>
        void backbone<Context>::intend_write(models::ssm::revision* r){
            base.intend_write(r); 
        }
        template<class Context>
        void backbone<Context>::schedule(){
            base.schedule();
        }
        template<class Context>
        ambient::mutex& backbone<Context>::get_mutex(){
            return mtx;
        }
}

#endif
