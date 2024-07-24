/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef CONTRACTIONS_APPLY_OP_H
#define CONTRACTIONS_APPLY_OP_H

#include "dmrg/mp_tensors/mpstensor.h"
#include "dmrg/mp_tensors/mpotensor.h"
#include "dmrg/mp_tensors/contractions/abelian/functors.hpp"
#include "dmrg/mp_tensors/contractions/abelian/detail.hpp"

namespace contraction {
namespace abelian {

using ::contraction::ContractionGrid;
using ::contraction::common::BoundaryMPSProduct;
using ::contraction::common::MPSBoundaryProduct;

template<class Matrix, class OtherMatrix, class SymmGroup>
void lbtm_kernel_allocate(size_t b2, ContractionGrid<Matrix, SymmGroup>& contr_grid,
                          Boundary<OtherMatrix, SymmGroup> const & left,
                          BoundaryMPSProduct<Matrix, OtherMatrix, SymmGroup, Gemms> const & left_mult_mps,
                          MPOTensor<Matrix, SymmGroup> const & mpo,
                          DualIndex<SymmGroup> const & ket_basis, DualIndex<SymmGroup> const & bra_basis,
                          Index<SymmGroup> const & right_i, Index<SymmGroup> const & out_left_i,
                          bool isHermitian=true)
{
    typedef typename MPOTensor<Matrix, SymmGroup>::index_type index_type;
    typedef typename MPOTensor<Matrix, SymmGroup>::col_proxy col_proxy;
    typedef typename SymmGroup::charge charge;
    typedef std::size_t size_t;
    col_proxy col_b2 = mpo.column(b2);
    for (typename col_proxy::const_iterator col_it = col_b2.begin(); col_it != col_b2.end(); ++col_it) {
        index_type b1 = col_it.index();
        DualIndex<SymmGroup> T_basis = detail::T_basis_left(left, left_mult_mps, mpo, ket_basis, bra_basis, b1, isHermitian);
        if (T_basis.size() == 0)
            continue;
        MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1,b2);
        for (size_t oi = 0; oi < access.size(); ++oi)
        {
            typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(oi);
            if(W.n_blocks() == 0)
                continue;
            charge operator_delta = SymmGroup::fuse(W.basis().right_charge(0), -W.basis().left_charge(0));
            charge        T_delta = SymmGroup::fuse(T_basis.right_charge(0), -T_basis.left_charge(0));
            charge    total_delta = SymmGroup::fuse(operator_delta, -T_delta);
            block_matrix<Matrix, SymmGroup>& ret = contr_grid(b1,b2);
            for(size_t r = 0; r < right_i.size(); ++r) {
                charge out_r_charge = right_i[r].first;
                charge out_l_charge = SymmGroup::fuse(out_r_charge, total_delta);
                if(!out_left_i.has(out_l_charge))
                    continue;
                size_t r_size = right_i[r].second;
                if(ret.find_block(out_l_charge, out_r_charge) == ret.n_blocks())
                    #ifdef USE_AMBIENT
                    // both versions should be fine for AMBIENT
                    ret.resize_block(ret.insert_block(Matrix(1,1), out_l_charge, out_r_charge),
                                     out_left_i.size_of_block(out_l_charge), r_size);
                    #else
                    ret.insert_block(Matrix(out_left_i.size_of_block(out_l_charge), r_size), out_l_charge, out_r_charge);
                    #endif
            }
        } // oi
    } // b1
    contr_grid.index_sizes(b2);
}

// SK: New version which generates same output but uses right-paired input.
//     The charge delta optimization is indepent from the changes needed to
//     skip the preceding reshapes.
template<class Matrix, class OtherMatrix, class SymmGroup>
void lbtm_kernel_execute(size_t b2, ContractionGrid<Matrix, SymmGroup>& contr_grid,
                         Boundary<OtherMatrix, SymmGroup> const & left,
                         BoundaryMPSProduct<Matrix, OtherMatrix, SymmGroup, Gemms> const & left_mult_mps,
                         MPOTensor<Matrix, SymmGroup> const & mpo,
                         DualIndex<SymmGroup> const & ket_basis,
                         Index<SymmGroup> const & right_i, Index<SymmGroup> const & out_left_i,
                         ProductBasis<SymmGroup> const & in_right_pb, ProductBasis<SymmGroup> const & out_left_pb)
{
    typedef typename MPOTensor<Matrix, SymmGroup>::index_type index_type;
    typedef typename MPOTensor<Matrix, SymmGroup>::col_proxy col_proxy;
    typedef typename SymmGroup::charge charge;
    typedef std::size_t size_t;
    col_proxy col_b2 = mpo.column(b2);
    for (typename col_proxy::const_iterator col_it = col_b2.begin(); col_it != col_b2.end(); ++col_it) {
        index_type b1 = col_it.index();
        block_matrix<Matrix, SymmGroup> local;
        block_matrix<Matrix, SymmGroup> const & T = left_mult_mps.at(b1, local);
        if(T.n_blocks() == 0)
            continue;
        MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1,b2);
        for (size_t oi = 0; oi < access.size(); ++oi)
        {
            typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(oi);
            if(W.n_blocks() == 0)
                continue;
            // charge deltas are constant for all blocks
            charge operator_delta = SymmGroup::fuse(W.basis().right_charge(0), -W.basis().left_charge(0));
            charge        T_delta = SymmGroup::fuse(T.basis().right_charge(0), -T.basis().left_charge(0));
            charge    total_delta = SymmGroup::fuse(operator_delta, -T_delta);
            block_matrix<Matrix, SymmGroup>& ret = contr_grid(b1,b2);
            parallel::guard group(contr_grid.where(b1,b2), contr_grid.granularity);
            parallel::scheduler_size_indexed scheduler(ret);
            for (size_t r = 0; r < right_i.size(); ++r){
                charge out_r_charge = right_i[r].first;
                charge out_l_charge = SymmGroup::fuse(out_r_charge, total_delta);
                if(!out_left_i.has(out_l_charge))
                    continue;
                size_t r_size = right_i[r].second;
                size_t o = ret.find_block(out_l_charge, out_r_charge);
                for (size_t w_block = 0; w_block < W.n_blocks(); ++w_block){
                    charge phys_c1 = W.basis().left_charge(w_block);
                    charge phys_c2 = W.basis().right_charge(w_block);

                    charge in_r_charge = SymmGroup::fuse(out_r_charge, -phys_c1);
                    charge in_l_charge = SymmGroup::fuse(in_r_charge, -T_delta);
                    size_t t_block = T.basis().position(in_l_charge, in_r_charge);
                    if(t_block == T.basis().size())
                        continue;
                    size_t in_right_offset = in_right_pb(phys_c1, out_r_charge);
                    size_t out_left_offset = out_left_pb(phys_c2, in_l_charge);
                    size_t phys_s1 = W.basis().left_size(w_block);
                    size_t phys_s2 = W.basis().right_size(w_block);
                    Matrix const & wblock = W[w_block];
                    Matrix const & iblock = T[t_block];
                    Matrix & oblock = ret[o];
                    parallel::guard proc(scheduler(o));
                    maquis::dmrg::detail::lb_tensor_mpo(oblock, iblock, wblock,
                                                        out_left_offset, in_right_offset,
                                                        phys_s1, phys_s2, T.basis().left_size(t_block), r_size, access.scale(oi));
                }
            } // right index block
        } // oi
    } // b1
}

