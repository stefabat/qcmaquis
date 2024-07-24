/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef MAQUIS_DMRG_PREBO_LATTICE
#define MAQUIS_DMRG_PREBO_LATTICE

#include "dmrg/models/lattice/lattice.h"
#include <sstream>
#include <vector>
#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <numeric>
#include "dmrg/utils/BaseParameters.h"

// +----------------------------+
//  PRE BORN OPPENHEIMER LATTICE
// +----------------------------+
//
// The PreBOLattice describes the sites for the full Hamiltonian.
//
class PreBOLattice : public lattice_impl
{
public:
    // Types definition
    typedef lattice_impl::pos_t  pos_t;
    // -- Constructor --
    // In addition to a standard lattice constructor, it also loads the number
    // of basis function per mode
    explicit PreBOLattice (BaseParameters & parms)
    {

        // Populate variables:
        auto vec_particles_str = parms["PreBO_ParticleTypeVector"].as<std::string>();
        auto isFermion_str = parms["PreBO_FermionOrBosonVector"].as<std::string>();
        auto orbitals_str = parms["PreBO_OrbitalVector"].as<std::string>();
        auto vec_ini_state_str = parms["PreBO_InitialStateVector"].as<std::string>();
        std::string max_m_str;
        if (parms.is_set("PreBO_MaxBondDimVector"))
            max_m_str = parms["PreBO_MaxBondDimVector"].as<std::string>();
        // convert strings to vectors
        std::istringstream is( vec_particles_str );
        vec_particles.assign(std::istream_iterator<int>( is ), std::istream_iterator<int>() );
        is.str(std::string());
        is.clear();
        is.str(isFermion_str);
        isFermion.assign(std::istream_iterator<int>( is ), std::istream_iterator<int>() );
        is.str(std::string());
        is.clear();
        is.str(orbitals_str);
        vec_orbitals.assign(std::istream_iterator<int>( is ), std::istream_iterator<int>() );
        is.str(std::string());
        is.clear();
        is.str(vec_ini_state_str);
        vec_ini_state.assign(std::istream_iterator<int>( is ), std::istream_iterator<int>() );
        is.str(std::string());
        is.clear();
        if (parms.is_set("PreBO_MaxBondDimVector")) {
            is.str(max_m_str);
            vec_max_m.assign(std::istream_iterator<int>( is ), std::istream_iterator<int>() );
            is.str(std::string());
            is.clear();
        }

        num_particle_types = vec_particles.size();
        // ATTENTION MINUS ONE!!!
        maximum_vertex = num_particle_types;

        // construct lattice containing the particle types
        // the length of the lattice is determined by the number of different particle types and the number
        // of the basis functions of each particle type.
        L = std::accumulate(vec_orbitals.begin(), vec_orbitals.end(), 0);
        parms.set("L", L);
        std::vector<part_type> vec_abs_index_part_type;
        for (part_type i = 0; i < num_particle_types; i++) {
            for (std::size_t j = 0; j < vec_orbitals[i]; j++) {
                vec_abs_index_part_type.push_back(i);
            }
        }
        // initialize the orbital lattice as the identity
        m_order.resize(L);
        m_inv_order.resize(L);
        for (pos_t i = 0; i < L; i++) {
            m_order[i] = i;
            m_inv_order[i] = i;
        }
        // If sites_order is given, permute the Hamiltonian MPO
        if (parms.is_set("orbital_order")) {
            m_order = parms["orbital_order"].as<std::vector<pos_t> >();
            vec_lattice_type.resize(L);
            m_inv_order.resize(L);
            if (m_order.size() != L)
                throw std::runtime_error("orbital_order length is not the same as the number of orbitals\n");
            for (int p = 0; p < m_order.size(); ++p)
                m_inv_order[p] = std::distance(m_order.begin(), std::find(m_order.begin(), m_order.end(), p));
            for (pos_t i = 0; i < L; i++) {
                vec_lattice_type[i] = vec_abs_index_part_type[m_order[i]];
            }
        }
        else
            vec_lattice_type=vec_abs_index_part_type;
        //populate vec_fer_bos
        unsigned int bos_temp = 0;
        unsigned int fer_temp = 0;
        for (unsigned int i=0; i<num_particle_types; i++) {
            if (isFermion[i]) {
                vec_fer_bos.push_back(fer_temp);
                fer_temp++;
            }
            else {
                vec_fer_bos.push_back(bos_temp);
                bos_temp++;
            }
        } // for loop


    } // Constructor
    // +-------+
    //  Methods
    // +-------+
    // -- GETTERS --
    /**
     * Takes the relative position of a given particle type and returns its absolute position on the lattice
     * @param pt
     * @param rel_pos
     * @return
     */
    pos_t get_abs_position(part_type const & pt, pos_t const & rel_pos) const {
        //throw std::runtime_error("get_abs_position must be debugged first.");
        //unsigned int abs_pos=0;
        //for (unsigned int i=0; i<pt; i++) {
        //    for (unsigned int j=0; j<vec_orbitals[i]; j++) {
        //        abs_pos++;
        //    }
        //}
        //abs_pos+=rel_pos;
        //return abs_pos;
//        auto it = (vec_orbitals.begin() + pt);
        return std::accumulate(vec_orbitals.begin(), (vec_orbitals.begin() + pt) , rel_pos);
    }
    // The following methods are the same as for the Orbital and the Open Chain Lattice.
    // The only difference is the way in which the lattice site type is extracted, which
    // is taken from the orbital lattice
    std::vector<pos_t> forward(pos_t i) const {
        std::vector<pos_t> ret;
        if (i < L-1)
            ret.push_back(i+1);
        return ret;
    }
    //
    std::vector<pos_t> all(pos_t i) const {
        std::vector<pos_t> ret;
        if (i < L-1)
            ret.push_back(i+1);
        if (i > 0)
            ret.push_back(i-1);
        return ret;
    }
    //
    /**
     * This function takes the property as input and returns its value.
     * E.g.: "type" at position 12 in lattice --> 0 (electron)
     * @param property
     * @param pos
     * @return boost::any
     */
    boost::any get_prop_(std::string const & property, std::vector<pos_t> const & pos) const
    {
        if (property == "type" && pos.size() == 1)
            return boost::any(vec_lattice_type[pos[0]]);
        else if (property == "Mmax" && pos.size() == 1)
            return boost::any(vec_max_m[pos[0]]);
        else if (property == "NumTypes")
            return boost::any(num_particle_types);
        else if (property == "ParticleType" && pos.size() == 1)
            return boost::any( vec_lattice_type[pos[0]] );
        else if (property == "label" && pos.size() == 2)
            return boost::any(bond_label(pos[0], pos[1]));
        else if (property == "vec_particles")
            return boost::any(vec_particles);
        else if (property == "num_particle_types")
            return boost::any(num_particle_types);
        else if (property == "isFermion")
            return boost::any(isFermion);
        else if (property == "vec_orbitals")
            return boost::any(vec_orbitals);
        else if (property == "vec_ini_state")
            return boost::any(vec_ini_state);
        else if (property == "vec_fer_bos")
            return boost::any(vec_fer_bos);
        else if (property == "order")
            return boost::any(m_order);
        else if (property == "inv_order")
            return boost::any(m_inv_order);
        else {
            std::ostringstream ss;
            ss << "No property '" << property << "' with " << pos.size() << " points implemented.";
            throw std::runtime_error(ss.str());
        }
    }

