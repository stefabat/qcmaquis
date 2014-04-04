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

#ifndef AMBIENT_INTERFACE_ACCESS
#define AMBIENT_INTERFACE_ACCESS

#define mapping typename T::ambient_desc::mapping

namespace ambient {

    using ambient::models::ssm::revision;

    template <typename T> static revision& naked(T& obj){
        return *obj.ambient_rc.desc->current;
    }

    template <typename T> static bool exclusive(T& obj){
        ctxt.get_controller().touch(obj.ambient_rc.desc);
        revision& c = *obj.ambient_rc.desc->current;
        if(ctxt.remote()){
            c.state = ambient::locality::remote;
            c.owner = ctxt.which();
            return true;
        }else{
            c.state = ambient::locality::local;
            if(!c.valid()) c.embed(get_allocator<T>::type::alloc(c.spec));
            return false;
        }
    }

    template <typename T> static mapping& load(T& obj){ 
        ctxt.get_controller().touch(obj.ambient_rc.desc);
        ambient::sync(); 
        revision& c = *obj.ambient_rc.desc->current;
        assert(c.state == ambient::locality::local || c.state == ambient::locality::common);
        if(!c.valid()){
            c.embed(get_allocator<T>::type::calloc(c.spec));
        }
        return *(mapping*)c;
    }

    template <typename T> static mapping& versioned(const T& obj){
        revision& c = *obj.ambient_before; if(c.valid()) return *(mapping*)c;
        c.embed(get_allocator<T>::type::calloc(c.spec));
        return *(mapping*)c;
    }

    template <typename T> static mapping& versioned(unbound< T >& obj){ 
        revision& c = *obj.ambient_after; if(c.valid()) return *(mapping*)c;
        revision& p = *obj.ambient_before;
        if(p.valid() && p.locked_once() && !p.referenced() && c.spec.conserves(p.spec)) c.reuse(p);
        else c.embed(get_allocator<T>::type::alloc(c.spec));
        return *(mapping*)c;
    }

    template <typename T> static mapping& versioned(T& obj){
        revision& c = *obj.ambient_after; if(c.valid()) return *(mapping*)c;
        revision& p = *obj.ambient_before;
        if(!p.valid()) c.embed(get_allocator<T>::type::calloc(c.spec));
        else if(p.locked_once() && !p.referenced() && c.spec.conserves(p.spec)) c.reuse(p);
        else{
            c.embed(get_allocator<T>::type::alloc(c.spec));
            memcpy((T*)c, (T*)p, p.spec.extent);
        }
        return *(mapping*)c;
    }
}

#undef mapping
#endif
