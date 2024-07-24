/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

/* $Id: jacobi.h,v 1.6 2003/09/05 08:12:38 troyer Exp $ */

#ifndef IETL_JACOBI_H
#define IETL_JACOBI_H

#include <ietl/traits.h>
#include <ietl/fmatrix.h>
#include <ietl/ietl2lapack.h> 
 
#include <ietl/cg.h>
#include <ietl/gmres.h>

#include <complex>
#include <vector>

#include <boost/function.hpp>

namespace ietl
{
    enum DesiredEigenvalue { Largest, Smallest };
    
    template <class MATRIX, class VS>
    class jcd_left_preconditioner
    {
    public:
        typedef typename vectorspace_traits<VS>::vector_type vector_type;
        typedef typename vectorspace_traits<VS>::scalar_type scalar_type;
        typedef typename ietl::number_traits<scalar_type>::magnitude_type magnitude_type;
        
        jcd_left_preconditioner(const MATRIX& matrix, const VS& vec, const int& max_iter);
        void operator()(const vector_type& u, const magnitude_type& theta, const vector_type& r, vector_type& t, const magnitude_type& rel_tol);
        
    private:
        void sysv(const char& uplo, fortran_int_t n, fortran_int_t nrhs, double a[], fortran_int_t lda, fortran_int_t ipiv[], double b[], fortran_int_t ldb, double work[], fortran_int_t lwork, fortran_int_t& info)
        { LAPACK_DSYSV(&uplo, &n, &nrhs, a, &lda, ipiv, b, &ldb, work, &lwork, &info); };
        void sysv(const char& uplo, fortran_int_t n, fortran_int_t nrhs, std::complex<double> a[], fortran_int_t lda, fortran_int_t ipiv[], std::complex<double> b[], fortran_int_t ldb, std::complex<double> work[], fortran_int_t lwork, fortran_int_t& info)
        { LAPACK_ZHESV(&uplo, &n, &nrhs, a, &lda, ipiv, b, &ldb, work, &lwork, &info); };
        MATRIX K;
        VS vecspace_;
        int n_;
        int max_iter_;
    };
    
    template <class MATRIX, class VS>
    class jcd_simple_solver
    {
    public:
        typedef typename vectorspace_traits<VS>::vector_type vector_type;
        typedef typename vectorspace_traits<VS>::scalar_type scalar_type;
        typedef typename ietl::number_traits<scalar_type>::magnitude_type magnitude_type;
        
        jcd_simple_solver(const MATRIX& matrix, const VS& vec);
        void operator()(const vector_type& u, const magnitude_type& theta, const vector_type& r, vector_type& t, const magnitude_type& rel_tol);
        
    private:
        MATRIX matrix_;
        VS vecspace_;
        int n_;
    };
    
    template<class Matrix, class VS, class Vector>
    class jcd_solver_operator
    {
    public:
        typedef typename vectorspace_traits<VS>::vector_type vector_type;
        typedef typename vectorspace_traits<VS>::scalar_type scalar_type;
        typedef typename ietl::number_traits<scalar_type>::magnitude_type magnitude_type;
        
        jcd_solver_operator(const vector_type& u,
            const magnitude_type& theta,
            const vector_type& r,
            const Matrix & m)
        : u_(u), r_(r), theta_(theta), m_(m) { }
        
        void operator()(vector_type const & x, vector_type & y) const;
        
    private:
        vector_type const & u_, r_;
        magnitude_type const & theta_;
        Matrix const & m_;
    };
    
    template<class Matrix, class VS, class Vector>
    void mult(jcd_solver_operator<Matrix, VS, Vector> const & m,
        typename jcd_solver_operator<Matrix, VS, Vector>::vector_type const & x,
        typename jcd_solver_operator<Matrix, VS, Vector>::vector_type & y)
    {
        m(x,y);
    }
    
    // kept for backward compatibility
    template<class Matrix, class VS>
    class jcd_gmres_solver
    {
    public:
        typedef typename vectorspace_traits<VS>::vector_type vector_type;
        typedef typename vectorspace_traits<VS>::scalar_type scalar_type;
        typedef typename ietl::number_traits<scalar_type>::magnitude_type magnitude_type;
        
