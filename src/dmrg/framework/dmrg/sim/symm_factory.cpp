/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2013 Institute for Theoretical Physics, ETH Zurich
 *               2011-2012 by Michele Dolfi <dolfim@phys.ethz.ch>
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

#include "runsim.h"

#include <cmath>
#include <iterator>
#include <iostream>
#include <sys/stat.h>

#include <boost/function.hpp>
#undef tolower
#undef toupper
#include <boost/tokenizer.hpp>

#include "symm_factory.h"

std::string guess_alps_symmetry(BaseParameters & parms)
{
    std::map<int, std::string> symm_names;
    symm_names[0] = "none";
    symm_names[1] = "u1";
    symm_names[2] = "2u1";
    
    int n=0;
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    if (parms.defined("CONSERVED_QUANTUMNUMBERS")) {
        boost::char_separator<char> sep(" ,");
        std::string qn_string = parms["CONSERVED_QUANTUMNUMBERS"];
        tokenizer qn_tokens(qn_string, sep);
        for (tokenizer::iterator it=qn_tokens.begin(); it != qn_tokens.end(); it++) {
            if (parms.defined(*it + "_total"))
                n += 1;
        }
    }
    return symm_names[n];
}

namespace maquis { namespace dmrg {
    
    void symm_factory(DmrgParameters & parms)
    {
        std::map<std::string, boost::function<void (DmrgParameters & p)> > factory_map;
        
        maquis::cout << "This binary contains symmetries: ";
#ifdef HAVE_NU1
        factory_map["nu1"] = run_sim<NU1>;
        maquis::cout << "nu1 ";
#endif
#ifdef HAVE_TrivialGroup
        factory_map["none"] = run_sim<TrivialGroup>;
        maquis::cout << "none ";
#endif
#ifdef HAVE_U1
        factory_map["u1"] = run_sim<U1>;
        maquis::cout << "u1 ";
#endif
#ifdef HAVE_TwoU1
        factory_map["2u1"] = run_sim<TwoU1>;
        maquis::cout << "2u1 ";
#endif
#ifdef HAVE_TwoU1PG
        factory_map["2u1pg"] = run_sim<TwoU1PG>;
        maquis::cout << "2u1pg ";
#endif
#ifdef HAVE_Ztwo
        factory_map["Z2"] = run_sim<Ztwo>;
        maquis::cout << "Z2 ";
#endif
        maquis::cout << std::endl;
        
        
        std::string symm_name;
        if (!parms.is_set("symmetry")) {
#ifdef HAVE_NU1
            symm_name = "nu1";
#else
            if (parms["model_library"] == "alps")
                symm_name = guess_alps_symmetry(parms);
#endif
        } else {
            symm_name = parms["symmetry"].str();
        }
        
        if (factory_map.find(symm_name) != factory_map.end())
            factory_map[symm_name](parms);
        else
            throw std::runtime_error("Don't know this symmetry group. Please, check your compilation flags.");
#ifdef AMBIENT
        ambient::sync();
#endif
    }

} }
