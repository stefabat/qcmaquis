/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2013 Institute for Theoretical Physics, ETH Zurich
 *               2013-2013 by Michele Dolfi <dolfim@phys.ethz.ch>
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

#include "dmrg/mp_tensors/mpo.h"
#include "dmrg/models/model.h"
#include "dmrg/models/generate_mpo.hpp"

#if defined(USE_TWOU1)
typedef TwoU1 symm;
#elif defined(USE_NONE)
typedef TrivialGroup symm;
#elif defined(USE_U1)
typedef U1 symm;
#endif

#include "dmrg/utils/DmrgParameters2.h"


int main(int argc, char ** argv)
{
    try {
        if (argc != 2 && argc != 3)
        {
            maquis::cout << "Usage: <parms> [<model_parms>]" << std::endl;
            exit(1);
        }
        
        maquis::cout.precision(10);
        
        /// Load parameters
        std::ifstream param_file(argv[1]);
        if (!param_file)
            throw std::runtime_error("Could not open parameter file.");
        DmrgParameters parms(param_file);
        
        /// Load model parameters from second input (if needed)
        std::string model_file;
        if (parms.is_set("model_file")) model_file = parms["model_file"].str();
        if (argc == 3)                  model_file = std::string(argv[2]);
        if (!model_file.empty()) {
            std::ifstream model_ifs(model_file.c_str());
            if (!model_ifs)
                throw std::runtime_error("Could not open model_parms file.");
            parms << ModelParameters(model_ifs);
        }
        
        
        /// Parsing model
        Lattice lattice = Lattice(parms);
        Model<matrix, symm> model = Model<matrix, symm>(lattice, parms);
        
        MPO<matrix, symm> mpo = make_mpo(lattice, model, parms);
        
        for (int p = 0; p < lattice.size(); ++p) {
            std::ofstream ofs(std::string("mpo_stats."+boost::lexical_cast<std::string>(p)+".dat").c_str());
            for (int b1 = 0; b1 < mpo[p].row_dim(); ++b1) {
                for (int b2 = 0; b2 < mpo[p].col_dim(); ++b2) {
                    if (mpo[p].has(b1, b2)) ofs << mpo[p].tag_number(b1,b2)+1 << " ";
                    else ofs << "0 ";
                }
                ofs << std::endl;
            }
        }
        
    } catch (std::exception& e) {
        std::cerr << "Error:" << std::endl << e.what() << std::endl;
        return 1;
    }
}
