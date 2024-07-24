/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#include "utils/function_objects.h"
#include "utils/bindings.hpp"

#include <boost/serialization/serialization.hpp>

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup>::SiteOperator()
{
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup>::SiteOperator(Index<SymmGroup> const & rows,
                                              Index<SymmGroup> const & cols) : bm_(rows, cols)
{
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup>::SiteOperator(DualIndex<SymmGroup> const & basis)
: bm_(basis)
{
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup>::SiteOperator(block_matrix<Matrix,SymmGroup> const& rhs,
                                              typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type const& sb)
: spin_basis(sb), bm_(rhs), sparse_op(rhs, sb)
{
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup> & SiteOperator<Matrix, SymmGroup>::operator=(SiteOperator rhs)
{
    swap(*this, rhs);
    return *this;
}

template<class Matrix, class SymmGroup>
template<class OtherMatrix>
SiteOperator<Matrix, SymmGroup> & SiteOperator<Matrix, SymmGroup>::operator=(const SiteOperator<OtherMatrix, SymmGroup> & rhs)
{
    block_matrix<Matrix, SymmGroup> cpy = rhs.bm_;
    sparse_op = rhs.sparse_op;
    spin_basis = rhs.spin_basis;
    swap(bm_, cpy);
    spin_ = rhs.spin();
    return *this;
}

namespace SiteOperator_detail
{

    template <class Matrix, class SymmGroup>
    symm_traits::disable_if_su2_t<SymmGroup>
    extend_spin_basis(typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type & spin_basis,
                      typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type const & rhs)
    {
    }

    template <class Matrix, class SymmGroup>
    symm_traits::enable_if_su2_t<SymmGroup>
    extend_spin_basis(typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type & spin_basis,
                      typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type const & rhs)
    {
        for (typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type::const_iterator it = rhs.begin(); it != rhs.end(); ++it)
        {
            std::vector<int>        & sbr = spin_basis[it->first].first;
            std::vector<int>        & sbl = spin_basis[it->first].second;
            std::vector<int> const & rhsr = it->second.first;
            std::vector<int> const & rhsl = it->second.second;

            sbr.resize(std::max(sbr.size(), rhsr.size()));
            sbl.resize(std::max(sbl.size(), rhsl.size()));

            for (std::size_t i = 0; i < std::min(sbr.size(), rhsr.size()); ++i)
                if(rhsr[i] != 0)
                    sbr[i] = rhsr[i];

            for (std::size_t i = 0; i < std::min(sbl.size(), rhsl.size()); ++i)
                if(rhsl[i] != 0)
                    sbl[i] = rhsl[i];
        }
    }
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup> & SiteOperator<Matrix, SymmGroup>::operator+=(SiteOperator const & rhs)
{
    assert (spin_.get() == rhs.spin().get() || n_blocks() == 0 || rhs.n_blocks() == 0);

    if (n_blocks() == 0) spin_ = rhs.spin();
    bm_ += rhs.bm_;

    SiteOperator_detail::extend_spin_basis<Matrix, SymmGroup>(spin_basis, rhs.spin_basis);

    return *this;
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup> & SiteOperator<Matrix, SymmGroup>::operator-=(SiteOperator const & rhs)
{
    assert (spin_.get() == rhs.spin().get() || n_blocks() == 0 || rhs.n_blocks() == 0);

    if (n_blocks() == 0) spin_ = rhs.spin();

    bm_ -= rhs.bm_;

    SiteOperator_detail::extend_spin_basis<Matrix, SymmGroup>(spin_basis, rhs.spin_basis);

    return *this;
}

template<class Matrix, class SymmGroup>
typename SiteOperator<Matrix, SymmGroup>::size_type SiteOperator<Matrix, SymmGroup>::insert_block(Matrix const & mtx, charge c1, charge c2)
{
    return bm_.insert_block(mtx, c1, c2);
}

template<class Matrix, class SymmGroup>
typename SiteOperator<Matrix, SymmGroup>::size_type SiteOperator<Matrix, SymmGroup>::insert_block(Matrix * mtx, charge c1, charge c2)
{
    return bm_.insert_block(mtx, c1, c2);
}

template<class Matrix, class SymmGroup>
Index<SymmGroup> SiteOperator<Matrix, SymmGroup>::left_basis() const
{
    return bm_.left_basis();
}

template<class Matrix, class SymmGroup>
Index<SymmGroup> SiteOperator<Matrix, SymmGroup>::right_basis() const
{
    return bm_.right_basis();
}

template<class Matrix, class SymmGroup>
DualIndex<SymmGroup> const & SiteOperator<Matrix, SymmGroup>::basis() const { return bm_.basis(); }

template<class Matrix, class SymmGroup>
typename Matrix::size_type SiteOperator<Matrix, SymmGroup>::n_blocks() const { return bm_.n_blocks(); }

template<class Matrix, class SymmGroup>
std::string SiteOperator<Matrix, SymmGroup>::description() const
{
    return bm_.description();
}

template<class Matrix, class SymmGroup>
Matrix & SiteOperator<Matrix, SymmGroup>::operator[](size_type c) { return bm_[c]; }

template<class Matrix, class SymmGroup>
Matrix const & SiteOperator<Matrix, SymmGroup>::operator[](size_type c) const { return bm_[c]; }

template<class Matrix, class SymmGroup>
typename SiteOperator<Matrix, SymmGroup>::size_type SiteOperator<Matrix, SymmGroup>::find_block(charge r, charge c) const
{
    return bm_.find_block(r,c);
}

template<class Matrix, class SymmGroup>
bool SiteOperator<Matrix, SymmGroup>::has_block(charge r, charge c) const
{
    return bm_.has_block(r,c);
}

template<class Matrix, class SymmGroup>
bool SiteOperator<Matrix, SymmGroup>::has_block(std::pair<charge, size_type> const & r,
                                                std::pair<charge, size_type> const & c) const
{
    return has_block(r.first, c.first);
}

template<class Matrix, class SymmGroup>
typename Matrix::value_type & SiteOperator<Matrix, SymmGroup>::operator()(std::pair<charge, size_type> const & r,
                                                                          std::pair<charge, size_type> const & c)
{
    return bm_(r, c);
}

template<class Matrix, class SymmGroup>
typename Matrix::value_type const & SiteOperator<Matrix, SymmGroup>::operator()(std::pair<charge, size_type> const & r,
                                                                                std::pair<charge, size_type> const & c) const
{
    return bm_(r, c);
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup> const & SiteOperator<Matrix, SymmGroup>::operator*=(const scalar_type& v)
{
    bm_ *= v;
    return *this;
}

template<class Matrix, class SymmGroup>
SiteOperator<Matrix, SymmGroup> const & SiteOperator<Matrix, SymmGroup>::operator/=(const scalar_type& v)
{
    bm_ /= v;
    return *this;
}

template<class Matrix, class SymmGroup>
typename SiteOperator<Matrix, SymmGroup>::real_type SiteOperator<Matrix, SymmGroup>::norm() const
{
    return bm_.norm();
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::transpose_inplace()
{
    bm_.transpose_inplace();
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::adjoint_inplace()
{
    bm_.adjoint_inplace();
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::clear()
{
    bm_.clear();
    spin_.clear();
}

template<class Matrix, class SymmGroup>
std::ostream& operator<<(symm_traits::disable_if_su2_t<SymmGroup, std::ostream&> os, SiteOperator<Matrix, SymmGroup> const & m)
//std::ostream& operator<<(std::ostream& os, SiteOperator<Matrix, SymmGroup> const & m)
{
    os << "Basis: " << m.basis() << std::endl;
    for (std::size_t k = 0; k < m.n_blocks(); ++k)
        os << "Block (" << m.basis()[k].lc << "," << m.basis()[k].rc
           << "):\n" << m[k] << std::endl;
    os << std::endl;
    return os;
}

template<class Matrix, class SymmGroup>
std::ostream& operator<<(symm_traits::enable_if_su2_t<SymmGroup, std::ostream&> os, SiteOperator<Matrix, SymmGroup> const & m)
{
    os << "Basis: " << m.basis() << std::endl;
    os << m.spin() << std::endl;
    for (std::size_t k = 0; k < m.n_blocks(); ++k)
    {
        os << "Block (" << m.basis()[k].lc << "," << m.basis()[k].rc
           << "):\n" << m[k];// << std::endl;

        try {
        std::vector<int> const & sbr = m.spin_basis.at(std::make_pair(m.basis()[k].lc, m.basis()[k].rc)).first;
        std::vector<int> const & sbl = m.spin_basis.at(std::make_pair(m.basis()[k].lc, m.basis()[k].rc)).second;
        std::copy(sbr.begin(), sbr.end(), std::ostream_iterator<int>(os, " ")); os << " | ";
        std::copy(sbl.begin(), sbl.end(), std::ostream_iterator<int>(os, " ")); os << std::endl << std::endl;
        }
        catch(...) {}
    }

    os << std::endl;
    return os;
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::match_and_add_block(Matrix const & mtx, charge c1, charge c2)
{
    bm_.match_and_add_block(mtx, c1, c2);
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::resize_block(charge r, charge c,
                                                   size_type new_r, size_type new_c,
                                                   bool pretend)
{
    bm_.resize_block(r, c, new_r, new_c, pretend);
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::resize_block(size_type pos,
                                                   size_type new_r, size_type new_c,
                                                   bool pretend)
{
    bm_.resize_block(pos, new_r, new_c, pretend);
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::remove_block(charge r, charge c)
{
    bm_.remove_block(r,c);
}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::remove_block(std::size_t which)
{
    bm_.remove_block(which);
}


template<class Matrix, class SymmGroup>
template <class Archive>
void SiteOperator<Matrix, SymmGroup>::serialize(Archive & ar, const unsigned int version)
{
    ar & bm_;
}

namespace SiteOperator_detail {

    template <class Matrix, class SymmGroup>
    symm_traits::disable_if_su2_t<SymmGroup>
    check_spin_basis(block_matrix<Matrix, SymmGroup> const & bm,
                     typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type &)
    {
    }

    template <class Matrix, class SymmGroup>
    symm_traits::enable_if_su2_t<SymmGroup>
    check_spin_basis(block_matrix<Matrix, SymmGroup> const & bm,
                     typename SparseOperator<Matrix, SymmGroup, void>::spin_basis_type & spin_basis)
    {
        //if (spin_basis.size() != bm.n_blocks())
        if (spin_basis.size() == 0)
        for(std::size_t b = 0; b < bm.n_blocks(); ++b)
            if (spin_basis.count(std::make_pair(bm.basis().left_charge(b), bm.basis().right_charge(b))) == 0)
                spin_basis[std::make_pair(bm.basis().left_charge(b), bm.basis().right_charge(b))]
                    = std::make_pair(std::vector<typename SymmGroup::subcharge>(num_rows(bm[b]), std::abs(SymmGroup::spin(bm.basis().left_charge(b)))),
                                     std::vector<typename SymmGroup::subcharge>(num_cols(bm[b]), std::abs(SymmGroup::spin(bm.basis().right_charge(b))))
                                     );
    }

}

template<class Matrix, class SymmGroup>
void SiteOperator<Matrix, SymmGroup>::update_sparse()
{
    SiteOperator_detail::check_spin_basis(bm_, spin_basis);
    sparse_op.update(bm_, spin_basis);
}
