/*****************************************************************************
*
* ALPS MPS DMRG Project
*
* Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
*               2011-2011 by Bela Bauer <bauerb@phys.ethz.ch>
*               2011-2013    Michele Dolfi <dolfim@phys.ethz.ch>
*               2014-2014    Sebastian Keller <sebkelle@phys.ethz.ch>
*               2019         Leon Freitag <lefreita@ethz.ch>
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
#include "maquis_cinterface.h"
#include <memory>
#include <string>
#include <array>
#include "maquis_dmrg.h"

std::unique_ptr<maquis::DMRGInterface<double> > interface_ptr;
DmrgParameters parms;
std::string pname;

extern "C"
{
    typedef double V;
    void qcmaquis_interface_preinit(int nel, int L, int spin, int irrep,
                                 int* site_types, V conv_thresh, int m, int nsweeps,
                                 int* sweep_m, int nsweepm, char* project_name,
                                 bool meas_2rdm)
    {
        parms.set("symmetry", "su2u1pg");
        parms.set("nelec", nel);
        parms.set("spin", spin);
        parms.set("L", L);
        parms.set("conv_thresh", conv_thresh);
        parms.set("nsweeps", nsweeps);
        parms.set("irrep", irrep);

        // for now, making things easier
        parms.set("MEASURE[1rdm]", 1);

        if (meas_2rdm) parms.set("MEASURE[2rdm]", 1);

        parms.set("max_bond_dimension", m);
        if (sweep_m != NULL)
        {
            std::string sweep_bond_dim;
            for (int i = 0; i < nsweepm; i++)
                sweep_bond_dim += std::to_string(sweep_m[i]) + ((i < nsweepm - 1) ? "," : "") ;
            parms.set("sweep_bond_dimension", sweep_bond_dim);
        }

        pname = project_name;

        // set checkpoint folder
        parms.set("chkpfile", pname + ".checkpoint_state.0.h5");
        parms.set("resultfile", pname + ".results_state.0.h5");

        if (site_types != NULL)
        {
            std::string site_types_str;
            for (int i = 0; i < L; i++)
                site_types_str += std::to_string(site_types[i]) + ((i < L - 1) ? "," : "") ;
            parms.set("site_types", site_types_str);
        }
        else
        {
            std::string site_types_str;
            for (int i = 0; i < L; i++)
                site_types_str += "0" + (i < L - 1) ? "," : "" ;
            parms.set("site_types", site_types_str);
        }

        parms.set("conv_thresh", conv_thresh);

    }

    void qcmaquis_interface_update_integrals(int* integral_indices, V* integral_values, int integral_size)
    {
        if (parms.is_set("integral_file")||parms.is_set("integrals"))
            throw std::runtime_error("updating integrals in the interface not supported yet in the FCIDUMP format");
        // set integrals
        maquis::integral_map<double> integrals;

        for (int i = 0; i < integral_size; i++)
        {
            std::array<int, 4> idx {integral_indices[4*i], integral_indices[4*i+1], integral_indices[4*i+2], integral_indices[4*i+3]};
            V value = integral_values[i];
            integrals[idx] = value;
        }

        if (!parms.is_set("integrals_binary")) // if we didn't initialize the integrals yet, initialize the interface
        {
            parms.set("integrals_binary", maquis::serialize<double>(integrals));
            qcmaquis_interface_reset();
        }
        else
        {
            interface_ptr->update_integrals(integrals);
        }

    }

    // Start a new simulation with stored parameters
    void qcmaquis_interface_reset()
    {
        interface_ptr.reset(new maquis::DMRGInterface<double>(parms));
    }

    void qcmaquis_interface_optimize()
    {
        interface_ptr->optimize();
    }
    double qcmaquis_interface_get_energy()
    {
        return interface_ptr->energy();
    }

    void qcmaquis_interface_set_state(int state)
    {
        std::string str;
        for (int i = 0; i < state; i++)
            str += pname + ".checkpoint_state." + std::to_string(state-1) + ".h5" + ((i < state-1) ? ";" : "") ;


        parms.set("ortho_states", str);
        parms.set("n_ortho_states", state-1);
        parms.set("chkpfile", pname + ".checkpoint_state." + std::to_string(state) + ".h5");
        parms.set("resultfile", pname + ".results_state." + std::to_string(state) + ".h5");

        qcmaquis_interface_reset();
    }

    void qcmaquis_interface_get_1rdm(int* indices, V* values, int size)
    {
        const typename maquis::DMRGInterface<double>::meas_with_results_type& meas = interface_ptr->onerdm();
        // the size attribute is pretty much useless if we allocate the output arrays outside of the interface
        // we'll just use it to check if the size matches the size of the measurement
        assert(size == meas.first.size() == meas.second.size());
        for (int i = 0; i < meas.first.size(); i++)
        {
            values[i] = meas.second[i];
            indices[2*i] = meas.first[i][0];
            indices[2*i+1] = meas.first[i][1];
        }
    }

    // hooray for copy-paste
    void qcmaquis_interface_get_2rdm(int* indices, V* values, int size)
    {
        const typename maquis::DMRGInterface<double>::meas_with_results_type& meas = interface_ptr->twordm();

        assert(size == meas.first.size() == meas.second.size());
        for (int i = 0; i < meas.first.size(); i++)
        {
            values[i] = meas.second[i];
            indices[4*i] = meas.first[i][0];
            indices[4*i+1] = meas.first[i][1];
            indices[4*i+2] = meas.first[i][2];
            indices[4*i+3] = meas.first[i][3];

        }
    }

    void qcmaquis_interface_get_iteration_results(int* nsweeps, std::size_t* m, V* truncated_weight, V* truncated_fraction, V* smallest_ev)
    {
        results_collector& iter = interface_ptr->get_iteration_results();
        const std::vector<boost::any>& m_vec = iter["BondDimension"].get();
        const std::vector<boost::any>& tw_vec = iter["TruncatedWeight"].get();
        const std::vector<boost::any>& tf_vec = iter["TruncatedFraction"].get();
        const std::vector<boost::any>& ev_vec = iter["SmallestEV"].get();

        *m = boost::any_cast<std::size_t>(m_vec[m_vec.size()-1]);
        *truncated_weight = boost::any_cast<V>(tw_vec[tw_vec.size()-1]);
        *truncated_fraction = boost::any_cast<V>(tf_vec[tf_vec.size()-1]);
        *smallest_ev = boost::any_cast<V>(ev_vec[ev_vec.size()-1]);
        *nsweeps = m_vec.size(); // TODO: check
    }
}