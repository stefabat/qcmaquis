/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2013 Institute for Theoretical Physics, ETH Zurich
 *               2011 by Michele Dolfi <dolfim@phys.ethz.ch>
 * 
 * This software is part of the ALPS Applications, published under the ALPS
 * Application License; you can use, redistribute it and/or modify it under
 * the terms of the license, either version 1 or (at your option) any later
 * version.
 * 
 * You should have received a copy of the ALPS Application License along with
 * the ALPS Applications; see the file LICENSE.txt. If not, the license is also
 * available from http://alps.comp-phys.org/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef GENERIC_RESHAPE_H
#define GENERIC_RESHAPE_H

#include "dmrg/block_matrix/block_matrix.h"
#include "dmrg/block_matrix/multi_index.h"


template<class Matrix, class SymmGroup>
void reshape(MultiIndex<SymmGroup> const & midx,
             typename MultiIndex<SymmGroup>::set_id in_set,
             typename MultiIndex<SymmGroup>::set_id out_set,
             block_matrix<Matrix, SymmGroup> const & m1,
             block_matrix<Matrix, SymmGroup> & m2)
{   
    
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    typedef typename MultiIndex<SymmGroup>::coord_t coord_t;
    
    for (int run = 0; run < 2; ++run) {
        bool pretend = (run == 0);
        
        if (!pretend)
            m2.allocate_blocks();
        
        for (size_t block = 0; block < m1.n_blocks(); ++block)
        {
            
            charge in_l_charge = m1.basis().lc(block);
            charge in_r_charge = m1.basis().rc(block);
            
            Matrix const & in_block = m1[block];
            
            for (size_t i=0; i<num_rows(in_block); ++i)
                for (size_t j=0; j<num_cols(in_block); ++j) {
                    coord_t in_left = std::make_pair(in_l_charge, i);
                    coord_t in_right = std::make_pair(in_r_charge, j);
                    coord_t out_left, out_right;
                    boost::tie(out_left, out_right) = midx.convert_coords(in_set, in_left, in_right, out_set);
                    
                    if (in_block(i, j) != 0.) {
                        if (pretend)
                            m2.reserve(out_left.first, out_right.first,
                                       midx.left_size(out_set, out_left.first), midx.right_size(out_set, out_right.first));
                        else
                            m2(out_left, out_right) = in_block(i, j);
                    }
                }
        }
    }
    
    // Removing empty blocks
    // IT SEEMS NOT NEEDED BECAUSE OF "if (in_block(i, j) != 0)"
    /*
    for (int n=0; n<m2.n_blocks(); ++n)
    {
        bool empty = true;
        for (int i=0; i<num_rows(m2[n]) && empty; ++i)
            for (int j=0; j<num_cols(m2[n]) && empty; ++j)
                if (m2[n](i,j) != 0)
                    empty=false;
        
        if (empty)
            m2.remove_block(n);
    }
     */
     
}

// easier code, but slower
template<class Matrix, class SymmGroup>
void reshape2(MultiIndex<SymmGroup> const & midx,
              typename MultiIndex<SymmGroup>::set_id in_set,
              typename MultiIndex<SymmGroup>::set_id out_set,
              block_matrix<Matrix, SymmGroup> const & m1,
              block_matrix<Matrix, SymmGroup> & m2)
{   
    m2 = block_matrix<Matrix, SymmGroup>();
    
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    typedef typename MultiIndex<SymmGroup>::coord_t coord_t;
    
    
    for (int run = 0; run < 2; ++run) {
        bool pretend = (run == 0);
        
        if (!pretend)
            m2.allocate_blocks();
        
        for(index_product_iterator<SymmGroup> it = midx.begin();
            it != midx.end();
            it++)
        {
            coord_t in_left, in_right;
            boost::tie(in_left, in_right) = midx.get_coords(in_set, *it);
            
            if (!m1.has_block(in_left.first, in_right.first))
                continue;
            
            coord_t out_left, out_right;
            boost::tie(out_left, out_right) = midx.get_coords(out_set, *it);
            
            if (pretend)
                m2.reserve(out_left.first, out_right.first,
                           midx.left_size(out_set, out_left.first), midx.right_size(out_set, out_right.first));
            else
                m2(out_left, out_right) = m1(in_left, in_right);
        }
    }
    
}


#endif
