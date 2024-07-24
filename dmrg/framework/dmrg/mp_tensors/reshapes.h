/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef RESHAPE_H
#define RESHAPE_H

#include <map>
#include "dmrg/block_matrix/indexing.h"

template<class Matrix, class SymmGroup>
void reshape_left_to_right(Index<SymmGroup> physical_i,
                           Index<SymmGroup> left_i,
                           Index<SymmGroup> right_i,
                           block_matrix<Matrix, SymmGroup> const & m1,
                           block_matrix<Matrix, SymmGroup> & m2)
{   
    
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_left(physical_i, left_i);
    ProductBasis<SymmGroup> out_right(physical_i, right_i,
                                      boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                          -boost::lambda::_1, boost::lambda::_2));
    
    for (int run = 0; run < 2; ++run) {
        if (run == 1)
            m2.allocate_blocks();
       
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            for (size_t s = 0; s < physical_i.size(); ++s)
            {
                size_t r = right_i.position(m1.basis().right_charge(block));
                if(r == right_i.size()) continue;
                size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                           -physical_i[s].first));
                if(l == left_i.size()) continue;

                {
                    bool pretend = (run == 0);
                    
                    charge in_l_charge = SymmGroup::fuse(physical_i[s].first, left_i[l].first);
                    charge in_r_charge = right_i[r].first;
                    charge out_l_charge = left_i[l].first;
                    charge out_r_charge = SymmGroup::fuse(-physical_i[s].first, right_i[r].first);

                    if (! m1.has_block(in_l_charge, in_r_charge) )
                        continue;
                    
                    size_t in_left_offset = in_left(physical_i[s].first, left_i[l].first);
                    size_t out_right_offset = out_right(physical_i[s].first, right_i[r].first);
                    
                    if (!pretend){
                        Matrix const & in_block = m1(in_l_charge, in_r_charge);
                        Matrix & out_block = m2(out_l_charge, out_r_charge);
                        
                        maquis::dmrg::detail::reshape_l2r(in_block, out_block, in_left_offset, out_right_offset,
                                                          physical_i[s].second, left_i[l].second, right_i[r].second);
                    }

                    if (pretend)
                        m2.reserve(out_l_charge, out_r_charge,
                                   left_i[l].second, out_right_offset + physical_i[s].second * right_i[r].second);
                }
            }
        }
    }
    
    
//    assert(m2.left_basis() == left_i);
}

/*
 pseudo-code kernel
 
 std::map<std::pair<charge, charge>, std::list<size_t> > block_requirements;
 
 for (s = 0; s < physical_i.size(); ++s)
	 for (l = 0; ...)
		 for (r = 0; ...) {
             charge in_l_charge = SymmGroup::fuse(physical_i[s].first, left_i[l].first);
             charge in_r_charge = right_i[r].first;
 
			 charge out_l_charge = left_i[l].first;
			 charge out_r_charge = SymmGroup::fuse(-physical_i[s].first, right_i[r].first);
 			
 			 block_requirements[make_pair(out_l_charge, out_r_charge)].push_back(m1.position(in_l_charge, in_r_charge);
 }
 
 A: logistics kernel
 
 // on the node that creates (l,r) in the final block_matrix,
 // get block_requirements[(l,r)] from other nodes
 
 B: compute kernel
 
 */
 

