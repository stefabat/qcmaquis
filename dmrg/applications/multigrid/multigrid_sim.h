/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef APP_MULTIGRID_SIM_H
#define APP_MULTIGRID_SIM_H

#include <cmath>
#include <iterator>
#include <iostream>
#include <sys/stat.h>

#include <boost/shared_ptr.hpp>

#include "dmrg/sim/sim.h"
#include "dmrg/models/continuum/factory_lattice.hpp"

#include "dmrg/optimize/optimize.h"
#include "dmrg/mp_tensors/multigrid.h"


inline BaseParameters compute_initial_parms(BaseParameters parms)
{
    int initial_graining = 0;

    std::string chkpfile = boost::trim_right_copy_if(parms["chkpfile"].str(), boost::is_any_of("/ "));
    boost::filesystem::path p(chkpfile);
    if (boost::filesystem::exists(p) && boost::filesystem::exists(p / "mps0.h5")) {
            storage::archive ar(chkpfile+"/props.h5");
            if (ar.is_data("/status/graining") && ar.is_scalar("/status/graining"))
                ar["/status/graining"] >> initial_graining;
    }

    parms << parms.iteration_params("graining", initial_graining);
    return parms;
}


template <class Matrix, class SymmGroup>
class multigrid_sim : public sim<Matrix, SymmGroup> {

    typedef sim<Matrix, SymmGroup> base;
    typedef optimizer_base<Matrix, SymmGroup, storage::disk> opt_base_t;
    typedef typename base::status_type status_type;
    typedef typename base::measurements_type measurements_type;

    enum measure_t {sweep_measure, mg_measure};

    using base::mps;
    using base::mpo;
    using base::lat;
    using base::mpoc;
    using base::parms;
    using base::model;
    using base::all_measurements;
    using base::stop_callback;
    using base::init_sweep;
    using base::init_site;
    using base::rfile;

public:
    multigrid_sim(DmrgParameters & parms_)
    : base(compute_initial_parms(parms_))
    , initial_graining(0)
    {
        if (this->restore)
        {
            storage::archive ar(this->chkpfile+"/props.h5");
            ar["/status/graining"] >> initial_graining;
        }
    }

    void model_init()
    {
        /// Model initialization
        this->lat = Lattice(this->parms);
        this->model = Model<Matrix, SymmGroup>(this->lat, this->parms);
        this->mpo = make_mpo(this->lat, this->model);
        this->all_measurements = this->model.measurements();
        this->all_measurements << overlap_measurements<Matrix, SymmGroup>(this->parms);
    }

    std::shared_ptr<mps_initializer<Matrix, SymmGroup> > empty_mps_initiatializer() const
    {
        int max_site_type = 0;
        std::vector<int> site_types(this->lat.size(), 0);
        for (int p = 0; p < this->lat.size(); ++p) {
            site_types[p] = this->lat.template get_prop<int>("type", p);
            max_site_type = std::max(site_types[p], max_site_type);
        }

        std::vector<Index<SymmGroup> > site_bases(max_site_type+1);
        for (int type = 0; type < site_bases.size(); ++type) {
            site_bases[type] = this->model.phys_dim(type);
        }

        std::shared_ptr<mps_initializer<Matrix, SymmGroup> > initializer(new empty_mps_init<Matrix, SymmGroup>(site_bases, site_types));
        return initializer;
    }

    void run()
    {
        /// Set current status in parms
        parms << parms.iteration_params("graining", initial_graining);
        /// Build current model and load/build MPS
        this->model_init();


        for (int graining=initial_graining; graining < parms["ngrainings"]; ++graining)
        {
            /// usual optimization
            if (init_sweep < parms["nsweeps"])
                dmrg_run(graining);

            if ( stop_callback() ) {
                maquis::cout << "Time limit reached." << std::endl;
                break;
            }

            /// fine graining
            if (graining < parms["ngrainings"]-1) {
                maquis::cout << "*** Starting grainings ***" << std::endl;

                BaseParameters iteration_params = parms.iteration_params("graining", graining+1);
                parms << iteration_params;
                this->model_init();
                measurements_type always_measurements = this->iteration_measurements(init_sweep);

                MPS<Matrix, SymmGroup> new_mps = MPS<Matrix, SymmGroup>(lat.size(), *empty_mps_initiatializer());

                int curL = mps.length();
                BaseParameters oldparms(parms);
                oldparms << parms.iteration_params("graining", graining);

                std::vector<MPO<Matrix, SymmGroup> > mpo_mix;
                if (parms["model_library"] == "continuum") {
                    mpo_mix.resize(curL+1, MPO<Matrix, SymmGroup>(0));
                    double r = lat.size() / curL;
                    for (int i=0; i<=curL; ++i)
                        mpo_mix[i] = mixed_mpo(base::parms, r*i, oldparms, curL-i);
                }

                results_collector graining_results;
                if (curL < new_mps.length())
                    graining_results = multigrid::extension_optim(base::parms, mps, new_mps, mpo_mix);
                else if (this->mps.length() > new_mps.length())
                    throw std::runtime_error("Restriction operation not really implemented.");
                // graining_results = multigrid::restriction(this->mps, initial_mps);

                /// swap mps
                swap(mps, new_mps);


                /// write iteration results
                {
                    storage::archive ar(rfile, "w");
                    ar[results_archive_path(0, graining, mg_measure) + "/parameters"] << iteration_params;
                    ar[results_archive_path(0, graining, mg_measure) + "/results"] << graining_results;
                }

                /// measure observables specified in 'always_measure'
                if (always_measurements.size() > 0)
                    this->measure(results_archive_path(0, graining, mg_measure) + "/results/", always_measurements);


                /// checkpoint new mps
                this->checkpoint_simulation(mps, 0, -1, graining+1);
            }

            if ( stop_callback() ) {
                maquis::cout << "Time limit reached." << std::endl;
                break;
            }
        }
    }

