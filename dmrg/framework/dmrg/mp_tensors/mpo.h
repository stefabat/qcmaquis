/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef MPO_H
#define MPO_H

#include <vector>
#include <set>

#include "dmrg/mp_tensors/mpotensor.h"

template<class Matrix, class SymmGroup, typename = void>
class MPO : public std::vector<MPOTensor<Matrix, SymmGroup> >
{
public:
    typedef MPOTensor<Matrix, SymmGroup> elem_type;

    MPO() { }

    MPO(std::size_t L, elem_type elem = elem_type())
    : std::vector<elem_type>(L, elem)
    , core_energy(0)
    { }

    std::size_t length() const { return this->size(); }

    void compress(double cutoff)
    {
        calc_charges();

        for (int p = 0; p < this->size()-1; ++p) {
            MPOTensor<Matrix, SymmGroup> b1 = (*this)[p], b2 = (*this)[p+1];

            block_matrix<Matrix, SymmGroup> left = make_left_matrix(p);
            block_matrix<Matrix, SymmGroup> right = make_right_matrix(p+1);

            block_matrix<Matrix, SymmGroup> M, U, V;

            block_matrix<typename alps::numeric::associated_real_diagonal_matrix<Matrix>::type, SymmGroup> S, Sqrt;
            gemm(left, right, M);
            svd_truncate(M, U, V, S, cutoff, 100000, false);
            Sqrt = sqrt(S);
            gemm(U, Sqrt, left);
            gemm(Sqrt, V, right);

            maquis::cout << "MPO bond truncation: " << bond_indices[p+1].sum_of_sizes() << " -> ";
            replace_pair(left, right, p);
            maquis::cout << bond_indices[p+1].sum_of_sizes() << std::endl;
        }
    }

    void setCoreEnergy(double e) { core_energy = e; }
    double getCoreEnergy() const { return core_energy; }

private:
    std::vector<std::map<std::size_t, typename SymmGroup::charge> > bond_index_charges;
    std::vector<Index<SymmGroup> > bond_indices;
    double core_energy;

    void calc_charges()
    {
        size_t L = this->size();
        bond_index_charges.clear();
        bond_index_charges.resize(L+1);

        bond_index_charges[0][0] = SymmGroup::IdentityCharge;

        for (size_t p = 1; p <= L; ++p)
        {
            for (size_t c = 0; c < (*this)[p-1].col_dim(); ++c) {
                std::set<typename SymmGroup::charge> charge_diffs;
                for (size_t r = 0; r < (*this)[p-1].row_dim(); ++r) {
                    assert( bond_index_charges[p-1].count(r) > 0 );
                    if (!(*this)[p-1].has(r,c))
                        continue;
                    for (size_t b = 0; b < (*this)[p-1].at(r, c).op().n_blocks(); ++b) {
                        charge_diffs.insert(SymmGroup::fuse(bond_index_charges[p-1][r],
                                                            SymmGroup::fuse((*this)[p-1].at(r,c).op().basis().left_charge(b),
                                                                            -(*this)[p-1].at(r,c).op().basis().right_charge(b))));
//                        maquis::cout << r << " " << c << std::endl;
//                        maquis::cout << bond_index_charges[p-1][r] << std::endl;
//                        maquis::cout << (*this)[p-1](r,c).basis().left_charge(b) << std::endl;
//                        maquis::cout << (*this)[p-1](r,c).basis().right_charge(b) << std::endl;
//                        std::copy(charge_diffs.begin(), charge_diffs.end(),
//                                  std::ostream_iterator<typename SymmGroup::charge>(maquis::cout, " "));
//                        maquis::cout << std::endl << std::endl;
                    }
                }
#ifndef NDEBUG
                assert( charge_diffs.size() <= 1 );
#endif
                if (charge_diffs.size() == 1)
                    bond_index_charges[p][c] = *charge_diffs.begin();
                else
                    bond_index_charges[p][c] = SymmGroup::IdentityCharge; //bond_index_charges[p-1][c];
            }
        }

        bond_indices.clear();
        bond_indices.resize(L+1);
        for (size_t p = 0; p <= L; ++p)
        {
            Index<SymmGroup> & index = bond_indices[p];
            for (typename std::map<std::size_t, typename SymmGroup::charge>::iterator it
                 = bond_index_charges[p].begin();
                 it != bond_index_charges[p].end();
                 ++it)
                if (index.has(it->second))
                    index[index.position(it->second)] = std::make_pair(it->second, index.size_of_block(it->second)+1);
                else
                    index.insert(std::make_pair(it->second, 1));
        }
    }