template<class Matrix, class SymmGroup>
void reshape_right_to_left(Index<SymmGroup> physical_i,
                           Index<SymmGroup> left_i,
                           Index<SymmGroup> right_i,
                           block_matrix<Matrix, SymmGroup> const & m1,
                           block_matrix<Matrix, SymmGroup> & m2)
{
//    assert(m1.left_basis() == left_i);
    
    
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_right(physical_i, right_i,
                                     boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                         -boost::lambda::_1, boost::lambda::_2));
    ProductBasis<SymmGroup> out_left(physical_i, left_i);
   
    for (int run = 0; run < 2; ++run) {
        
        if (run == 1)
            m2.allocate_blocks();
    
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            for (size_t s = 0; s < physical_i.size(); ++s)
            {
                size_t l = left_i.position(m1.basis().left_charge(block));
                if(l == left_i.size()) continue;
                size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block),
                                                            physical_i[s].first));
                if(r == right_i.size()) continue;

                {
                    bool pretend = (run == 0);
                    
                    charge in_l_charge = left_i[l].first;
                    charge in_r_charge = SymmGroup::fuse(-physical_i[s].first, right_i[r].first);
                    charge out_l_charge = SymmGroup::fuse(physical_i[s].first, left_i[l].first);
                    charge out_r_charge = right_i[r].first;
                    
                    if (! m1.has_block(in_l_charge, in_r_charge) )
                        continue;
                    
                    size_t in_right_offset = in_right(physical_i[s].first, right_i[r].first);
                    size_t out_left_offset = out_left(physical_i[s].first, left_i[l].first);
                    
                    if (!pretend) {
                        Matrix const & in_block = m1(in_l_charge, in_r_charge);
                        Matrix & out_block = m2(out_l_charge, out_r_charge);
                        maquis::dmrg::detail::reshape_r2l(out_block, in_block, out_left_offset, in_right_offset, 
                                                          physical_i[s].second, left_i[l].second, right_i[r].second);
                    }

                    if (pretend)
                        m2.reserve(out_l_charge, out_r_charge,
                                   out_left_offset + physical_i[s].second * left_i[l].second, right_i[r].second);
                }
            }
        }
    }
    
    
//    assert(m2.right_basis() == right_i);
}

// Moving physical index from left to right
// [(phys_i, left_i), right_i]  -->  [left_i, (-phys_i, right_i)]
template<class Matrix, class OtherMatrix, class SymmGroup>
void reshape_left_to_right_new(Index<SymmGroup> const & physical_i,
                               Index<SymmGroup> const & left_i,
                               Index<SymmGroup> const & right_i,
                               block_matrix<OtherMatrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
{
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_left(physical_i, left_i);
    ProductBasis<SymmGroup> out_right(physical_i, right_i,
                                      boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                          -boost::lambda::_1, boost::lambda::_2));
    
    for (size_t block = 0; block < m1.n_blocks(); ++block)
    {
        size_t r = right_i.position(m1.basis().right_charge(block));
        if(r == right_i.size()) continue;
        charge in_r_charge = right_i[r].first;
        charge in_l_charge = m1.basis().left_charge(block);

        for (size_t s = 0; s < physical_i.size(); ++s)
        {
            size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block), -physical_i[s].first));
            if(l == left_i.size()) continue;

            charge out_l_charge = left_i[l].first;
            charge out_r_charge = SymmGroup::fuse(-physical_i[s].first, in_r_charge);

            if(!m2.has_block(out_l_charge, out_r_charge)) 
                m2.insert_block(Matrix(left_i[l].second, out_right.size(out_r_charge), 0),
                                out_l_charge, out_r_charge);

            size_t in_left_offset = in_left(physical_i[s].first, left_i[l].first);
            size_t out_right_offset = out_right(physical_i[s].first, in_r_charge);
            
            OtherMatrix const & in_block = m1[block];
            Matrix & out_block = m2(out_l_charge, out_r_charge);
            
            maquis::dmrg::detail::reshape_l2r(in_block, out_block, in_left_offset, out_right_offset,
                                              physical_i[s].second, left_i[l].second, right_i[r].second);
        }
    }
}

