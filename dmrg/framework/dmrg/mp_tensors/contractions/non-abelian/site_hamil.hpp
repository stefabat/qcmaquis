/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef CONTRACTIONS_SU2_SITE_HAMIL_HPP
#define CONTRACTIONS_SU2_SITE_HAMIL_HPP

namespace contraction {

// forward declarations
template<class Matrix, class OtherMatrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup>
site_hamil_lbtm(MPSTensor<Matrix, SymmGroup> ket_tensor, MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                Boundary<OtherMatrix, SymmGroup> const & left, Boundary<OtherMatrix, SymmGroup> const & right,
                MPOTensor<Matrix, SymmGroup> const & mpo, bool isHermitian);

template<class Matrix, class OtherMatrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup>
site_hamil_rbtm(MPSTensor<Matrix, SymmGroup> ket_tensor, MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                Boundary<OtherMatrix, SymmGroup> const & left, Boundary<OtherMatrix, SymmGroup> const & right,
                MPOTensor<Matrix, SymmGroup> const & mpo, bool isHermitian);

// *************************************************************

template<class Matrix, class OtherMatrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup>
Engine<Matrix, OtherMatrix, SymmGroup, symm_traits::enable_if_su2_t<SymmGroup>>::
site_hamil2(MPSTensor<Matrix, SymmGroup> ket_tensor,
            Boundary<OtherMatrix, SymmGroup> const & left, Boundary<OtherMatrix, SymmGroup> const & right,
            MPOTensor<Matrix, SymmGroup> const & mpo, bool isHermitian)
{
    if ( (mpo.row_dim() - mpo.num_one_rows()) < (mpo.col_dim() - mpo.num_one_cols()) )
        return site_hamil_lbtm(ket_tensor, ket_tensor, left, right, mpo, isHermitian);
    else
        return site_hamil_rbtm(ket_tensor, ket_tensor, left, right, mpo, isHermitian);
}

template<class Matrix, class OtherMatrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup>
Engine<Matrix, OtherMatrix, SymmGroup, symm_traits::enable_if_su2_t<SymmGroup>>::
site_hamil2(MPSTensor<Matrix, SymmGroup> ket_tensor, MPSTensor<Matrix, SymmGroup> const & bra_tensor,
            Boundary<OtherMatrix, SymmGroup> const & left,Boundary<OtherMatrix, SymmGroup> const & right,
            MPOTensor<Matrix, SymmGroup> const & mpo, bool isHermitian)
{
    if ( (mpo.row_dim() - mpo.num_one_rows()) < (mpo.col_dim() - mpo.num_one_cols()) )
        return site_hamil_lbtm(ket_tensor, bra_tensor, left, right, mpo, isHermitian);
    else
        return site_hamil_rbtm(ket_tensor, bra_tensor, left, right, mpo, isHermitian);
}

// *************************************************************
// specialized variants

template<class Matrix, class OtherMatrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup>
site_hamil_lbtm(MPSTensor<Matrix, SymmGroup> ket_tensor, MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                Boundary<OtherMatrix, SymmGroup> const & left, Boundary<OtherMatrix, SymmGroup> const & right,
                MPOTensor<Matrix, SymmGroup> const & mpo, bool isHermitian)
{
    // Types definition
    typedef typename SymmGroup::charge charge;
    typedef typename MPOTensor<Matrix, SymmGroup>::index_type index_type;
    typedef typename Matrix::value_type value_type;
    // Setup indices
    Index<SymmGroup> const & physical_i = ket_tensor.site_dim();
    Index<SymmGroup> const & left_i = bra_tensor.row_dim();
    Index<SymmGroup> right_i = ket_tensor.col_dim();
    Index<SymmGroup> out_left_i = physical_i * left_i;
    Index<SymmGroup> right_i_bra = bra_tensor.col_dim();
    common_subset(out_left_i, right_i_bra);
    ProductBasis<SymmGroup> out_left_pb(physical_i, left_i);
    ProductBasis<SymmGroup> in_right_pb(physical_i, right_i,
                            boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                    -boost::lambda::_1, boost::lambda::_2));
    bra_tensor.make_right_paired();
    Index<SymmGroup> indexForTrim = bra_tensor.data().left_basis(); 
    contraction::common::BoundaryMPSProduct<Matrix, OtherMatrix, SymmGroup, ::SU2::SU2Gemms> t(ket_tensor, left, mpo, indexForTrim, isHermitian);
    // Prepares final data
    MPSTensor<Matrix, SymmGroup> ret;
    ret.phys_i  = bra_tensor.site_dim();
    ret.left_i  = bra_tensor.row_dim();
    ret.right_i = bra_tensor.col_dim();
    index_type loop_max = mpo.col_dim();
    DualIndex<SymmGroup> ket_basis_transpose = ket_tensor.data().basis();
    for (std::size_t i = 0; i < ket_basis_transpose.size(); ++i) {
        std::swap(ket_basis_transpose[i].lc, ket_basis_transpose[i].rc);
        std::swap(ket_basis_transpose[i].ls, ket_basis_transpose[i].rs);
    }
    bra_tensor.make_right_paired();
    DualIndex<SymmGroup> bra_basis = bra_tensor.data().basis();
#ifdef USE_AMBIENT
        {
            block_matrix<Matrix, SymmGroup> empty;
            swap(ket_tensor.data(), empty); // deallocating mpstensor before exiting the stack
        }
        parallel::sync();
        ContractionGrid<Matrix, SymmGroup> contr_grid(mpo, left.aux_dim(), mpo.col_dim());