    block_matrix<Matrix, SymmGroup> make_left_matrix(std::size_t p)
    {
        typedef typename SymmGroup::charge charge;

        Index<SymmGroup> phys_i;
        for (size_t r = 0; r < (*this)[p].row_dim(); ++r)
            for (size_t c = 0; c < (*this)[p].col_dim(); ++c)
            {
                if (!(*this)[p].has(r,c))
                    continue;
                for (size_t cs = 0; cs < (*this)[p].at(r, c).op().basis().size(); ++cs) {
                    //std::pair<charge, size_t> sector = (*this)[p].at(r, c).op.left_basis()[cs];
                    typename DualIndex<SymmGroup>::value_type sector = (*this)[p].at(r, c).op().basis()[cs];
                    //if (! phys_i.has(sector.first))
                    if (! phys_i.has(sector.lc))
                        phys_i.insert(std::make_pair(sector.lc, sector.ls));
                }
            }

        Index<SymmGroup> left_i = phys_i * adjoin(phys_i) * bond_indices[p];
        Index<SymmGroup> right_i = bond_indices[p+1];

        left_i = common_subset(left_i, right_i);

        block_matrix<Matrix, SymmGroup> ret(left_i, right_i);

        std::map<charge, size_t> visited_c_basis;
        for (size_t c = 0; c < (*this)[p].col_dim(); ++c) {
            int outr = -1;
            for (size_t r = 0; r < (*this)[p].row_dim(); ++r)
                for (size_t ls = 0; ls < phys_i.size(); ++ls)
                    for (size_t rs = 0; rs < phys_i.size(); ++rs) {
                        charge lc = SymmGroup::fuse(bond_index_charges[p][r],
                                                    SymmGroup::fuse(phys_i[ls].first,
                                                                    -phys_i[rs].first));
                        charge rc = bond_index_charges[p+1][c];

                        if (lc != rc)
                            continue;

                        outr++;

                        if (! (*this)[p].has(r,c))
                            continue;
                        if (! (*this)[p].at(r,c).op().has_block(phys_i[ls].first, phys_i[rs].first) )
                            continue;

                        //std::size_t cs = (*this)[p].at(r, c).op().left_basis().position(phys_i[ls].first);
                        std::size_t cs = (*this)[p].at(r, c).op().basis().position(phys_i[ls].first, phys_i[rs].first);

                        assert( lc == rc );
                        assert( outr < left_i.size_of_block(lc) );

                        // ...for now...
                        assert( num_rows((*this)[p].at(r,c).op()[cs]) == 1 );
                        assert( num_cols((*this)[p].at(r,c).op()[cs]) == 1 );

                        ret(std::make_pair(lc, outr),
                            std::make_pair(rc, visited_c_basis[rc])) =
                        (*this)[p].at(r,c).op()[cs](0,0);

//                        maquis::cout << (*this)[p](r,c)[cs](0,0) << " | ";
//                        maquis::cout << r << " " << c << " " << phys_i[ls].first << " " << phys_i[rs].first;
//                        maquis::cout << " -> ";
//                        maquis::cout << "(" << lc << "," << outr << ") (" << rc << "," << visited_c_basis[rc] << ")" << std::endl;
                    }
            visited_c_basis[bond_index_charges[p+1][c]]++;
        }

//        maquis::cout << ret << std::endl;
//        maquis::cout << "###" << std::endl;

        return ret;
    }