/// Input: m1, left-paired
/// Output: m2, must already be allocated with correct charge blocks
/// returns m2 left-paired dropping blocks that are not present in out_left_i
template<class Matrix, class SymmGroup>
void reshape_and_pad_left(Index<SymmGroup> physical_i,
                          Index<SymmGroup> in_left_i,
                          Index<SymmGroup> in_right_i,
                          Index<SymmGroup> out_left_i,
                          Index<SymmGroup> out_right_i,
                          block_matrix<Matrix, SymmGroup> const & m1,
                          block_matrix<Matrix, SymmGroup> & m2)
{
    m2 *= 0;
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_left(physical_i, in_left_i);
    ProductBasis<SymmGroup> out_left(physical_i, out_left_i);
    
    for (size_t block = 0; block < m1.n_blocks(); ++block)
    {
        for (size_t s = 0; s < physical_i.size(); ++s)
        {
            size_t r = in_right_i.position(m1.basis().right_charge(block));
            if(r == in_right_i.size()) continue;
            
            size_t l = in_left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                       -physical_i[s].first));
            if(l == in_left_i.size()) continue;
            
            {
                charge l_charge = SymmGroup::fuse(physical_i[s].first, in_left_i[l].first);
                charge r_charge = in_right_i[r].first;
                
                if (! out_left_i.has(in_left_i[l].first)) continue;
                if (! m1.has_block(l_charge, r_charge) )  continue;
                if (! m2.has_block(l_charge, r_charge) )  continue;
                
                size_t in_left_offset = in_left(physical_i[s].first, in_left_i[l].first);
                size_t out_left_offset = out_left(physical_i[s].first, in_left_i[l].first);
                
                Matrix const & in_block = m1(l_charge, r_charge);
                Matrix & out_block = m2(l_charge, r_charge);
                
                for (size_t ss = 0; ss < physical_i[s].second; ++ss)
                    for (size_t rr = 0; rr < in_right_i[r].second; ++rr)
                        for (size_t ll = 0; ll < in_left_i[l].second; ++ll)
                            out_block(out_left_offset + ss*in_left_i[l].second+ll, rr)
                            = in_block(in_left_offset + ss*in_left_i[l].second+ll, rr);
            }
        }
    }
    
    
    //    assert(m2.left_basis() == left_i);
}

// MD: here we reshape without the `pretend` loop. This way i) we are faster, ii) we avoid the bug that the size is not correct.
// example for ii): the output is going to be multiplied with a left_i containing more elements, than the size of some blocks
// might be wrong. Such a situation originates in overlap() if right_i and left_i are very different: the block_matrix will contain
// only diagonal blocks.
// Moving physical index from right to left
// [left_i, (-phys_i, right_i)]  -->  [(phys_i, left_i), right_i]
template<class Matrix, class OtherMatrix, class SymmGroup>
void reshape_right_to_left_new(Index<SymmGroup> const & physical_i,
                               Index<SymmGroup> const & left_i,
                               Index<SymmGroup> const & right_i,
                               block_matrix<OtherMatrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
{
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_right(physical_i, right_i,
                                     boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                         -boost::lambda::_1, boost::lambda::_2));
    ProductBasis<SymmGroup> out_left(physical_i, left_i);
   
    for (size_t block = 0; block < m1.n_blocks(); ++block)
    {
        size_t l = left_i.position(m1.basis().left_charge(block));
        if(l == left_i.size()) continue;
        charge in_l_charge = left_i[l].first;
        charge in_r_charge = m1.basis().right_charge(block);

        for (size_t s = 0; s < physical_i.size(); ++s)
        {
            size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block),
                                                        physical_i[s].first));
            if(r == right_i.size()) continue;

            charge out_l_charge = SymmGroup::fuse(physical_i[s].first, in_l_charge);
            charge out_r_charge = right_i[r].first;
            if (! m2.has_block(out_l_charge, out_r_charge))
                m2.insert_block(Matrix(out_left.size(physical_i[s].first, in_l_charge), right_i[r].second, 0),
                                out_l_charge, out_r_charge);
            
            size_t in_right_offset = in_right(physical_i[s].first, out_r_charge);
            size_t out_left_offset = out_left(physical_i[s].first, in_l_charge);
            
            OtherMatrix const & in_block = m1[block];
            Matrix & out_block = m2(out_l_charge, out_r_charge);
            
            maquis::dmrg::detail::reshape_r2l(out_block, in_block, out_left_offset, in_right_offset, 
                                              physical_i[s].second, left_i[l].second, right_i[r].second);
        }
    }
}

