/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef MODELS_CODED_NU1_H
#define MODELS_CODED_NU1_H

#include "dmrg/models/measurements.h"
#include "dmrg/utils/BaseParameters.h"
#include "dmrg/models/prebo/prebo_TermGenerator.hpp"
#include "nu1_nBodyTerm.hpp"
#include "dmrg/models/prebo/prebo_parse_integrals.h"
#include "dmrg/models//model_helper.hpp"
#include "dmrg/models/measurements/prebo_particle_rdm.h"
#include "dmrg/models/measurements/prebo_mutual_information.h"

#include <algorithm>
#include <alps/hdf5/pair.hpp>
#include <map>
#include <sstream>
#include <unordered_map>
#include <chrono>

#ifdef DMRG_PREBO

/**
 * @brief Pre-Born Oppenheimer model class
 * @class preBO models_nu1.hpp
 * 
 * This class enables DMRG calculations for Pre-Born-Oppenheimer Hamiltonians associated 
 * with multiple indistinguishable fermionic particles.
 * Bosonic particles will be implemented soon.
 * 
 * @tparam Matrix matrix type underlying the MPS definition
 */
template<class Matrix, int N>
class PreBO : public model_impl<Matrix, NU1_template<N>> {
    // Types definition
    using NU1 = NU1_template<N>;
    using base = model_impl<Matrix, NU1>;
    using table_type = typename base::table_type;
    using table_ptr = typename base::table_ptr;
    using tag_type = typename base::tag_type;
    using term_descriptor = typename base::term_descriptor;
    using op_t = typename base::op_t;
    using measurements_type = typename base::measurements_type;
    using pos_t = typename Lattice::pos_t;
    using part_type = typename Lattice::part_type;
    using ValueType = typename Matrix::value_type;

private:
    /** Verbosity flag */
    const bool verbose = false;
    /** Size of the DMRG lattice */
    pos_t L;
    /** Overall number of particle types */
    int num_particle_types;
    /** Vector of size [num_particle_types] with true for fermions and false for bosons */
    std::vector<bool> isFermion;
    /** Symmetry sector for each U(1) charge */
    std::vector<int> vec_ini_state;
    /** Reference to the underlying lattice */
    const Lattice& lat;
    /** Input parameters container */
    BaseParameters& parms;
    /** Physical basis per particle type */
    std::vector<Index<NU1>> phys_indexes;
    /** Tag handler */
    std::shared_ptr<TagHandler<Matrix, NU1>> ptr_tag_handler;
    /** Pointer to the term generator (helper class to generate the SQ Hamiltonian) */
    std::shared_ptr<prebo::TermGenerator<Matrix, N>> ptr_term_generator;
public:
    // +----------------+
    //  Main constructor
    // +----------------+
    PreBO(const Lattice& lat_, BaseParameters& parms_, bool verbose_=true) 
        : lat(lat_), parms(parms_), ptr_tag_handler(new table_type()), phys_indexes(0), verbose(verbose_)
    {
        // Get only required variables from the lattice
        num_particle_types = lat.template get_prop<int>("num_particle_types");
        isFermion = lat.template get_prop<std::vector<bool>>("isFermion");
        vec_ini_state = lat.template get_prop<std::vector<int>>("vec_ini_state");
        L = lat.size();
        std::shared_ptr<Lattice> ptr_lat = std::make_shared<Lattice>(lat);
        ptr_term_generator = std::make_shared<prebo::TermGenerator<Matrix, N>>(ptr_lat, ptr_tag_handler, verbose);
        phys_indexes = ptr_term_generator->getPhysIndexes();

    }

    /**
     * @brief Hamiltonian terms creation
     * 
     * Note that, unlike the other classes, the term vector is *not* populated at construction level,
     * and [create_terms] must be called externally to make terms_ get pupulated.
     */
    void create_terms() {
        auto start = std::chrono::high_resolution_clock::now();
        auto integrals = prebo::detail::parse_integrals<ValueType>(parms, lat);
        this->terms_ = ptr_term_generator->generate_Hamiltonian(integrals);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
        if (verbose)
            std::cout << "Construction of the Hamiltonian took "
                      << duration.count() << " seconds" << std::endl << std::endl;
    }