        jcd_gmres_solver(Matrix const & matrix, VS const & vec,
            std::size_t max_iter = 5, bool verbose = false)
        : matrix_(matrix)
        , vecspace_(vec)
        , n_(vec_dimension(vec))
        , max_iter_(max_iter)
        , verbose_(verbose) { }
        
        void operator()(const vector_type& u,
                        const magnitude_type& theta,
                        const vector_type& r, vector_type& t,
                        const magnitude_type& rel_tol)
        {
            jcd_solver_operator<Matrix, VS, vector_type> op(u, theta, r, matrix_);
            ietl_gmres gmres(max_iter_, verbose_);
            
            vector_type inh = -r;
            
            // initial guess for better convergence
            scalar_type dru = ietl::dot(r,u);
            scalar_type duu = ietl::dot(u,u);
            t = -r + dru/duu*u;
            if (max_iter_ > 0)
                t = gmres(op, inh, t, rel_tol);
        }
        
    private:
        Matrix const & matrix_;
        VS vecspace_;
        std::size_t n_, max_iter_;
        bool verbose_;
    };
    
    template<class Matrix, class VS>
    class jcd_solver
    {
    public:
        typedef typename vectorspace_traits<VS>::vector_type vector_type;
        typedef typename vectorspace_traits<VS>::scalar_type scalar_type;
        typedef typename ietl::number_traits<scalar_type>::magnitude_type magnitude_type;
        
        template<class Solver>
        jcd_solver(Matrix const & matrix,
                   VS const & vec,
                   Solver solv)
        : matrix_(matrix)
        , vecspace_(vec)
        , solv_(solv)
        , n_(vec_dimension(vec)) { }
        
        template<class Solver>
        void replace_solver(Solver solv) { solv_ = solv; }
        
        void operator()(const vector_type& u,
                        const magnitude_type& theta,
                        const vector_type& r, vector_type& t,
                        const magnitude_type& rel_tol)
        {
            jcd_solver_operator<Matrix, VS, vector_type> op(u, theta, r, matrix_);
            
            vector_type inh = -r;
            
            // initial guess for better convergence
            scalar_type dru = ietl::dot(r,u);
            scalar_type duu = ietl::dot(u,u);
            t = -r + dru/duu*u;
            if (max_iter_ > 0)
                t = solv_(op, inh, t, rel_tol);
        }
        
    private:
        Matrix const & matrix_;
        VS vecspace_;
        boost::function<vector_type(jcd_solver_operator<Matrix, VS, vector_type> const &, vector_type const &, vector_type const &, double)> solv_;
        std::size_t n_, max_iter_;
        bool verbose_;
    };
    
    template <class MATRIX, class VS>
    class jacobi_davidson
    {
    public:
        typedef typename vectorspace_traits<VS>::vector_type vector_type;
        typedef typename vectorspace_traits<VS>::scalar_type scalar_type;
        typedef typename ietl::number_traits<scalar_type>::magnitude_type magnitude_type;
        
        
        jacobi_davidson(const MATRIX& matrix, 
                        const VS& vec,
                        DesiredEigenvalue desired = Largest);
        ~jacobi_davidson();
        
        template <class GEN, class SOLVER, class ITER>
        std::pair<magnitude_type, vector_type> calculate_eigenvalue(const GEN& gen, 
                                                                    SOLVER& solver,
                                                                    ITER& iter);
        
    private:
        void get_extremal_eigenvalue(magnitude_type& theta, std::vector<double>& s, fortran_int_t dim);
        void get_extremal_eigenvalue(magnitude_type& theta, std::vector<float>& s, fortran_int_t dim);
        void get_extremal_eigenvalue(magnitude_type& theta, std::vector<std::complex<double> >& s, fortran_int_t dim);
        void get_extremal_eigenvalue(magnitude_type& theta, std::vector<std::complex<float> >& s, fortran_int_t dim);

        MATRIX const & matrix_;
        VS vecspace_;
        int n_;
        FortranMatrix<scalar_type> M;
        magnitude_type atol_;
        DesiredEigenvalue desired_;
    };
    
