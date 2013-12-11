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

namespace ambient { namespace controllers { namespace velvet {

    // {{{ transformable

    inline void get<transformable>::spawn(transformable& t){
        ambient::controller.queue(new get(t));
    }
    inline get<transformable>::get(transformable& t){
        handle = ambient::channel.bcast(t, ambient::controller.which());
    }
    inline bool get<transformable>::ready(){
        return handle->test();
    }
    inline void get<transformable>::invoke(){}

    // }}}
    // {{{ revision

    inline void get<revision>::spawn(revision& r){
        get*& transfer = (get*&)r.assist.second;
        if(ambient::controller.update(r)) transfer = new get(r);
        *transfer += ambient::controller.which();
        channel.index();
    }
    inline get<revision>::get(revision& r) : t(r) {
        handle = ambient::channel.get(t);
        t.invalidate();
    }
    inline void get<revision>::operator += (int rank){
        *handle += rank;
        if(handle->involved() && !t.valid()){
            t.use();
            t.generator = this;
            t.embed(ambient::pool::malloc<bulk>(t.spec)); 
            ambient::controller.queue(this);
        }
    }
    inline bool get<revision>::ready(){
        return handle->test();
    }
    inline void get<revision>::invoke(){
        ambient::controller.squeeze(&t);
        t.release();
        t.complete();
    }

    // }}}

} } }


