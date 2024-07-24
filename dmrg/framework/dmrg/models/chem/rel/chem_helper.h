/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef REL_QC_CHEM_DETAIL_H
#define REL_QC_CHEM_DETAIL_H

#include "dmrg/models/chem/parse_integrals.h"

namespace chem {
namespace detail {

    template <typename M, class S>
    class RelChemHelper
    {
    public:
        typedef typename M::value_type value_type;
        typedef ::term_descriptor<value_type> term_descriptor;
        typedef typename TagHandler<M, S>::tag_type tag_type;
        typedef Lattice::pos_t pos_t;
        using InputType = std::complex<double>;

        RelChemHelper(BaseParameters & parms, Lattice const & lat_,
                   std::vector<tag_type> const & ident_, std::vector<tag_type> const & fill_,
                   std::shared_ptr<TagHandler<M, S> > tag_handler_)
            : lat(lat_), ident(ident_), fill(fill_), tag_handler(tag_handler_)
        {
			boost::tie(idx_, matrix_elements) = parse_integrals<InputType, S>(parms, lat);

            for (std::size_t m=0; m < matrix_elements.size(); ++m) {
                IndexTuple pos;
				std::copy(idx_.row(m).first, idx_.row(m).second, pos.begin());
                coefficients[pos] = matrix_elements[m];
            }
        }

        std::vector<value_type> & getMatrixElements() { return matrix_elements; }

        int idx(int m, int pos) const {
            return idx_(m,pos);
        }

        void commit_terms(std::vector<term_descriptor> & tagterms) {
            for (typename std::map<IndexTuple, term_descriptor>::const_iterator it = two_terms.begin();
                    it != two_terms.end(); ++it)
                tagterms.push_back(it->second);

            for (typename std::map<SixTuple, term_descriptor>::const_iterator it = three_terms.begin();
                    it != three_terms.end(); ++it)
                tagterms.push_back(it->second);

            for (typename std::map<EightTuple, term_descriptor>::const_iterator it = four_terms.begin();
                    it != four_terms.end(); ++it)
                tagterms.push_back(it->second);
        }

        void add_term(std::vector<term_descriptor> & tagterms,
                      value_type scale, int p1, int p2, std::vector<tag_type> const & op_1, std::vector<tag_type> const & op_2) {

            term_descriptor
            term = TermMaker<M, S>::two_term(false, ident, scale, p1, p2, op_1, op_2, tag_handler, lat);
            IndexTuple id(p1, p2, op_1[lat.get_prop<typename S::subcharge>("type", p1)],
                                  op_2[lat.get_prop<typename S::subcharge>("type", p2)]);
            if (two_terms.count(id) == 0) {
                two_terms[id] = term;
            }
            else
                two_terms[id].coeff += term.coeff;
        }

        void add_term(std::vector<term_descriptor> & tagterms,
                      value_type scale, int s, int p1, int p2,
                      std::vector<tag_type> const & op_i, std::vector<tag_type> const & op_k,
                      std::vector<tag_type> const & op_l, std::vector<tag_type> const & op_j)
        {
            term_descriptor
            term = TermMaker<M, S>::three_term(ident, fill, scale, s, p1, p2, op_i, op_k, op_l, op_j, tag_handler, lat);
            SixTuple id(term.position(0), term.position(1), term.position(2),
                        term.operator_tag(0), term.operator_tag(1), term.operator_tag(2));
            if (three_terms.count(id) == 0) {
                three_terms[id] = term;
            }
            else
                three_terms[id].coeff += term.coeff;
        }

        void add_term(std::vector<term_descriptor> & tagterms, value_type scale,
                      int i, int k, int l, int j,
                      std::vector<tag_type> const & op_i, std::vector<tag_type> const & op_k,
                      std::vector<tag_type> const & op_l, std::vector<tag_type> const & op_j)
        {
			term_descriptor
			term = TermMaker<M, S>::four_term(ident, fill, scale, i, k, l, j, op_i, op_k, op_l, op_j, tag_handler, lat);
			if (i<k) std::swap(i,k);
			if (j<l) std::swap(j,l);
			EightTuple id(IndexTuple(i,k,l,j),IndexTuple(op_i[lat.get_prop<typename S::subcharge>("type",i)],
                                                         op_k[lat.get_prop<typename S::subcharge>("type",k)],
                                                         op_l[lat.get_prop<typename S::subcharge>("type",l)],
                                                         op_j[lat.get_prop<typename S::subcharge>("type",j)]));
			if (four_terms.count(id) == 0) {
				four_terms[id] = term;
			}
			else
				four_terms[id].coeff += term.coeff;
        }

    private:

        std::vector<tag_type> const & ident;
        std::vector<tag_type> const & fill;
        std::shared_ptr<TagHandler<M, S> > tag_handler;
        Lattice const & lat;
        std::vector<value_type> matrix_elements;
        alps::numeric::matrix<Lattice::pos_t> idx_;
        std::map<IndexTuple, InputType> coefficients;
        std::map<EightTuple, term_descriptor> four_terms;
        std::map<SixTuple, term_descriptor> three_terms;
        std::map<IndexTuple, term_descriptor> two_terms;
    };
}
}
#endif
