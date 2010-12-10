#ifndef MPSTENSOR_H
#define MPSTENSOR_H

#include "block_matrix.h"
#include "indexing.h"

#include <iostream>

enum MPSStorageLayout { LeftPaired, RightPaired };
// these are actually used in several places
enum Indicator { U, L, R };
enum DecompMethod { QR, SVD };

template<class Matrix, class SymmGroup>
class MPSTensor
{
public:
    typedef typename Matrix::value_type scalar_type;
    typedef double real_type;
    
    MPSTensor(Index<SymmGroup> const & sd = Index<SymmGroup>(),
              Index<SymmGroup> const & ld = Index<SymmGroup>(),
              Index<SymmGroup> const & rd = Index<SymmGroup>(),
              bool fillrand = true);
    
    Index<SymmGroup> const & site_dim() const;
    Index<SymmGroup> const & row_dim() const;
    Index<SymmGroup> const & col_dim() const;
    bool isobccompatible(Indicator) const;
    
    // these are not const because after a numerical test
    // they may update the status
    bool isleftnormalized(bool test = false);
    bool isrightnormalized(bool test = false);
    bool isnormalized(bool test = false);
    
    block_matrix<Matrix, SymmGroup> normalize_left(DecompMethod method = QR,
                                                   bool multiplied = true,
                                                   double truncation = 0,
                                                   Index<SymmGroup> bond_dim = Index<SymmGroup>());
    block_matrix<Matrix, SymmGroup> normalize_right(DecompMethod method = QR,
                                                    bool multiplied = true,
                                                    double truncation = 0,
                                                    Index<SymmGroup> bond_dim = Index<SymmGroup>());
    
    void multiply_from_left(block_matrix<Matrix, SymmGroup> const &);
    void multiply_from_right(block_matrix<Matrix, SymmGroup> const &);
    void multiply_by_scalar(scalar_type);
    
    scalar_type scalar_overlap(MPSTensor&);
    real_type scalar_norm();
    
    // this is completely useless in C++, only exists for consistency with Python
    MPSTensor copy() const;
    
    block_matrix<Matrix, SymmGroup> & data();
    block_matrix<Matrix, SymmGroup> const & data() const;
    
    template<class Matrix_, class SymmGroup_>
    friend std::ostream& operator<<(std::ostream&, MPSTensor<Matrix_, SymmGroup_> const &);
    
    friend struct contraction;
    
private:
    Index<SymmGroup> phys_i, left_i, right_i;
    mutable block_matrix<Matrix, SymmGroup> data_;
    mutable MPSStorageLayout cur_storage;
    Indicator cur_normalization;
    
    MPSTensor get_reflected() const;
    
public:
    void make_left_paired() const;
    void make_right_paired() const;
};

#include "mpstensor.hpp"

#endif