    ~multigrid_sim()
    {
        storage::disk::sync();
    }

private:

    std::string results_archive_path(status_type const& status) const
    {
        throw std::runtime_error("do not use in multigrid.");
    }

    void checkpoint_simulation(MPS<Matrix, SymmGroup> const& state, int sweep, int site, int graining)
    {
        status_type status;
        status["sweep"]    = sweep;
        status["site"]     = site;
        status["graining"] = graining;
        return base::checkpoint_simulation(state, status);
    }

    std::string results_archive_path(int sweep, int graining, measure_t m_type) const
    {
        std::ostringstream oss;
        oss.str("");
        switch (m_type) {
            case sweep_measure:
                oss << "/simulation/iteration/graining/" << graining << "/sweep" << sweep;
                break;
            case mg_measure:
                oss << "/simulation/iteration/graining/" << graining;
                break;
        }
        return oss.str();
    }

    void dmrg_run(int graining)
    {
        int meas_each = parms["measure_each"];
        int chkp_each = parms["chkp_each"];

        /// Optimizer initialization
        std::shared_ptr<opt_base_t> optimizer;
        if (parms["optimization"] == "singlesite")
        {
            optimizer.reset( new ss_optimize<Matrix, SymmGroup, storage::disk>
                            (mps, mpo, parms, stop_callback, init_site) );
        }
        else if(parms["optimization"] == "twosite")
        {
            optimizer.reset( new ts_optimize<Matrix, SymmGroup, storage::disk>
                            (mps, mpo, parms, stop_callback, init_site) );
        }
        else {
            throw std::runtime_error("Don't know this optimizer");
        }

        measurements_type always_measurements = this->iteration_measurements(init_sweep);
        try {
            for (int sweep=init_sweep; sweep < parms["nsweeps"]; ++sweep) {
                // TODO: introduce some timings

                optimizer->sweep(sweep, Both);
                storage::disk::sync();

                if ((sweep+1) % meas_each == 0 || (sweep+1) == parms["nsweeps"])
                {
                    /// write iteration results
                    {
                        storage::archive ar(rfile, "w");
                        ar[results_archive_path(sweep, graining, sweep_measure) + "/parameters"] << parms;
                        ar[results_archive_path(sweep, graining, sweep_measure) + "/results"] << optimizer->iteration_results();
                        // ar[results_archive_path(sweep, graining, sweep_measure) + "/results/Runtime/mean/value"] << std::vector<double>(1, elapsed_sweep + elapsed_measure);
                    }

                    /// measure observables specified in 'always_measure'
                    if (always_measurements.size() > 0)
                        this->measure(results_archive_path(sweep, graining, sweep_measure) + "/results/", always_measurements);

                }

                /// write checkpoint
                bool stopped = stop_callback();
                if (stopped || (sweep+1) % chkp_each == 0 || (sweep+1) == parms["nsweeps"])
                    this->checkpoint_simulation(mps, sweep, -1, graining);

                if (stopped) break;
            }
        } catch (dmrg::time_limit const& e) {
            maquis::cout << e.what() << " checkpointing partial result." << std::endl;
            this->checkpoint_simulation(mps, e.sweep(), e.site(), graining);

            {
                storage::archive ar(rfile, "w");
                ar[results_archive_path(e.sweep(), graining, sweep_measure) + "/parameters"] << parms;
                ar[results_archive_path(e.sweep(), graining, sweep_measure) + "/results"] << optimizer->iteration_results();
                // ar[results_archive_path(e.sweep(), graining, sweep_measure) + "/results/Runtime/mean/value"] << std::vector<double>(1, elapsed_sweep + elapsed_measure);
            }
        }

        /// for the next graining level
        init_sweep = 0;
        init_site  = -1;
    }


    MPO<Matrix, SymmGroup> mixed_mpo(BaseParameters & parms1, int L1, BaseParameters & parms2, int L2)
    {
        assert( parms1["LATTICE"] == parms2["LATTICE"] );

        typedef std::shared_ptr<lattice_impl> lattice_ptr;

        lattice_ptr latptr;
        if (parms1["LATTICE"] == "continuous_chain"
            || parms1["LATTICE"] == std::string("continuous_left_chain"))
            latptr = lattice_ptr(new MixedContChain(parms1, L1, parms2, L2));
        else if (parms2["LATTICE"] == std::string("continuous_center_chain"))
            latptr = lattice_ptr(new MixedContChain_c(parms1, L1, parms2, L2));
        else
            throw std::runtime_error("Don't know this lattice!");
        Lattice lat(latptr);

#ifndef NDEBUG
        // debugging output, to be removed soon!
//        maquis::cout << "MIXED LATTICE ( " << L1 << ", " <<  L2 << " )" << std::endl;
//        for (int p=0; p<lat->size(); ++p) {
//            maquis::cout << lat->get_prop<std::string>("label", p) << ": " << lat->get_prop<double>("dx", p) << std::endl;
//            maquis::cout << lat->get_prop<std::string>("label", p, p+1) << ": " << lat->get_prop<double>("dx", p, p+1) << std::endl;
//            maquis::cout << lat->get_prop<std::string>("label", p, p-1) << ": " << lat->get_prop<double>("dx", p, p-1) << std::endl;
//        }
#endif

        Model<Matrix, SymmGroup> tmpmodel = model_factory<Matrix, SymmGroup>(lat, parms1);;
        return make_mpo(lat, tmpmodel);
    }

private:
    int initial_graining;
};

#endif