// Moving two auxiliary indexes to the right
// [(phys_i, left_i), right_i]  -->  [phys_i, (-left_i, right_i)]
template<class Matrix, class SymmGroup>
void reshape_left_to_physleft(Index<SymmGroup> const& physical_i,
                              Index<SymmGroup> const& left_i,
                              Index<SymmGroup> const& right_i,
                              block_matrix<Matrix, SymmGroup> const & m1,
                              block_matrix<Matrix, SymmGroup> & m2)
{
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_left(physical_i, left_i);
    ProductBasis<SymmGroup> out_right(left_i, right_i,
                                      boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                          -boost::lambda::_1, boost::lambda::_2));
    
    for (size_t block = 0; block < m1.n_blocks(); ++block)
    {
        for (size_t s = 0; s < physical_i.size(); ++s)
        {
            size_t r = right_i.position(m1.basis().right_charge(block));
            if(r == right_i.size()) continue;
            size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                       -physical_i[s].first));
            if(l == left_i.size()) continue;
            
            {
                charge in_l_charge = SymmGroup::fuse(physical_i[s].first, left_i[l].first);
                charge in_r_charge = right_i[r].first;
                charge out_l_charge = physical_i[s].first;
                charge out_r_charge = SymmGroup::fuse(-left_i[l].first, right_i[r].first);
                
                if (! m1.has_block(in_l_charge, in_r_charge) )
                    continue;
                
                if (! m2.has_block(out_l_charge, out_r_charge) )
                    m2.insert_block(Matrix(physical_i[s].second, out_right.size(out_r_charge), 0),
                                    out_l_charge, out_r_charge);
                
                size_t in_left_offset = in_left(physical_i[s].first, left_i[l].first);
                size_t out_right_offset = out_right(left_i[l].first, right_i[r].first);
                
                Matrix const & in_block = m1(in_l_charge, in_r_charge);
                Matrix & out_block = m2(out_l_charge, out_r_charge);

                for (size_t ss = 0; ss < physical_i[s].second; ++ss)
                    for (size_t rr = 0; rr < right_i[r].second; ++rr)
                        for (size_t ll = 0; ll < left_i[l].second; ++ll)
                            out_block(ss, out_right_offset + ll*right_i[r].second+rr) = in_block(in_left_offset + ss*left_i[l].second+ll, rr);
            }
        }
    }
}

// Split merged auxiliary indexes
// [phys_i, (-left_i, right_i)]  -->  [(phys_i, left_i), right_i]
template<class Matrix, class SymmGroup>
void reshape_physleft_to_left(Index<SymmGroup> const& physical_i,
                              Index<SymmGroup> const& left_i,
                              Index<SymmGroup> const& right_i,
                              block_matrix<Matrix, SymmGroup> const & m1,
                              block_matrix<Matrix, SymmGroup> & m2)
{
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> out_left(physical_i, left_i);
    ProductBasis<SymmGroup> in_right(left_i, right_i,
                                      boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                          -boost::lambda::_1, boost::lambda::_2));
    
    for (size_t block = 0; block < m1.n_blocks(); ++block)
    {
        charge const& s_charge = m1.basis().left_charge(block);
        size_t s = physical_i.position(s_charge);
        if (s == physical_i.size()) continue;
        
        for (size_t l = 0; l < left_i.size(); ++l)
        {
            charge const& l_charge = left_i[l].first;
            
            size_t r = right_i.position(SymmGroup::fuse(m1.basis().right_charge(block),
                                                       l_charge));
            if(r == right_i.size()) continue;
            charge const& r_charge = right_i[r].first;
            
            {
                charge in_l_charge = s_charge;
                charge in_r_charge = SymmGroup::fuse(-l_charge, r_charge);
                charge out_l_charge = SymmGroup::fuse(s_charge, l_charge);
                charge out_r_charge = r_charge;
                
                if (! m1.has_block(in_l_charge, in_r_charge) )
                    continue;
                
                if (! m2.has_block(out_l_charge, out_r_charge) )
                    m2.insert_block(Matrix(out_left.size(out_l_charge), right_i[r].second, 0),
                                    out_l_charge, out_r_charge);
                
                size_t in_right_offset = in_right(l_charge, r_charge);
                size_t out_left_offset = out_left(s_charge, l_charge);
                
                Matrix const & in_block = m1(in_l_charge, in_r_charge);
                Matrix & out_block = m2(out_l_charge, out_r_charge);
                
                for (size_t rr = 0; rr < right_i[r].second; ++rr)
                    for (size_t ss = 0; ss < physical_i[s].second; ++ss)
                        for (size_t ll = 0; ll < left_i[l].second; ++ll)
                            out_block(out_left_offset + ss*left_i[l].second+ll, rr) = in_block(ss, in_right_offset + ll*right_i[r].second+rr);
            }
        }
    }
}

