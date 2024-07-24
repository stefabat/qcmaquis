/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef MAQUIS_DMRG_PREBO_TERMGENERATOR_HPP
#define MAQUIS_DMRG_PREBO_TERMGENERATOR_HPP

#include "dmrg/models/lattice/lattice.h"
#include "dmrg/models/OperatorHandlers/TagHandler.h"
#include "nu1/nu1_nBodyTerm.hpp"
#include "dmrg/models/model_helper.hpp"
#include "prebo_TagGenerator.hpp"
#include "integral_interface.h"

#ifdef DMRG_PREBO

namespace prebo {

    /**
     * @class TermGenerator prebo_TermGenerator.hpp
     * General info: the ordering of the lattice for Hamiltonian and particle RDM is handled by the nu1_nBodyTerm class.
     * For the orbital-RDMs, it is handled here.
     *
     * @brief This class handles the correct generation of all terms.
     * @tparam Matrix numeric matrix type
     * @tparam N integer associated with the NU1 symmetry class
     */
    template<class Matrix, int N>
    class TermGenerator {
        // Types definition
        using NU1 = NU1_template<N>;
        using base = model_impl<Matrix, NU1>;
        using tag_type = typename base::tag_type;
        using term_descriptor = typename base::term_descriptor;
        using terms_type = typename std::vector<term_descriptor>;
        using operators_type = typename std::vector<tag_type>;
        using pos_t = typename Lattice::pos_t;
        using part_type = typename Lattice::part_type;
        using positions_type = typename std::vector<pos_t>;
        using value_type = typename Matrix::value_type;
        using symbolic_terms = typename std::vector<std::pair<value_type, std::vector<SymbolicOperator>>>;
        using ValueType = typename Matrix::value_type;

    private:
        // Class members
        bool verbose_, termVerbose_=false;
        std::vector<bool> isFermion;
        std::vector<int>  vec_orbitals;
        std::vector<int>  vec_fer_bos;
        std::vector<pos_t>  inv_order;
        std::shared_ptr<Lattice> ptr_lat;
        std::shared_ptr<TagHandler<Matrix, NU1>> ptr_tag_handler;

    private:
        std::shared_ptr<prebo::TagGenerator<Matrix, N>> ptr_tag_container;
        std::vector<tag_type> fer_ident_tag, fer_filling_tag, fer_create_up_tag, fer_create_down_tag, fer_dest_up_tag,
                fer_dest_down_tag, bos_ident_tag, bos_create_tag, bos_dest_tag;
        std::vector<Index<NU1>> phys_indexes;

    public:

        /** @brief Default constructor */
        TermGenerator() = default;

        /**
         * Constructor. Automatically updates the tag_handler, but the updated phys_indexes have to be retrieved
         * after the constructor was called.
         * @param lat
         * @param ptr_tag_handler_
         */
        TermGenerator(std::shared_ptr<Lattice> &lat, std::shared_ptr<TagHandler<Matrix, NU1>> &ptr_tag_handler_, bool verbose) 
            : ptr_lat(lat), ptr_tag_handler(ptr_tag_handler_), verbose_(verbose)
        {
            ptr_tag_container = std::make_shared<prebo::TagGenerator<Matrix, N>>(lat, ptr_tag_handler);

            // Fermion
            fer_create_up_tag = ptr_tag_container->get_tag_vector(Type::Fermion, OpType::Create, Spin::Up);
            fer_dest_up_tag = ptr_tag_container->get_tag_vector(Type::Fermion, OpType::Annihilate, Spin::Up);
            fer_create_down_tag = ptr_tag_container->get_tag_vector(Type::Fermion, OpType::Create, Spin::Down);
            fer_dest_down_tag = ptr_tag_container->get_tag_vector(Type::Fermion, OpType::Annihilate, Spin::Down);
            fer_filling_tag = ptr_tag_container->get_tag_vector(Type::Fermion, OpType::Filling, Spin::None);
            fer_ident_tag = ptr_tag_container->get_tag_vector(Type::Fermion, OpType::Ident, Spin::None);
            // Bosons
            bos_create_tag = ptr_tag_container->get_tag_vector(Type::Boson, OpType::Create, Spin::Zero);
            bos_dest_tag = ptr_tag_container->get_tag_vector(Type::Boson, OpType::Annihilate, Spin::Zero);
            bos_ident_tag = ptr_tag_container->get_tag_vector(Type::Boson, OpType::Ident, Spin::None);
            // tag handler
            ptr_tag_handler = ptr_tag_container->get_tag_handler();
            phys_indexes = ptr_tag_container->getPhysIndexes();
            //
            isFermion = ptr_lat->template get_prop<std::vector<bool>>("isFermion");
            vec_fer_bos = ptr_lat->template get_prop<std::vector<int>>("vec_fer_bos");
            vec_orbitals = ptr_lat->template get_prop<std::vector<int>>("vec_orbitals");
            inv_order = ptr_lat->template get_prop<std::vector<pos_t>>("inv_order");
        }

