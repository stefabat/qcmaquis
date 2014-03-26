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

#ifndef AMBIENT_CONTROLLERS_SSM_WORKFLOW
#define AMBIENT_CONTROLLERS_SSM_WORKFLOW

namespace ambient { 

    class workflow {
    public:
        typedef controllers::ssm::controller controller_type;
        typedef typename controller_type::model_type model_type;

        struct thread_context {
            controller_type controller;
            scope* domain;
            struct sid_t {
                struct divergence_guard {
                    divergence_guard();
                   ~divergence_guard();
                };
                sid_t() : value(1), inc(1) {}
                void offset(int offset, int increment);
                void maximize();
                int generate();
                int value;
                int inc;
                int max;
                int min;
            } sid;
        };

        mutable std::vector<thread_context> context_lane;
        mutable base_scope base;
        mutable ambient::mutex mtx;

        workflow();
        scope& get_domain() const;
        thread_context& get_context() const;
        controller_type& get_controller() const;
        controller_type* provide_controller();
        void revoke_controller(controller_type* c);
        void push(scope* s);
        void pop();
        void sync();
        bool scoped() const;
        bool remote() const;
        bool local()  const;
        bool common() const;
        int  which()  const;
        int  generate_sid();
        int  get_sid() const;
        bool tunable() const; 
        void schedule() const;
        void intend_read(models::ssm::revision* o) const;
        void intend_write(models::ssm::revision* o) const;
        ambient::mutex& get_mutex() const;
    };

    #ifdef AMBIENT_BUILD_LIBRARY
    workflow ctxt;
    void sync(){ ctxt.sync(); }
    #else
    extern workflow ctxt;
    #endif

    typedef typename workflow::thread_context::sid_t sid_t;
}

#endif