template<class Matrix, class OtherMatrix, class SymmGroup>
void rbtm_kernel_allocate(size_t b1, block_matrix<Matrix, SymmGroup> & ret,
                          Boundary<OtherMatrix, SymmGroup> const & right,
                          MPSBoundaryProduct<Matrix, OtherMatrix, SymmGroup, Gemms> const & right_mult_mps,
                          MPOTensor<Matrix, SymmGroup> const & mpo,
                          DualIndex<SymmGroup> const & ket_basis, DualIndex<SymmGroup> const & bra_basis,
                          Index<SymmGroup> const & left_i, Index<SymmGroup> const & out_right_i,
                          bool isHermitian)
{
    typedef typename MPOTensor<Matrix, SymmGroup>::index_type index_type;
    typedef typename MPOTensor<Matrix, SymmGroup>::row_proxy row_proxy;
    typedef typename SymmGroup::charge charge;
    typedef std::size_t size_t;
    row_proxy row_b1 = mpo.row(b1);
    for (typename row_proxy::const_iterator row_it = row_b1.begin(); row_it != row_b1.end(); ++row_it) {
        index_type b2 = row_it.index();
        DualIndex<SymmGroup> T_basis = detail::T_basis_right(right, right_mult_mps, mpo, ket_basis, bra_basis, b2, isHermitian);
        if (T_basis.size() == 0)
            continue;
        MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1,b2);
        for (size_t oi = 0; oi < access.size(); ++oi)
        {
            typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(oi);
            if(W.n_blocks() == 0)
                continue;
            charge operator_delta = SymmGroup::fuse(W.basis().right_charge(0), -W.basis().left_charge(0));
            charge        T_delta = SymmGroup::fuse(T_basis.right_charge(0), -T_basis.left_charge(0));
            charge    total_delta = SymmGroup::fuse(operator_delta, -T_delta);
            for(size_t l = 0; l < left_i.size(); ++l) {
                charge out_l_charge = left_i[l].first;
                charge out_r_charge = SymmGroup::fuse(out_l_charge, -total_delta);
                if(!out_right_i.has(out_r_charge))
                    continue;
                size_t l_size = left_i[l].second;
                if(ret.find_block(out_l_charge, out_r_charge) == ret.n_blocks()) {
                    #ifdef USE_AMBIENT
                    // both versions should be fine for AMBIENT
                    ret.resize_block(ret.insert_block(Matrix(1,1), out_l_charge, out_r_charge),
                                     l_size, out_right_i.size_of_block(out_r_charge));
                    #else
                    ret.insert_block(Matrix(l_size, out_right_i.size_of_block(out_r_charge)), out_l_charge, out_r_charge);
                    #endif
                }
            }
        } // oi
    } // b2
    ret.index_sizes();
}