    template <class MATRIX, class VS>
    jcd_simple_solver<MATRIX, VS>::jcd_simple_solver(const MATRIX& matrix, const VS& vec) :
    matrix_(matrix),
    vecspace_(vec)
    {
        n_ = vec_dimension(vecspace_);
    }
    
    template <class MATRIX, class VS>
    void jcd_simple_solver<MATRIX, VS>::operator()(const vector_type& u, const magnitude_type& theta, const vector_type& r, vector_type& t, const magnitude_type& rel_tol)
    {
       //for (int i=0;i<n_;i++)
         //  t[i] = -r[i] / ( matrix_(i,i) - theta );
        // std::cout << "Preconditioner, theta = " << theta << std::endl;
        // t = -1*r / (1-theta);
        t = -r + ietl::dot(r,u)/ietl::dot(u,u)*u;
    }
    
    template<class Matrix, class VS, class Vector>
    void jcd_solver_operator<Matrix, VS, Vector>::operator()(vector_type const & x, vector_type & y) const
    {
        // calculate (1-uu*)(A-theta*1)(1-uu*)
        
        vector_type t, t2, t3;
        // t2 = (1-uu*) x
        scalar_type ust = dot(u_, x);
        t2 = x - ust*u_;
        
        // y = (A-theta*1) t2
        mult(m_, t2, t3);
        y = t3 - theta_*t2;
        
        // t = (1-uu*) y
        ust = dot(u_, y);
        t = y - ust*u_;
        
        y = t;
    }
    
    template <class MATRIX, class VS>
    jcd_left_preconditioner<MATRIX, VS>::jcd_left_preconditioner(const MATRIX& matrix, const VS& vec, const int& max_iter) :
    K(matrix),
    vecspace_(vec),
    max_iter_(max_iter)
    {
        n_ = vecspace_.vec_dimension();
    }
    
    template <class MATRIX, class VS>
    void jcd_left_preconditioner<MATRIX, VS>::operator()(const vector_type& u, const magnitude_type& theta, const vector_type& r, vector_type& t, const magnitude_type& rel_tol)
    {
        // define variables
        FortranMatrix<scalar_type> Ktmp(n_,n_);
        vector_type u_hat = new_vector(vecspace_);
        vector_type vec1  = new_vector(vecspace_);
        vector_type vec2  = new_vector(vecspace_);
        magnitude_type mu, norm;
        
        // initialize variables
        for (int i=0;i<n_;i++) for (int j=0;j<n_;j++)
            Ktmp(i,j) = K(i,j);
        for (int i=0;i<n_;i++)
            Ktmp(i,i) -= theta;
        
        // define variables for LAPACK
        char uplo='U';
        fortran_int_t n=n_;
        fortran_int_t nrhs=1;
        fortran_int_t lda=n;
        fortran_int_t ldb=n;
        fortran_int_t lwork=n*n;
        fortran_int_t info;
        
        fortran_int_t *ipiv;
        ipiv = new fortran_int_t[n];
        scalar_type *work; 
        work = new scalar_type[lwork]; 
        
        // Solve u_hat from K*u_hat = u,  mu = u^star * u_hat
        ietl::copy(u,u_hat);
        sysv(uplo, n, nrhs, Ktmp.data(), lda, ipiv, u_hat.data(), ldb, work, lwork, info);
        mu = std::real(ietl::dot(u,u_hat));
        
        // compute r_tilde = K_tilde^{-1} * r as
        //   solve r_hat from K*r_hat = r
        //   r_tilde = r_hat - \frac{u^\star r_hat}{mu} u_hat
        ietl::copy(r,vec1);
        for (int i=0;i<n_;i++)
            Ktmp(i,i) = K(i,i);
        uplo='L';
        sysv(uplo, n, nrhs, Ktmp.data(), lda, ipiv, vec1.data(), ldb, work, lwork, info);
        vec1 -= ietl::dot(u,vec1)/mu*u_hat;
        vec1*=-1.;
        
        // aplly a Krylov subspace method with t_0=0; operator K_tilde ^{-1} A_tilde
        // and right hand side -r_tilde; given v, z=K_tilde ^{-1} A_tilde v is computed as
        //   y = (A-\theta I)v
        //   solve y_hat from K y_hat = y
        for (int i=0; i<max_iter_; i++) {
            t = vec1/ietl::two_norm(vec1);
            ietl::mult(K,t,vec1);
            for (int j=0;j<n_;j++) for (int k=0;k<n_;k++)
                Ktmp(j,k) = K(j,k);
            for (int j=0;j<n_;j++)
                Ktmp(j,j) -= theta;
            sysv(uplo, n, nrhs, Ktmp.data(), lda, ipiv, vec1.data(), ldb, work, lwork, info);
            vec1 -= -ietl::dot(u,vec1)/mu * u_hat;
            norm = ietl::dot(t,vec1);
            vec2 = vec1-norm*t;
            if ( ietl::two_norm(vec2) < rel_tol * std::abs(norm) )
                break;
        }
        
        delete [] ipiv;
        delete [] work;
    }
    
