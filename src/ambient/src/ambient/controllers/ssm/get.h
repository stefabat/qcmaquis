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

#ifndef AMBIENT_CONTROLLERS_SSM_GET
#define AMBIENT_CONTROLLERS_SSM_GET

namespace ambient { namespace controllers { namespace ssm {
    
    using ambient::models::ssm::revision;
    using ambient::models::ssm::transformable;
    using ambient::channels::mpi::collective;
    using ambient::memory::data_bulk;

    template<class T> class get {};

    template<>
    class get<transformable> : public functor, public memory::use_bulk_new<get<transformable> > {
    public:
        static void spawn(transformable& v);
        get(transformable& v);
        virtual void invoke();
        virtual bool ready();
    private:
        collective<transformable>* handle;
    };

    template<>
    class get<revision> : public functor, public memory::use_bulk_new<get<revision> >  {
    public:
        static void spawn(revision& r);
        get(revision& r);
        virtual void invoke();
        virtual bool ready();
        void operator += (int rank);
    private:
        collective<revision>* handle;
        revision& t;
    };

} } }

#endif

