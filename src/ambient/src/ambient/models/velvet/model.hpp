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

namespace ambient { namespace models { namespace velvet {

    #ifdef AMBIENT_TRACKING
    inline void model::index(history* h){
        h->id = this->sid++;
        this->sid %= AMBIENT_MAX_SID;
    }
    #endif

    inline void model::index(const transformable* v){
        v->sid = this->sid++;
        this->sid %= AMBIENT_MAX_SID;
    }

    template<ambient::locality L, typename G>
    inline void model::add_revision(history* o, G g){
        o->add_state<L>(g);
    }

    inline void model::use_revision(history* o){
        o->back()->use();
    }

    inline void model::touch(const history* o){
        if(o->back() == NULL)
            const_cast<history*>(o)->init_state();
    }

    inline size_t model::time(const history* o){
        this->touch(o);
        return o->time();
    }

    inline bool model::feeds(const revision* r){
        return (r->state == ambient::local);
    }

    inline bool model::remote(const revision* r){
        return (r->state == ambient::remote);
    }

    inline bool model::common(const revision* r){
        return (r->state == ambient::common);
    }

} } }
