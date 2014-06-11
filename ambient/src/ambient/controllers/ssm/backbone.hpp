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

#ifndef AMBIENT_CONTROLLERS_SSM_BACKBONE_HPP
#define AMBIENT_CONTROLLERS_SSM_BACKBONE_HPP

namespace ambient { 

        inline backbone::backbone() : thread_context_lane(ambient::num_threads()) {
            for(thread_context& k : thread_context_lane) k.stack.push(&base);
            if(ambient::isset("AMBIENT_VERBOSE")){
                ambient::cout << "ambient: initialized ("                   << AMBIENT_THREADING_TAGLINE      << ")\n";
                if(ambient::isset("AMBIENT_MKL_NUM_THREADS")) ambient::cout << "ambient: selective threading (mkl)\n";
                ambient::cout << "ambient: size of instr bulk chunks: "     << AMBIENT_INSTR_BULK_CHUNK       << "\n";
                ambient::cout << "ambient: size of data bulk chunks: "      << AMBIENT_DATA_BULK_CHUNK        << "\n";
                if(ambient::isset("AMBIENT_BULK_LIMIT")) ambient::cout << "ambient: max chunks of data bulk: " << ambient::getint("AMBIENT_BULK_LIMIT") << "\n";
                if(ambient::isset("AMBIENT_BULK_REUSE")) ambient::cout << "ambient: enabled bulk garbage collection\n";
                if(ambient::isset("AMBIENT_BULK_DEALLOCATE")) ambient::cout << "ambient: enabled bulk deallocation\n";
                ambient::cout << "ambient: maximum sid value: "             << AMBIENT_MAX_SID                << "\n";
                ambient::cout << "ambient: number of procs: "               << ambient::num_procs()           << "\n";
                ambient::cout << "ambient: number of threads per proc: "    << ambient::num_threads()         << "\n";
                ambient::cout << "\n";
            }
            if(ambient::isset("AMBIENT_MKL_NUM_THREADS")) mkl_parallel();
            std::vector<int> procs; for(int i = 0; i < ambient::num_procs(); i++) procs.push_back(i);
            ambient::scope* global = new ambient::scope(procs.begin(), procs.end());
        }
        inline backbone::thread_context::sid_t::divergence_guard::divergence_guard(){
            for(auto& k : selector.thread_context_lane){
                k.sid.max = k.sid.min = selector.thread_context_lane[0].sid.value;
                k.stack.push(selector.thread_context_lane[0].stack.top());
            }
        }
        inline backbone::thread_context::sid_t::divergence_guard::~divergence_guard(){
            int& max = selector.thread_context_lane[0].sid.value;
            for(auto& k : selector.thread_context_lane){
                max = std::max(max, k.sid.max);
                k.sid.inc = 1;
                k.stack.pop();
            }
        }
        inline void backbone::thread_context::sid_t::offset(int offset, int increment){
            this->value = this->min + offset;
            this->inc = increment;
        }
        inline void backbone::thread_context::sid_t::maximize(){
            if(value < min) value += AMBIENT_MAX_SID;
            if(value > max) max = value;
        }
        inline int backbone::thread_context::sid_t::generate(){
            value = (value + inc) % AMBIENT_MAX_SID;
            return value;
        }
        inline void backbone::reset_sid(){
            selector.thread_context_lane[0].sid.value = 1;
        }
        inline int backbone::generate_sid(){
            return get_thread_context().sid.generate();
        }
        inline int backbone::get_sid() const {
            return get_thread_context().sid.value;
        }
        inline backbone::thread_context& backbone::get_thread_context() const {
            return thread_context_lane[AMBIENT_THREAD_ID];
        }
        inline typename backbone::controller_type& backbone::get_controller() const {
            return *get_actor().controller; // caution: != get_thread_context().controller;
        }
        inline typename backbone::controller_type* backbone::provide_controller(){
            return &get_thread_context().controller;
        }
        inline void backbone::revoke_controller(controller_type* c){
            // some cleanups ?
        }
        inline void backbone::sync_all(){
            for(int k = 1; k < thread_context_lane.size(); k++){
                for(auto i : *thread_context_lane[k].controller.chains) thread_context_lane[0].controller.queue(i);
                thread_context_lane[k].controller.chains->clear();
            }
            for(auto& k : thread_context_lane){
                k.controller.flush();
                k.controller.clear();
            }
            memory::data_bulk::drop();
        }
        inline bool backbone::has_nested_actor() const {
            return (&get_actor() != &this->base);
        }
        inline actor& backbone::get_actor() const {
            return *get_thread_context().stack.top();
        }
        inline void backbone::pop_actor(){
            get_thread_context().stack.pop();
        }
        inline void backbone::push_actor(actor* s){
            get_thread_context().stack.push(s);
        }
        inline scope& backbone::get_scope() const {
            return *scopes.top();
        }
        inline void backbone::pop_scope(){
            scopes.pop();
        }
        inline void backbone::push_scope(scope* s){
            scopes.push(s);
        }
        inline bool backbone::tunable() const { 
            return (!get_controller().is_serial() && !has_nested_actor());
        }
        inline void backbone::intend_read(models::ssm::revision* r) const {
            base.intend_read(r); 
        }
        inline void backbone::intend_write(models::ssm::revision* r) const {
            base.intend_write(r); 
        }
        inline void backbone::schedule() const {
            base.schedule();
        }
        inline ambient::mutex& backbone::get_mutex() const {
            return mtx;
        }
}

#endif