    pos_t size() const { return L; }
    int getMaxType() const { return maximum_vertex; }

private:
    // +----------+
    //  Attributes
    // +----------+
    pos_t L;       // Size of the DMRG lattice
    int num_particle_types;                    // Number of particle types
    std::vector<int> vec_particles;            // Number of particles per type
    std::vector<int> vec_ini_state;            // Specifies the initial state of the system.
    std::vector<bool> isFermion;               // Fermion 1, Boson 0
    std::vector<int> vec_orbitals;             // Number of orbitals per type
    std::vector<part_type> vec_lattice_type;   // Stores the index of the particle type for each site.
    int maximum_vertex;                        // Largest index for site types
    std::vector<int> vec_ibs;                  // Just in here for safety checks
    std::vector<int> vec_fer_bos;              // vector that maps the particle types vector
                                                // to a "fermion -- boson count vector"
                                                // isFermion      = {1, 1, 0, 0, 1}
                                                // vec_fer_bos    = {0, 1, 0, 1, 2}
    // example H2O:
    // num_particle_types = 3
    // vec_particles = {10, 2, 1}
    // isFermion = {1, 1, 0}
    // vec_orbitals = {42, 42, 42}
    std::vector<pos_t> m_order;                // ordering of the sites -> E.g. canonical or Fiedler
    std::vector<pos_t> m_inv_order;            // inverted ordering of the sites
    std::vector<std::size_t> vec_max_m;        // Stores max bond dim for all particle types

    // +-------------------+
    //   Printing routines
    // +-------------------+
    std::string site_label (int i) const {
        return "( " + boost::lexical_cast<std::string>(i) + " )";
    }

    std::string bond_label (int i, int j) const {
        return (  "( " + boost::lexical_cast<std::string>(i) + " )"
                  + " -- " + "( " + boost::lexical_cast<std::string>(j) + " )");
    }
};

#endif