        parallel_for(index_type b2, parallel::range<index_type>(0,loop_max), {
            SU2::lbtm_kernel(b2, contr_grid, left, t, mpo, ket_tensor.data().basis(), right_i, out_left_i, in_right_pb, out_left_pb);
        });
        omp_for(index_type b2, parallel::range<index_type>(0,loop_max), {
            contr_grid.multiply_column(b2, right[b2]);
        });
        t.clear();
        parallel::sync();

        swap(ret.data(), contr_grid.reduce());
        parallel::sync();

#else
    omp_for(index_type b2, parallel::range<index_type>(0,loop_max), {
        ContractionGrid<Matrix, SymmGroup> contr_grid(mpo, 0, 0);
        block_matrix<Matrix, SymmGroup> tmp, tmp2;
        typename MPOTensor<OtherMatrix, SymmGroup>::col_proxy cp = mpo.column(b2);
        index_type num_ops = std::distance(cp.begin(), cp.end());
        if (num_ops > 3) {
            SU2::lbtm_kernel_rp(b2, contr_grid, left, t, mpo, ket_basis_transpose, right_i, out_left_i, in_right_pb, out_left_pb);
            reshape_right_to_left_new(physical_i, left_i, right_i, contr_grid(0,0), tmp2);
            contr_grid(0,0).clear();
            swap(contr_grid(0,0), tmp2);
        }
        else {
            SU2::lbtm_kernel(b2, contr_grid, left, t, mpo, ket_basis_transpose, right_i, out_left_i, in_right_pb, out_left_pb);
        }
        if (mpo.herm_info.right_skip(b2) && isHermitian) {
            std::vector<value_type> phases = ::contraction::common::conjugate_phases(adjoint(right[mpo.herm_info.right_conj(b2)]), mpo, b2, false, true);
            ::SU2::gemm_trim(contr_grid(0,0), adjoint(right[mpo.herm_info.right_conj(b2)]), tmp, phases, false);
        }
        else
            ::SU2::gemm_trim(contr_grid(0,0), right[b2], tmp, std::vector<value_type>(contr_grid(0,0).n_blocks(), 1.), true);
        contr_grid(0,0).clear();
        if (num_ops > 3) {
            for (std::size_t k = 0; k < tmp.n_blocks(); ++k)
                if (!out_left_i.has(tmp.basis().left_charge(k)))
                    tmp.remove_block(k--);
        }
        parallel_critical
        for (std::size_t k = 0; k < tmp.n_blocks(); ++k)
            ret.data().match_and_add_block(tmp[k], tmp.basis().left_charge(k), tmp.basis().right_charge(k));
    });
#endif
    return ret;
}