    // C L A S S :   J A C O B I _ D A V I D S O N ////////////////////////////////////
    
    template <class MATRIX, class VS>
    
    jacobi_davidson<MATRIX, VS>::jacobi_davidson(const MATRIX& matrix, const VS& vec, DesiredEigenvalue desired) :
    matrix_(matrix),
    vecspace_(vec),
    M(1,1),
    desired_(desired)
    {
        //      n_ = vecspace_.vec_dimension();
        n_ = vec_dimension(vecspace_);
    }
    
    template <class MATRIX, class VS>
    jacobi_davidson<MATRIX, VS>::~jacobi_davidson()
    {
        
    }
    
//    template<class Vector>
//    Vector orthogonalize(Vector input, std::vector<Vector> const & against)
//    {
//        
//    }
    
    template <class MATRIX, class VS> 
    template <class GEN, class SOLVER, class ITER>
    std::pair<typename jacobi_davidson<MATRIX,VS>::magnitude_type, typename jacobi_davidson<MATRIX,VS>::vector_type> 
    jacobi_davidson<MATRIX, VS>::calculate_eigenvalue(const GEN& gen,
                                                      SOLVER& solver,
                                                      ITER& iter)
    {
        std::vector<scalar_type> s(iter.max_iterations());
        std::vector<vector_type> V(iter.max_iterations());
        std::vector<vector_type> VA(iter.max_iterations());
        M.resize(iter.max_iterations(), iter.max_iterations());
        magnitude_type theta, tau, rel_tol;
        magnitude_type kappa = 0.25;
        atol_ = iter.absolute_tolerance();
        
        // Start with t=v_o, starting guess
        ietl::generate(V[0],gen); const_cast<GEN&>(gen).clear();
        ietl::project(V[0], vecspace_);
        
        // Start iteration
        do {
            vector_type& t = V[iter.iterations()];

            // Modified Gram-Schmidt Orthogonalization with Refinement
            tau = ietl::two_norm(t);
            for(int i = 1; i <= iter.iterations(); i++)
                t -= ietl::dot(V[i-1],t)*V[i-1];
            if(ietl::two_norm(t) < kappa * tau)
                for(int i = 1; i <= iter.iterations(); i++)
                    t -= ietl::dot(V[i-1],t) * V[i-1];
            
            // Project out orthogonal subspace
            ietl::project(t,vecspace_);
            
            // v_m = t / |t|_2,  v_m^A = A v_m
            t /= ietl::two_norm(t);
            ietl::mult(matrix_, t, VA[iter.iterations()]);
            
            // for i=1, ..., iter
            //   M_{i,m} = v_i ^\star v_m ^A
            for(int i = 1; i <= iter.iterations()+1; i++)
                M(i-1,iter.iterations()) = ietl::dot(V[i-1], VA[iter.iterations()]);
            
            // compute the largest eigenpair (\theta, s) of M (|s|_2 = 1)
            get_extremal_eigenvalue(theta,s,iter.iterations()+1);
            
            // u = V s
            #ifdef USE_AMBIENT
            std::vector<vector_type> u_parts; u_parts.reserve(iter.iterations()+1);
            for(int j = 0; j <= iter.iterations(); ++j) u_parts.push_back(V[j] * s[j]);
            vector_type u = ambient::reduce_sync(u_parts, [](vector_type& dst, vector_type& src){ dst += src; src.clear(); });
            std::vector<vector_type>().swap(u_parts);
            #else
            vector_type u = V[0] * s[0];
            for(int j = 1; j <= iter.iterations(); ++j)
                u += V[j] * s[j];
            #endif

            // u^A = V^A s
            // ietl::mult(matrix_,u,uA);
            #ifdef USE_AMBIENT
            std::vector<vector_type> uA_parts; uA_parts.reserve(iter.iterations()+1);
            for(int j = 0; j <= iter.iterations(); ++j) uA_parts.push_back(VA[j] * s[j]);
            vector_type uA = ambient::reduce_sync(uA_parts, [](vector_type& dst, vector_type& src){ dst += src; src.clear(); });
            std::vector<vector_type>().swap(uA_parts);
            #else
            vector_type uA = VA[0] * s[0];
            for(int j = 1; j <= iter.iterations(); ++j)
                uA += VA[j] * s[j];
            #endif

            ietl::project(uA,vecspace_);
            
            // r = u^A - \theta u
            vector_type& r = uA; r -= theta*u;
            
            // if (|r|_2 < \epsilon) stop
            ++iter;
            // accept lambda=theta and x=u
            if(iter.finished(ietl::two_norm(r),theta)) return std::make_pair(theta, u);
            
            // solve (approximately) a t orthogonal to u from
            //   (I-uu^\star)(A-\theta I)(I- uu^\star)t = -r
            rel_tol = 1. / pow(2.,double(iter.iterations()+1));
            solver(u, theta, r, V[iter.iterations()], rel_tol);

            V[iter.iterations()].data().iter_index = VA[iter.iterations()-1].data().iter_index;
            storage::migrate(V[iter.iterations()], parallel::scheduler_balanced_iterative(V[iter.iterations()].data()));
        } while(true);
        
    }
    