// Split merged auxiliary indexes
// [(left_i, -right_i), phys_i]  -->  [(phys_i, left_i), right_i]
template<class Matrix, class SymmGroup>
void reshape_physright_to_left(Index<SymmGroup> const& physical_i,
                               Index<SymmGroup> const& left_i,
                               Index<SymmGroup> const& right_i,
                               block_matrix<Matrix, SymmGroup> const & m1,
                               block_matrix<Matrix, SymmGroup> & m2)
{
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    
    ProductBasis<SymmGroup> in_left(left_i, right_i,
                                    boost::lambda::bind(static_cast<charge(*)(charge, charge)>(SymmGroup::fuse),
                                                        boost::lambda::_1, -boost::lambda::_2));
    ProductBasis<SymmGroup> out_left(physical_i, left_i);
    
    
    for (size_t block = 0; block < m1.n_blocks(); ++block)
    {
        for (size_t r = 0; r < right_i.size(); ++r)
        {
            size_t s = physical_i.position(-m1.basis().right_charge(block));
            if (s == physical_i.size()) continue;
            size_t l = left_i.position(SymmGroup::fuse(m1.basis().left_charge(block),
                                                       right_i[r].first));
            if (l == left_i.size()) continue;
            
            {
                charge in_l_charge = SymmGroup::fuse(left_i[l].first, -right_i[r].first);
                charge in_r_charge = -physical_i[s].first;
                charge out_l_charge = SymmGroup::fuse(physical_i[s].first, left_i[l].first);
                charge out_r_charge = right_i[r].first;
                
                size_t outb = m2.find_block(out_l_charge, out_r_charge);
                if (outb == m2.n_blocks())
                    outb = m2.insert_block(new Matrix(out_left.size(out_l_charge), right_i[r].second, 0),
                                           out_l_charge, out_r_charge);
                
                size_t in_left_offset = in_left(left_i[l].first, right_i[r].first);
                size_t out_left_offset = out_left(physical_i[s].first, left_i[l].first);
                
                Matrix const & in_block = m1(in_l_charge, in_r_charge);
                Matrix & out_block = m2[outb];
                
                for (size_t ss = 0; ss < physical_i[s].second; ++ss)
                    for (size_t rr = 0; rr < right_i[r].second; ++rr)
                        for (size_t ll = 0; ll < left_i[l].second; ++ll)
                            out_block(out_left_offset + ss*left_i[l].second+ll, rr) = in_block(in_left_offset + rr*left_i[l].second+ll, ss);
            }
        }
    }
}