        /**
         * This method generates the terms of the pre-BO Hamiltonian. It is called inside the model.
         * @param integrals
         * @return
         */
        auto generate_Hamiltonian(const std::pair<std::vector<chem::index_type<chem::Hamiltonian::PreBO>>,
                                  const std::vector<value_type>>& integrals) -> terms_type
        {
            terms_type hamiltonian;
            
            if (verbose_) {
                std::cout << "======================================================" << std::endl;
                std::cout << "Starting to construct the Hamiltonian..." << std::endl;
            }
            std::vector<std::pair<part_type, pos_t>> nbody_term;

            auto getTermOrder = [] (const chem::index_type<chem::Hamiltonian::PreBO>& indices) {
                auto ans = std::count(indices.begin(), indices.end(), -1);
                if (ans == 8)
                    return 0;
                else if (ans == 4)
                    return 1;
                else if (ans == 0)
                    return 2;
                else {
                    throw std::runtime_error("Term has invlaid order");
                }
            };

            unsigned termOrder=0;
            for (auto line=0; line<integrals.first.size(); ++line) {
                /// -- Initialization --
                nbody_term.clear();
                // Extract the order of the term:
                termOrder = getTermOrder(integrals.first[line]);
                // Nuclear repulsion:
                const auto& indices = integrals.first[line];
                const auto& integral = integrals.second[line];
                if (termOrder==0) {
                    positions_type pos{0};
                    operators_type ops;
                    if (isFermion[ptr_lat->template get_prop<int>("type", pos)])
                        ops.push_back(fer_ident_tag[ptr_lat->template get_prop<int>("type", pos)]);
                    else
                        ops.push_back(bos_ident_tag[ptr_lat->template get_prop<int>("type", pos)]);
                    modelHelper<Matrix, NU1>::add_term(pos, ops, integral, ptr_tag_handler, hamiltonian);
                    continue;
                }
                // 1-body and 2-body terms
                for (auto i=0; i<termOrder*4; i+=2) {
                    nbody_term.push_back(std::make_pair(indices[i], indices[i+1]));
                }
                // Assert tat for a one-body term, the particle types are equal
                // and that for a two-body term, the rule a(i)+ a(m)+ a(m) a(i) is
                // followed.
                assert((termOrder == 1 && nbody_term[0].first == nbody_term[1].first) ||
                       (termOrder == 2 && nbody_term[0].first == nbody_term[3].first &&
                        nbody_term[1].first == nbody_term[2].first));
                // Assert that the particle type index is less than the number of particle
                // types and that the orbital index is less than the number of orbitals of
                // the given type

                #ifndef NDEBUG
                for (auto const& iter : nbody_term) {
                    //assert(iter.first < num_particle_types);
                    assert(iter.second < vec_orbitals.at(iter.first));
                }
                #endif

                // The nbody term is now ready to be transformed to the second quantized
                // operators with proper symmetry and spin configurations.
                NBodyTerm nBodyTerm = NBodyTerm(nbody_term, isFermion, vec_orbitals, inv_order);

                std::vector<std::vector<SymbolicOperator>> vec_SymOpStr = nBodyTerm.getVecSymOpStr();

                for (auto const& OpStr : vec_SymOpStr) {
                    positions_type pos;
                    operators_type ops;
                    Symbols2Tag(pos, ops, OpStr);
                    modelHelper<Matrix, NU1>::add_term(pos, ops, integral, ptr_tag_handler, hamiltonian);
                }
            }
            //
            // And finally ... tidy up the mess.
            //
            return clean_hamiltonian(hamiltonian);
        }