template<class Matrix, class OtherMatrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup>
site_hamil_rbtm(MPSTensor<Matrix, SymmGroup> ket_tensor, MPSTensor<Matrix, SymmGroup> const & bra_tensor,
                Boundary<OtherMatrix, SymmGroup> const & left, Boundary<OtherMatrix, SymmGroup> const & right,
                MPOTensor<Matrix, SymmGroup> const & mpo, bool isHermitian)
{
    typedef typename SymmGroup::charge charge;
    typedef typename MPOTensor<Matrix, SymmGroup>::index_type index_type;
    typedef typename Matrix::value_type value_type;
    bra_tensor.make_left_paired();
    Index<SymmGroup> indexForTrim = bra_tensor.data().left_basis();
    contraction::common::MPSBoundaryProduct<Matrix, OtherMatrix, SymmGroup, ::SU2::SU2Gemms> t(ket_tensor, right, mpo, indexForTrim, isHermitian);
    DualIndex<SymmGroup> kb1 = ket_tensor.data().basis();
    Index<SymmGroup> const & physical_i = ket_tensor.site_dim(),
                             right_i = bra_tensor.col_dim();
    Index<SymmGroup> left_i = ket_tensor.row_dim(),
                     out_right_i = adjoin(physical_i) * right_i;
    Index<SymmGroup> left_i_ket = ket_tensor.row_dim();
    common_subset(out_right_i, left_i_ket);
    ProductBasis<SymmGroup> in_left_pb(physical_i, left_i);
    ProductBasis<SymmGroup> out_right_pb(physical_i, right_i,
                                         boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                            -boost::lambda::_1, boost::lambda::_2));
    block_matrix<Matrix, SymmGroup> collector;
    MPSTensor<Matrix, SymmGroup> ret;
    ret.phys_i = bra_tensor.site_dim();
    ret.left_i = bra_tensor.row_dim();
    ret.right_i = bra_tensor.col_dim();
    index_type loop_max = mpo.row_dim();
    omp_for(index_type b1, parallel::range<index_type>(0,loop_max), {
        block_matrix<Matrix, SymmGroup> tmp, tmp2;
        SU2::task_capsule<Matrix, SymmGroup> tasks_cap;
        SU2::rbtm_tasks(b1, t, mpo, ket_tensor.data().basis(), left_i, out_right_i, in_left_pb, out_right_pb, tasks_cap);
        if (mpo.herm_info.left_skip(b1) && isHermitian)
            SU2::rbtm_axpy_gemm(b1, tasks_cap, tmp2, out_right_i, left, mpo, conjugate(left[mpo.herm_info.left_conj(b1)]), t);
        else
            SU2::rbtm_axpy_gemm(b1, tasks_cap, tmp2, out_right_i, left, mpo, transpose(left[b1]), t);
        t.free(b1);
        //SU2::rbtm_kernel(b1, tmp, left, t, mpo, ket_tensor.data().basis(), left_i, out_right_i, in_left_pb, out_right_pb);
        //if (mpo.herm_info.left_skip(b1)) {
        //    std::vector<value_type> phases = ::contraction::common::conjugate_phases(left[mpo.herm_info.left_conj(b1)], mpo, b1, true, false);
        //    ::SU2::gemm_trim(left[mpo.herm_info.left_conj(b1)], tmp, tmp2, phases, true);
        //}
        //else
        //    ::SU2::gemm_trim(transpose(left[b1]), tmp, tmp2, std::vector<value_type>(tmp.n_blocks(), 1.), false);
        //tmp.clear();
        parallel_critical
        for (std::size_t k = 0; k < tmp2.n_blocks(); ++k)
            collector.match_and_add_block(tmp2[k], tmp2.basis().left_charge(k), tmp2.basis().right_charge(k));
    });
    reshape_right_to_left_new(physical_i, left_i, right_i, collector, ret.data());
    DualIndex<SymmGroup> kb2 = ket_tensor.data().basis();
    if (!(kb1 == kb2))
        throw std::runtime_error("XX\n");
    return ret;
}

} // namespace contraction

#endif