    block_matrix<Matrix, SymmGroup> make_right_matrix(std::size_t p)
    {
        typedef typename SymmGroup::charge charge;

        Index<SymmGroup> phys_i;
        for (size_t r = 0; r < (*this)[p].row_dim(); ++r)
            for (size_t c = 0; c < (*this)[p].col_dim(); ++c)
            {
                if (!(*this)[p].has(r,c))
                    continue;
                for (size_t cs = 0; cs < (*this)[p].at(r, c).op().basis().size(); ++cs) {
                    //std::pair<charge, size_t> sector = (*this)[p].at(r, c).op.left_basis()[cs];
                    typename DualIndex<SymmGroup>::value_type sector = (*this)[p].at(r, c).op().basis()[cs];
                    //if (! phys_i.has(sector.first))
                    if (! phys_i.has(sector.lc))
                        //phys_i.insert(sector);
                        phys_i.insert(std::make_pair(sector.lc, sector.ls));
                }
            }

        Index<SymmGroup> left_i = bond_indices[p];
        Index<SymmGroup> right_i = adjoin(phys_i) * phys_i * bond_indices[p+1];

        right_i = common_subset(right_i, left_i);

        block_matrix<Matrix, SymmGroup> ret(left_i, right_i);

        std::map<charge, size_t> visited_r_basis;
        for (size_t r = 0; r < (*this)[p].row_dim(); ++r) {
            int outc = -1;
            for (size_t c = 0; c < (*this)[p].col_dim(); ++c)
                for (size_t ls = 0; ls < phys_i.size(); ++ls)
                    for (size_t rs = 0; rs < phys_i.size(); ++rs) {
                        charge lc = bond_index_charges[p][r];
                        charge rc = SymmGroup::fuse(bond_index_charges[p+1][c],
                                                    SymmGroup::fuse(-phys_i[ls].first,
                                                                    phys_i[rs].first));

                        if (lc != rc)
                            continue;

                        outc++;

                        if (! (*this)[p].has(r,c))
                            continue;
                        if (! (*this)[p].at(r, c).op().has_block(phys_i[ls].first, phys_i[rs].first) )
                            continue;

                        //std::size_t cs = (*this)[p].at(r, c).op().left_basis().position(phys_i[ls].first);
                        std::size_t cs = (*this)[p].at(r, c).op().basis().position(phys_i[ls].first, phys_i[rs].first);

                        assert( lc == rc );
                        assert( outc < right_i.size_of_block(rc) );

                        // ...for now...
                        assert( num_rows((*this)[p].at(r,c).op()[cs]) == 1 );
                        assert( num_cols((*this)[p].at(r,c).op()[cs]) == 1 );

                        ret(std::make_pair(lc, visited_r_basis[lc]),
                            std::make_pair(rc, outc)) =
                        (*this)[p].at(r,c).op()[cs](0,0);

//                        maquis::cout << (*this)[p](r,c)[cs](0,0) << " | ";
//                        maquis::cout << r << " " << c << " " << phys_i[ls].first << " " << phys_i[rs].first;
//                        maquis::cout << " -> ";
//                        maquis::cout << "(" << lc << "," << visited_r_basis[lc] << ") (" << rc << "," << outc << ")" << std::endl;
                    }
            visited_r_basis[bond_index_charges[p][r]]++;
        }

//        maquis::cout << ret << std::endl;
//        maquis::cout << "###" << std::endl;

        return ret;
    }

