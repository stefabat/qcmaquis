/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2011 by Bela Bauer <bauerb@phys.ethz.ch>
 *               2011-2013    Michele Dolfi <dolfim@phys.ethz.ch>
 *               2014-2014    Sebastian Keller <sebkelle@phys.ethz.ch>
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

#ifndef MEASUREMENTS_TAGGED_NRANKRDM_H
#define MEASUREMENTS_TAGGED_NRANKRDM_H

#include <algorithm>
#include <functional>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/filesystem.hpp>

#include "dmrg/models/term_descriptor.h"
#include "dmrg/models/measurement.h"

namespace measurements {
    
    template <class Matrix, class SymmGroup>
    class TaggedNRankRDM : public measurement<Matrix, SymmGroup> {

        typedef measurement<Matrix, SymmGroup> base;

        typedef typename Model<Matrix, SymmGroup>::term_descriptor term_descriptor;

        typedef Lattice::pos_t pos_t;
        typedef std::vector<pos_t> positions_type;

        typedef typename base::op_t op_t;
        typedef typename OPTable<Matrix, SymmGroup>::tag_type tag_type;
        typedef std::vector<tag_type> tag_vec;
        typedef std::vector<tag_vec> bond_term;
    
    public:
        TaggedNRankRDM(std::string const& name_, const Lattice & lat,
                       boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler_,
                       tag_vec const & identities_, tag_vec const & fillings_, std::vector<bond_term> const& ops_,
                       bool half_only_, bool nearest_neighbors_only,
                       positions_type const& positions_ = positions_type(),
                       std::string const& ckp_ = std::string(""))
        : base(name_)
        , lattice(lat)
        , tag_handler(tag_handler_)
        , positions_first(positions_)
        , identities(identities_)
        , fillings(fillings_)
        , operator_terms(ops_)
        , half_only(half_only_)
        , is_nn(nearest_neighbors_only)
        , bra_ckp(ckp_)
        {
            pos_t extent = operator_terms.size() > 2 ? lattice.size() : lattice.size()-1;
            if (positions_first.size() == 0)
                std::copy(boost::counting_iterator<pos_t>(0), boost::counting_iterator<pos_t>(extent),
                          back_inserter(positions_first));
            
            //this->cast_to_real = is_hermitian_meas(ops[0]);
            this->cast_to_real = false;
        }
        
        void evaluate(MPS<Matrix, SymmGroup> const& ket_mps, boost::optional<reduced_mps<Matrix, SymmGroup> const&> rmps = boost::none)
        {
            this->vector_results.clear();
            this->labels.clear();

            MPS<Matrix, SymmGroup> bra_mps;
            if (bra_ckp != "") {
                if(boost::filesystem::exists(bra_ckp))
                    load(bra_ckp, bra_mps);
                else
                    throw std::runtime_error("The bra checkpoint file " + bra_ckp + " was not found\n");
            }

            //if (ops[0].size() == 2)
            //    measure_correlation(bra_mps, ket_mps, ops);
            //else if (ops[0].size() == 4)
                measure_2rdm(bra_mps, ket_mps);
            //else
            //    throw std::runtime_error("correlation measurements at the moment supported with 2 and 4 operators");
        }
        
    protected:
        typedef boost::shared_ptr<generate_mpo::CorrMakerBase<Matrix, SymmGroup> > maker_ptr;

        measurement<Matrix, SymmGroup>* do_clone() const
        {
            return new TaggedNRankRDM(*this);
        }
        
        //void measure_correlation(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
        //                         MPS<Matrix, SymmGroup> const & ket_mps,
        //                         std::vector<bond_element> const & ops,
        //                         std::vector<pos_t> const & order = std::vector<pos_t>())
        //{
        //    // Test if a separate bra state has been specified
        //    bool bra_neq_ket = (dummy_bra_mps.length() > 0);
        //    MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