        // +-------------------+
        // | CLEAN_HAMILTONIAN |
        // +-------------------+
        /**! \brief
         * The purpose of this function is to remove all duplicates of terms that
         * appear more than one time in the Hamiltonian and add the coefficient if
         * they belong to the same combination of operators.
         *     => This drastically reduces the bond dimensions.
         */
        auto clean_hamiltonian(const terms_type& terms_temp) -> terms_type {

            terms_type result;

            if (verbose_) {
                std::cout << "======================================================" << std::endl;
                std::cout << "Cleaning the Hamiltonian..." << std::endl;
            }

            std::unordered_map<term_descriptor, value_type, term_descriptor_hasher> count_map;

            for (auto const& term1 : terms_temp) {
                // If key not found in map iterator to end is returned
                if (count_map.find(term1) == count_map.end()) {
                    count_map[term1] = term1.coeff;
                }
                    // If key found then iterator to that key is returned
                else {
                    count_map[term1] += term1.coeff;
                }
            }

            if (verbose_) {
                std::cout << "======================================================" << std::endl;
                std::cout << "The final size of the Hamiltonian is:" << std::endl << std::endl;
                std::cout << count_map.size() << std::endl << std::endl;
                std::cout << "======================================================" << std::endl;
            }

            result.reserve(count_map.size());
            for (const auto& iter : count_map) {
                auto term = iter.first;
                term.coeff = iter.second;
                result.push_back(std::move(term));
            }

            return result;

        } // clean_hamiltonian

        /**
         * @breif hasher for the clean hamiltonian method.
         */
        struct term_descriptor_hasher {
            std::size_t operator()(const term_descriptor& key) const {
                using boost::hash_combine;
                using boost::hash_value;

                // Start with a hash value of 0    .
                std::size_t seed = 0;

                // Modify 'seed' by XORing and bit-shifting in
                // one member of 'Key' after the other:
                for (unsigned int i = 0; i < key.size(); i++) {
                    hash_combine(seed, key.position(i));
                    hash_combine(seed, key.operator_tag(i));
                }
                // Return the result.
                return seed;
            }
        };


        /**
         * @brief This method takes the symbolic operator string and populates operators and positions vector.
         * --> This method is the link between the symbolic algebra routine and the terms used for the MPO
         *     construction.
         * @param pos
         * @param ops
         * @param SymOpStr
         */
        void Symbols2Tag(positions_type& pos, operators_type& ops, const std::vector<SymbolicOperator>& SymOpStr) {
            for (auto const& SymOp : SymOpStr) {
                pos.push_back(SymOp.getSite());
                if (SymOp.getOpType() == OpType::Filling) {
                    ops.push_back(fer_filling_tag[vec_fer_bos[SymOp.getPartType()]]);
                }
                else if (SymOp.getOpType() == OpType::Ident) {
                    if (isFermion[SymOp.getPartType()])
                        ops.push_back(fer_ident_tag[vec_fer_bos[SymOp.getPartType()]]);
                    else
                        ops.push_back(bos_ident_tag[vec_fer_bos[SymOp.getPartType()]]);
                }
                else if (SymOp.getOpType() == OpType::Create) {
                    if (SymOp.getSpin() == Spin::Down)
                        ops.push_back(fer_create_down_tag[vec_fer_bos[SymOp.getPartType()]]);
                    else if (SymOp.getSpin() == Spin::Up)
                        ops.push_back(fer_create_up_tag[vec_fer_bos[SymOp.getPartType()]]);
                    else if (SymOp.getSpin() == Spin::Zero)
                        ops.push_back(bos_create_tag[vec_fer_bos[SymOp.getPartType()]]);
                }
                else if (SymOp.getOpType() == OpType::Annihilate) {
                    if (SymOp.getSpin() == Spin::Down)
                        ops.push_back(fer_dest_down_tag[vec_fer_bos[SymOp.getPartType()]]);
                    else if (SymOp.getSpin() == Spin::Up)
                        ops.push_back(fer_dest_up_tag[vec_fer_bos[SymOp.getPartType()]]);
                    else if (SymOp.getSpin() == Spin::Zero)
                        ops.push_back(bos_dest_tag[vec_fer_bos[SymOp.getPartType()]]);
                }
            }
        }