    template <class MATRIX, class VS>
    void jacobi_davidson<MATRIX, VS>::get_extremal_eigenvalue(magnitude_type& theta, std::vector<double>& s, fortran_int_t dim)
    {
        FortranMatrix<scalar_type> M_(dim,dim);
        for (int i=0;i<dim;i++) for (int j=0;j<=i;j++)
            M_(j,i) = M(j,i);
        double abstol = atol_;
        char jobz='V';     char range='I';   char uplo='U';
        fortran_int_t n=dim;
        fortran_int_t lda=dim;       
        fortran_int_t il, iu;
        if (desired_ == Largest)
            il = iu = n;
        else
            il = iu = 1;

        fortran_int_t m;
        fortran_int_t ldz=n;
        fortran_int_t lwork=8*n;
        fortran_int_t info;

        double vl, vu;

        double *w = new double[n];
        double *z = new double[n];
        double *work = new double[lwork];
        fortran_int_t *iwork = new fortran_int_t[5*n];
        fortran_int_t *ifail = new fortran_int_t[n];

        LAPACK_DSYEVX(&jobz, &range, &uplo, &n, M_.data(), &lda, &vl, &vu, &il, &iu, &abstol, &m, w, z, &ldz, work, &lwork, iwork, ifail, &info);
        
        theta = w[0];
        for (int i=0;i<n;i++)
            s[i] = z[i];
        
        delete [] w;
        delete [] z;
        delete [] work;
        delete [] iwork;
        delete [] ifail;
    }
    
