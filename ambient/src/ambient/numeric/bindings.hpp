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

#ifndef AMBIENT_NUMERIC_BINDINGS
#define AMBIENT_NUMERIC_BINDINGS

namespace ambient { namespace numeric { namespace bindings {

    // {{{ overloaded convertion functions
    template <typename T, typename D>
    void convert(ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >& pm, const ambient::numeric::tiles<ambient::numeric::diagonal_matrix<D> >& m){
        for(size_t k = 0; k < m.data.size(); ++k)
            ambient::numeric::kernels::template cast_double_complex<T,D>::template spawn(pm[k],m[k]);
    } 

    template <typename T, typename S, template<class M, class SS> class C>
    void convert(std::vector< std::vector<T> >& set, const C<ambient::numeric::diagonal_matrix<T>, S>& m){
        for(size_t k = 0; k < m.n_blocks(); ++k) 
            set.push_back(std::vector<T>(m[k].num_rows()));
        size_t num_cols(1);
        size_t offset(0);
        size_t num_rows;
        for(size_t k = 0; k < m.n_blocks(); ++k){
            num_rows = m[k].num_rows();
            std::vector<T>* v_ptr = &set[k];
            ambient::numeric::kernels::template cast_to_vector<T>::template spawn(v_ptr, m[k], num_rows, num_cols, num_rows, offset);
        }
        ambient::sync();
    }

    template <typename T, class A>
    void convert(ambient::numeric::matrix<T,A>& pm, const alps::numeric::matrix<T>& m){
        size_t num_rows = m.num_rows();
        size_t num_cols = m.num_cols();
        size_t lda = m.stride2();
        size_t offset(0);
        const std::vector<typename alps::numeric::matrix<T>::value_type>* v_ptr = &m.get_values();
        ambient::numeric::kernels::template cast_from_vector<T>::template spawn(v_ptr, pm, num_rows, num_cols, lda, offset);
        ambient::sync();
    }

    template <typename T, class A>
    void convert(alps::numeric::matrix<T>& m, const ambient::numeric::matrix<T,A>& pm){
        size_t num_rows = pm.num_rows();
        size_t num_cols = pm.num_cols();
        size_t offset(0);
        std::vector<typename alps::numeric::matrix<T>::value_type>* v_ptr = &m.get_values();
        ambient::numeric::kernels::template cast_to_vector<T>::template spawn(v_ptr, pm, num_rows, num_cols, num_rows, offset);
        ambient::sync();
    }

    template <typename T>
    void convert(ambient::numeric::diagonal_matrix<T>& pm, const alps::numeric::diagonal_matrix<T>& m){
        size_t num_rows(m.num_rows());
        size_t num_cols(1);
        size_t offset(0);
        const std::vector<typename alps::numeric::diagonal_matrix<T>::value_type>* v_ptr = &m.get_values();
        ambient::numeric::kernels::template cast_from_vector<T>::template spawn(v_ptr, pm, num_rows, num_cols, num_rows, offset);
        ambient::sync();
    }

    template <typename T>
    void convert(alps::numeric::diagonal_matrix<T>& m, const ambient::numeric::diagonal_matrix<T>& pm){
        size_t offset(0);
        size_t num_cols(1);
        size_t num_rows = pm.num_rows();
        std::vector<typename alps::numeric::diagonal_matrix<T>::value_type>* v_ptr = &m.get_values();
        ambient::numeric::kernels::template cast_to_vector<T>::template spawn(v_ptr, pm, num_rows, num_cols, num_rows, offset);
        ambient::sync();
    }

    template <typename T, typename S, template<class M, class SS> class C>
    void convert(std::vector< std::vector<T> >& set, const C<ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >, S>& m){
        for(size_t k = 0; k < m.n_blocks(); ++k) 
            set.push_back(std::vector<T>(m[k].num_rows()));
        size_t num_cols(1);
        for(size_t k = 0; k < m.n_blocks(); ++k){
            std::vector<T>* v_ptr = &set[k];
            size_t offset = 0;
            for(size_t kk = 0; kk < m[k].data.size(); kk++){
                size_t num_rows = m[k][kk].num_rows();
                ambient::numeric::kernels::template cast_to_vector<T>::template spawn(v_ptr, m[k][kk], num_rows, num_cols, num_rows, offset);
                offset += num_rows;
            }
        }
        ambient::sync();
    }

