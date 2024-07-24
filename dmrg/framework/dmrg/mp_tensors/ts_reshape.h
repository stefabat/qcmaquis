/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#include <vector>
#include <utility>
#include <alps/numeric/real.hpp>

#include "dmrg/mp_tensors/mpstensor.h"
#include "dmrg/mp_tensors/reshapes.h"
#include "dmrg/block_matrix/block_matrix_algorithms.h"


namespace ts_reshape {
    
    template<class Matrix, class SymmGroup>
    void reshape_both_to_left(Index<SymmGroup> const & physical_i_left,
                              Index<SymmGroup> const & physical_i_right,
                              Index<SymmGroup> const & left_i,
                              Index<SymmGroup> const & right_i,
                              block_matrix<Matrix, SymmGroup> const & m1,
                              block_matrix<Matrix, SymmGroup> & m2)
    {   
        m2 = block_matrix<Matrix, SymmGroup>();
        
        typedef std::size_t size_t;
        typedef typename SymmGroup::charge charge;
        
        Index<SymmGroup> phys2_i = physical_i_left*physical_i_right;
        ProductBasis<SymmGroup> phys_pb(physical_i_left, physical_i_right);
        ProductBasis<SymmGroup> in_left(physical_i_left, left_i);
        ProductBasis<SymmGroup> in_right(physical_i_right, right_i,
                                         boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                             -boost::lambda::_1, boost::lambda::_2));
        ProductBasis<SymmGroup> out_left(phys2_i, left_i);
       
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            for (size_t s1 = 0; s1 < physical_i_left.size(); ++s1)
            {
                size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                          -physical_i_left[s1].first));
                if(l == left_i.size()) continue;
                for (size_t s2 = 0; s2 < physical_i_right.size(); ++s2)
                {
                    size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block),
                                                                physical_i_right[s2].first));
                    if(r == right_i.size()) continue;
                    
                    {                            
                        charge s_charge = SymmGroup::fuse(physical_i_left[s1].first, physical_i_right[s2].first);
                        
                        charge out_l_charge = SymmGroup::fuse(s_charge, left_i[l].first);
                        charge out_r_charge = right_i[r].first;
                        
                        //if (! m1.has_block(in_l_charge, in_r_charge) ) continue;
                        
                        // Why is this supposed to work?
                        if (! m2.has_block(out_l_charge, out_r_charge) )
                            m2.insert_block(new Matrix(out_left.size(s_charge, left_i[l].first), right_i[r].second, 0),
                                            out_l_charge, out_r_charge);
                        
                        //maquis::dmrg::detail::reshape_b2l( m2(out_l_charge, out_r_charge), m1(in_l_charge, in_r_charge), 
                        maquis::dmrg::detail::reshape_b2l( m2(out_l_charge, out_r_charge), m1[block], 
                                                           in_left(physical_i_left[s1].first, left_i[l].first), in_right(physical_i_right[s2].first, right_i[r].first),
                                                           out_left(s_charge, left_i[l].first), phys_pb(physical_i_left[s1].first, physical_i_right[s2].first),
                                                           physical_i_left[s1].second, physical_i_right[s2].second, left_i[l].second, right_i[r].second );
                    }
                }
            }
        }
        
    }
    
    /* 
    template<class Matrix, class SymmGroup>
    void reshape_right_to_left(Index<SymmGroup> physical_i,
                               Index<SymmGroup> left_i,
                               Index<SymmGroup> right_i,
                               block_matrix<Matrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
    {   }
    */
    
    template<class Matrix, class SymmGroup>
    void reshape_left_to_both(Index<SymmGroup> const & physical_i_left,
                              Index<SymmGroup> const & physical_i_right,
                              Index<SymmGroup> const & left_i,
                              Index<SymmGroup> const & right_i,
                              block_matrix<Matrix, SymmGroup> const & m1,
                              block_matrix<Matrix, SymmGroup> & m2)
    {   
        
        m2 = block_matrix<Matrix, SymmGroup>();
        
        typedef std::size_t size_t;
        typedef typename SymmGroup::charge charge;
        
        Index<SymmGroup> phys2_i = physical_i_left*physical_i_right;
        ProductBasis<SymmGroup> phys_pb(physical_i_left, physical_i_right);
        ProductBasis<SymmGroup> in_left(phys2_i, left_i);
        
        ProductBasis<SymmGroup> out_right(physical_i_right, right_i,
                                          boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                              -boost::lambda::_1, boost::lambda::_2));
        ProductBasis<SymmGroup> out_left(physical_i_left, left_i);
       
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            size_t r = right_i.position(m1.basis().right_charge(block));
            if(r == right_i.size()) throw std::runtime_error("m1 matrix inconsistent with right_i.");

            for (size_t s1 = 0; s1 < physical_i_left.size(); ++s1)
                for (size_t s2 = 0; s2 < physical_i_right.size(); ++s2)
                {
                    charge s_charge = SymmGroup::fuse(physical_i_left[s1].first, physical_i_right[s2].first);
                    
                    size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block), -s_charge));
                    if(l == left_i.size()) continue;

                    {
                        charge out_l_charge = SymmGroup::fuse(physical_i_left[s1].first, left_i[l].first);
                        charge out_r_charge = SymmGroup::fuse(-physical_i_right[s2].first, right_i[r].first);
                        //charge in_l_charge = SymmGroup::fuse(s_charge, left_i[l].first);
                        //charge in_r_charge = right_i[r].first;
                        
                        size_t o = m2.find_block(out_l_charge, out_r_charge);
                        if ( o == m2.n_blocks() )
                            o = m2.insert_block(new Matrix(out_left.size(physical_i_left[s1].first, left_i[l].first),
                                                   out_right.size(-physical_i_right[s2].first, right_i[r].first), 0),
                                            out_l_charge, out_r_charge);
                        
                        maquis::dmrg::detail::reshape_l2b( m2[o], m1[block],
                                                           in_left(s_charge, left_i[l].first), phys_pb(physical_i_left[s1].first, physical_i_right[s2].first),
                                                           out_left(physical_i_left[s1].first, left_i[l].first), out_right(physical_i_right[s2].first, right_i[r].first),
                                                           physical_i_left[s1].second, physical_i_right[s2].second, left_i[l].second, right_i[r].second );
                    }
                }
        }
        
    }
    
    template<class Matrix, class SymmGroup>
    void reshape_right_to_both(Index<SymmGroup> const & physical_i_left,
                               Index<SymmGroup> const & physical_i_right,
                               Index<SymmGroup> const & left_i,
                               Index<SymmGroup> const & right_i,
                               block_matrix<Matrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
    {
        
        m2 = block_matrix<Matrix, SymmGroup>();
        
        typedef std::size_t size_t;
        typedef typename SymmGroup::charge charge;
        
        Index<SymmGroup> phys2_i = physical_i_left*physical_i_right;
        ProductBasis<SymmGroup> phys_pb(physical_i_left, physical_i_right);
        ProductBasis<SymmGroup> in_right(phys2_i, right_i,
                                         boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                             -boost::lambda::_1, boost::lambda::_2));
        
        ProductBasis<SymmGroup> out_right(physical_i_right, right_i,
                                          boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                              -boost::lambda::_1, boost::lambda::_2));
        ProductBasis<SymmGroup> out_left(physical_i_left, left_i);
        
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            size_t l = left_i.position(m1.basis().left_charge(block));
            if(l == left_i.size()) continue;
                  
            for (size_t s1 = 0; s1 < physical_i_left.size(); ++s1)
                for (size_t s2 = 0; s2 < physical_i_right.size(); ++s2)
                {
                    charge s_charge = SymmGroup::fuse(physical_i_left[s1].first, physical_i_right[s2].first);
                    //size_t s_out = phys2_i.position(s_charge);
                    size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block), s_charge));
                    if(r == right_i.size()) continue;

                    {
                        charge out_l_charge = SymmGroup::fuse(physical_i_left[s1].first, left_i[l].first);
                        charge out_r_charge = SymmGroup::fuse(-physical_i_right[s2].first, right_i[r].first);
                        //charge in_l_charge = left_i[l].first;
                        //charge in_r_charge = SymmGroup::fuse(-s_charge, right_i[r].first);
                        
                        //if (! m1.has_block(in_l_charge, in_r_charge) ) continue;
                        
                        if (! m2.has_block(out_l_charge, out_r_charge) )
                            m2.insert_block(new Matrix(out_left.size(physical_i_left[s1].first, left_i[l].first),
                                                   out_right.size(-physical_i_right[s2].first, right_i[r].first), 0),
                                            out_l_charge, out_r_charge);
                        
                        size_t in_right_offset  = in_right  (s_charge            , right_i[r].first);
                        size_t out_right_offset = out_right (physical_i_right[s2].first, right_i[r].first);
                        size_t out_left_offset  = out_left  (physical_i_left[s1].first, left_i[l].first);
                        size_t in_phys_offset   = phys_pb   (physical_i_left[s1].first, physical_i_right[s2].first);
                        
                        //Matrix const & in_block = m1(in_l_charge, in_r_charge);
                        Matrix const & in_block = m1[block];
                        Matrix & out_block = m2(out_l_charge, out_r_charge);
                        
                        for (size_t ss1 = 0; ss1 < physical_i_left[s1].second; ++ss1)
                            for (size_t ss2 = 0; ss2 < physical_i_right[s2].second; ++ss2)
                            {
                                size_t ss_out = in_phys_offset + ss1*physical_i_right[s2].second + ss2;
                                for (size_t rr = 0; rr < right_i[r].second; ++rr)
                                    for (size_t ll = 0; ll < left_i[l].second; ++ll)
                                        out_block(out_left_offset + ss1*left_i[l].second + ll, out_right_offset + ss2*right_i[r].second + rr) = in_block(ll, in_right_offset + ss_out*right_i[r].second + rr);
                            }
                        
                    }
                }
        }
    }
    
    /*
    template<class Matrix, class SymmGroup>
    void reshape_left_to_right(Index<SymmGroup> physical_i,
                               Index<SymmGroup> left_i,
                               Index<SymmGroup> right_i,
                               block_matrix<Matrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
    {   }
    */
    
    template<class Matrix, class SymmGroup>
    void reshape_both_to_right(Index<SymmGroup> const & physical_i_left,
                               Index<SymmGroup> const & physical_i_right,
                               Index<SymmGroup> const & left_i,
                               Index<SymmGroup> const & right_i,
                               block_matrix<Matrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
    {   
        
        m2 = block_matrix<Matrix, SymmGroup>();
        
        typedef std::size_t size_t;
        typedef typename SymmGroup::charge charge;
        
        Index<SymmGroup> phys2_i = physical_i_left*physical_i_right;
        ProductBasis<SymmGroup> phys_pb(physical_i_left, physical_i_right);
        ProductBasis<SymmGroup> in_left(physical_i_left, left_i);
        ProductBasis<SymmGroup> in_right(physical_i_right, right_i,
                                         boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                             -boost::lambda::_1, boost::lambda::_2));
        ProductBasis<SymmGroup> out_right(phys2_i, right_i,
                                          boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                              -boost::lambda::_1, boost::lambda::_2));
       
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            for (size_t s1 = 0; s1 < physical_i_left.size(); ++s1)
            {
                size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                               -physical_i_left[s1].first));
                if(l == left_i.size()) continue;

                for (size_t s2 = 0; s2 < physical_i_right.size(); ++s2)
                {
                    size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block),
                                                                physical_i_right[s2].first));
                    if(r == right_i.size()) continue;

                    {
                        charge s_charge = SymmGroup::fuse(physical_i_left[s1].first, physical_i_right[s2].first);
                        
                        charge out_l_charge = left_i[l].first;
                        charge out_r_charge = SymmGroup::fuse(-s_charge, right_i[r].first);
                        
                        //if (! m1.has_block(in_l_charge, in_r_charge) ) continue;
                        
                        if (! m2.has_block(out_l_charge, out_r_charge) )
                            m2.insert_block(new Matrix(left_i[l].second, out_right.size(-s_charge, right_i[r].first), 0),
                                            out_l_charge, out_r_charge);
                        
                        maquis::dmrg::detail::reshape_b2r( m2(out_l_charge, out_r_charge), m1[block], 
                                                           in_left(physical_i_left[s1].first, left_i[l].first), in_right(physical_i_right[s2].first, right_i[r].first),
                                                           out_right(s_charge, right_i[r].first), phys_pb(physical_i_left[s1].first, physical_i_right[s2].first),
                                                           physical_i_left[s1].second, physical_i_right[s2].second, left_i[l].second, right_i[r].second );
                    }
                }
            }
        }
        
    }

    template<class Matrix, class SymmGroup>
    void reshape_both_to_physright(Index<SymmGroup> const & physical_i_left,
                                   Index<SymmGroup> const & physical_i_right,
                                   Index<SymmGroup> const & left_i,
                                   Index<SymmGroup> const & right_i,
                                   block_matrix<Matrix, SymmGroup> const & m1,
                                   block_matrix<Matrix, SymmGroup> & m2)
    {
        
        m2 = block_matrix<Matrix, SymmGroup>();
        
        typedef std::size_t size_t;
        typedef typename SymmGroup::charge charge;
        
        ProductBasis<SymmGroup> in_left(physical_i_left, left_i);
        ProductBasis<SymmGroup> in_right(physical_i_right, right_i,
                                         boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                             -boost::lambda::_1, boost::lambda::_2));
        ProductBasis<SymmGroup> out_left(left_i, right_i,
                                          boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                              boost::lambda::_1, -boost::lambda::_2));
        ProductBasis<SymmGroup> out_right(physical_i_left, physical_i_right,
                                          boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                              -boost::lambda::_1, -boost::lambda::_2));
        
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            for (size_t s1 = 0; s1 < physical_i_left.size(); ++s1)
            {
                size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                           -physical_i_left[s1].first));
                if(l == left_i.size()) continue;
                
                for (size_t s2 = 0; s2 < physical_i_right.size(); ++s2)
                {
                    size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block),
                                                                physical_i_right[s2].first));
                    if(r == right_i.size()) continue;
                    
                    {
                        charge s_charge = SymmGroup::fuse(physical_i_left[s1].first, physical_i_right[s2].first);
                        
                        charge out_l_charge = SymmGroup::fuse(left_i[l].first, -right_i[r].first);
                        charge out_r_charge = -s_charge;
                        
                        size_t outb = m2.find_block(out_l_charge, out_r_charge);
                        if (outb == m2.n_blocks())
                            outb = m2.insert_block(new Matrix(out_left.size(out_l_charge), out_right.size(out_r_charge), 0),
                                                   out_l_charge, out_r_charge);
                        
                        size_t in_left_offset = in_left(physical_i_left[s1].first, left_i[l].first);
                        size_t in_right_offset = in_right(physical_i_right[s2].first, right_i[r].first);
                        size_t out_left_offset = out_left(left_i[l].first, right_i[r].first);
                        size_t out_right_offset = out_right(physical_i_left[s1].first, physical_i_right[s2].first);
                        
                        //Matrix const & in_block = m1(in_l_charge, in_r_charge);
                        Matrix const & in_block = m1[block];
                        Matrix & out_block = m2[outb];
                        
                        for (size_t ss1 = 0; ss1 < physical_i_left[s1].second; ++ss1)
                            for (size_t ss2 = 0; ss2 < physical_i_right[s2].second; ++ss2)
                                for (size_t rr = 0; rr < right_i[r].second; ++rr)
                                    for (size_t ll = 0; ll < left_i[l].second; ++ll)
                                        out_block(out_left_offset + rr*left_i[l].second+ll, out_right_offset + ss2*physical_i_left[s1].second + ss1) = in_block(in_left_offset + ss1*left_i[l].second+ll, in_right_offset + ss2*right_i[r].second+rr);
                    }
                }
            }
        }
        
    }

} // namespace ts_reshape
