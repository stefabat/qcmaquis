/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef ABELIAN_ENGINE_H
#define ABELIAN_ENGINE_H

#include <boost/shared_ptr.hpp>

#include "dmrg/mp_tensors/mpstensor.h"
#include "dmrg/mp_tensors/mpotensor.h"

#include "dmrg/mp_tensors/contractions/common/common.h"
#include "dmrg/mp_tensors/contractions/abelian/apply_op.hpp"
#include "dmrg/mp_tensors/contractions/abelian/functors.hpp"
#include "dmrg/mp_tensors/contractions/abelian/h_diag.hpp"

namespace contraction {

using ::contraction::common::BoundaryMPSProduct;
using ::contraction::common::MPSBoundaryProduct;

template <class Matrix, class OtherMatrix, class SymmGroup, class SymmType = void>
class Engine
{
    // abelian functors
    struct lbtm_functor
    {
        void operator()(size_t b2, contraction::ContractionGrid<Matrix, SymmGroup>& contr_grid,
                        Boundary<OtherMatrix, SymmGroup> const & left,
                        BoundaryMPSProduct<Matrix, OtherMatrix, SymmGroup, abelian::Gemms> const & left_mult_mps,
                        MPOTensor<Matrix, SymmGroup> const & mpo,
                        DualIndex<SymmGroup> const & ket_basis, DualIndex<SymmGroup> const & bra_basis,
                        Index<SymmGroup> const & right_i, Index<SymmGroup> const & out_left_i,
                        ProductBasis<SymmGroup> const & in_right_pb, ProductBasis<SymmGroup> const & out_left_pb,
                        bool isHermitian)
        {
            abelian::lbtm_kernel(b2, contr_grid, left, left_mult_mps, mpo, ket_basis, bra_basis,
                                         right_i, out_left_i, in_right_pb, out_left_pb, isHermitian);
        }
    };

    struct rbtm_functor
    {
        void operator()(size_t b1, block_matrix<Matrix, SymmGroup> & ret, Boundary<OtherMatrix, SymmGroup> const & right,
                        MPSBoundaryProduct<Matrix, OtherMatrix, SymmGroup, abelian::Gemms> const & right_mult_mps,
                        MPOTensor<Matrix, SymmGroup> const & mpo,
                        DualIndex<SymmGroup> const & ket_basis, DualIndex<SymmGroup> const & bra_basis,
                        Index<SymmGroup> const & left_i, Index<SymmGroup> const & out_right_i,
                        ProductBasis<SymmGroup> const & in_left_pb, ProductBasis<SymmGroup> const & out_right_pb,
                        bool isHermitian)
        {
            abelian::rbtm_kernel(b1, ret, right, right_mult_mps, mpo, ket_basis, bra_basis,
                                         left_i, out_right_i, in_left_pb, out_right_pb, isHermitian);
        }
    };

public:
    // generic methods forward

    static block_matrix<OtherMatrix, SymmGroup>
    overlap_left_step(MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                      MPSTensor<Matrix, SymmGroup> const & ket_tensor,
                      block_matrix<OtherMatrix, SymmGroup> const & left,
                      block_matrix<OtherMatrix, SymmGroup> * localop = NULL)
    {
        return common::overlap_left_step<Matrix, OtherMatrix, SymmGroup, abelian::Gemms>(bra_tensor, ket_tensor, left, localop);
    }

    static block_matrix<OtherMatrix, SymmGroup>
    overlap_right_step(MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                       MPSTensor<Matrix, SymmGroup> const & ket_tensor,
                       block_matrix<OtherMatrix, SymmGroup> const & right,
                       block_matrix<OtherMatrix, SymmGroup> * localop = NULL)
    {
        return common::overlap_right_step<Matrix, OtherMatrix, SymmGroup, abelian::Gemms>(bra_tensor, ket_tensor, right, localop);
    }

    static Boundary<Matrix, SymmGroup>
    left_boundary_tensor_mpo(MPSTensor<Matrix, SymmGroup> mps,
                             Boundary<OtherMatrix, SymmGroup> const & left,
                             MPOTensor<Matrix, SymmGroup> const & mpo,
                             Index<SymmGroup> const * in_low = NULL)
    {
        return common::left_boundary_tensor_mpo<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, lbtm_functor>
               (mps, left, mpo, in_low);
    }

