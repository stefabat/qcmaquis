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

#ifndef AMBIENT_NUMERIC_MATRIX_KERNELS
#define AMBIENT_NUMERIC_MATRIX_KERNELS

#include "ambient/numeric/kernels/math.hpp"
#include "ambient/numeric/kernels/utils.hpp"
#include "ambient/numeric/traits.hpp"
#include "ambient/utils/numeric.h"

namespace ambient { namespace numeric { namespace kernels {
    namespace detail {

        using ambient::unbound;
        using ambient::numeric::matrix;
        using ambient::numeric::traits::real_type;
       
        template<typename T>
        void geqrt(matrix<T>& a, matrix<T>& t){
            T* tau  = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB>(); 
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::geqrt(a.num_rows(), a.num_cols(), PLASMA_IB,
                                    (T*)revised(a), a.num_rows(),
                                    (T*)updated(t), t.num_rows(),
                                    tau, work);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(tau); 
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<typename T, typename TR>
        void ormqr(const size_t& k, const matrix<T>& a, const matrix<T>& t, matrix<T>& c){
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::ormqr(PlasmaLeft, TR::value, c.num_rows(), c.num_cols(), k, PLASMA_IB,
                                    (T*)current(a), a.num_rows(),
                                    (T*)current(t), t.num_rows(),
                                    (T*)revised(c), c.num_rows(),
                                     work, AMBIENT_IB);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(work);
            #endif 
        }
       
        template<typename T>
        void tsqrt(matrix<T>& a1, matrix<T>& a2, matrix<T>& t){
            T* tau  = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB>();
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::tsqrt(a2.num_rows(), a2.num_cols(), PLASMA_IB,
                                    (T*)revised(a1), a1.num_rows(),
                                    (T*)revised(a2), a2.num_rows(),
                                    (T*)updated(t), t.num_rows(),
                                    tau, work);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(tau); 
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<typename T, typename TR>
        void tsmqr(const size_t& k, matrix<T>& a1, matrix<T>& a2, const matrix<T>& v, const matrix<T>& t){
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::tsmqr(PlasmaLeft, TR::value,
                                    AMBIENT_IB, a1.num_cols(), a2.num_rows(), a2.num_cols(), k, PLASMA_IB,
                                    (T*)revised(a1), a1.num_rows(),
                                    (T*)revised(a2), a2.num_rows(),
                                    (T*)current(v), v.num_rows(),
                                    (T*)current(t), t.num_rows(),
                                    work, PLASMA_IB);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<typename T>
        void gelqt(matrix<T>& a, matrix<T>& t){
            T* tau  = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB>();
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::gelqt(a.num_rows(), a.num_cols(), PLASMA_IB,
                                    (T*)revised(a), a.num_rows(), 
                                    (T*)updated(t),   t.num_rows(),
                                    tau, work);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(tau); 
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<typename T, typename TR>
        void ormlq(const size_t& k, const matrix<T>& a, const matrix<T>& t, matrix<T>& c){
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::ormlq(PlasmaRight, TR::value,
                                    c.num_rows(), c.num_cols(), k, PLASMA_IB,
                                    (T*)current(a), a.num_rows(),
                                    (T*)current(t), t.num_rows(),
                                    (T*)revised(c), c.num_rows(),
                                    work, AMBIENT_IB);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<typename T>
        void tslqt(matrix<T>& a1, matrix<T>& a2, matrix<T>& t){
            T* tau  = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB>();
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::tslqt(a2.num_rows(), a2.num_cols(), PLASMA_IB,
                                    (T*)revised(a1), a1.num_rows(),
                                    (T*)revised(a2), a2.num_rows(),
                                    (T*)updated(t),     t.num_rows(),
                                    tau, work);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(tau); 
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<typename T, typename TR>
        void tsmlq(const size_t& k, matrix<T>& a1, matrix<T>& a2, const matrix<T>& v, const matrix<T>& t){
            T* work = (T*)ambient::pool::malloc<bulk,sizeof(T)*AMBIENT_IB*PLASMA_IB>();
            helper_plasma<T>::tsmlq(PlasmaRight, TR::value,
                                    a1.num_rows(), AMBIENT_IB, a2.num_rows(), a2.num_cols(), k, PLASMA_IB,
                                    (T*)revised(a1), a1.num_rows(),
                                    (T*)revised(a2), a2.num_rows(),
                                    (T*)current(v), v.num_rows(),
                                    (T*)current(t), t.num_rows(),
                                    work, AMBIENT_IB);
            #ifdef AMBIENT_MEMORY_SQUEEZE
            ambient::memory::bulk::reuse(work); 
            #endif
        }
       
        template<class ViewA, class ViewB, class ViewC, typename T>
        void gemm(const matrix<T,typename ViewA::allocator_type>& a, 
                  const matrix<T,typename ViewB::allocator_type>& b, 
                  unbound< matrix<T,typename ViewC::allocator_type> >& c){
            if(!raw(a).valid() || !raw(b).valid()){
                emptied(c);
                return;
            }
            T* ad = current(a);
            T* bd = current(b);
            T* cd = updated(c);
            int m = ViewA::rows(a);
            int k = ViewA::cols(a);
            int n = ViewB::cols(b);
            int lda = a.num_rows();
            int ldb = b.num_rows();
            int ldc = c.num_rows();
            static const double alfa(1.0); 
            static const double beta(0.0);
            helper_blas<T>::gemm(ViewA::code(), ViewB::code(), &m, &n, &k, &alfa, ad, &lda, bd, &ldb, &beta, cd, &ldc);
        }
            
        template<class ViewB, typename T, typename D>
        void gemm_diagonal_lhs(const matrix<D>& a_diag, const matrix<T>& b, unbound< matrix<T> >& c){
            if(boost::is_same<ViewB, transpose_view<matrix<T> > >::value){
                size_t sizex = b.num_cols();
                int size  = a_diag.num_rows();
                static const int ONE = 1;
                T* bd = current(b);
                T* cd = emptied(c);
                D* alfa = current(a_diag);
                
                for(int k = 0 ; k < sizex; k++)
                    helper_blas<T>::axpy(&size, &alfa[k], &bd[k*size], &ONE, &cd[k], &size);// C - check carefully for TE a_diag double, b complex
            }else{
                int sizey = a_diag.num_rows();
                int size = b.num_cols();
                T* bd = current(b);
                T* cd = emptied(c);
                D* alfa = current(a_diag);
               
                for(int k = 0 ; k < sizey; k++)
                    helper_blas<T>::axpy(&size, &alfa[k], &bd[k], &sizey, &cd[k], &sizey);
            }
        }
            
        template<class ViewA, typename T, typename D>
        void gemm_diagonal_rhs(const matrix<T>& a, const matrix<D>& b_diag, unbound< matrix<T> >& c){
            if(boost::is_same<ViewA, transpose_view<matrix<T> > >::value){
                int sizey = b_diag.num_rows();
                int size = a.num_cols();
                static const int ONE = 1;
                T* ad = current(a);
                T* cd = emptied(c);
                D* alfa = current(b_diag);
                
                for(int k = 0 ; k < sizey; k++)
                    helper_blas<T>::axpy(&size, &alfa[k], &ad[k], &sizey, &cd[k*size], &ONE);// C - check carefully for TE b_diag double, b complex
            }else{
                size_t sizex = b_diag.num_rows();
                int size = a.num_rows(); // for the case of complex
                static const int ONE = 1;
                T* ad = current(a);
                T* cd = emptied(c);
                D* alfa = current(b_diag);
         
                for(int k = 0 ; k < sizex; k++)
                    helper_blas<T>::axpy(&size, &alfa[k], &ad[k*size], &ONE, &cd[k*size], &ONE);
            }
        }
       
        template<typename T>
        void copy_rt(const matrix<T>& a, unbound< matrix<T> >& t){
            T* ad  = current(a);
            T* td  = emptied(t);
            size_t sda = a.num_cols();
            size_t lda = a.num_rows();
            size_t ldt = t.num_rows();
       
            for(int j = 0; j < sda; ++j)
            for(int i = 0; i <= j && i < ldt; ++i)
            td[i+ldt*j] = ad[i+lda*j]; 
        }
       
        template<typename T>
        void copy_lt(const matrix<T>& a, unbound< matrix<T> >& t){
            T* ad  = current(a);
            T* td  = emptied(t);
            size_t sdt = t.num_cols();
            size_t lda = a.num_rows();
            size_t ldt = t.num_rows();
       
            for(int j = 0; j < sdt; ++j)
            for(int i = j; i < lda; ++i)
            td[i+ldt*j] = ad[i+lda*j]; 
        }
       
        template<class A1, class A2, typename T>
        void copy_block(const matrix<T,A1>& src, const size_t& si, const size_t& sj,
                        matrix<T,A2>& dst, const size_t& di, const size_t& dj, 
                        const size_t& m, const size_t& n)
        {
            T* sd = current(src);
            T* dd = m*n < ambient::square_dim(dst) ? revised(dst) : updated(dst);
            ambient::memptf<T, ambient::memcpy>(dd, dst.num_rows(), dim2(dj, di), 
                                                sd, src.num_rows(), dim2(sj, si), 
                                                dim2( n, m ));
        }
       
        template<typename T>
        void copy_block_s(const matrix<T>& src, const size_t& si, const size_t& sj,
                          matrix<T>& dst, const size_t& di, const size_t& dj, 
                          const matrix<T>& alfa, const size_t& ai, const size_t& aj,
                          const size_t& m, const size_t& n)
        {
            T* sd = current(src);
            T* dd = m*n < ambient::square_dim(dst) ? revised(dst) : updated(dst);
            T factor = ((T*)current(alfa))[ai + aj*alfa.num_rows()];
            ambient::memptf<T, ambient::memscal>(dd, dst.num_rows(), dim2(dj, di), 
                                                 sd, src.num_rows(), dim2(sj, si), 
                                                 dim2( n, m ), factor);
        }
       
        template<class A1, class A2, class A3, typename T>
        void copy_block_sa(const matrix<T,A1>& src, const size_t& si, const size_t& sj,
                           matrix<T,A2>& dst, const size_t& di, const size_t& dj, 
                           const matrix<T,A3>& alfa, const size_t& ai, const size_t& aj,
                           const size_t& m, const size_t& n, const T& alfa_scale)
        {
            T factor = alfa_scale * ((T*)current(alfa))[ai + aj*alfa.num_rows()];
            ambient::memptf<T, ambient::memscala>(revised(dst), dst.num_rows(), dim2(dj, di), 
                                                  current(src), src.num_rows(), dim2(sj, si), 
                                                  dim2( n, m ), factor);
        }
            
        template<typename T, class A>
        void trace(const matrix<T,A>& a, future<T>& trace){
            size_t m = a.num_rows();
            size_t n = a.num_cols();
            T* ad = current(a);
        
            size_t sizex = std::min(n,m);
            for(size_t jj = 0; jj < sizex; jj++)
                trace.get_naked() += ad[jj + jj*m];
        }
            
        template<typename T, class A>
        void scalar_norm(const matrix<T,A>& a, future<double>& norm){
            T* ad = current(a);
            norm.get_naked() = ambient::dot(ad, ad, ambient::square_dim(a));
        }
            
        template<typename T>
        void overlap(const matrix<T>& a, const matrix<T>& b, future<T>& overlap){
            T* ad = current(a);
            T* bd = current(b);
            overlap.get_naked() = ambient::dot(ad, bd, ambient::square_dim(a));
        }
       
        template<typename T, class A>
        void add(matrix<T,A>& a, const matrix<T,A>& b){
            T* ad = current(a);
            T* bd = current(b);
            T* ar = updated(a);
       
            int size = ambient::square_dim(a);
            #pragma vector always
            for(int k = 0; k < size; k++)
                ar[k] = ad[k] + bd[k];
        }
       
            
        template<typename T>
        void sub(matrix<T>& a, const matrix<T>& b){
            T* ad = current(a);
            T* bd = current(b);
            T* ar = updated(a);
       
            int size = ambient::square_dim(a);
            #pragma vector always
            for(int k = 0; k < size; k++)
                ar[k] = ad[k] - bd[k];
        }
            
        template<typename T>
        void scale(matrix<T>& a, const future<T>& t){
            T* ad = current(a);
            T* ar = updated(a);
            T factor = t.get_naked();
            int size = ambient::square_dim(a);
            #pragma vector always
            for(int k = 0; k < size; k++)
                ar[k] = ad[k] * factor;
        }
            
        template<typename T>
        void scale_offset(matrix<T>& a, const size_t& ai, const size_t& aj, const matrix<T>& alfa, const size_t& alfai){
            int m = num_rows(a);
            T* ad = &((T*)revised(a))[aj*m];
            T factor = ((T*)current(alfa))[alfai];
            for(int k = ai; k < m; k++) ad[k] *= factor;
        }
            
        template<typename T>
        void scale_inverse(matrix<T>& a, const future<T>& t){
            T* ad = current(a);
            T* ar = updated(a);
            T factor = t.get_naked();
            int size = ambient::square_dim(a);
            #pragma vector always
            for(int k = 0; k < size; k++)
                ar[k] = ad[k] / factor;
        }
            
        template<typename T>
        void sqrt_diagonal(matrix<T>& a){
            size_t size = a.num_rows();
            T* ad = current(a);
            T* ar = updated(a);
            for(size_t i = 0; i < size; ++i) ar[i] = std::sqrt(ad[i]);
        }
            
        template<typename T>
        void exp_diagonal(matrix<T>& a, const T& alfa){
            size_t size = a.num_rows();
            T* ad = current(a);
            T* ar = updated(a);
            for(size_t i = 0; i < size; ++i) ar[i] = std::exp(alfa*ad[i]);
        }
       
        template<typename T, class A>
        void transpose_out(const matrix<T,A>& a, unbound< matrix<T,A> >& t){
            T* od = current(a);
            T* td = updated(t);
            int m = a.num_rows();
            int n = a.num_cols();
       
            for(int i = 0; i < m; i++){
                for(int j = 0; j < n; j++) *td++ = od[j*m];
                od++;
            }
        }
       
        template<typename T, class A>
        void conj_inplace(matrix<T,A>& a){
            size_t size = a.num_rows()*a.num_cols();
            T* ad = current(a);
            T* ar = updated(a);
            for(int i=0; i < size; ++i)
                ar[i] = helper_complex<T>::conj(ad[i]);   
        }
       
        template<typename T, class A>
        void resize(unbound< matrix<T,A> >& r, const matrix<T,A>& a, const size_t& m, const size_t& n){
            T* dd = m*n == ambient::square_dim(r) ? updated(r) : emptied(r);
            ambient::memptf<T, ambient::memcpy>(dd, r.num_rows(), dim2(0,0),
                                                current(a), a.num_rows(), dim2(0,0), dim2(n, m)); 
        }
            
        template<typename T>
        void init_identity(unbound< matrix<T> >& a){
            size_t n = a.num_cols();
            size_t m = a.num_rows();
            T* ad = emptied(a);
       
            size_t sizex = std::min(m,n); // respecting borders
            for(size_t jj = 0; jj < sizex; ++jj) ad[jj + m*jj] = 1.;
        }
           
        inline void randomize(double& a){ 
            a = drand48();
        }
       
        inline void randomize(std::complex<double>& a){
            a.real(drand48());
            a.imag(drand48());
        }
       
        template<typename T>
        void init_random(unbound< matrix<T> >& a){
            size_t size = ambient::square_dim(a);
            T* ad = updated(a);
            for(size_t i = 0; i < size; ++i) randomize(ad[i]);
        }
            
        template<typename T, class A>
        void init_value(unbound< matrix<T,A> >& a, const T& value){
            size_t size = ambient::square_dim(a);
            T* ad = updated(a);
            for(size_t i = 0; i < size; ++i) ad[i] = value; // not a memset due to complex
        }
            
        template<typename T>
        void round_square(const matrix<T>& a, std::vector<T>*& ac){
            T* ad = current(a);
            size_t sizey = a.num_rows();
            for(int i=0; i < sizey; i++){
                double v = std::abs(ad[i]);
                if(v > 1e-10) ac->push_back(v*v);
            }
        }
       
        template<typename T>
        void cast_to_vector(std::vector<T>*& ac, const matrix<T>& a, const size_t& m, const size_t& n, const size_t& lda, const size_t& offset){
            T* ad = current(a);
            for(int j=0; j < n; ++j) std::memcpy((void*)&(*ac)[j*lda + offset],(void*)&ad[j*m], m*sizeof(T));  
        }
            
        template<typename T>
        void cast_from_vector(const std::vector<T>*& ac, matrix<T>& a, const size_t& m, const size_t& n, const size_t& lda, const size_t& offset){
            T* ad = updated(a);
            for(int j=0; j < n; ++j) std::memcpy((void*)&ad[j*m],(void*)&(*ac)[offset + j*lda], m*sizeof(T));
        }
       
        template<typename T1, typename T2>
        void cast_from_vector_t(const std::vector<T1>*& ac, matrix<T2>& a, const size_t& m, const size_t& n, const size_t& lda, const size_t& offset){
            T2* ad = updated(a);
            const T1* sd = &(*ac)[offset];
            for(int j=0; j < n; ++j) 
                for(int i=0; i < m; ++i)
                    ad[j*m + i] = sd[j*lda + i];
        }
       
        template<typename T, typename D>
        void cast_double_complex(matrix<T>& a, const matrix<D>& b){
            T* ad = updated(a);
            D* bd = current(b);
            size_t size = a.num_rows();
            for(size_t i = 0; i < size; ++i)
                ad[i] = helper_cast<T,D>::cast(bd[i]);
        };
       
        template<typename T, class A>
        void touch(const matrix<T,A>& a){ }
       
        template<typename T, class A>
        void migrate(matrix<T,A>& a){ revised(a); }
       
        inline double distance(const std::complex<double>& a, const std::complex<double>& b){ 
            return fabs(std::norm(a) - std::norm(b));
        }
        inline double magnitude(const std::complex<double>& a, const std::complex<double>& b){
            return std::max(fabs(std::norm(a)), fabs(std::norm(b)));
        }
        inline double distance(double a, double b) { 
            return fabs(fabs(a) - fabs(b));    
        }
        inline double magnitude(double a, double b){ 
            return std::max(fabs(a), fabs(b)); 
        }
        template<typename T>
        void validation(const matrix<T>& a, const matrix<T>& b, future<bool>& ret){ // see paper for Reference Dongara 
            T* ad = current(a); 
            T* bd = current(b); 
            double epsilon = std::numeric_limits<double>::epsilon();
            int count = 0;
            size_t sizey = std::min(a.num_rows(), b.num_rows());
            size_t sizex = std::min(a.num_cols(), b.num_cols());
            
            std::cout.precision(16);
            std::cout.setf( std::ios::fixed, std:: ios::floatfield );
       
            for(size_t i=0; i < sizey; ++i){
                for(size_t j=0; j < sizex; ++j){
                    T av = ad[i+j*a.num_rows()];
                    T bv = bd[i+j*b.num_rows()];
                    double d = distance(av, bv);
                    double m = magnitude(av, bv);
                    if(d > epsilon*256 && d/m > epsilon*256){ // || av*bv < 0 // 16 is recommended, 256 because MKL isn't bitwise stable
                        std::cout << i << " " << j << " : " << av << " " << bv << ", eps: " << d << "\n";
                        ret.get_naked() = false;
                        if(++count > 10) return;
                    }
       
                }
            }
        }
       
        template<typename T>
        void svd(const matrix<T>& a, unbound< matrix<T> >& u, unbound< matrix<T> >& vt, unbound< matrix<typename real_type<T>::type> >& s){
            int m = a.num_rows();
            int n = a.num_cols();
            int k = std::min(m,n);
            int info;
            int lwork = -1;
            T wkopt;
            T* ad  = current(a);
            T* ud  = updated(u);
            T* vtd = updated(vt);
            typename real_type<T>::type* sd  = updated(s);
            helper_lapack<T>::gesvd( "S", "S", &m, &n, ad, &m, sd, ud, &m, vtd, &k, &wkopt, &lwork, &info );
        }
       
        template<typename T>
        void geev(const matrix<T>& a, unbound< matrix<T> >& lv, unbound< matrix<T> >& rv, unbound< matrix<T> >& s){
            int n = a.num_cols();
            int info;
            int lwork = -1;
            T wkopt;
            T* ad  = current(a);
            T* lvd = updated(lv);
            T* rvd = updated(rv);
            T* sd  = updated(s);
            helper_lapack<T>::geev("N", "V", &n, ad, &n, sd, lvd, &n, rvd, &n, &wkopt, &lwork, &info); 
        }
       
        template<typename T>
        void inverse(matrix<T> & a){
            int info;
            int m = a.num_rows();
            int n = a.num_cols();
            T* ad = (T*)revised(a);    
            int* ipivd = new int[n];
            helper_lapack<T>::getrf(&m, &n, ad, &m, ipivd, &info);
            helper_lapack<T>::getri(&n, ad, &n, ipivd, &info);
            delete [] ipivd;
        }
       
        template<typename T>
        void heev(matrix<T>& a, unbound< matrix<typename real_type<T>::type> >& w){
            int m = a.num_rows();
            int info, lwork = -1;
            T wkopt;
            T* work;
            T* ad = (T*)std::malloc(ambient::size(a));
            typename real_type<T>::type* wd = (typename real_type<T>::type*)std::malloc(ambient::size(w));
            std::memcpy(ad, (T*)current(a), ambient::size(a));
            std::memcpy(wd, (typename real_type<T>::type*)current(w), ambient::size(w));
       
            helper_lapack<T>::syev("V","U",&m,ad,&m,wd,&wkopt,&lwork,&info);
       
            typename real_type<T>::type s;
            for(int i=0; i < (int)(m/2); i++){
                s = wd[i];
                wd[i] = wd[m-i-1];
                wd[m-i-1] = s;
            } 
            // reversing eigenvectors
            size_t len = m*sizeof(T);
            work = (T*)std::malloc(len);
            for (int i=0; i < (int)(m/2); i++){
                std::memcpy(work, &ad[i*m], len);
                std::memcpy(&ad[i*m], &ad[(m-1-i)*m], len);
                std::memcpy(&ad[(m-1-i)*m], work, len);
            }
            std::free(work);
            std::memcpy((T*)updated(a), ad, ambient::size(a)); std::free(ad);
            std::memcpy((typename real_type<T>::type*)updated(w), wd, ambient::size(w));
            std::free(wd);
        }
    
    }

    ambient_reg(detail::geqrt, geqrt)
    ambient_reg(detail::ormqr, ormqr)
    ambient_reg(detail::tsqrt, tsqrt)
    ambient_reg(detail::tsmqr, tsmqr)
    ambient_reg(detail::gelqt, gelqt)
    ambient_reg(detail::ormlq, ormlq)
    ambient_reg(detail::tslqt, tslqt)
    ambient_reg(detail::tsmlq, tsmlq)
    ambient_reg(detail::gemm,  gemm)
    ambient_reg(detail::gemm_diagonal_lhs, gemm_diagonal_lhs)
    ambient_reg(detail::gemm_diagonal_rhs, gemm_diagonal_rhs)
    ambient_reg(detail::trace, trace)
    ambient_reg(detail::scalar_norm, scalar_norm)
    ambient_reg(detail::overlap, overlap)
    ambient_reg(detail::add, add)
    ambient_reg(detail::sub, sub)
    ambient_reg(detail::scale, scale)
    ambient_reg(detail::scale_offset, scale_offset)
    ambient_reg(detail::scale_inverse, scale_inverse)
    ambient_reg(detail::sqrt_diagonal, sqrt_diagonal)
    ambient_reg(detail::exp_diagonal, exp_diagonal)
    ambient_reg(detail::transpose_out,transpose_out)
    ambient_reg(detail::conj_inplace, conj_inplace)
    ambient_reg(detail::resize, resize)
    ambient_reg(detail::init_identity, init_identity)
    ambient_reg(detail::init_value, init_value)
    ambient_reg(detail::round_square, round_square)
    ambient_reg(detail::cast_to_vector, cast_to_vector)
    ambient_reg(detail::cast_from_vector, cast_from_vector)
    ambient_reg(detail::cast_from_vector_t, cast_from_vector_t)
    ambient_reg(detail::cast_double_complex, cast_double_complex)
    ambient_reg(detail::touch, touch)
    ambient_reg(detail::migrate, migrate)
    ambient_reg(detail::svd, svd)
    ambient_reg(detail::geev, geev)
    ambient_reg(detail::inverse, inverse)
    ambient_reg(detail::heev, heev)
    ambient_reg(detail::copy_rt, copy_rt)
    ambient_reg(detail::copy_lt, copy_lt)
    ambient_reg(detail::copy_block, copy_block)
    ambient_reg(detail::copy_block_s, copy_block_s)
    ambient_reg(detail::copy_block_sa, copy_block_sa)
    ambient_reg(detail::init_random, init_random)
    ambient_reg(detail::validation, validation)

} } }

#endif
