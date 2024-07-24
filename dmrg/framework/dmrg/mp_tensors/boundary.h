/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef BOUNDARY_H
#define BOUNDARY_H

#include "dmrg/utils/storage.h"
#include "dmrg/block_matrix/block_matrix.h"
#include "dmrg/block_matrix/indexing.h"
#include "utils/function_objects.h"
#include "dmrg/utils/parallel.hpp"

#include <iostream>
#include <set>

template<class Matrix, class SymmGroup>
class Boundary : public storage::disk::serializable<Boundary<Matrix, SymmGroup> >
{
public:
    typedef typename maquis::traits::scalar_type<Matrix>::type scalar_type;
    typedef typename Matrix::value_type value_type;
    typedef std::pair<typename SymmGroup::charge, std::size_t> access_type;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & data_;
    }
    
    Boundary(Index<SymmGroup> const & ud = Index<SymmGroup>(),
             Index<SymmGroup> const & ld = Index<SymmGroup>(),
             std::size_t ad = 1)
    : data_(ad, block_matrix<Matrix, SymmGroup>(ud, ld))
    { }
    
    template <class OtherMatrix>
    Boundary(Boundary<OtherMatrix, SymmGroup> const& rhs)
    {
        data_.reserve(rhs.aux_dim());
        for (std::size_t n=0; n<rhs.aux_dim(); ++n)
            data_.push_back(rhs[n]);
    }

    std::size_t aux_dim() const { 
        return data_.size(); 
    }

    void resize(size_t n){
        if(n < data_.size()) 
            return data_.resize(n);
        data_.reserve(n);
        for(int i = data_.size(); i < n; ++i)
            data_.push_back(block_matrix<Matrix, SymmGroup>());
    }
    
    std::vector<scalar_type> traces() const {
        std::vector<scalar_type> ret; ret.reserve(data_.size());
        for (size_t k=0; k < data_.size(); ++k) ret.push_back(data_[k].trace());
        return ret;
    }

    bool reasonable() const {
        for(size_t i = 0; i < data_.size(); ++i)
            if(!data_[i].reasonable()) return false;
        return true;
    }
   
    template<class Archive> 
    void load(Archive & ar){
        std::vector<std::string> children = ar.list_children("data");
        data_.resize(children.size());
        parallel::scheduler_balanced scheduler(children.size());
        for(size_t i = 0; i < children.size(); ++i){
             parallel::guard proc(scheduler(i));
             ar["data/"+children[i]] >> data_[alps::cast<std::size_t>(children[i])];
        }
    }
    
    template<class Archive> 
    void save(Archive & ar) const {
        ar["data"] << data_;
    }

    Boundary const & operator+=(Boundary const & rhs)
    {
        assert (this->data_.size() == rhs.data_.size()) ;
        for(size_t i = 0; i < this->data_.size(); ++i)
            this->data_[i] += rhs.data_[i] ;
        return *this;
    };
    //
    Boundary const & operator-=(Boundary const & rhs)
    {
        assert (this->data_.size() == rhs.data_.size()) ;
        for(size_t i = 0; i < this->data_.size(); ++i)
            this->data_[i] -= rhs.data_[i] ;
        return *this;
    };
    //
    Boundary const & operator*=(scalar_type const & rhs)
    {
        for(size_t i = 0; i < this->data_.size(); ++i)
            this->data_[i] *= rhs ;
        return *this;
    };
    //
    Boundary const & operator/=(scalar_type const & rhs)
    {
        for(size_t i = 0; i < this->data_.size(); ++i)
            this->data_[i] /= rhs ;
        return *this;
    };
    //
    friend Boundary operator*(scalar_type const & rhs, const Boundary& b_rhs)
    {
        Boundary res(b_rhs);
        for(size_t i = 0; i < res.data_.size(); ++i)
            res.data_[i] *= rhs ;
        return res;
    };
    //
    friend Boundary operator/(scalar_type const & rhs, const Boundary& b_rhs)
    {
      Boundary res(b_rhs);
      for(size_t i = 0; i < res.data_.size(); ++i)
            res.data_[i] /= rhs ;
      return res;
    };

    void print() const
    {
      for (auto x : data_)
        std::cout << x << std::endl;
    };
    
    block_matrix<Matrix, SymmGroup> & operator[](std::size_t k) { return data_[k]; }
    block_matrix<Matrix, SymmGroup> const & operator[](std::size_t k) const { return data_[k]; }
    //value_type & operator()(std::size_t i, access_type j, access_type k) { return data_[i](j, k); } // I hope this is never used (30.04.2012 / scalar/value discussion)
    //value_type const & operator()(std::size_t i, access_type j, access_type k) const { return data_[i](j, k); }
private:
    std::vector<block_matrix<Matrix, SymmGroup> > data_;
};


template<class Matrix, class SymmGroup>
Boundary<Matrix, SymmGroup> simplify(Boundary<Matrix, SymmGroup> b)
{
    typedef typename alps::numeric::associated_real_diagonal_matrix<Matrix>::type dmt;
    
    for (std::size_t k = 0; k < b.aux_dim(); ++k)
    {
        block_matrix<Matrix, SymmGroup> U, V, t;
        block_matrix<dmt, SymmGroup> S;
        
        if (b[k].basis().sum_of_left_sizes() == 0)
            continue;
        
        svd_truncate(b[k], U, V, S, 1e-4, 1, false);
        
        gemm(U, S, t);
        gemm(t, V, b[k]);
    }
    
    return b;
}

template<class Matrix, class SymmGroup>
std::size_t size_of(Boundary<Matrix, SymmGroup> const & m)
{
    size_t r = 0;
    for (size_t i = 0; i < m.aux_dim(); ++i)
        r += size_of(m[i]);
    return r;
}


#endif
