/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef MPS_INIT_HF_HPP
#define MPS_INIT_HF_HPP

#include "dmrg/mp_tensors/compression.h"

template<class Matrix, class SymmGroup, class = void>
struct hf_mps_init : public mps_initializer<Matrix, SymmGroup>
{
    hf_mps_init(BaseParameters parms_,
                std::vector<Index<SymmGroup> > const& phys_dims_,
                typename SymmGroup::charge right_end,
                std::vector<int> const& site_type)
    : parms(parms_)
    , init_bond_dimension(parms["init_bond_dimension"])
    , phys_dims(phys_dims_)
    , site_types(site_type)
    , di(parms, phys_dims_, right_end, site_type)
    {}

    typedef Lattice::pos_t pos_t;
    typedef std::size_t size_t;

    void operator()(MPS<Matrix, SymmGroup> & mps)
    {
        di.init_sectors(mps, 5, false, 1.);

        std::vector<std::size_t> hf_init = parms["hf_occ"];

        std::vector<pos_t> order(mps.length());
        if (!parms.is_set("orbital_order"))
            for (pos_t p = 0; p < mps.length(); ++p)
                order[p] = p+1;
        else
            order = parms["orbital_order"].template as<std::vector<pos_t> >();

        std::transform(order.begin(), order.end(), order.begin(), boost::lambda::_1-1);

        if (hf_init.size() != mps.length())
            throw std::runtime_error("HF occupation vector length != MPS length\n");

        typename SymmGroup::charge max_charge = SymmGroup::IdentityCharge;
        for (pos_t i = 0; i < mps.length(); ++i)
        {
            mps[i].multiply_by_scalar(0.0);

            size_t sc_input = hf_init[order[i]];
            typename SymmGroup::charge site_charge(0);

			size_t loc_dim = phys_dims[0].size();

            if (sc_input > loc_dim)
                throw std::runtime_error(
                    "The hf occ exceeds local basis dimension\n"
                );

			site_charge = phys_dims[site_types[i]][loc_dim-sc_input].first;

            max_charge = SymmGroup::fuse(max_charge, site_charge);

            #ifndef NDEBUG
            maquis::cout << "site " << i << " activating sector " << max_charge << std::endl;
            #endif

            // Set largest charge sector = all 1
            size_t max_pos = mps[i].data().left_basis().position(max_charge);
            if (max_pos >= mps[i].data().n_blocks()) {
                maquis::cout << "ERROR: Symmetry block " << max_charge << " not found\n";
                maquis::cout << "site " << i << ", site_charge " << site_charge << ", cumulated_charge "
                         << max_charge << ", block_pos: "
                         << max_pos << ", number of blocks: " << mps[i].data().n_blocks() << std::endl;

                maquis::cout << "This error occurs if the specified HF determinant is not in the same symmetry sector as the target state\n";
                exit(1);
            }
            Matrix & mfirst = mps[i].data()[max_pos];
            size_t nrow = mfirst.num_rows();
            size_t ncol = mfirst.num_cols();
            mps[i].data()[max_pos] = Matrix(nrow, ncol, 1.);

            mps[i].multiply_by_scalar(1. / mps[i].scalar_norm());
        }

        //mps = compression::l2r_compress(mps, init_bond_dimension, 1e-6);

        //maquis::cout << "\nMPS AFTER COMPRESSION:\n";
        //for(int i = 0; i < mps.length(); ++i) {
        //    maquis::cout << "mps[" << i << "]:\n" << mps[i] << std::endl;
        //    maquis::cout << mps[i].scalar_norm() << std::endl;
        //}
    }

    BaseParameters parms;
    std::size_t init_bond_dimension;
    std::vector<Index<SymmGroup> > phys_dims;
    std::vector<int> site_types;
    default_mps_init<Matrix, SymmGroup> di;
};