    template <typename T, class A>
    void convert(ambient::numeric::tiles<ambient::numeric::matrix<T,A> >& pm, const alps::numeric::matrix<T>& m){
        size_t num_rows = m.num_rows();
        size_t num_cols = m.num_cols();
        size_t lda = m.stride2();
        const std::vector<typename alps::numeric::matrix<T>::value_type>* v_ptr = &m.get_values();

        for(size_t j = 0; j < pm.nt; ++j){
            size_t offset = j*lda*AMBIENT_IB;
            for(size_t i = 0; i < pm.mt; ++i){
                ambient::numeric::matrix<T,A>& tile = pm.tile(i,j);
                size_t rows = tile.num_rows();
                size_t cols = tile.num_cols();
                ambient::numeric::kernels::template cast_from_vector<T>::template spawn(v_ptr, tile, rows, cols, lda, offset);
                offset += rows;
            }
        }

        ambient::sync();
    }

    template <typename T2, typename T1, class A>
    void convert(ambient::numeric::tiles<ambient::numeric::matrix<T2,A> >& pm, const alps::numeric::matrix<T1>& m){
        size_t num_rows = m.num_rows();
        size_t num_cols = m.num_cols();
        size_t lda = m.stride2();
        const std::vector<typename alps::numeric::matrix<T1>::value_type>* v_ptr = &m.get_values();

        for(size_t j = 0; j < pm.nt; ++j){
            size_t offset = j*lda*AMBIENT_IB;
            for(size_t i = 0; i < pm.mt; ++i){
                ambient::numeric::matrix<T2,A>& tile = pm.tile(i,j);
                size_t rows = tile.num_rows();
                size_t cols = tile.num_cols();
                ambient::numeric::kernels::template cast_from_vector_t<T1,T2>::template spawn(v_ptr, tile, rows, cols, lda, offset);
                offset += rows;
            }
        }

        ambient::sync();
    }

    template <typename T, class A>
    void convert(alps::numeric::matrix<T>& m, const ambient::numeric::tiles<ambient::numeric::matrix<T,A> >& pm){
        size_t num_rows = pm.num_rows();
        size_t num_cols = pm.num_cols();
        std::vector<typename alps::numeric::matrix<T>::value_type>* v_ptr = &m.get_values();
        size_t lda = m.stride2();
        for(size_t j = 0; j < pm.nt; ++j){
            size_t offset = j*lda*AMBIENT_IB;
            for(size_t i = 0; i < pm.mt; ++i){
                const ambient::numeric::matrix<T,A>& tile = pm.tile(i,j);
                size_t rows = tile.num_rows();
                size_t cols = tile.num_cols();
                ambient::numeric::kernels::template cast_to_vector<T>::template spawn(v_ptr, tile, rows, cols, lda, offset);
                offset += rows;
            }
        }

        ambient::sync();
    }

    template <typename T>
    void convert(ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >& pm, const alps::numeric::diagonal_matrix<T>& m){
        size_t num_rows = m.num_rows();
        size_t num_cols(1);
        const std::vector<typename alps::numeric::diagonal_matrix<T>::value_type>* v_ptr = &m.get_values();

        size_t offset(0);
        for(size_t i = 0; i < pm.nt; ++i){
            ambient::numeric::diagonal_matrix<T>& tile = pm[i];
            size_t rows = tile.num_rows();
            ambient::numeric::kernels::template cast_from_vector<T>::template spawn(v_ptr, tile, rows, num_cols, num_rows, offset);
            offset += rows;
        }

        ambient::sync();
    }

