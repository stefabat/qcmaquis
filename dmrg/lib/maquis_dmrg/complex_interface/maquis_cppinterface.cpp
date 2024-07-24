/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */
#include "maquis_cppinterface.h"
#include <memory>
#include <string>
#include <array>
#include "../maquis_dmrg.h"

std::unique_ptr<maquis::DMRGInterface<std::complex<double>> > cpp_interface_ptr;
DmrgParameters cpp_parms;

    typedef std::complex<double> V;
    void qcmaquis_interface_preinit(int nel_, int L_, int irrep_,
                                    int* site_types_,
                                    double conv_thresh_, int m_,
                                    bool meas_2rdm_, bool entropy_, bool magnetism_,
                                    const std::string& init_type_, const std::string& optimization_,
                                    int ietl_jcd_maxiter_,
                                    double ietl_jcd_tol_, double truncation_initial_,
                                    double truncation_final_, double integral_cutoff_,
                                    const std::string& twosite_truncation_, const std::string& orb_order_)

    {

        maquis::prepare_relativistic(cpp_parms, magnetism_);
        // optimization conditions
        cpp_parms.set("orbital_order", orb_order_);
        cpp_parms.set("init_type", init_type_);
        cpp_parms.set("optimization", optimization_);
        cpp_parms.set("integral_cutoff", integral_cutoff_);
        cpp_parms.set("truncation_initial", truncation_initial_);
        cpp_parms.set("truncation_final", truncation_final_);
        cpp_parms.set("ietl_jcd_tol", ietl_jcd_tol_);
        cpp_parms.set("ietl_jcd_maxiter", ietl_jcd_maxiter_);
        cpp_parms.set("conv_thresh", conv_thresh_);
        cpp_parms.set("twosite_truncation", twosite_truncation_);
        cpp_parms.set("storagedir", "./tmp/");

        cpp_parms.set("alpha_initial", 0.0);
        cpp_parms.set("alpha_main", 0.0);
        cpp_parms.set("alpha_final", 0.0);

        // for now, making things easier
        cpp_parms.set("MEASURE[1rdm]", 1);
        if (meas_2rdm_) cpp_parms.set("MEASURE[2rdm]", 1);

        if(entropy_)
            cpp_parms.set("MEASURE[ChemEntropy]",1);


        // bond dimension
        cpp_parms.set("max_bond_dimension", m_);
        cpp_parms.set("init_bond_dimension", m_);

        // lattice, #e-, symmetry
        cpp_parms.set("L", L_);
        cpp_parms.set("nelec", nel_);
        cpp_parms.set("irrep", irrep_);

        // site types
        if (site_types_ != nullptr)
        {
            std::string s;
            for (int i = 0; i < L_; i++)
                s += (std::to_string(site_types_[i])+ (i < (L_-1) ? "," : ""));
            cpp_parms.set("site_types", s);
        }
        else
        {
            std::string s;
            for (int i = 0; i < L_; i++)
                s += (std::to_string(1)+ (i < (L_-1) ? "," : ""));
            cpp_parms.set("site_types", s);
        }

        //std::cout << " parms are set -> " << std::endl;
        //std::cout << cpp_parms << std::endl;
    }

    void qcmaquis_interface_update_integrals(std::vector<std::vector<int>> integral_indices, std::vector<V> integral_values, int integral_size)
    {
        if (cpp_parms.is_set("integral_file")||cpp_parms.is_set("integrals"))
            throw std::runtime_error("updating integrals in the interface not supported yet in the FCIDUMP format");

        // set integrals
        maquis::integral_map<V> integrals;

        for (int i = 0; i < integral_size; i++)
        {
            std::array<int, 4> idx {integral_indices[i][0], integral_indices[i][1], integral_indices[i][2],
integral_indices[i][3]};
            V value = integral_values.at(i);
            integrals[idx] = value;
        }

        /*
        std::cout << " integrals from map1 after ... " <<  std::endl;
        for (auto&& t: integrals){
            std::cout << "element " << t.first[0] << t.first[1] << t.first[2] << t.first[3] << " is " << t.second << std::endl;
        }
        */

        if (!cpp_parms.is_set("integrals_binary"))
            cpp_parms.set("integrals_binary", maquis::serialize(integrals));

        // Call an integral update only if the interface has been initialised
        if (cpp_interface_ptr){
            cpp_interface_ptr->update_integrals(integrals);
        }
        //qcmaquis_interface_report("BLUBB BLUBB");

    }

    void qcmaquis_interface_set_nsweeps(int nsweeps_, int ngrowsweeps_, int nmainsweeps_)
    {
        cpp_parms.set("nsweeps", nsweeps_);
        cpp_parms.set("ngrowsweeps", ngrowsweeps_);
        cpp_parms.set("nmainsweeps", nmainsweeps_);
    }

    void qcmaquis_interface_report(const std::string& reportid_)
    {
        std::cout << " reporting parms for -> " << reportid_ << std::endl;
        std::cout << cpp_parms << std::endl;
    }

    // Start a new simulation with stored parameters
    void qcmaquis_interface_reset()
    {
        cpp_interface_ptr.reset(new maquis::DMRGInterface<V>(cpp_parms));
    }

    void qcmaquis_interface_optimize()
    {
        cpp_interface_ptr->optimize();
    }
    std::complex<double> qcmaquis_interface_get_energy()
    {
        return cpp_interface_ptr->energy();
    }

    void qcmaquis_interface_set_state(int state_)
    {
        std::string str;
        for (int i = 0; i < state_; i++){
            str += "chk." + std::to_string(i) + ".h5" + ((i < state_-1) ? " " : "") ;
        }

        cpp_parms.set("ortho_states", str);
        cpp_parms.set("n_ortho_states", state_);
        cpp_parms.set("chkpfile", "chk." + std::to_string(state_) + ".h5");
        cpp_parms.set("resultfile", "res." + std::to_string(state_) + ".h5");

        qcmaquis_interface_reset();
    }

    void qcmaquis_interface_get_1rdm(std::vector<std::pair<V,std::array<int,2>>> *rdm1) // BAGEL
    {
        const typename maquis::DMRGInterface<V>::meas_with_results_type& meas = cpp_interface_ptr->onerdm();
        for (int i = 0; i < meas.first.size(); i++)
        {
            std::array<int,2> pos_ = {meas.first[i][0], meas.first[i][1]};
            rdm1->push_back(std::make_pair(meas.second[i], pos_));
        }
    }

    void qcmaquis_interface_get_1rdm(V* rdm1) // CQ
    {
        const typename maquis::DMRGInterface<V>::meas_with_results_type& meas = cpp_interface_ptr->onerdm();

        int L_ = cpp_parms.get<int>("L");

        // clean memory
        std::fill_n(rdm1,L_*L_,V(0.));

        // setup spinorOrder from QCM to CQ
        std::vector<int> revSpinorOrder;
        // initialize spinor reordering index vector (first unbarred than the barred partner)
        revSpinorOrder.reserve(L_);
        for (int i = 0; i < L_; i++){
            if(i < L_/2){ /* unbarred */
                revSpinorOrder.push_back(2*i         );
            }else{ /* barred */
                revSpinorOrder.push_back(2*i - L_ + 1);
            }
        }

        for (int i = 0; i < meas.first.size(); i++)
        {
            //std::cout << " 1-rdm from meas ... element " <<  meas.first[i][0]<<" " << meas.first[i][1] << " is --> " << meas.second[i] << std::endl;
                rdm1[revSpinorOrder[meas.first[i][0]] + revSpinorOrder[meas.first[i][1]]*L_] =           meas.second[i];
            if(meas.first[i][0] < meas.first[i][1])
                rdm1[revSpinorOrder[meas.first[i][1]] + revSpinorOrder[meas.first[i][0]]*L_] = std::conj(meas.second[i]);
        }
    }

    // hooray for copy-paste
    void qcmaquis_interface_get_2rdm(std::vector<std::pair<V,std::array<int,4>>> *rdm2) // BAGEL
    {
        const typename maquis::DMRGInterface<V>::meas_with_results_type& meas = cpp_interface_ptr->twordm();

        for (int i = 0; i < meas.first.size(); i++)
        {
            //std::cout << " 2-rdm from meas ... element " <<  i << " is --> " << meas.second[i] << std::endl;
            std::array<int,4> pos_ = {meas.first[i][0], meas.first[i][1], meas.first[i][2], meas.first[i][3]};
            rdm2->push_back(std::make_pair(meas.second[i], pos_));
        }
    }

    void qcmaquis_interface_get_2rdm(V* rdm2)
    {
        const typename maquis::DMRGInterface<V>::meas_with_results_type& meas = cpp_interface_ptr->twordm();

        int L_  = cpp_parms.get<int>("L");
        int L2_ = L_*L_;
        int L3_ = L_*L_*L_;

        // clean memory
        std::fill_n(rdm2,L_*L_*L_*L_,V(0.));

        // setup spinorOrder from QCM to CQ
        std::vector<int> revSpinorOrder;
        // initialize spinor reordering index vector (first unbarred than the barred partner)
        revSpinorOrder.reserve(L_);
        for (int i = 0; i < L_; i++){
            if(i < L_/2){ /* unbarred */
                revSpinorOrder.push_back(2*i         );
            }else{ /* barred */
                revSpinorOrder.push_back(2*i - L_ + 1);
            }
        }

        for (int i = 0; i < meas.first.size(); i++)
        {
            std::array<int,4> pos_ = {meas.first[i][0], meas.first[i][1], meas.first[i][2], meas.first[i][3]};

            // permutation           m                         n                            o                             p
            rdm2[revSpinorOrder[pos_[0]] + revSpinorOrder[pos_[1]]*L_ + revSpinorOrder[pos_[2]]*L2_ + revSpinorOrder[pos_[3]]*L3_] =  meas.second[i];
            // permutation           o                         p                            m                             n
            rdm2[revSpinorOrder[pos_[2]] + revSpinorOrder[pos_[3]]*L_ + revSpinorOrder[pos_[0]]*L2_ + revSpinorOrder[pos_[1]]*L3_] =  meas.second[i];
            // permutation           m                         p                            o                             n
            rdm2[revSpinorOrder[pos_[0]] + revSpinorOrder[pos_[3]]*L_ + revSpinorOrder[pos_[2]]*L2_ + revSpinorOrder[pos_[1]]*L3_] = -meas.second[i];
            // permutation           o                         n                            m                             p
            rdm2[revSpinorOrder[pos_[2]] + revSpinorOrder[pos_[1]]*L_ + revSpinorOrder[pos_[0]]*L2_ + revSpinorOrder[pos_[3]]*L3_] = -meas.second[i];
        }
    }

    void qcmaquis_interface_get_iteration_results(int* nsweeps, std::size_t* m, V* truncated_weight, V* truncated_fraction, V* smallest_ev)
    {
        // Get iteration results from the last sweep
        results_collector& iter = cpp_interface_ptr->get_iteration_results();

        // iter contains results, one element per microiteration
        const std::vector<boost::any>& m_vec = iter["BondDimension"].get();
        const std::vector<boost::any>& tw_vec = iter["TruncatedWeight"].get();
        const std::vector<boost::any>& tf_vec = iter["TruncatedFraction"].get();
        const std::vector<boost::any>& ev_vec = iter["SmallestEV"].get();

        // We return the sum of these values for the last sweep
        // this should be done with transform_reduce

        *m = 0; for (auto&& m_ : m_vec) *m += boost::any_cast<std::size_t>(m_);
        *truncated_weight = 0; for (auto&& tw_ : tw_vec) *truncated_weight += boost::any_cast<V>(tw_);
        *truncated_fraction = 0; for (auto&& tf_ : tf_vec) *truncated_fraction += boost::any_cast<V>(tf_);
        *smallest_ev = 0; for (auto&& ev_ : ev_vec) *smallest_ev += boost::any_cast<V>(ev_);
        *nsweeps = cpp_interface_ptr->get_last_sweep();
    }