template<class Matrix, class SymmGroup>
struct hf_mps_init<Matrix, SymmGroup, symm_traits::enable_if_su2_t<SymmGroup> >
        : public mps_initializer<Matrix, SymmGroup>
{
    typedef Lattice::pos_t pos_t;
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    typedef std::set<charge> container_type;

    hf_mps_init(BaseParameters parms_,
                std::vector<Index<SymmGroup> > const& phys_dims_,
                charge right_end, std::vector<int> const& site_type)
    : parms(parms_)
    , phys_dims(phys_dims_)
    , site_types(site_type)
    , di(parms, phys_dims_, right_end, site_type)
    {}

    void operator()(MPS<Matrix, SymmGroup> & mps)
    {
        di.init_sectors(mps, 5, false, 1.0);

        std::vector<std::size_t> hf_init = parms["hf_occ"];

        std::vector<pos_t> order(mps.length());
        if (!parms.is_set("orbital_order"))
            for (pos_t p = 0; p < mps.length(); ++p)
                order[p] = p+1;
        else
            order = parms["orbital_order"].template as<std::vector<pos_t> >();

        std::transform(order.begin(), order.end(), order.begin(), boost::lambda::_1-1);

        if (hf_init.size() != mps.length())
            throw std::runtime_error("HF occupation vector length != MPS length\n");

        container_type bond_charges;
        bond_charges.insert(SymmGroup::IdentityCharge);

        // count total number of electrons in the given HF guess to make sure it matches
        int nelec = parms["nelec"];
        int nelec_tmp = 0;

        for (pos_t i = 0; i < mps.length(); ++i)
        {
            mps[i].multiply_by_scalar(0.0);

            size_t sc_input = hf_init[order[i]];
            container_type site_charges;

            if (sc_input > 4)
                throw std::runtime_error(
                    "The hf_occ format has been changed to: 1=empty, 2=down, 3=up, 4=updown\n (not cumulative anymore)\n"
                );
            if (phys_dims[site_types[i]].size() != 4) throw std::runtime_error("HF init expects 4 states per orbital\n");


            switch(sc_input) {
                case 4:
                    site_charges.insert(phys_dims[site_types[i]][0].first); // doubly-occ
                    // add the number of electrons on each site to the total count
                    nelec_tmp += phys_dims[site_types[i]][0].first[0];
                    break;
                case 3:
                    site_charges.insert(phys_dims[site_types[i]][1].first); // singly-occ
                    site_charges.insert(phys_dims[site_types[i]][2].first); // singly-occ
                    nelec_tmp += phys_dims[site_types[i]][1].first[0];
                    break;
                case 2:
                    site_charges.insert(phys_dims[site_types[i]][1].first); // singly-occ
                    site_charges.insert(phys_dims[site_types[i]][2].first); // singly-occ
                    nelec_tmp += phys_dims[site_types[i]][2].first[0];
                    break;
                case 1:
                    site_charges.insert(phys_dims[site_types[i]][3].first); // empty
                    nelec_tmp += phys_dims[site_types[i]][3].first[0];
                    break;
            }

            container_type next_bond_charges;
            for (typename container_type::const_iterator it = bond_charges.begin(); it != bond_charges.end(); ++it)
                for (typename container_type::const_iterator it2 = site_charges.begin(); it2 != site_charges.end(); ++it2)
                {
                    charge sector = SymmGroup::fuse(*it, *it2);
                    if (mps[i].col_dim().has(sector))
                        next_bond_charges.insert(sector);
                }

            #ifndef NDEBUG
            for (typename container_type::const_iterator it = next_bond_charges.begin(); it != next_bond_charges.end(); ++it)
                maquis::cout << "site " << i << " activating sector " << *it << std::endl;

            maquis::cout << std::endl;
            #endif

            for (typename container_type::const_iterator it = next_bond_charges.begin(); it != next_bond_charges.end(); ++it)
            {
                charge max_charge = *it;
                size_t max_pos = mps[i].data().left_basis().position(max_charge);
                if (max_pos >= mps[i].data().n_blocks()) {
                    maquis::cout << "ERROR: Symmetry block " << max_charge << " not found\n";
                    maquis::cout << "site " << i << ", cumulated_charge "
                             << max_charge << ", block_pos: "
                             << max_pos << ", number of blocks: " << mps[i].data().n_blocks() << std::endl;

                    maquis::cout << "This error occurs if the specified HF determinant is not in the same symmetry sector as the target state\n";
                    exit(1);
                }
                Matrix & mfirst = mps[i].data()[max_pos];
                size_t nrow = mfirst.num_rows();
                size_t ncol = mfirst.num_cols();
                // Set current charge sector = all 1
                mps[i].data()[max_pos] = Matrix(nrow, ncol, 1.);
            }


            mps[i].multiply_by_scalar(1. / mps[i].scalar_norm());
            std::swap(next_bond_charges, bond_charges);
        }

        if (nelec_tmp != nelec)
            throw std::runtime_error("Number of electrons in the HF guess is incorrect: expected " + std::to_string(nelec) + " but got "
                                        + std::to_string(nelec_tmp));
    }

    BaseParameters parms;
    std::vector<Index<SymmGroup> > phys_dims;
    std::vector<int> site_types;
    default_mps_init<Matrix, SymmGroup> di;
};

#endif
