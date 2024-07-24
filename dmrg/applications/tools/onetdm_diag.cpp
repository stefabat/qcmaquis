/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */
#ifdef USE_AMBIENT
#include <mpi.h>
#endif
#include <cmath>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <sys/stat.h>

using std::cerr;
using std::cout;
using std::endl;

#ifdef USE_AMBIENT
#include "dmrg/block_matrix/detail/ambient.hpp"
typedef ambient::numeric::tiles<ambient::numeric::matrix<double> > matrix;
#else
#include "dmrg/block_matrix/detail/alps.hpp"
typedef alps::numeric::matrix<double> matrix;
#endif

#include <alps/hdf5.hpp>

#include "dmrg/block_matrix/indexing.h"
#include "dmrg/block_matrix/block_matrix.h"

#include "dmrg/mp_tensors/mps.h"
#include "dmrg/mp_tensors/mpo.h"

#include "dmrg/models/model.h"
#include "dmrg/models/generate_mpo.hpp"
#include "dmrg/mp_tensors/contractions.h"
#include "dmrg/mp_tensors/mps_mpo_ops.h"
#include "dmrg/mp_tensors/mpo_ops.h"

#if defined(USE_TWOU1)
typedef TwoU1 symm;
#elif defined(USE_TWOU1PG)
typedef TwoU1PG symm;
#elif defined(USE_SU2U1)
typedef SU2U1 symm;
#elif defined(USE_SU2U1PG)
typedef SU2U1PG symm;
#elif defined(USE_NONE)
typedef TrivialGroup symm;
#elif defined(USE_U1)
typedef U1 symm;
#endif

#include "dmrg/utils/DmrgOptions.h"
#include "dmrg/utils/DmrgParameters.h"

#include "dmrg/utils/checks.h"


int main(int argc, char ** argv)
{
    try {
        if (argc != 2) {
            std::cout << "Usage: " << argv[0] << " inputfile " << std::endl;
            return 1;
        }

        DmrgOptions opt(argc, argv);
        if (!opt.valid) return 0;
        DmrgParameters parms = opt.parms;

        std::cout << "parameter " << parms << std::endl;

        typedef operator_selector<matrix, symm>::type op_t;
        typedef Model<matrix, symm>::table_ptr table_ptr;
        typedef Model<matrix, symm>::tag_type tag_type;
        typedef std::vector<op_t> op_vec;
        
        std::vector<MPS<matrix, symm>::scalar_type> vector_results;
        std::vector<std::string> labels;

        // Check orbital order of ket MPS
        maquis::checks::orbital_order_check(parms, parms["chkpfile"].str());
        // Check orbital order of bra MPS
        maquis::checks::orbital_order_check(parms, parms["chkp_lhs"].str());

        /// Parsing model
        Lattice lattice = Lattice(parms);
        Model<matrix, symm> model = Model<matrix, symm>(lattice, parms);
        // types are point group irreps...
        int ntypes = lattice.maximum_vertex_type()+1;
        table_ptr tag_handler = model.operators_table();

        //std::cout << "ntypes " << ntypes << std::endl;

        maquis::cout.precision(15);
        std::size_t L = lattice.size();

        // load bra (chkp_lhs) and ket (chkpfile) MPS
        MPS<matrix, symm> mps1, mps2;
        load(parms["chkpfile"].str(),mps1);
        load(parms["chkp_lhs"].str(),mps2);

        /// identities and fillings
        std::vector<op_t> identities(ntypes), fillings(ntypes);
        for (size_t type = 0; type < ntypes; ++type) {
            identities[type] = model.identity_matrix(type);
            fillings[type]   = model.filling_matrix(type);
        }


        std::string measurement;
        std::string op_name;
        op_vec meas_op;
        meas_op.resize(ntypes);
        /// we need to measure the following local ops: 
        //  c+up cup
        //  c+down cdown
        //  c+up cdown
        //  c+down cup
        for (int i = 0; i < 4; ++i){
              
            /// make sure all previous results/labels are wiped out
            labels.clear();
            vector_results.clear();

            if (i == 0){
               measurement = "count_aa_1TDM";
               op_name     = "count_up";
            }
            else if (i == 1){
               measurement = "count_bb_1TDM";
               op_name     = "count_down";
            }
            else if (i == 2){
               measurement = "a2b_1TDM";
               op_name     = "u2d";
            }
            else if (i == 3){
               measurement = "b2a_1TDM";
               op_name     = "d2u";
            }
            else
               throw std::runtime_error("Invalid observable\n");


            /// TODO: the following part can be taken out as a separate function
            std::map<std::string, MPO<matrix, symm> > mpos;
            for (std::size_t p = 0; p < L; ++p)
            {
                int type = lattice.get_prop<int>("type", p);
                meas_op[type] = tag_handler->get_op(model.get_operator_tag(op_name, type));
                //std::cout << "number of blocks for operator at site " << p << ": "<< meas_op[type].n_blocks() << std::endl;

                // generate MPO
                if (meas_op[type].n_blocks() > 0) {


                   generate_mpo::MPOMaker<matrix, symm> mpom(lattice, identities, fillings);
                   generate_mpo::OperatorTerm<matrix, symm> term;
                   term.operators.push_back( std::make_pair(p, meas_op[type]) );
                   mpom.add_term(term);
                    
                   mpos[ lattice.get_prop<std::string>("label", p) ] = mpom.create_mpo();
                   //std::cout << "label for site p " << p << ": "<< lattice.get_prop<std::string>("label", p)  << std::endl;
                }
            }
            /// end of TODO

            typedef std::map<std::string, MPO<matrix, symm> > mpo_map;
            typedef std::map<std::string, matrix::value_type> result_type;
            result_type res;

            for (mpo_map::const_iterator mit = mpos.begin(); mit != mpos.end(); ++mit) {
                 result_type::iterator match = res.find(mit->first);
                 if (match == res.end())
                     boost::tie(match, boost::tuples::ignore) = res.insert( std::make_pair(mit->first, 0.) );

                     std::vector<MPS<matrix, symm>::scalar_type> dct = multi_expval(mps1, mps2, mit->second);
                     match->second = dct[0];
            }

            /// copy results to base and save the data
            vector_results.reserve(vector_results.size() + res.size());
            labels.reserve(labels.size() + res.size());
            for (result_type::const_iterator it = res.begin(); it != res.end(); ++it) {

                /// debug print
                std::cout << "indices:" << it->first << it->first << " --> value: " <<  it->second << std::endl;

                labels.push_back(it->first);
                vector_results.push_back(it->second);
            }

            std::cout << "saving data for measurement " << measurement << " amount of data: " << vector_results.size() << std::endl;

            alps::hdf5::archive oh5(parms["resultfile"].str(), "w");
            oh5["/spectrum/results/" + measurement + "/mean/value"] << vector_results;
            oh5["/spectrum/results/" + measurement + "/labels"] << labels;
        }
        
    } catch (std::exception& e) {
        std::cerr << "Error:" << e.what() << std::endl;
        return 1;
    }
}