    void replace_pair(block_matrix<Matrix, SymmGroup> & left,
                      block_matrix<Matrix, SymmGroup> & right,
                      std::size_t p)
    {
        typedef typename SymmGroup::charge charge;

        Index<SymmGroup> phys_i;
        for (size_t r = 0; r < (*this)[p].row_dim(); ++r)
            for (size_t c = 0; c < (*this)[p].col_dim(); ++c)
            {
                for (size_t cs = 0; cs < (*this)[p].at(r, c).op().basis().size(); ++cs) {
                    //std::pair<charge, size_t> sector = (*this)[p].at(r, c).op.left_basis()[cs];
                    typename DualIndex<SymmGroup>::value_type sector = (*this)[p].at(r, c).op().basis()[cs];
                    //if (! phys_i.has(sector.first))
                    if (! phys_i.has(sector.lc))
                        //phys_i.insert(sector);
                        phys_i.insert(std::make_pair(sector.lc, sector.ls));
                }
            }

        assert( left.right_basis() == right.left_basis() );
        bond_indices[p+1] = left.right_basis();
        {
            std::size_t count = 0;
            bond_index_charges[p+1].clear();
            Index<SymmGroup> left_right_basis_cp = left.right_basis();
            for (typename Index<SymmGroup>::basis_iterator it = left_right_basis_cp.basis_begin();
                 !it.end(); ++it)
                bond_index_charges[p+1][count++] = (*it).first;
        }

        (*this)[p] = MPOTensor<Matrix, SymmGroup>((*this)[p].row_dim(),
                                                  bond_indices[p+1].sum_of_sizes());

        std::map<charge, size_t> visited_c_basis;
        for (size_t c = 0; c < (*this)[p].col_dim(); ++c) {
            int outr = -1;
            for (size_t r = 0; r < (*this)[p].row_dim(); ++r)
                for (size_t ls = 0; ls < phys_i.size(); ++ls)
                    for (size_t rs = 0; rs < phys_i.size(); ++rs)
                    {
                        charge lc = SymmGroup::fuse(bond_index_charges[p][r],
                                                    SymmGroup::fuse(phys_i[ls].first,
                                                                    -phys_i[rs].first));
                        charge rc = bond_index_charges[p+1][c];

                        if (lc != rc)
                            continue;

                        outr++;

                        typename Matrix::value_type val = left(std::make_pair(lc, outr),
                                                               std::make_pair(rc, visited_c_basis[rc]));

                        if (std::abs(val) > 1e-40) {
                            typename operator_selector<Matrix, SymmGroup>::type block;
                            charge blc = phys_i[ls].first, brc = phys_i[rs].first;
                            if ( (*this)[p].has(r,c) )
                                block = (*this)[p].at(r,c).op();
                            block.insert_block(Matrix(1, 1, val), blc, brc);
                            (*this)[p].set(r, c, block);

//                            maquis::cout << val << " | ";
//                            maquis::cout << r << " " << c << " " << phys_i[ls].first << " " << phys_i[rs].first;
//                            maquis::cout << " <- ";
//                            maquis::cout << "(" << lc << "," << outr << ") (" << rc << "," << visited_c_basis[rc] << ")" << std::endl;
                        }
                    }
            visited_c_basis[bond_index_charges[p+1][c]]++;
        }

        (*this)[p+1] = MPOTensor<Matrix, SymmGroup>(bond_indices[p+1].sum_of_sizes(),
                                                    (*this)[p+1].col_dim());

        std::map<charge, size_t> visited_r_basis;
        for (size_t r = 0; r < (*this)[p+1].row_dim(); ++r) {
            int outc = -1;
            for (size_t c = 0; c < (*this)[p+1].col_dim(); ++c)
                for (size_t ls = 0; ls < phys_i.size(); ++ls)
                    for (size_t rs = 0; rs < phys_i.size(); ++rs)
                    {
                        charge lc = bond_index_charges[p+1][r];
                        charge rc = SymmGroup::fuse(bond_index_charges[p+2][c],
                                                    SymmGroup::fuse(-phys_i[ls].first,
                                                                    phys_i[rs].first));

                        if (lc != rc)
                            continue;

                        outc++;

                        typename Matrix::value_type val = right(std::make_pair(lc, visited_r_basis[lc]),
                                                                std::make_pair(rc, outc));

                        if (std::abs(val) > 1e-40) {
                            typename operator_selector<Matrix, SymmGroup>::type block;
                            charge blc = phys_i[ls].first, brc = phys_i[rs].first;
                            if ( (*this)[p+1].has(r,c) )
                                block = (*this)[p+1].at(r,c).op();
                            block.insert_block(Matrix(1, 1, val), blc, brc);
                            (*this)[p+1].set(r, c, block);

//                            maquis::cout << val << " | ";
//                            maquis::cout << r << " " << c << " " << phys_i[ls].first << " " << phys_i[rs].first;
//                            maquis::cout << " <- ";
//                            maquis::cout << "(" << lc << "," << visited_r_basis[lc] << ") (" << rc << "," << outc << ")" << std::endl;
                        }
                    }
            visited_r_basis[bond_index_charges[p+1][r]]++;
        }
    }
};

template<class Matrix, class SymmGroup>
class MPO<Matrix, SymmGroup, symm_traits::enable_if_su2_t<SymmGroup> >
    : public std::vector<MPOTensor<Matrix, SymmGroup> >
{
public:
    typedef MPOTensor<Matrix, SymmGroup> elem_type;

    MPO() { }

    MPO(std::size_t L, elem_type elem = elem_type())
    : std::vector<elem_type>(L, elem)
    { }

    std::size_t length() const { return this->size(); }

    void compress(double cutoff)
    {
        throw std::runtime_error("MPO compression for SU2 not implemented\n");
    }

    void setCoreEnergy(double e) { core_energy = e; }
    double getCoreEnergy() const { return core_energy; }

private:
    double core_energy;
};

#endif
