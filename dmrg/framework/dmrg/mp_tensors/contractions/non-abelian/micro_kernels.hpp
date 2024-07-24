/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef CONTRACTIONS_SU2_MICRO_KERNELS_HPP
#define CONTRACTIONS_SU2_MICRO_KERNELS_HPP

#include "dmrg/block_matrix/block_matrix.h"
#include "dmrg/block_matrix/sparse_operator.h"

namespace contraction {
namespace SU2 {
namespace detail {


    template<class Matrix, class SymmGroup>
    void lbtm(Matrix const & iblock, Matrix & oblock, typename operator_selector<Matrix, SymmGroup>::type const & W,
              std::size_t in_right_offset, std::size_t out_left_offset, std::size_t l_size, std::size_t r_size, std::size_t w_block,
              typename Matrix::value_type couplings[])
    {
        auto blocks = W.get_sparse().block(w_block);

        for(size_t rr = 0; rr < r_size; ++rr) {
            for (auto it = blocks.first; it != blocks.second; ++it)
            {
                std::size_t ss1 = it->row;
                std::size_t ss2 = it->col;
                std::size_t rspin = it->row_spin;
                std::size_t cspin = it->col_spin;
                std::size_t casenr = 0;
                if (rspin == 2 && cspin == 2) casenr = 3;
                else if (rspin == 2) casenr = 1;
                else if (cspin == 2) casenr = 2;

                typename Matrix::value_type alfa_t = it->coefficient * couplings[casenr];
                maquis::dmrg::detail::iterator_axpy(&iblock(0, in_right_offset + ss1*r_size + rr),
                                                    &iblock(0, in_right_offset + ss1*r_size + rr) + l_size,
                                                    &oblock(out_left_offset + ss2*l_size, rr),
                                                    alfa_t);
            }
        }
    }


    template<class Matrix, class SymmGroup>
    void rbtm_blocked(Matrix const & iblock, Matrix & oblock, typename operator_selector<Matrix, SymmGroup>::type const & W,
                                 std::size_t in_right_offset, std::size_t out_right_offset, std::size_t l_size, std::size_t r_size, std::size_t w_block,
                                 typename Matrix::value_type couplings[])
    {
        auto blocks = W.get_sparse().block(w_block);

        const size_t chunk = 1024;
        const size_t blength = r_size*l_size;
        for(size_t rr = 0; rr < blength/chunk; ++rr) {
            for (auto it = blocks.first; it != blocks.second; ++it)
            {
                std::size_t ss1 = it->row;
                std::size_t ss2 = it->col;
                std::size_t rspin = it->row_spin;
                std::size_t cspin = it->col_spin;
                std::size_t casenr = 0;
                if (rspin == 2 && cspin == 2) casenr = 3;
                else if (rspin == 2) casenr = 1;
                else if (cspin == 2) casenr = 2;

                typename Matrix::value_type alfa_t = it->coefficient * couplings[casenr];

                assert(rr + chunk <= r_size*l_size);
                maquis::dmrg::detail::iterator_axpy(&iblock(0, in_right_offset + ss1*r_size) + rr*chunk,
                                                    &iblock(0, in_right_offset + ss1*r_size) + rr*chunk + chunk,
                                                    &oblock(0, out_right_offset + ss2*r_size) + rr*chunk,
                                                    alfa_t);
            }
        }

        for (auto it = blocks.first; it != blocks.second; ++it)
        {
            std::size_t ss1 = it->row;
            std::size_t ss2 = it->col;
            std::size_t rspin = it->row_spin;
            std::size_t cspin = it->col_spin;
            std::size_t casenr = 0;
            if (rspin == 2 && cspin == 2) casenr = 3;
            else if (rspin == 2) casenr = 1;
            else if (cspin == 2) casenr = 2;

            typename Matrix::value_type alfa_t = it->coefficient * couplings[casenr];

            std::size_t start = blength - blength%chunk;
            maquis::dmrg::detail::iterator_axpy(&iblock(0, in_right_offset + ss1*r_size) + start,
                                                &iblock(0, in_right_offset + ss1*r_size) + blength,
                                                &oblock(0, out_right_offset + ss2*r_size) + start,
                                                alfa_t);
        }
    }