    template <typename T>
    void convert(alps::numeric::diagonal_matrix<T>& m, const ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >& pm){
        size_t num_rows = pm.num_rows();
        size_t num_cols(1);
        size_t offset(0);
        std::vector<typename alps::numeric::diagonal_matrix<T>::value_type>* v_ptr = &m.get_values();
        for(size_t i = 0; i < pm.nt; ++i){
            const ambient::numeric::diagonal_matrix<T>& tile = pm[i];
            size_t rows = tile.num_rows();
            ambient::numeric::kernels::template cast_to_vector<T>::template spawn(v_ptr, tile, rows, num_cols, num_rows, offset);
            offset += rows;
        }

        ambient::sync();
    }
    // }}}

    template <typename O, typename I> struct adaptor {};

    template <typename O, typename I> O cast(I const& input){
       return adaptor<O,I>::convert(input);
    }

    template <typename T, typename D>
    struct adaptor< ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >, ambient::numeric::tiles<ambient::numeric::diagonal_matrix<D> > >{
        static ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > convert(const ambient::numeric::tiles<ambient::numeric::diagonal_matrix<D> >& m){
            ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > pm(num_rows(m), num_cols(m));
            ambient::numeric::bindings::template convert<T,D>(pm, m);
            return pm;
        }
    };

    template <typename T, typename S, template<class M, class SS> class C>
    struct adaptor< std::vector< std::vector<T> >, C<ambient::numeric::diagonal_matrix<T>, S> > {
        static std::vector< std::vector<T> > convert(const C<ambient::numeric::diagonal_matrix<T>, S>& m){
            std::vector< std::vector<T> > set;
            ambient::numeric::bindings::convert(set, m);
            return set;
        }
    };

    template <typename T, class A>
    struct adaptor< ambient::numeric::matrix<T,A>, alps::numeric::matrix<T> > {
        static ambient::numeric::matrix<T,A> convert(const alps::numeric::matrix<T>& m){
            ambient::numeric::matrix<T,A> pm(num_rows(m), num_cols(m));    
            ambient::numeric::bindings::convert(pm, m);
            return pm;
        }
    };

    template <typename T, class A>
    struct adaptor< alps::numeric::matrix<T>, ambient::numeric::matrix<T,A> > {
        static alps::numeric::matrix<T> convert(const ambient::numeric::matrix<T,A>& pm){
            alps::numeric::matrix<T> m(num_rows(pm), num_cols(pm));    
            ambient::numeric::bindings::convert(m, pm);
            return m;
        }
    };

    template <typename T>
    struct adaptor< ambient::numeric::diagonal_matrix<T>, alps::numeric::diagonal_matrix<T> > {
        static ambient::numeric::diagonal_matrix<T> convert(const alps::numeric::diagonal_matrix<T>& m){
            ambient::numeric::diagonal_matrix<T> pm(num_rows(m));    
            ambient::numeric::bindings::convert(pm, m);
            return pm;
        }
    };

    template <typename T>
    struct adaptor< alps::numeric::diagonal_matrix<T>, ambient::numeric::diagonal_matrix<T> > {
        static alps::numeric::diagonal_matrix<T> convert(const ambient::numeric::diagonal_matrix<T>& pm){
            alps::numeric::diagonal_matrix<T> m(num_rows(pm));
            convert(m, pm);
            return m;
        }
    };

    template <typename T>
    struct adaptor< std::vector<T>, ambient::numeric::diagonal_matrix<T> > {
        static std::vector<T> convert(const ambient::numeric::diagonal_matrix<T>& pm){
            return adaptor<alps::numeric::diagonal_matrix<T>, ambient::numeric::diagonal_matrix<T> >::convert(pm).get_values();
        }
    };

    template <typename T, typename S, template<class M, class SS> class C>
    struct adaptor< std::vector< std::vector<T> >, C<ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >, S> > {
        static std::vector< std::vector<T> > convert(const C<ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >, S>& m){
            std::vector< std::vector<T> > set;
            ambient::numeric::bindings::convert(set, m);
            return set;
        }
    };