template<class Matrix, class OtherMatrix, class SymmGroup>
void rbtm_kernel_execute(size_t b1, block_matrix<Matrix, SymmGroup> & ret,
                         Boundary<OtherMatrix, SymmGroup> const & right,
                         MPSBoundaryProduct<Matrix, OtherMatrix, SymmGroup, Gemms> const & right_mult_mps,
                         MPOTensor<Matrix, SymmGroup> const & mpo,
                         DualIndex<SymmGroup> const & ket_basis,
                         Index<SymmGroup> const & left_i, Index<SymmGroup> const & out_right_i,
                         ProductBasis<SymmGroup> const & in_left_pb, ProductBasis<SymmGroup> const & out_right_pb)
{
    parallel::scheduler_size_indexed scheduler(ret);
    typedef typename MPOTensor<Matrix, SymmGroup>::index_type index_type;
    typedef typename MPOTensor<Matrix, SymmGroup>::row_proxy row_proxy;
    typedef typename SymmGroup::charge charge;
    typedef std::size_t size_t;
    row_proxy row_b1 = mpo.row(b1);
    for (typename row_proxy::const_iterator row_it = row_b1.begin(); row_it != row_b1.end(); ++row_it) {
        index_type b2 = row_it.index();
        block_matrix<Matrix, SymmGroup> const & T = right_mult_mps.at(b2);
        if(T.n_blocks() == 0)
            continue;
        MPOTensor_detail::term_descriptor<Matrix, SymmGroup, true> access = mpo.at(b1,b2);
        for (size_t oi = 0; oi < access.size(); ++oi)
        {
            typename operator_selector<Matrix, SymmGroup>::type const & W = access.op(oi);
            if(W.n_blocks() == 0)
                continue;
            // charge deltas are constant for all blocks
            charge operator_delta = SymmGroup::fuse(W.basis().right_charge(0), -W.basis().left_charge(0));
            charge        T_delta = SymmGroup::fuse(T.basis().right_charge(0), -T.basis().left_charge(0));
            charge    total_delta = SymmGroup::fuse(operator_delta, -T_delta);
            for (size_t l = 0; l < left_i.size(); ++l){
                charge out_l_charge = left_i[l].first;
                charge out_r_charge = SymmGroup::fuse(out_l_charge, -total_delta);
                if(!out_right_i.has(out_r_charge))
                    continue;
                size_t l_size = left_i[l].second;
                size_t o = ret.find_block(out_l_charge, out_r_charge);
                for (size_t w_block = 0; w_block < W.n_blocks(); ++w_block) {
                    charge phys_c1 = W.basis().left_charge(w_block);
                    charge phys_c2 = W.basis().right_charge(w_block);
                    charge in_l_charge = SymmGroup::fuse(out_l_charge, phys_c1);
                    charge in_r_charge = SymmGroup::fuse(in_l_charge, T_delta);
                    size_t t_block = T.basis().position(in_l_charge, in_r_charge);
                    if (t_block == T.basis().size())
                        continue;
                    size_t in_left_offset = in_left_pb(phys_c1, out_l_charge);
                    size_t out_right_offset = out_right_pb(phys_c2, in_r_charge);
                    size_t phys_s1 = W.basis().left_size(w_block);
                    size_t phys_s2 = W.basis().right_size(w_block);
                    const Matrix & wblock = W[w_block];
                    const Matrix & iblock = T[t_block];
                    Matrix & oblock = ret[o];
                    parallel::guard proc(scheduler(o));
                    maquis::dmrg::detail::rb_tensor_mpo(oblock, iblock, wblock, out_right_offset, in_left_offset,
                                                        phys_s1, phys_s2, l_size, T.basis().right_size(t_block), access.scale(oi));
                }
            }
        } // oi
    } // b2
    right_mult_mps.free(b1);
}

