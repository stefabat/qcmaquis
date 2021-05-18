/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2013 by Michele Dolfi <dolfim@phys.ethz.ch>
 *               2021 by Robin Feldmann <robin.feldmann@phys.chem.ethz.ch>
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
#include "utils/fpcomparison.h"
#include "utils/io.hpp" // has to be first include because of impi
#include <iostream>

#include "maquis_dmrg.h"


// Test 1: H2 with d=3 Angstrom, singlet, 6-31G* basis set, FCI, integrals generated by Blueberry
BOOST_AUTO_TEST_CASE( PreBO_Test1 )
{

    DmrgParameters p;

    const maquis::integral_map<double, chem::Hamiltonian::PreBO> integrals {
        {{-1,-1,-1,-1,-1,-1,-1,-1},0.1763923530387111},
        {{0,0,0,0,-1,-1,-1,-1},-0.6802711028685324},
        {{0,0,0,2,-1,-1,-1,-1},0.07526840630272968},
        {{0,1,0,1,-1,-1,-1,-1},-0.6543632168967157},
        {{0,1,0,3,-1,-1,-1,-1},0.08728167494843309},
        {{0,2,0,0,-1,-1,-1,-1},0.07526840630272949},
        {{0,2,0,2,-1,-1,-1,-1},0.2283902278635599},
        {{0,3,0,1,-1,-1,-1,-1},0.08728167494843292},
        {{0,3,0,3,-1,-1,-1,-1},0.3423882983918999},
        {{0,0,0,0,0,0,0,0},0.1842790692925557},
        {{0,0,0,0,0,0,0,2},-0.03763420337488414},
        {{0,0,0,0,0,1,0,1},0.09872952891658415},
        {{0,0,0,0,0,1,0,3},-0.03477123411947585},
        {{0,0,0,0,0,2,0,0},-0.03763420337488413},
        {{0,0,0,0,0,2,0,2},0.03655689640589711},
        {{0,0,0,0,0,3,0,1},-0.03477123411947588},
        {{0,0,0,0,0,3,0,3},0.03327236099371499},
        {{0,0,0,1,0,0,0,1},0.09872952891658418},
        {{0,0,0,1,0,0,0,3},-0.03477123411947584},
        {{0,0,0,1,0,1,0,0},0.1866311043452651},
        {{0,0,0,1,0,1,0,2},-0.04128027869732104},
        {{0,0,0,1,0,2,0,1},-0.04481688352860439},
        {{0,0,0,1,0,2,0,3},0.03664246246971625},
        {{0,0,0,1,0,3,0,0},-0.03920603585723025},
        {{0,0,0,1,0,3,0,2},0.03658798479823142},
        {{0,0,0,2,0,0,0,0},-0.03763420337488416},
        {{0,0,0,2,0,0,0,2},0.03655689640589712},
        {{0,0,0,2,0,1,0,1},-0.04481688352860438},
        {{0,0,0,2,0,1,0,3},0.03664246246971625},
        {{0,0,0,2,0,2,0,0},0.1947198053616306},
        {{0,0,0,2,0,2,0,2},-0.05831200436614524},
        {{0,0,0,2,0,3,0,1},0.1068127293548811},
        {{0,0,0,2,0,3,0,3},-0.05322835824445955},
        {{0,0,0,3,0,0,0,1},-0.03477123411947588},
        {{0,0,0,3,0,0,0,3},0.03327236099371499},
        {{0,0,0,3,0,1,0,0},-0.03920603585723025},
        {{0,0,0,3,0,1,0,2},0.03658798479823138},
        {{0,0,0,3,0,2,0,1},0.1068127293548811},
        {{0,0,0,3,0,2,0,3},-0.05322835824445957},
        {{0,0,0,3,0,3,0,0},0.1906570567143694},
        {{0,0,0,3,0,3,0,2},-0.05745793841483472},
        {{0,1,0,0,0,0,0,1},0.1866311043452651},
        {{0,1,0,0,0,0,0,3},-0.03920603585723022},
        {{0,1,0,0,0,1,0,0},0.09872952891658415},
        {{0,1,0,0,0,1,0,2},-0.0448168835286044},
        {{0,1,0,0,0,2,0,1},-0.04128027869732109},
        {{0,1,0,0,0,2,0,3},0.03658798479823136},
        {{0,1,0,0,0,3,0,0},-0.03477123411947588},
        {{0,1,0,0,0,3,0,2},0.03664246246971627},
        {{0,1,0,1,0,0,0,0},0.09872952891658415},
        {{0,1,0,1,0,0,0,2},-0.04481688352860439},
        {{0,1,0,1,0,1,0,1},0.189844745754033},
        {{0,1,0,1,0,1,0,3},-0.04219754853151676},
        {{0,1,0,1,0,2,0,0},-0.04481688352860438},
        {{0,1,0,1,0,2,0,2},0.04106123456023925},
        {{0,1,0,1,0,3,0,1},-0.04219754853151678},
        {{0,1,0,1,0,3,0,3},0.03750279013385537},
        {{0,1,0,2,0,0,0,1},-0.04128027869732104},
        {{0,1,0,2,0,0,0,3},0.03658798479823134},
        {{0,1,0,2,0,1,0,0},-0.04481688352860438},
        {{0,1,0,2,0,1,0,2},0.04106123456023927},
        {{0,1,0,2,0,2,0,1},0.1984667878135254},
        {{0,1,0,2,0,2,0,3},-0.06104527396609304},
        {{0,1,0,2,0,3,0,0},0.1068127293548811},
        {{0,1,0,2,0,3,0,2},-0.06414087787820752},
        {{0,1,0,3,0,0,0,0},-0.03477123411947587},
        {{0,1,0,3,0,0,0,2},0.0366424624697162},
        {{0,1,0,3,0,1,0,1},-0.04219754853151677},
        {{0,1,0,3,0,1,0,3},0.03750279013385537},
        {{0,1,0,3,0,2,0,0},0.1068127293548812},
        {{0,1,0,3,0,2,0,2},-0.06414087787820755},
        {{0,1,0,3,0,3,0,1},0.1950618399114621},
        {{0,1,0,3,0,3,0,3},-0.05920528748005911},
        {{0,2,0,0,0,0,0,0},-0.03763420337488416},
        {{0,2,0,0,0,0,0,2},0.1947198053616306},
        {{0,2,0,0,0,1,0,1},-0.0448168835286044},
        {{0,2,0,0,0,1,0,3},0.1068127293548811},
        {{0,2,0,0,0,2,0,0},0.03655689640589711},
        {{0,2,0,0,0,2,0,2},-0.05831200436614524},
        {{0,2,0,0,0,3,0,1},0.03664246246971621},
        {{0,2,0,0,0,3,0,3},-0.05322835824445955},
        {{0,2,0,1,0,0,0,1},-0.04481688352860439},
        {{0,2,0,1,0,0,0,3},0.1068127293548811},
        {{0,2,0,1,0,1,0,0},-0.04128027869732105},
        {{0,2,0,1,0,1,0,2},0.1984667878135254},
        {{0,2,0,1,0,2,0,1},0.04106123456023928},
        {{0,2,0,1,0,2,0,3},-0.06414087787820746},
        {{0,2,0,1,0,3,0,0},0.03658798479823136},
        {{0,2,0,1,0,3,0,2},-0.06104527396609301},
        {{0,2,0,2,0,0,0,0},0.03655689640589711},
        {{0,2,0,2,0,0,0,2},-0.05831200436614522},
        {{0,2,0,2,0,1,0,1},0.04106123456023928},
        {{0,2,0,2,0,1,0,3},-0.06414087787820748},
        {{0,2,0,2,0,2,0,0},-0.0583120043661453},
        {{0,2,0,2,0,2,0,2},0.2278333714168219},
        {{0,2,0,2,0,3,0,1},-0.06414087787820751},
        {{0,2,0,2,0,3,0,3},0.13577048665041},
        {{0,2,0,3,0,0,0,1},0.0366424624697162},
        {{0,2,0,3,0,0,0,3},-0.05322835824445953},
        {{0,2,0,3,0,1,0,0},0.03658798479823135},
        {{0,2,0,3,0,1,0,2},-0.061045273966093},
        {{0,2,0,3,0,2,0,1},-0.06414087787820749},
        {{0,2,0,3,0,2,0,3},0.13577048665041},
        {{0,2,0,3,0,3,0,0},-0.05745793841483472},
        {{0,2,0,3,0,3,0,2},0.2231085965042993},
        {{0,3,0,0,0,0,0,1},-0.03920603585723025},
        {{0,3,0,0,0,0,0,3},0.1906570567143694},
        {{0,3,0,0,0,1,0,0},-0.03477123411947589},
        {{0,3,0,0,0,1,0,2},0.1068127293548812},
        {{0,3,0,0,0,2,0,1},0.03658798479823135},
        {{0,3,0,0,0,2,0,3},-0.05745793841483469},
        {{0,3,0,0,0,3,0,0},0.03327236099371495},
        {{0,3,0,0,0,3,0,2},-0.05322835824445952},
        {{0,3,0,1,0,0,0,0},-0.03477123411947589},
        {{0,3,0,1,0,0,0,2},0.1068127293548812},
        {{0,3,0,1,0,1,0,1},-0.04219754853151678},
        {{0,3,0,1,0,1,0,3},0.1950618399114621},
        {{0,3,0,1,0,2,0,0},0.03664246246971622},
        {{0,3,0,1,0,2,0,2},-0.06414087787820745},
        {{0,3,0,1,0,3,0,1},0.03750279013385537},
        {{0,3,0,1,0,3,0,3},-0.059205287480059},
        {{0,3,0,2,0,0,0,1},0.03658798479823136},
        {{0,3,0,2,0,0,0,3},-0.05745793841483467},
        {{0,3,0,2,0,1,0,0},0.03664246246971622},
        {{0,3,0,2,0,1,0,2},-0.06414087787820748},
        {{0,3,0,2,0,2,0,1},-0.06104527396609303},
        {{0,3,0,2,0,2,0,3},0.2231085965042992},
        {{0,3,0,2,0,3,0,0},-0.05322835824445951},
        {{0,3,0,2,0,3,0,2},0.1357704866504099},
        {{0,3,0,3,0,0,0,0},0.03327236099371494},
        {{0,3,0,3,0,0,0,2},-0.0532283582444595},
        {{0,3,0,3,0,1,0,1},0.03750279013385537},
        {{0,3,0,3,0,1,0,3},-0.05920528748005899},
        {{0,3,0,3,0,2,0,0},-0.05322835824445953},
        {{0,3,0,3,0,2,0,2},0.1357704866504099},
        {{0,3,0,3,0,3,0,1},-0.05920528748005899},
        {{0,3,0,3,0,3,0,3},0.2196551979193185}};



    p.set("integrals_binary", maquis::serialize(integrals));
    p.set("L", 4);
    p.set("LATTICE", "preBO lattice");
    p.set("MODEL", "PreBO");

    p.set("PreBO_NumParticles",              2     );
    p.set("PreBO_NumParticleTypes",          1     );
    p.set("PreBO_ParticleTypeVector",        "2"   );
    p.set("PreBO_FermionOrBosonVector",      "1"   );
    p.set("PreBO_OrbitalVector",             "4"   );
    p.set("PreBO_InitialStateVector",        "1 1" );
    p.set("PreBO_MaxBondDimVector",        "1000 500" );

    p.set("nsweeps",4);
    //p.set("max_bond_dimension",1000);

    p.set("symmetry", "nu1");

    std::vector<std::string> optimizer;
    optimizer.push_back("singlesite");
    optimizer.push_back("twosite");

    // Measure RDMs
    p.set("MEASURE[1rdm]","1");

    for (auto&& o: optimizer)
    {
        p.set("optimization", o);

        maquis::cout << "Running Pre-BO test for symmetry nu1 with optimization: " << o << std::endl;

        maquis::DMRGInterface<double, Hamiltonian::PreBO> interface(p);
        interface.optimize();

        // test energy
        BOOST_CHECK_CLOSE(interface.energy(), -0.997454827563674, 1e-7);

        //// test 1-RDM
        //const typename maquis::DMRGInterface<double>::meas_with_results_type& meas1 = interface.onerdm();
        //double value = 0.0;

        //// we don't have a map for the measurements yet, so we'll do it the stupid way
        //for (int i = 0; i < meas1.first.size(); i++)
        //    if ((meas1.first[i] == std::vector<int>{0,0}) || (meas1.first[i] == std::vector<int>{1,1}))
        //        value += meas1.second[i];
        //BOOST_CHECK_CLOSE(value, 2.0 , 1e-7);


        //// test 2-RDM
        //const typename maquis::DMRGInterface<double>::meas_with_results_type& meas2 = interface.twordm();

        //value = 0.0;

        //for (int i = 0; i < meas2.first.size(); i++)
        //    if (meas2.first[i] == std::vector<int>{0,0,0,0})
        //    {
        //        value = meas2.second[i];
        //        break;
        //    }

        //BOOST_CHECK_CLOSE(value, 1.1796482258 , 1e-7);
    }


}
