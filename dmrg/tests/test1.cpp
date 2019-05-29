/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2013 by Michele Dolfi <dolfim@phys.ethz.ch>
 *               2019 by Leon Freitag <lefreita@ethz.ch>
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

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "utils/io.hpp" // has to be first include because of impi
#include <iostream>


#include "maquis_dmrg.h"
// Test 1: H2 with d=3 Angstrom, singlet, cc-pVDZ basis set, CAS(2,2), integrals generated by MOLCAS
BOOST_AUTO_TEST_CASE( Test1 )
{

    DmrgParameters p;
    // // Integrals can be alternatively defined like this (as the FCIDUMP file without header)
    //
    // const std::string integrals("   0.354237848011            1     1     1     1 \n"
    //                             "  -0.821703816101E-13        1     1     2     1 \n"
    //                             "  0.185125251547             2     1     2     1 \n"
    //                             "  0.782984788117E-13         2     2     2     1 \n"
    //                             "  0.361001163519             1     1     2     2 \n"
    //                             "  0.371320200119             2     2     2     2 \n"
    //                             " -0.678487901790             1     1     0     0 \n"
    //                             " -0.539801158857E-14         2     1     0     0 \n"
    //                             " -0.653221638776             2     2     0     0 \n"
    //                             "  0.176392403557             0     0     0     0 \n");
    // p.set("integrals", integrals);

    const maquis::integral_map<double> integrals {
                                                        { { 1, 1, 1, 1 },   0.354237848011       }, // 2e integrals
                                                        { { 1, 1, 2, 1 },  -0.821703816101E-13   },
                                                        { { 2, 1, 2, 1 },  0.185125251547        },
                                                        { { 2, 2, 2, 1 },  0.782984788117E-13    },
                                                        { { 1, 1, 2, 2 },  0.361001163519        },
                                                        { { 2, 2, 2, 2 },  0.371320200119        },
                                                        { { 1, 1, 0, 0 }, -0.678487901790        }, // 1e integrals
                                                        { { 2, 1, 0, 0 }, -0.539801158857E-14    },
                                                        { { 2, 2, 0, 0 }, -0.653221638776        },
                                                        { { 0, 0, 0, 0 },  0.176392403557        }  // Nuclear repulsion energy
                                                        };

    p.set("integrals_binary", maquis::serialize<double>(integrals));
    p.set("site_types", "1,1");
    p.set("L", 2);
    p.set("irrep", 0);

    p.set("nsweeps",2);
    p.set("max_bond_dimension",100);

    // for SU2U1
    p.set("nelec", 2);
    p.set("spin", 0);

    // for 2U1

    p.set("u1_total_charge1", 1);
    p.set("u1_total_charge2", 1);

    // Measure 2-RDM
    p.set("MEASURE[2rdm]","1");

    std::vector<std::string> symmetries;
    #ifdef HAVE_SU2U1PG
    symmetries.push_back("su2u1pg");
    #endif
    #ifdef HAVE_SU2U1
    symmetries.push_back("su2u1");
    #endif
    #ifdef HAVE_TwoU1PG
    symmetries.push_back("2u1pg");
    #endif
    #ifdef HAVE_TwoU1
    symmetries.push_back("2u1");
    #endif

    for (auto&& s: symmetries)
    {
        maquis::cout << "Running test for symmetry " << s << std::endl;
        p.set("symmetry", s);

        maquis::DMRGInterface<double> interface(p);
        interface.optimize();

        BOOST_CHECK_CLOSE(interface.energy(), -0.980724992658492 , 1e-7);

        typename maquis::DMRGInterface<double>::results_map_type res = interface.measure();
        const typename maquis::DMRGInterface<double>::meas_with_results_type& meas = res["twoptdm"];

        // we don't have a map for the measurements yet, so we'll do it the stupid way
        double value = 0.0;

        for (int i = 0; i < meas.first.size(); i++)
            if (meas.first[i] == std::vector<int>{0,0,0,0})
            {
                value = meas.second[i];
                break;
            }

        BOOST_CHECK_CLOSE(value, 1.1796482258 , 1e-7);
    }


}