        /**
         * @brief Returns the `terms` element that contains the RDM terms for a given particle type and two sites
         * This function relies on the symbolic algebra routine but does not expose it.
         * @param i --> particle type
         * @param mu --> site index
         * @param nu --> site index
         * @return terms
         */
        auto generate_terms1RDM(const int& i, const int& mu, const int& nu) -> terms_type {
            //
            // < Psi | a+^i_mu a^j_nu | Psi> => Generate operator string for particle type i and sites mu and nu
            //
            terms_type res;
            // the first element corresponds to the particle type, the second one is
            // the orbital index.
            std::vector<std::pair<part_type, pos_t>> nbody_term;
            nbody_term.push_back(std::make_pair(i, mu));
            nbody_term.push_back(std::make_pair(i, nu));

            // Assert that the particle type index is less than the number of particle
            // types and that the orbital index is less than the number of orbitals of
            // the given type
            #ifndef NDEBUG
            for (auto const& iter : nbody_term) {
                assert(iter.first < vec_orbitals.size());
                assert(iter.second < vec_orbitals.at(iter.first));
            }
            #endif
            // The nbody term is now ready to be transformed to the second quantized
            // operators with proper symmetry and spin configurations.
            NBodyTerm nBodyTerm = NBodyTerm(nbody_term, isFermion, vec_orbitals, inv_order);

            std::vector<std::vector<SymbolicOperator>> vec_SymOpStr = nBodyTerm.getVecSymOpStr();

            for (auto const& OpStr : vec_SymOpStr) {
                positions_type pos;
                operators_type ops;
                Symbols2Tag(pos, ops, OpStr);
                value_type scaling = 1.;
                std::pair<term_descriptor, bool> ret = modelHelper<Matrix, NU1>::arrange_operators(pos, ops, scaling, ptr_tag_handler);
                if (!ret.second) {
                    auto term = ret.first;
                    term.coeff = scaling;
                    res.push_back(term);
                    if (this->termVerbose_)
                        std::cout << term << std::endl;
                }
            }
            return res;
        }


        /**
         * Multiplies two `symbolic_terms` and returns their product.
         * @param lhs
         * @param rhs
         * @return product
         */
        auto multiply(const symbolic_terms & lhs, const symbolic_terms & rhs) -> symbolic_terms {
            symbolic_terms res;
            res.reserve(lhs.size()*rhs.size());
            // Multiply the two strings of symbolic operators.
            // Loop over terms on the left hand side and multiply for each term with every other term on the right hand side
            for (auto const & itlhs: lhs)
                for (auto const & itrhs: rhs) {
                    value_type val=itlhs.first*itrhs.first;
                    std::vector<SymbolicOperator> tmp_str = itlhs.second;
                    // Concatenate two strings of operators
                    tmp_str.insert(tmp_str.end(), itrhs.second.begin(), itrhs.second.end());
                    res.push_back(std::make_pair(val, tmp_str));
                }
            return res;
        }