template<class Matrix, class SymmGroup>
block_matrix<Matrix, SymmGroup> reshape_2site_op (Index<SymmGroup> const & phys1, Index<SymmGroup> const & phys2,
                                                  block_matrix<Matrix, SymmGroup> const & A)
{ // only for the dense matrices in MPO (30.04.2012 / scalar / value types discussion)
  // TODO: (scatter alps::numeric::matrix during building of MPO)
    typedef typename SymmGroup::charge charge;
    block_matrix<Matrix, SymmGroup> ret;
    
    ProductBasis<SymmGroup> pb(phys1, phys2);
    ProductBasis<SymmGroup> pb_out_left(phys1, adjoin(phys1));
    ProductBasis<SymmGroup> pb_out_right(phys2, adjoin(phys2));
    
    /// s1 \in phys1, input of op on site1
    /// s2 \in phys2, input of op on site2
    /// s3 \in phys1, output of op on site1
    /// s4 \in phys2, output of op on site2
    
    typedef typename Index<SymmGroup>::basis_iterator bi_t;
    for (bi_t s1 = phys1.basis_begin(); !s1.end(); ++s1)
        for (bi_t s2 = phys2.basis_begin(); !s2.end(); ++s2)
            for (bi_t s3 = phys1.basis_begin(); !s3.end(); ++s3)
                for (bi_t s4 = phys2.basis_begin(); !s4.end(); ++s4)
                {                    
                    charge in_left_c = SymmGroup::fuse(s1->first, s2->first);
                    charge in_right_c = SymmGroup::fuse(s3->first, s4->first);
                    
                    if (!A.has_block(in_left_c, in_right_c))
                        continue;
                    
                    charge out_left_c = SymmGroup::fuse(s1->first, -s3->first);
                    charge out_right_c = SymmGroup::fuse(s4->first, -s2->first);
                    
                    if (!ret.has_block(out_left_c, out_right_c))
                        ret.insert_block(new Matrix(pb_out_left.size(s1->first, -s3->first),
                                                    pb_out_right.size(s4->first, -s2->first),
                                                    0),
                                         out_left_c, out_right_c);
                    
                    std::size_t in_left_offset = pb(s1->first, s2->first);
                    std::size_t in_right_offset = pb(s3->first, s4->first);
                    
                    std::size_t out_left_offset = pb_out_left(s1->first, -s3->first);
                    std::size_t out_right_offset = pb_out_right(s4->first, -s2->first);
                    
                    ret(std::make_pair(out_left_c, out_left_offset + s1->second*phys1.size_of_block(s3->first)+s3->second),
                        std::make_pair(out_right_c, out_right_offset + s2->second*phys2.size_of_block(s4->first)+s4->second))
                    = A(std::make_pair(in_left_c, in_left_offset + s1->second*phys2.size_of_block(s2->first) + s2->second),
                        std::make_pair(in_right_c, in_right_offset + s3->second*phys2.size_of_block(s4->first) + s4->second));
                    
                }
    
    // Removing empty blocks
    Index<SymmGroup> out_left = phys1*adjoin(phys1);
    Index<SymmGroup> out_right = phys2*adjoin(phys2);
    for (typename Index<SymmGroup>::const_iterator it1 = out_left.begin(); it1 != out_left.end(); it1++)
    {
        for (typename Index<SymmGroup>::const_iterator it2 = out_right.begin(); it2 != out_right.end(); it2++)
        {
            bool empty = true;
            
            if (!ret.has_block(it1->first, it2->first)) continue;
            
            Matrix tmp = ret(it1->first, it2->first);
            for (int i=0; i<num_rows(tmp); ++i)
            {
                for (int j=0; j<num_cols(tmp); ++j)
                    if (tmp(i,j) != typename Matrix::value_type()) {
                        empty=false;
                        break;
                    }
                if (!empty) break;
            }
            
            if (empty)
                ret.remove_block(it1->first, it2->first);
        }
    }
    
    return ret;
}