    template <typename T, class A>
    struct adaptor< ambient::numeric::tiles<ambient::numeric::matrix<T,A> >, alps::numeric::matrix<T> > {
        static ambient::numeric::tiles<ambient::numeric::matrix<T,A> > convert(const alps::numeric::matrix<T>& m){
            ambient::numeric::tiles<ambient::numeric::matrix<T,A> > pm(num_rows(m), num_cols(m));    
            ambient::numeric::bindings::convert(pm, m);
            return pm;
        }
    };

    template <typename T2, typename T1, class A>
    struct adaptor< ambient::numeric::tiles<ambient::numeric::matrix<T2,A> >, alps::numeric::matrix<T1> > {
        static ambient::numeric::tiles<ambient::numeric::matrix<T2,A> > convert(const alps::numeric::matrix<T1>& m){
            ambient::numeric::tiles<ambient::numeric::matrix<T2,A> > pm(num_rows(m), num_cols(m));    
            ambient::numeric::bindings::convert(pm, m);
            return pm;
        }
    };

    template <typename T, class A>
    struct adaptor< alps::numeric::matrix<T>, ambient::numeric::tiles<ambient::numeric::matrix<T,A> > > {
        static alps::numeric::matrix<T> convert(const ambient::numeric::tiles<ambient::numeric::matrix<T,A> >& pm){
            alps::numeric::matrix<T> m(num_rows(pm), num_cols(pm));    
            ambient::numeric::bindings::convert(m, pm);
            return m;
        }
    };

    template <typename T>
    struct adaptor< ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >, alps::numeric::diagonal_matrix<T> > {
        static ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > convert(const alps::numeric::diagonal_matrix<T>& m){
            ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > pm(num_rows(m));
            ambient::numeric::bindings::convert(pm, m);
            return pm;
        }
    };

    template <typename T>
    struct adaptor< alps::numeric::diagonal_matrix<T>, ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > > {
        static alps::numeric::diagonal_matrix<T> convert(const ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> >& pm){
            alps::numeric::diagonal_matrix<T> m(num_rows(pm));    
            ambient::numeric::bindings::convert(m, pm);
            return m;
        }
    };

} } }

template<typename T, class A>
bool operator == (alps::numeric::matrix<T> const & m, ambient::numeric::matrix<T,A> const & pm){
    return (ambient::numeric::bindings::cast<ambient::numeric::matrix<T,A> >(m) == pm);
}
template<typename T>
bool operator == (alps::numeric::diagonal_matrix<T> const & m, ambient::numeric::diagonal_matrix<T> const & pm){
    return (ambient::numeric::bindings::cast<ambient::numeric::diagonal_matrix<T> >(m) == pm);
}
template<typename T, class A>
bool operator == (alps::numeric::matrix<T> const & m, ambient::numeric::tiles<ambient::numeric::matrix<T,A> > const & pm){
    return (ambient::numeric::bindings::cast<ambient::numeric::tiles<ambient::numeric::matrix<T,A> > >(m) == pm);
}
template<typename T>
bool operator == (alps::numeric::diagonal_matrix<T> const & m, ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > const & pm){
    return (ambient::numeric::bindings::cast<ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > >(m) == pm);
}
template<typename T, class A> bool operator == (ambient::numeric::matrix<T,A> const & pm, alps::numeric::matrix<T> const & m){ return (m == pm); }
template<typename T> bool operator == (ambient::numeric::diagonal_matrix<T> const & pm, alps::numeric::diagonal_matrix<T> const & m){ return (m == pm); }
template<typename T, class A> bool operator == (ambient::numeric::tiles<ambient::numeric::matrix<T,A> > const & pm, alps::numeric::matrix<T> const & m){ return (m == pm); }
template<typename T> bool operator == (ambient::numeric::tiles<ambient::numeric::diagonal_matrix<T> > const & pm, alps::numeric::diagonal_matrix<T> const & m){ return (m == pm); }

#endif