    template<class Matrix, class SymmGroup>
    void rbtm(Matrix const & iblock, Matrix & oblock, typename operator_selector<Matrix, SymmGroup>::type const & W,
                         std::size_t in_left_offset, std::size_t out_right_offset, std::size_t l_size, std::size_t r_size, std::size_t w_block,
                         typename Matrix::value_type couplings[])
    {
        auto blocks = W.get_sparse().block(w_block);

        for (size_t rr = 0; rr < r_size; ++rr) {
            for (auto it = blocks.first; it != blocks.second; ++it)
            {
                std::size_t ss1 = it->row;
                std::size_t ss2 = it->col;
                std::size_t rspin = it->row_spin;
                std::size_t cspin = it->col_spin;
                std::size_t casenr = 0;
                if (rspin == 2 && cspin == 2) casenr = 3;
                else if (rspin == 2) casenr = 1;
                else if (cspin == 2) casenr = 2;

                typename Matrix::value_type alfa_t = it->coefficient * couplings[casenr];
                maquis::dmrg::detail::iterator_axpy(&iblock(in_left_offset + ss1*l_size, rr),
                                                    &iblock(in_left_offset + ss1*l_size, rr) + l_size,
                                                    &oblock(0, out_right_offset + ss2*r_size + rr),
                                                    alfa_t);
            }
        }
    }

    template <typename T>
    struct micro_task
    {
        typedef unsigned short IS;

        //T const* source;
        T scale;
        IS b2, k;
        IS l_size, r_size, stripe, out_offset;
        unsigned in_offset;
    };

    template <typename T>
    struct task_compare
    {
        bool operator ()(micro_task<T> const & t1, micro_task<T> const & t2)
        {
            return t1.out_offset < t2.out_offset;
        }
    };

    template <class Matrix, class SymmGroup>
    void op_iterate(typename operator_selector<Matrix, SymmGroup>::type const & W, std::size_t w_block,
                    typename Matrix::value_type couplings[],
                    std::vector<micro_task<typename Matrix::value_type> > & tasks,
                    micro_task<typename Matrix::value_type> tpl,
                    //const typename Matrix::value_type * source,
                    size_t in_offset,
                    size_t r_size_cache, size_t r_size, size_t out_right_offset)
    {
        auto blocks = W.get_sparse().block(w_block);
        for (auto it = blocks.first; it != blocks.second; ++it)
        {
            micro_task<typename Matrix::value_type> task = tpl;

            std::size_t ss1 = it->row;
            std::size_t ss2 = it->col;
            std::size_t rspin = it->row_spin;
            std::size_t cspin = it->col_spin;
            std::size_t casenr = 0;
            if (rspin == 2 && cspin == 2) casenr = 3;
            else if (rspin == 2) casenr = 1;
            else if (cspin == 2) casenr = 2;

            //task.source = source + ss1*tpl.l_size;
            task.in_offset = in_offset + ss1*tpl.l_size;
            task.scale = it->coefficient * couplings[casenr];
            task.r_size = r_size_cache;
            task.out_offset = out_right_offset + ss2*r_size;
            tasks.push_back(task);
        }
    }

    template<typename T>
    void task_axpy(micro_task<T> const & task, T * oblock, T const * source)
    {
        std::size_t l_size = task.l_size;
        std::size_t r_size = task.r_size;
        std::size_t stripe = task.stripe;
        std::size_t out_right_offset = task.out_offset;

        //if (source2 != task.source) throw std::runtime_error("source mismatch \n");

        for(size_t rr = 0; rr < r_size; ++rr) {
            T alfa_t = task.scale;
            maquis::dmrg::detail::iterator_axpy(source + stripe * rr,
                                                source + stripe * rr + l_size,
                                                oblock + (out_right_offset + rr) * l_size,
                                                alfa_t);
        }
    }

} // namespace detail
} // namespace SU2
} // namespace contraction

#endif