template<class Op, class Op2, class SymmGroup>
//std::vector<Op> reshape_right_to_list (Index<SymmGroup> const & phys, block_matrix<typename Op::matrix_type, SymmGroup> const & A)
std::vector<Op> reshape_right_to_list (Index<SymmGroup> const & phys, Op2 const & A)
{
    typedef typename Op::matrix_type Matrix;
    typedef typename SymmGroup::charge charge;
    std::vector<Op> ret;
    
    Index<SymmGroup> aux_i = A.right_basis();
    ProductBasis<SymmGroup> pb(phys, adjoin(phys));

    typedef typename Index<SymmGroup>::basis_iterator bi_t;
    for (bi_t b = aux_i.basis_begin(); !b.end(); ++b)
    {
    	Op Ai;
        for (bi_t s1 = phys.basis_begin(); !s1.end(); ++s1)
            for (bi_t s2 = phys.basis_begin(); !s2.end(); ++s2)
            {                    
                charge in_left_c = SymmGroup::fuse(s1->first, -s2->first);
                charge in_right_c = b->first;
                
                if (!A.has_block(in_left_c, in_right_c))
                    continue;
                
                charge out_left_c = s1->first;
                charge out_right_c = s2->first;
                
                if (!Ai.has_block(out_left_c, out_right_c))
                    Ai.insert_block(new Matrix(phys.size_of_block(s1->first),
                                               phys.size_of_block(s2->first),
                                               0),
                                     out_left_c, out_right_c);
                
                std::size_t in_left_offset = pb(s1->first, -s2->first);
                std::size_t in_right_offset = 0;
                
                std::size_t out_left_offset = 0;
                std::size_t out_right_offset = 0;
                
                Ai(*s1, *s2)
                = A(std::make_pair(in_left_c, in_left_offset + s1->second*phys.size_of_block(s2->first) + s2->second),
                    *b);
                
            }

        // Removing empty blocks
        for (int n=0; n<Ai.n_blocks(); ++n)
        {
        	bool empty = true;
        	for (int i=0; i<num_rows(Ai[n]) && empty; ++i)
        		for (int j=0; j<num_cols(Ai[n]) && empty; ++j)
        			if (Ai[n](i,j) != typename Matrix::value_type())
        				empty=false;

        	if (empty)
        		Ai.remove_block(n);
        }

        ret.push_back(Ai);
    }
    assert( ret.size() == aux_i.sum_of_sizes() );

    return ret;
}

template<class Op, class Op2, class SymmGroup>
//std::vector<Op> reshape_left_to_list (Index<SymmGroup> const & phys, block_matrix<typename Op::matrix_type, SymmGroup> const & A)
std::vector<Op> reshape_left_to_list (Index<SymmGroup> const & phys, Op2 const & A)
{
    typedef typename Op::matrix_type Matrix;
	typedef typename SymmGroup::charge charge;
	std::vector<Op> ret;

	Index<SymmGroup> aux_i = A.left_basis();
	ProductBasis<SymmGroup> pb(phys, adjoin(phys));

	typedef typename Index<SymmGroup>::basis_iterator bi_t;
	for (bi_t b = aux_i.basis_begin(); !b.end(); ++b)
	{
		Op Ai;
		for (bi_t s1 = phys.basis_begin(); !s1.end(); ++s1)
			for (bi_t s2 = phys.basis_begin(); !s2.end(); ++s2)
			{
				charge in_right_c = SymmGroup::fuse(s2->first, -s1->first);
				charge in_left_c = b->first;

				if (!A.has_block(in_left_c, in_right_c))
					continue;

				charge out_left_c = s1->first;
				charge out_right_c = s2->first;

				if (!Ai.has_block(out_left_c, out_right_c))
					Ai.insert_block(new Matrix(phys.size_of_block(s1->first),
							phys.size_of_block(s2->first),
							0),
							out_left_c, out_right_c);

				std::size_t in_left_offset = 0;
				std::size_t in_right_offset = pb(s2->first, -s1->first);

				std::size_t out_left_offset = 0;
				std::size_t out_right_offset = 0;

				Ai(std::make_pair(out_left_c, out_left_offset + s1->second),
						std::make_pair(out_right_c, out_right_offset + s2->second))
				= A(std::make_pair(in_left_c, in_left_offset + b->second),
						std::make_pair(in_right_c, in_right_offset + s1->second*phys.size_of_block(s2->first) + s2->second));

			}

		// Removing empty blocks
		for (int n=0; n<Ai.n_blocks(); ++n)
		{
			bool empty = true;
			for (int i=0; i<num_rows(Ai[n]) && empty; ++i)
				for (int j=0; j<num_cols(Ai[n]) && empty; ++j)
					if (Ai[n](i,j) != typename Matrix::value_type())
						empty=false;

			if (empty)
				Ai.remove_block(n);
		}

		ret.push_back(Ai);
	}
	assert( ret.size() == aux_i.sum_of_sizes() );

	return ret;
}


#endif /* RESHAPE_H */