        //    // TODO: test with ambient in due time
        //    #ifdef MAQUIS_OPENMP
        //    #pragma omp parallel for
        //    #endif
        //    for (std::size_t i = 0; i < positions_first.size(); ++i) {
        //        pos_t p = positions_first[i];
        //        #ifndef NDEBUG
        //        maquis::cout << "  site " << p << std::endl;
        //        #endif
        //        
        //        maker_ptr dcorr(new generate_mpo::BgCorrMaker<Matrix, SymmGroup>(lattice, identities, fillings,
        //                                                                         ops[0], std::vector<pos_t>(1, p)));
        //        // measure
        //        MPO<Matrix, SymmGroup> mpo = dcorr->create_mpo();
        //        std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct = multi_expval(bra_mps, ket_mps, mpo);
        //        
        //        std::vector<std::vector<pos_t> > num_labels = dcorr->numeric_labels();
        //        std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
        //                                    ? detail::resort_labels(num_labels, order, is_nn) : num_labels );
        //        // save results and labels
        //        #ifdef MAQUIS_OPENMP
        //        #pragma omp critical
        //        #endif
        //        {
        //        this->vector_results.reserve(this->vector_results.size() + dct.size());
        //        std::copy(dct.begin(), dct.end(), std::back_inserter(this->vector_results));

        //        this->labels.reserve(this->labels.size() + dct.size());
        //        std::copy(lbt.begin(), lbt.end(), std::back_inserter(this->labels));
        //        }
        //    }
        //}

        void measure_2rdm(MPS<Matrix, SymmGroup> const & dummy_bra_mps,
                          MPS<Matrix, SymmGroup> const & ket_mps,
                          std::vector<pos_t> const & order = std::vector<pos_t>())
        {
            // Test if a separate bra state has been specified
            bool bra_neq_ket = (dummy_bra_mps.length() > 0);
            MPS<Matrix, SymmGroup> const & bra_mps = (bra_neq_ket) ? dummy_bra_mps : ket_mps;

            // TODO: test with ambient in due time
            #ifdef MAQUIS_OPENMP
            #pragma omp parallel for collapse(1)
            #endif
            for (pos_t p1 = 0; p1 < lattice.size(); ++p1)
            for (pos_t p2 = 0; p2 < lattice.size(); ++p2)
            {
                // Permutation symmetry for bra == ket: ijkl == jilk == klji == lkji
                pos_t subref = std::min(p1, p2);

                // if bra != ket, pertmutation symmetry is only ijkl == jilk
                if (bra_neq_ket)
                    pos_t subref = 0;

                for (pos_t p3 = subref; p3 < lattice.size(); ++p3)
                { 
                    std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct;
                    std::vector<std::vector<pos_t> > num_labels;

                    for (pos_t p4 = p3; p4 < lattice.size(); ++p4)
                    { 
                        pos_t pos_[4] = {p1, p2, p3, p4};
                        std::vector<pos_t> positions(pos_, pos_ + 4);

                        // Loop over operator terms that are measured synchronously and added together
                        // Used e.g. for the four spin combos of the 2-RDM
                        typename MPS<Matrix, SymmGroup>::scalar_type value = 0;
                        for (std::size_t synop = 0; synop < operator_terms.size(); ++synop) {

                            tag_vec operators(4);
                            operators[0] = operator_terms[synop][0][lattice.get_prop<typename SymmGroup::subcharge>("type", p1)];
                            operators[1] = operator_terms[synop][1][lattice.get_prop<typename SymmGroup::subcharge>("type", p2)];
                            operators[2] = operator_terms[synop][2][lattice.get_prop<typename SymmGroup::subcharge>("type", p3)];
                            operators[3] = operator_terms[synop][3][lattice.get_prop<typename SymmGroup::subcharge>("type", p4)];
                            
                            std::copy(operator_terms[synop][0].begin(), operator_terms[synop][0].end(), std::ostream_iterator<unsigned>(std::cout, " "));
                            maquis::cout << std::endl;
                            std::copy(operator_terms[synop][1].begin(), operator_terms[synop][1].end(), std::ostream_iterator<unsigned>(std::cout, " "));
                            maquis::cout << std::endl;
                            std::copy(operator_terms[synop][2].begin(), operator_terms[synop][2].end(), std::ostream_iterator<unsigned>(std::cout, " "));
                            maquis::cout << std::endl;
                            std::copy(operator_terms[synop][3].begin(), operator_terms[synop][3].end(), std::ostream_iterator<unsigned>(std::cout, " "));
                            maquis::cout << std::endl;

                            std::copy(operators.begin(), operators.end(), std::ostream_iterator<unsigned>(std::cout, " "));
                            maquis::cout << std::endl;

                            term_descriptor term = generate_mpo::arrange_operators(tag_handler, positions, operators);
                            MPO<Matrix, SymmGroup> mpo = generate_mpo::make_1D_mpo(term, identities, fillings, tag_handler, lattice);
                            //value += expval(bra_mps, ket_mps, mpo);
                            value += expval(ket_mps, mpo);
                            #ifdef MAQUIS_OPENMP
                            #pragma omp critical
                            #endif
                            {
                            maquis::cout << p1 << p2 << p3 << p4 << " " << value << std::endl;
                            maquis::cout << term << std::endl;
                            }
                        }

                        dct.push_back(value);
                        pos_t label[4] = {p1, p2, p3, p4};
                        num_labels.push_back(std::vector<pos_t>(label, label + 4));
                    }

                    std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                                                ? detail::resort_labels(num_labels, order, is_nn) : num_labels );

                    // save results and labels
                    #ifdef MAQUIS_OPENMP
                    #pragma omp critical
                    #endif
                    {
                        this->vector_results.reserve(this->vector_results.size() + dct.size());
                        std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));

                        this->labels.reserve(this->labels.size() + dct.size());
                        std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                    }

