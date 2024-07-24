/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef JOINS_H
#define JOINS_H

#include "dmrg/mp_tensors/mpstensor.h"

template <class Matrix, class SymmGroup>
MPSTensor<Matrix, SymmGroup> join(MPSTensor<Matrix, SymmGroup> const & m1, MPSTensor<Matrix, SymmGroup> const & m2,
                                  boundary_flag_t boundary_f=no_boundary_f)
{
    Index<SymmGroup> phys_i = m1.site_dim();
    if (m1.site_dim() != m2.site_dim())
        for (typename Index<SymmGroup>::const_iterator it = m2.site_dim().begin(); it != m2.site_dim().end(); ++it)
            if (!phys_i.has(it->first))
               phys_i.insert(*it);  
    
    m1.make_left_paired();
    m2.make_left_paired();
    
    
    MPSTensor<Matrix, SymmGroup> ret;
    // phys_i is untouched
    ret.phys_i = phys_i;
    
    // computing new left_i
    ret.left_i = m1.left_i;
    if (boundary_f != l_boundary_f) {
        for (typename Index<SymmGroup>::const_iterator it = m2.left_i.begin();
             it != m2.left_i.end(); ++it) {
            if (ret.left_i.has(it->first))
                ret.left_i[ret.left_i.position(it->first)].second += it->second;
            else
                ret.left_i.insert(*it);
        }
    }
    
    // computing new right_i
    ret.right_i = m1.right_i;
    if (boundary_f != r_boundary_f) {
        for (typename Index<SymmGroup>::const_iterator it = m2.right_i.begin();
             it != m2.right_i.end(); ++it) {
            if (ret.right_i.has(it->first))
                ret.right_i[ret.right_i.position(it->first)].second += it->second;
            else
                ret.right_i.insert(*it);
        }
    }
    
    ProductBasis<SymmGroup> out_left_pb(phys_i, ret.left_i);
    Index<SymmGroup> const& out_right = ret.right_i;
    
    using std::size_t;
    
    for (size_t t=0; t<2; ++t) // t=0 --> mps1, t=1 --> mps2
    {
        MPSTensor<Matrix, SymmGroup> const & m = (t==0) ? m1 : m2;
        Index<SymmGroup> const & phys_i_m = (t==0) ? m1.site_dim() : m2.site_dim();
        ProductBasis<SymmGroup> in_left(phys_i_m, m.row_dim());
        
        for (size_t b = 0; b < m.data().n_blocks(); ++b) {
            typename SymmGroup::charge const& sl_charge = m.data().basis().left_charge(b); // phys + left
            typename SymmGroup::charge const& r_charge = m.data().basis().right_charge(b); // right
            size_t out_r_charge_i = out_right.position(r_charge);

            if (!ret.data().has_block(sl_charge, r_charge))
                ret.data().insert_block(Matrix(out_left_pb.size(sl_charge), out_right[out_r_charge_i].second),
                                       sl_charge, r_charge);

            Matrix & nb = ret.data()(sl_charge, r_charge);
            
            size_t in_r_size = m.data().basis().right_size(b);
            size_t out_r_offset = 0;
            if (t == 1 && boundary_f != r_boundary_f)
                out_r_offset += m1.col_dim().size_of_block(r_charge, true);
            
            for (size_t s=0; s<phys_i_m.size(); ++s) {
                typename SymmGroup::charge const& s_charge = phys_i_m[s].first;
                typename SymmGroup::charge l_charge = SymmGroup::fuse(sl_charge, -s_charge); // left
                
                if (!m.row_dim().has(l_charge))
                    continue;
                
                size_t in_l_size = m.row_dim().size_of_block(l_charge, true);
                size_t in_l_offset = in_left(s_charge, l_charge);

                size_t out_l_size = ret.row_dim().size_of_block(l_charge, true);
                size_t out_l_offset = out_left_pb(s_charge, l_charge);
                if (t == 1 && boundary_f != l_boundary_f)
                    out_l_offset += m1.row_dim().size_of_block(l_charge, true);
                
                for (size_t ss=0; ss<phys_i[s].second; ++ss) {
                    copy_block(m.data()[b], in_l_offset+ss*in_l_size, 0, 
                               nb, out_l_offset+ss*out_l_size, out_r_offset,
                               in_l_size, in_r_size);
                }
            }
        }
    }
    
    // check right_pairing
    assert( weak_equal(ret.right_i, ret.data().right_basis()) );
    
    return ret;
}


#endif