    template <class MATRIX, class VS>
    void jacobi_davidson<MATRIX, VS>::get_extremal_eigenvalue(magnitude_type& theta, std::vector<float>& s, fortran_int_t dim)
    {
        FortranMatrix<scalar_type> M_(dim,dim);
        for (int i=0;i<dim;i++) for (int j=0;j<=i;j++)
            M_(j,i) = M(j,i);
        float abstol = atol_;
        char jobz='V';     char range='I';   char uplo='U';
        fortran_int_t n=dim;
        fortran_int_t lda=dim;       
        fortran_int_t il, iu;
        if (desired_ == Largest)
            il = iu = n;
        else
            il = iu = 1;

        fortran_int_t m;
        fortran_int_t ldz=n;
        fortran_int_t lwork=8*n;
        fortran_int_t info;

        float vl, vu;

        float *w = new float[n];
        float *z = new float[n];
        float *work = new float[lwork];
        fortran_int_t *iwork = new fortran_int_t[5*n];
        fortran_int_t *ifail = new fortran_int_t[n];

        LAPACK_SSYEVX(&jobz, &range, &uplo, &n, M_.data(), &lda, &vl, &vu, &il, &iu, &abstol, &m, w, z, &ldz, work, &lwork, iwork, ifail, &info);
        
        theta = w[0];
        for (int i=0;i<n;i++)
            s[i] = z[i];
        
        delete [] w;
        delete [] z;
        delete [] work;
        delete [] iwork;
        delete [] ifail;
    }
        
    template <class MATRIX, class VS>
    void jacobi_davidson<MATRIX, VS>::get_extremal_eigenvalue(magnitude_type& theta, std::vector<std::complex<double> >& s, fortran_int_t dim)
    {
        FortranMatrix<scalar_type> M_(dim,dim);
        for (int i=0;i<dim;i++) for (int j=0;j<=i;j++)
            M_(j,i) = M(j,i);
        double abstol = atol_;
        char jobz='V';     char range='I';   char uplo='U';
        fortran_int_t n=dim;
        fortran_int_t lda=dim;
        fortran_int_t il, iu;
        if (desired_ == Largest)
            il = iu = n;
        else
            il = iu = 1;
        fortran_int_t m;
        fortran_int_t ldz=n;
        fortran_int_t lwork=8*n;
        fortran_int_t info; 
        double vl, vu;

        double * w = new double[n];
        std::complex<double> * z = new std::complex<double>[n];
        std::complex<double> * work = new std::complex<double>[lwork];
        fortran_int_t *iwork = new fortran_int_t[5*n];
        fortran_int_t *ifail = new fortran_int_t[n];
        double * rwork = new double[7*n];

        LAPACK_ZHEEVX(&jobz, &range, &uplo, &n, M_.data(), &lda, &vl, &vu, &il, &iu, &abstol, &m, w, z, &ldz, work, &lwork, rwork, iwork, ifail, &info);
            
        theta = w[0];
        for (int i=0;i<n;i++)
            s[i] = z[i];
        
        
        delete [] w;
        delete [] z;
        delete [] work;
        delete [] iwork;
        delete [] ifail;
        delete [] rwork;
    }
        
    template <class MATRIX, class VS>
    void jacobi_davidson<MATRIX, VS>::get_extremal_eigenvalue(magnitude_type& theta, std::vector<std::complex<float> >& s, fortran_int_t dim)
    {
        FortranMatrix<scalar_type> M_(dim,dim);
        for (int i=0;i<dim;i++) for (int j=0;j<=i;j++)
            M_(j,i) = M(j,i);
        float abstol = atol_;
        char jobz='V';     char range='I';   char uplo='U';
        fortran_int_t n=dim;
        fortran_int_t lda=dim;
        fortran_int_t il, iu;
        if (desired_ == Largest)
            il = iu = n;
        else
            il = iu = 1;
        fortran_int_t m;
        fortran_int_t ldz=n;
        fortran_int_t lwork=8*n;
        fortran_int_t info; 
        float vl, vu;

        float * w = new float[n];
        std::complex<float> * z = new std::complex<float>[n];
        std::complex<float> * work = new std::complex<float>[lwork];
        fortran_int_t *iwork = new fortran_int_t[5*n];
        fortran_int_t *ifail = new fortran_int_t[n];
        float * rwork = new float[7*n];

        LAPACK_CHEEVX(&jobz, &range, &uplo, &n, M_.data(), &lda, &vl, &vu, &il, &iu, &abstol, &m, w, z, &ldz, work, &lwork, rwork, iwork, ifail, &info);
            
        theta = w[0];
        for (int i=0;i<n;i++)
            s[i] = z[i];
        
        
        delete [] w;
        delete [] z;
        delete [] work;
        delete [] iwork;
        delete [] ifail;
        delete [] rwork;
    }
    
}
#endif