    /**
     * @brief Getter for the physical (sigma) indices 
     * Note that this method is called in the constructor of the MPS Tensor Network
     */
    Index<NU1> const& phys_dim(size_t type) const {
        return phys_indexes[type];
    }

    /**
     * @brief Getter for the QN associated with the calculation.
     * 
     * Returns the total quantum number associated to the Hamiltonian, i.e., the number of particles 
     * per type and Sz value
     */
    typename NU1::charge total_quantum_numbers(BaseParameters& parms) const {
        return typename NU1::charge(vec_ini_state);
    }

    /** @brief Getter for the identity matrix tag */
    tag_type identity_matrix_tag(size_t type) const {
        auto vec_fer_bos = lat.template get_prop<std::vector<int>>("vec_fer_bos");
        // recover identity of the according particle type
        if (isFermion[type]) {
            auto tag_vec = ptr_term_generator->getTagContainer()->get_tag_vector(Type::Fermion, OpType::Ident, Spin::None);
            std::size_t fer = vec_fer_bos[type];
            return tag_vec[fer];
        }
        else {
            auto tag_vec = ptr_term_generator->getTagContainer()->get_tag_vector(Type::Boson, OpType::Ident, Spin::None);
            std::size_t bos = vec_fer_bos[type];
            return tag_vec[bos];
        }
    }

    /** @brief Getter for the filling matrix tag */
    tag_type filling_matrix_tag(size_t type) const {
        auto vec_fer_bos = lat.template get_prop<std::vector<int>>("vec_fer_bos");
        // recover identity of the according particle type
        if (isFermion[type]) {
            auto tag_vec = ptr_term_generator->getTagContainer()->get_tag_vector(Type::Fermion, OpType::Filling, Spin::None);
            std::size_t fer = vec_fer_bos[type];
            return tag_vec[fer];
        }
        else {
            auto tag_vec = ptr_term_generator->getTagContainer()->get_tag_vector(Type::Boson, OpType::Ident, Spin::None);
            std::size_t bos = vec_fer_bos[type];
            return tag_vec[bos];
        }
    }

    /** @brief Converts a string to an operator tag (NYI in this case ) */
    tag_type get_operator_tag(std::string const& name, size_t type) const {
        throw std::runtime_error("Operator not valid for this model.");
    }

    /** @brief Update function */
    void update(BaseParameters const& p) {
        throw std::runtime_error("update() not yet implemented for this model.");
    }
    
    /** @brief Getter for the tag handler */
    table_ptr operators_table() const {
        return ptr_tag_handler;
    }

    // TODO ALB Check if this really true, the index 0 has been hardcoded for the
    //         moment
    measurements_type measurements() const {
        measurements_type meas;

        std::vector<tag_type> identities;
        std::vector<tag_type> fillings;

        for (int iType = 0; iType < lat.getMaxType(); iType++)
        {
            identities.push_back(this->identity_matrix_tag(iType));
            fillings.push_back(this->filling_matrix_tag(iType));
        }

        std::regex expression_1ParticleRDM("^MEASURE\\[1rdm\\]");
        std::regex expression_MutualInformation("^MEASURE\\[mutinf\\]");
        std::smatch what;

        for (auto&& it: parms.get_range()) {
            std::string lhs = it.first;
            // 1-RDM and transition-1RDM
            if (std::regex_match(lhs, what, expression_1ParticleRDM)) {
                meas.push_back( new measurements::PreBOParticleRDM<Matrix, N>(parms, lat, identities, fillings, ptr_term_generator));
            }
            if (std::regex_match(lhs, what, expression_MutualInformation)) {
                meas.push_back( new measurements::PreBOMutualInformation<Matrix, N>(parms, lat, identities, fillings, ptr_term_generator));
            }
        }
        return meas;
    }
};

#endif // DMRG_PREBO

#endif