        /**
         * @brief Generates the `symbolic_terms` of the transition operator for the evaluation of the orbital-RDMs.
         * Note 1: that there are 16 transition operators for fermions.
         * Note 2: Bosons are not implemented, yet.
         * @param i --> particle type
         * @param mu --> site index
         * @param which  --> which transition operator {1, 2, ..., 16}
         * @return symbolic_terms
         */
        auto generate_transitionOps(const part_type& i, const pos_t& mu, const int& which) -> symbolic_terms {

            symbolic_terms transition_op;
            std::vector<SymbolicOperator> SymOp_temp;
            positions_type pos{mu};
            operators_type ops;
            value_type scaling = 1.;

            if (isFermion[i]) {
                switch (which) {
                    case 1:
                        //
                        // Operator 1: 1 - n_up - n_down + n_up n_down
                        //
                        // 1
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Ident, i));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // + n_up n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 2:
                        //
                        // Operator 2: c_down  - n_up c_down
                        //
                        // F c_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_up c_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 3:
                        //
                        // Operator 3: c_up - n_down c_up
                        //
                        // c_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_down c_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 4:
                        //
                        // Operator 4:  c_down c_up
                        //
                        // c_down c_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 5:
                        //
                        // Operator 5: c+_down  - n_up c+_down
                        //
                        // c+_down F
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_up c+_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 6:
                        //
                        // Operator 6: n_down - n_up n_down
                        //
                        // n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_up n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 7:
                        //
                        // Operator 7:  c+_down c_up
                        //
                        transition_op.resize(0);
                        // c+_down c_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 8:
                        //
                        // Operator 8: - n_down c_up
                        //
                        transition_op.resize(0);
                        // - n_down c_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 9:
                        //
                        // Operator 9: c+_up - n_down c+_up
                        //
                        // c+_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_down c+_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 10:
                        //
                        // Operator 10:  c_down c+_up
                        //
                        // c_down c+_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 11:
                        //
                        // Operator 11: n_up - n_up n_down
                        //
                        // n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        // - n_up n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 12:
                        //
                        // Operator 12: n_up c_down
                        //
                        // n_up c_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 13:
                        //
                        // Operator 13:  c+_down c+_up
                        //
                        // c+_down c+_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 14:
                        //
                        // Operator 14: - n_down c+_up
                        //
                        // - n_down c+_up
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        scaling = -1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 15:
                        //
                        // Operator 15: n_up c+_down
                        //
                        // n_up c+_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                    case 16:
                        //
                        // Operator 16: n_up n_down
                        //
                        // n_up n_down
                        SymOp_temp.clear();
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Up));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Create, i, Spin::Down));
                        SymOp_temp.push_back(SymbolicOperator(mu, OpType::Annihilate, i, Spin::Down));
                        scaling = 1.;
                        transition_op.push_back(std::make_pair(scaling, SymOp_temp));
                        break;
                }
            }
            else {
                std::cout << "Bosonic Transition Operators not implemented, yet." << std::endl;
            }
            return transition_op;
        }



        /**
         * Generates the symbolic transition operators for the two-orbital RDM and handles the JW transformation,
         * and the two cases for the same and different particle types.
         * @param i
         * @param j
         * @param mu
         * @param nu
         * @param which1
         * @param which2
         * @return symbolic_terms
         */
        auto generate_transitionOps(const part_type& i, const part_type& j, const pos_t& mu, const pos_t& nu, const int& which1,
                               const int& which2) -> symbolic_terms {

            SymbolicJordanWigner JW;

            symbolic_terms transition_ops1 =
                    generate_transitionOps(i, mu, which1);
            symbolic_terms transition_ops2 =
                    generate_transitionOps(j, nu, which2);

            if (i!=j) {
                for (size_t it=0; it < transition_ops1.size(); it++) {
                    JW = SymbolicJordanWigner(transition_ops1.at(it).second);
                    transition_ops1.at(it).second = JW.getSymOpStr();
                }
                for (size_t it=0; it < transition_ops2.size(); it++) {
                    JW = SymbolicJordanWigner(transition_ops2.at(it).second);
                    transition_ops2.at(it).second = JW.getSymOpStr();
                }
            }
            symbolic_terms transition_ops_res = multiply(transition_ops1, transition_ops2);
            if (i==j) {
                for (size_t it=0; it < transition_ops_res.size(); it++) {
                    JW = SymbolicJordanWigner(transition_ops_res.at(it).second);
                    transition_ops_res.at(it).second = JW.getSymOpStr();
                }
            }
            return transition_ops_res;

        }

        /**
         * @brief Generates the actual `terms` of the one-orbital RDM. Takes care of the orbital ordering.
         * This function relies on the symbolic algebra routine but does not expose it.
         * ATTENTION: Insert already ordered site index!!
         * @param i --> particle type
         * @param mu --> !unordered! site index belonging to i
         * @param which
         * @return terms
         */
        auto generate_termsTransitionOp(const int& i, const int& mu, const int& which) -> terms_type {

            terms_type res;

            symbolic_terms transition_ops =
                    generate_transitionOps(i, mu, which);

            SymbolicJordanWigner JW;
            for (size_t it=0; it < transition_ops.size(); it++) {
                JW = SymbolicJordanWigner(transition_ops.at(it).second);
                transition_ops.at(it).second = JW.getSymOpStr();
            }

            for (auto const& transOp : transition_ops ) {
                positions_type pos;
                operators_type ops;
                Symbols2Tag(pos, ops, transOp.second);
                value_type scaling = transOp.first;
                std::pair<term_descriptor, bool> ret = modelHelper<Matrix, NU1>::arrange_operators(pos, ops, scaling, ptr_tag_handler);
                if (!ret.second) {
                    auto term = ret.first;
                    term.coeff = scaling;
                    res.push_back(term);
                    if (termVerbose_) {
                        std::cout << term << std::endl;
                        for (int i=0; i<term.size(); ++i)
                            std::cout << ptr_tag_handler->get_op(term.operator_tag(i)) << std::endl;
                    }
                }
            }
            return res;
        }


        /**
         * @brief Generates the actual `terms` of the two-orbital RDM. Takes care of the orbital ordering.
         * This function relies on the symbolic algebra routine but does not expose it.
         * ATTENTION: Insert already ordered site index!!
         * @param i --> particle type
         * @param j --> particle type
         * @param mu --> !unordered! site index belonging to i
         * @param nu --> !unordered! site index belonging to j
         * @param which1
         * @param which2
         * @return terms
         */
        auto generate_termsTransitionOp(const int& i, const int& j, const int& mu, const int& nu, const int& which1,
                                        const int& which2) -> terms_type {

            terms_type res;

            symbolic_terms transition_ops =
                    generate_transitionOps(i, j, mu, nu, which1, which2);

            for (auto const& transOp : transition_ops ) {
                positions_type pos;
                operators_type ops;
                Symbols2Tag(pos, ops, transOp.second);
                value_type scaling = transOp.first;
                std::pair<term_descriptor, bool> ret = modelHelper<Matrix, NU1>::arrange_operators(pos, ops, scaling, ptr_tag_handler);
                if (!ret.second) {
                    auto term = ret.first;
                    term.coeff = scaling;
                    res.push_back(term);
                    if (termVerbose_) {
                        std::cout << term << std::endl;
                    }
                }
            }
            return res;
        }

    public:
        /** @brief Getter for the tag container */
        const auto& getTagContainer() const {
            return ptr_tag_container;
        }

        /** @brief Getter for the physical indices */
        const std::vector<Index<NU1>>& getPhysIndexes() const {
            return phys_indexes;
        }

        /** @brief getter for the tag Handler */
        const std::shared_ptr<TagHandler<Matrix, NU1>>& getTagHandler() const {
            return ptr_tag_handler;
        }
    };

} // namespace prebo

#endif // DMRG_PREBO

#endif // MAQUIS_DMRG_PREBO_TERMGENERATOR_HPP