    static Boundary<Matrix, SymmGroup>
    right_boundary_tensor_mpo(MPSTensor<Matrix, SymmGroup> mps,
                              Boundary<OtherMatrix, SymmGroup> const & right,
                              MPOTensor<Matrix, SymmGroup> const & mpo,
                              Index<SymmGroup> const * in_low = NULL)
    {
        return common::right_boundary_tensor_mpo<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, rbtm_functor>
               (mps, right, mpo, in_low);
    }

    static Boundary<OtherMatrix, SymmGroup>
    overlap_mpo_left_step(MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                          MPSTensor<Matrix, SymmGroup> const & ket_tensor,
                          Boundary<OtherMatrix, SymmGroup> const & left,
                          MPOTensor<Matrix, SymmGroup> const & mpo,
                          bool isHermitian=true)
    {
        return common::overlap_mpo_left_step<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, lbtm_functor>
               (bra_tensor, ket_tensor, left, mpo, isHermitian);
    }

    static Boundary<OtherMatrix, SymmGroup>
    overlap_mpo_right_step(MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                           MPSTensor<Matrix, SymmGroup> const & ket_tensor,
                           Boundary<OtherMatrix, SymmGroup> const & right,
                           MPOTensor<Matrix, SymmGroup> const & mpo,
                           bool isHermitian=true)
    {
        return common::overlap_mpo_right_step<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, rbtm_functor>
               (bra_tensor, ket_tensor, right, mpo, isHermitian);
    }

    static std::pair<MPSTensor<Matrix, SymmGroup>, truncation_results>
    predict_new_state_l2r_sweep(MPSTensor<Matrix, SymmGroup> const & mps,
                                MPOTensor<Matrix, SymmGroup> const & mpo,
                                Boundary<OtherMatrix, SymmGroup> const & left,
                                Boundary<OtherMatrix, SymmGroup> const & right,
                                double alpha, double cutoff, std::size_t Mmax,
                                bool perturbDM, bool verbose)
    {
        return common::predict_new_state_l2r_sweep<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, lbtm_functor>
               (mps, mpo, left, right, alpha, cutoff, Mmax, perturbDM, verbose);
    }

    static MPSTensor<Matrix, SymmGroup>
    predict_lanczos_l2r_sweep(MPSTensor<Matrix, SymmGroup> B,
                              MPSTensor<Matrix, SymmGroup> const & psi,
                              MPSTensor<Matrix, SymmGroup> const & A)
    {
        return common::predict_lanczos_l2r_sweep<Matrix, OtherMatrix, SymmGroup, abelian::Gemms>(B, psi, A);
    }

    static block_matrix<Matrix, SymmGroup>
    getZeroSiteTensorL2R(MPSTensor<Matrix, SymmGroup> B,
                         MPSTensor<Matrix, SymmGroup> const & psi,
                         MPSTensor<Matrix, SymmGroup> const & A)
    {
        return common::getZeroSiteTensorL2R<Matrix, OtherMatrix, SymmGroup, abelian::Gemms>(B, psi, A);
    }

    static std::pair<MPSTensor<Matrix, SymmGroup>, truncation_results>
    predict_new_state_r2l_sweep(MPSTensor<Matrix, SymmGroup> const & mps,
                                MPOTensor<Matrix, SymmGroup> const & mpo,
                                Boundary<OtherMatrix, SymmGroup> const & left,
                                Boundary<OtherMatrix, SymmGroup> const & right,
                                double alpha, double cutoff, std::size_t Mmax,
                                bool perturbDM, bool verbose)
    {
        return common::predict_new_state_r2l_sweep<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, rbtm_functor>
               (mps, mpo, left, right, alpha, cutoff, Mmax, perturbDM, verbose);
    }