                    //maker_ptr dcorr(new generate_mpo::BgCorrMaker<Matrix, SymmGroup>(lattice, identities, fillings, ops[0], ref, true));
                    //MPO<Matrix, SymmGroup> mpo = dcorr->create_mpo();
                    //std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> dct = multi_expval(bra_mps, ket_mps, mpo);

                    //// Loop over operator terms that are measured synchronously and added together
                    //// Used e.g. for the four spin combos of the 2-RDM
                    //for (std::size_t synop = 1; synop < ops.size(); ++synop) {
                    //    maker_ptr syndcorr(new generate_mpo::BgCorrMaker<Matrix, SymmGroup>(lattice, identities, fillings, ops[synop], ref, true));

                    //    // measure
                    //    MPO<Matrix, SymmGroup> synmpo = syndcorr->create_mpo();
                    //    std::vector<typename MPS<Matrix, SymmGroup>::scalar_type> syndct = multi_expval(bra_mps, ket_mps, synmpo);

                    //    // add synchronous terms
                    //    std::transform(syndct.begin(), syndct.end(), dct.begin(), dct.begin(),
                    //                   std::plus<typename MPS<Matrix, SymmGroup>::scalar_type>());
                    //}
                    //
                    //std::vector<std::vector<pos_t> > num_labels = dcorr->numeric_labels();
                    //std::vector<std::string> lbt = label_strings(lattice,  (order.size() > 0)
                    //                            ? detail::resort_labels(num_labels, order, is_nn) : num_labels );
                    //// save results and labels
                    //#ifdef MAQUIS_OPENMP
                    //#pragma omp critical
                    //#endif
                    //{
                    //this->vector_results.reserve(this->vector_results.size() + dct.size());
                    //std::copy(dct.rbegin(), dct.rend(), std::back_inserter(this->vector_results));

                    //this->labels.reserve(this->labels.size() + dct.size());
                    //std::copy(lbt.rbegin(), lbt.rend(), std::back_inserter(this->labels));
                    //}
                }
            }
        }
        
    private:
        Lattice lattice;
        boost::shared_ptr<TagHandler<Matrix, SymmGroup> > tag_handler;
        positions_type positions_first;
        tag_vec identities, fillings;
        std::vector<bond_term> operator_terms;
        bool half_only, is_nn;

        std::string bra_ckp;
    };
}

#endif