template<class Matrix, class OtherMatrix, class SymmGroup>
void lbtm_kernel(size_t b2, ContractionGrid<Matrix, SymmGroup>& contr_grid,
                 Boundary<OtherMatrix, SymmGroup> const & left,
                 BoundaryMPSProduct<Matrix, OtherMatrix, SymmGroup, Gemms> const & left_mult_mps,
                 MPOTensor<Matrix, SymmGroup> const & mpo,
                 DualIndex<SymmGroup> const & ket_basis, DualIndex<SymmGroup> const & bra_basis,
                 Index<SymmGroup> const & right_i, Index<SymmGroup> const & out_left_i,
                 ProductBasis<SymmGroup> const & in_right_pb, ProductBasis<SymmGroup> const & out_left_pb,
                 bool isHermitian=true)
{
    lbtm_kernel_allocate(b2, contr_grid, left, left_mult_mps, mpo, ket_basis, bra_basis, right_i, out_left_i, isHermitian);
    lbtm_kernel_execute(b2, contr_grid, left, left_mult_mps, mpo, ket_basis, right_i, out_left_i, in_right_pb, out_left_pb);
}

template<class Matrix, class OtherMatrix, class SymmGroup>
void rbtm_kernel(size_t b1, block_matrix<Matrix, SymmGroup> & ret,
                 Boundary<OtherMatrix, SymmGroup> const & right,
                 MPSBoundaryProduct<Matrix, OtherMatrix, SymmGroup, Gemms> const & right_mult_mps,
                 MPOTensor<Matrix, SymmGroup> const & mpo,
                 DualIndex<SymmGroup> const & ket_basis, DualIndex<SymmGroup> const & bra_basis,
                 Index<SymmGroup> const & left_i, Index<SymmGroup> const & out_right_i,
                 ProductBasis<SymmGroup> const & in_left_pb, ProductBasis<SymmGroup> const & out_right_pb,
                 bool isHermitian=true)
{
    rbtm_kernel_allocate(b1, ret, right, right_mult_mps, mpo, ket_basis, bra_basis, left_i, out_right_i, isHermitian);
    rbtm_kernel_execute(b1, ret, right, right_mult_mps, mpo, ket_basis, left_i, out_right_i, in_left_pb, out_right_pb);
}

} // namespace abelian
} // namespace contraction
#endif
