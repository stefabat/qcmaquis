/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2013 Institute for Theoretical Physics, ETH Zurich
 *               2011-2011 by Michele Dolfi <dolfim@phys.ethz.ch>
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

#include "dmrg/models/alps/symm_handler.hpp"
#include "dmrg/block_matrix/symmetry/nu1pg.h"

// Symmetry dependent implementation

// TwoU1PG Symmetry
template <>
TwoU1PG::charge state_to_charge<TwoU1PG>(alps::site_state<short> const & state, alps::SiteBasisDescriptor<short> const& b,
                                     std::map<std::string, int> const& all_conserved_qn)
{
    typedef std::map<std::string, int> qn_map_type;
    TwoU1PG::charge c = TwoU1PG::IdentityCharge;
    for (alps::SiteBasisDescriptor<short>::const_iterator it = b.begin(); it != b.end(); ++it) {
        qn_map_type::const_iterator match = all_conserved_qn.find(it->name());
        if (match != all_conserved_qn.end())
            c[match->second] = detail::to_integer( get_quantumnumber(state, it->name(), b) );
    }
    return c;
}

template <>
TwoU1PG::charge init_charge<TwoU1PG> (const alps::Parameters& parms, std::map<std::string, int> const& all_conserved_qn)
{
    typedef std::map<std::string, int> qn_map_type;
    assert(all_conserved_qn.size() == 2);

    TwoU1PG::charge c = TwoU1PG::IdentityCharge;
    for (qn_map_type::const_iterator it=all_conserved_qn.begin(); it!=all_conserved_qn.end(); ++it) {
        alps::half_integer<short> tmp = alps::evaluate<double>(static_cast<std::string>(parms[it->first+"_total"]), parms);
        c[it->second] = detail::to_integer(tmp);
    }
    
    return c;
}