    static MPSTensor<Matrix, SymmGroup>
    predict_lanczos_r2l_sweep(MPSTensor<Matrix, SymmGroup> B,
                              MPSTensor<Matrix, SymmGroup> const & psi,
                              MPSTensor<Matrix, SymmGroup> const & A)
    {
        return common::predict_lanczos_r2l_sweep<Matrix, OtherMatrix, SymmGroup, abelian::Gemms>(B, psi, A);
    }

    static block_matrix<Matrix, SymmGroup>
    getZeroSiteTensorR2L(MPSTensor<Matrix, SymmGroup> B,
                         MPSTensor<Matrix, SymmGroup> const & psi,
                         MPSTensor<Matrix, SymmGroup> const & A)
    {
        return common::getZeroSiteTensorR2L<Matrix, OtherMatrix, SymmGroup, abelian::Gemms>(B, psi, A);
    }

    static Boundary<OtherMatrix, SymmGroup>
    generate_left_mpo_basis(MPSTensor<Matrix, SymmGroup> const & bra_tensor, MPSTensor<Matrix, SymmGroup> const & ket_tensor,
                             Boundary<OtherMatrix, SymmGroup> const & left, MPOTensor<Matrix, SymmGroup> const & mpo)
    {
        return common::generate_left_mpo_basis<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, lbtm_functor>(bra_tensor, ket_tensor, left, mpo);
    }

    static Boundary<OtherMatrix, SymmGroup>
    generate_right_mpo_basis(MPSTensor<Matrix, SymmGroup> const & bra_tensor, MPSTensor<Matrix, SymmGroup> const & ket_tensor,
                             Boundary<OtherMatrix, SymmGroup> const & right, MPOTensor<Matrix, SymmGroup> const & mpo)
    {
        return common::generate_right_mpo_basis<Matrix, OtherMatrix, SymmGroup, abelian::Gemms, rbtm_functor>(bra_tensor, ket_tensor, right, mpo);
    }

    // non-generic method

    static MPSTensor<Matrix, SymmGroup>
    site_hamil2(MPSTensor<Matrix, SymmGroup> ket_tensor,
                Boundary<OtherMatrix, SymmGroup> const & left,
                Boundary<OtherMatrix, SymmGroup> const & right,
                MPOTensor<Matrix, SymmGroup> const & mpo,
                bool isHermitian=true);

    static MPSTensor<Matrix, SymmGroup>
    site_hamil2(MPSTensor<Matrix, SymmGroup> ket_tensor,
                MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                Boundary<OtherMatrix, SymmGroup> const & left,
                Boundary<OtherMatrix, SymmGroup> const & right,
                MPOTensor<Matrix, SymmGroup> const & mpo,
                bool isHermitian=true);

    // Zero-site Hamiltonian
    static block_matrix<Matrix, SymmGroup>
    zerosite_hamil2(block_matrix<Matrix, SymmGroup> ket_tensor, Boundary<OtherMatrix, SymmGroup> const & left,
                    Boundary<OtherMatrix, SymmGroup> const & right, MPOTensor<Matrix, SymmGroup> const & mpo_left, MPOTensor<Matrix, SymmGroup> const & mpo_right,
                    bool isHermitian=true);

    static block_matrix<Matrix, SymmGroup>
    zerosite_hamil2_kernel(block_matrix<Matrix, SymmGroup> ket_tensor, Boundary<OtherMatrix, SymmGroup> const & left,
                           Boundary<OtherMatrix, SymmGroup> const & right, MPOTensor<Matrix, SymmGroup> const & mpo_left, MPOTensor<Matrix, SymmGroup> const & mpo_right,
                           bool isHermitian=true);

    static block_matrix<Matrix, SymmGroup>
    diagonal_hamiltonian(Boundary<OtherMatrix, SymmGroup> const &left, Boundary<OtherMatrix, SymmGroup> const &right,
                         MPOTensor <Matrix, SymmGroup> const &mpo, MPSTensor<Matrix, SymmGroup> const &x)
    {
        return contraction::diagonal_hamiltonian(left, right, mpo, x);
    }


};

} // namespace contraction

#include "dmrg/mp_tensors/contractions/abelian/site_hamil.hpp"
#include "dmrg/mp_tensors/contractions/abelian/zero_site_hamil.hpp"

#endif
