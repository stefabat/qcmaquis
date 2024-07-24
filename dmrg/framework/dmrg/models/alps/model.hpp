/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef APP_ALPS_MODEL_H
#define APP_ALPS_MODEL_H

#include "dmrg/models/model.h"
#include "dmrg/models/measurements.h"
#include "dmrg/models/alps/lattice.hpp"

#include <alps/parameter.h>
#include <alps/lattice.h>
#include <alps/model.h>

#undef tolower
#undef toupper
#include <boost/tokenizer.hpp>
#include <regex>
#include <boost/container/flat_map.hpp>

#include "symm_handler.hpp"

namespace detail {
    inline alps::graph_helper<> const& get_graph(Lattice const& lat_)
    {
        alps_lattice const* alattice = static_cast<alps_lattice const*>(lat_.impl().get());
        return alattice->alps_graph();
    }
}

template <class I>
bool safe_is_fermionic(alps::SiteBasisDescriptor<I> const& b, alps::SiteOperator const& op)
{
    using boost::bind;
    std::set<std::string> operator_names = op.operator_names();
    return std::count_if(operator_names.begin(), operator_names.end(), boost::bind(&alps::SiteBasisDescriptor<I>::is_fermionic, b, _1)) % 2;
}


template <class Matrix, class SymmGroup>
class ALPSModel : public model_impl<Matrix, SymmGroup>
{
    typedef model_impl<Matrix, SymmGroup> base;

    typedef alps::SiteOperator SiteOperator;
    typedef alps::BondOperator BondOperator;

    typedef typename Matrix::value_type value_type;
    typedef typename maquis::traits::scalar_type<Matrix>::type scalar_type;
    typedef boost::multi_array<value_type,2> alps_matrix;
    typedef std::map<std::string, int> qn_map_type;

    typedef short I;
    typedef alps::graph_helper<> graph_type;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;


public:
    typedef typename base::table_type table_type;
    typedef typename base::table_ptr table_ptr;
    typedef typename base::tag_type tag_type;

    typedef typename base::term_descriptor value_term;
    typedef typename alps::expression::Term<value_type> expression_type;
    typedef ::term_descriptor<expression_type> expression_term;
    typedef typename base::terms_type terms_type;
    typedef typename base::op_t op_t;
    typedef typename base::measurements_type measurements_type;
    typedef typename base::initializer_ptr initializer_ptr;

    typedef typename base::size_t size_t;
    typedef Lattice::pos_t pos_t;

    typedef std::vector<std::pair<std::vector<op_t>, bool> > meas_operators_type;

    typedef std::pair<std::string, int> opkey_type;
    typedef std::map<opkey_type, tag_type> opmap_type;
    typedef typename opmap_type::const_iterator opmap_const_iterator;

    typedef typename SymmGroup::charge charge;


    ALPSModel (Lattice const& lattice_, const alps::Parameters& parms_)
    : parms(parms_)
    , raw_lattice(lattice_)
    , lattice(detail::get_graph(lattice_))
    , model(lattice, parms, true)
    , tag_handler(new table_type())
    {
        parallel::guard::serial guard;

        size_t num_vertex_types = alps::maximum_vertex_type(lattice.graph())+1;
        symm_basis.reserve(num_vertex_types);
        basis_descriptors.reserve(num_vertex_types);
        site_bases.reserve(num_vertex_types);

        /// Parsing conserved quantum numbers
        for (int type=0; type<=alps::maximum_vertex_type(lattice.graph()); ++type) {
            std::set<std::string> type_qn = model.quantum_numbers(type);
            all_qn.insert(type_qn.begin(), type_qn.end());
        }

        if (parms.defined("CONSERVED_QUANTUMNUMBERS")) {
            boost::char_separator<char> sep(" ,");
            std::string qn_string = parms["CONSERVED_QUANTUMNUMBERS"];
            tokenizer qn_tokens(qn_string, sep);
            int n=0;
            for (tokenizer::iterator it=qn_tokens.begin(); it != qn_tokens.end(); it++) {
                if (parms.defined(*it + "_total")) {
                    if (all_qn.find(*it) != all_qn.end())
                        all_conserved_qn.insert( std::make_pair(*it, n++) );
                    else
                        throw std::runtime_error("quantumnumber "+(*it)+" not defined in the model.");
                }
            }
        }

        /// Load all possible basis
        for (int type=0; type<=alps::maximum_vertex_type(lattice.graph()); ++type) {
            basis_descriptors.push_back(model.site_basis(type));
            site_bases.push_back(alps::site_basis<I>(basis_descriptors[type]));
            symm_basis.push_back(symmetric_basis_descriptor<SymmGroup>(basis_descriptors[type], all_conserved_qn));

            op_t ident, fill;
            for (int i=0; i<symm_basis[type].size(); ++i) {
                charge c = symm_basis[type].charge(i);
                size_t bsize = symm_basis[type].block_size(i);
                // maquis::cout << "Inserting " << c << " for " << site_bases[type][i] << std::endl;

                if (!ident.has_block(c, c))
                    ident.insert_block(Matrix::identity_matrix(bsize), c, c);

                int sign = (alps::is_fermionic(basis_descriptors[type], site_bases[type][i])) ? -1 : 1;
                if (!fill.has_block(c, c))
                    fill.insert_block(Matrix::identity_matrix(bsize), c, c);
                fill(symm_basis[type].coords(i), symm_basis[type].coords(i)) = sign;
            }
            operators[opkey_type("ident", type)] = tag_handler->register_op(ident, tag_detail::bosonic);
            operators[opkey_type("fill",  type)] = tag_handler->register_op(fill,  tag_detail::bosonic);
        }


        /// site_term loop with cache to avoid recomputing matrices
        std::vector<std::vector<std::pair<value_type, tag_type> > > site_terms(num_vertex_types);
        for (graph_type::site_iterator it=lattice.sites().first; it!=lattice.sites().second; ++it) {
            int p = lattice.vertex_index(*it);
            int type = lattice.site_type(*it);

            if (lattice.inhomogeneous_sites())
                alps::throw_if_xyz_defined(parms,*it); // check whether x, y, or z is set
            alps::expression::ParameterEvaluator<value_type> coords(coordinate_as_parameter(lattice.graph(), *it));

            if (site_terms[type].size() == 0) {
                typedef std::vector<boost::tuple<alps::expression::Term<value_type>,alps::SiteOperator> > V;
                V  ops = model.site_term(type).template templated_split<value_type>();

                for (int n=0; n<ops.size(); ++n) {
                    SiteOperator op = boost::get<1>(ops[n]);
                    opmap_const_iterator match = operators.find(opkey_type(simplify_name(op), type));
                    if (match == operators.end())
                        match = register_operator(op, type, parms);
                    // site_terms[type].push_back( std::make_pair(boost::get<0>(ops[n]).value(), match->second)  );

                    if (lattice.inhomogeneous_sites())
                        boost::get<0>(ops[n]).partial_evaluate(coords);

                    expression_term term;
                    term.coeff = boost::get<0>(ops[n]);
                    term.is_fermionic = false;
                    term.push_back( boost::make_tuple(p, match->second) );
                    expression_coeff.insert( std::make_pair(term.coeff, value_type()) );
                    expression_terms.push_back(term);
                }
            }

            // All site terms summed into one
//            if (site_terms[type].size() > 0) {
//                opmap_const_iterator match = operators.find(opkey_type("site_terms", type));
//                if (match == operators.end()) {
//                    op_t op_matrix;
//                    for (int n=0; n<site_terms[type].size(); ++n)
//                        op_matrix += site_terms[type][n].first * tag_handler->get_op(site_terms[type][n].second);
//                    tag_type mytag = tag_handler->register_op(op_matrix, tag_detail::bosonic);
//                    boost::tie(match, boost::tuples::ignore) = operators.insert( std::make_pair(opkey_type("site_terms", type), mytag) );
//                }
//
//                term_descriptor term;
//                term.coeff = 1.;
//                term.is_fermionic = false;
//                term.push_back( boost::make_tuple(p, match->second) );
//                this->terms_.push_back(term);
//            }


        }

        /// bond terms loop
        for (graph_type::bond_iterator it=lattice.bonds().first; it!=lattice.bonds().second; ++it) {
            int p_s = lattice.source(*it);
            int p_t = lattice.target(*it);
            int type = lattice.bond_type(*it);
            int type_s = lattice.site_type(lattice.source(*it));
            int type_t = lattice.site_type(lattice.target(*it));

            bool wrap_pbc = boost::get(alps::boundary_crossing_t(), lattice.graph(), *it);

            BondOperator bondop = model.bond_term(type);

            typedef std::vector<boost::tuple<alps::expression::Term<value_type>,alps::SiteOperator,alps::SiteOperator > > V;
            alps::SiteBasisDescriptor<I> const& b1 = basis_descriptors[type_s];
            alps::SiteBasisDescriptor<I> const& b2 = basis_descriptors[type_t];

            if (lattice.inhomogeneous_bonds())
                alps::throw_if_xyz_defined(parms, lattice.graph()); // check whether x, y, or z is set
            alps::expression::ParameterEvaluator<value_type> coords(coordinate_as_parameter(lattice.graph(), *it));

            V  ops = bondop.template templated_split<value_type>(b1,b2);
            for (typename V::iterator tit=ops.begin(); tit!=ops.end();++tit) {
                SiteOperator op1 = boost::get<1>(*tit);
                SiteOperator op2 = boost::get<2>(*tit);

                opmap_const_iterator match1 = operators.find(opkey_type(simplify_name(op1), type_s));
                if (match1 == operators.end())
                    match1 = register_operator(op1, type_s, parms);
                opmap_const_iterator match2 = operators.find(opkey_type(simplify_name(op2), type_t));
                if (match2 == operators.end())
                    match2 = register_operator(op2, type_t, parms);

                bool with_sign = fermionic(b1, op1, b2, op2);

                if (lattice.inhomogeneous_bonds())
                    boost::get<0>(*tit).partial_evaluate(coords);

                expression_term term;
                term.coeff = boost::get<0>(*tit);
                term.is_fermionic = with_sign;

                {
                    tag_type mytag = match1->second;
                    if (with_sign && !wrap_pbc) {
                        // Note inverse notation because of notation in operator.
                        std::pair<tag_type, value_type> ptag = tag_handler->get_product_tag(operators[opkey_type("fill",type_s)],
                                                                                            mytag);
                        mytag = ptag.first;
                        term.coeff *= ptag.second;
                    }
                    if (with_sign && wrap_pbc)
                        term.coeff *= value_type(-1.);
                    term.push_back( boost::make_tuple(p_s, mytag) );
                }
                {
                    tag_type mytag = match2->second;
                    if (with_sign && wrap_pbc) {
                        // Note inverse notation because of notation in operator.
                        std::pair<tag_type, value_type> ptag = tag_handler->get_product_tag(operators[opkey_type("fill",type_t)],
                                                                                            mytag);
                        mytag = ptag.first;
                        term.coeff *= ptag.second;
                    }
                    term.push_back( boost::make_tuple(p_t, mytag) );
                }

                expression_coeff.insert( std::make_pair(term.coeff, value_type()) );
                expression_terms.push_back(term);
            }
        }

        generate_terms();
    }

    void update(BaseParameters const& p)
    {
        parms << p;
        generate_terms();
    }

    Index<SymmGroup> const& phys_dim(size_t type) const
    {
        return symm_basis[type].phys_dim();
    }

    typename SymmGroup::charge total_quantum_numbers(BaseParameters& parms_) const
    {
        return init_charge<SymmGroup>(parms_, all_conserved_qn);
    }

    tag_type identity_matrix_tag(size_t type) const
    {
        return operators[opkey_type("ident", type)]; // TODO: avoid using map here
    }

    tag_type filling_matrix_tag(size_t type) const
    {
        return operators[opkey_type("fill", type)]; // TODO: avoid using map here
    }

    tag_type get_operator_tag(std::string const & name, size_t type) const
    {
        if (name == "id" || name == "ident" || name == "identity") {
            return operators[opkey_type("ident", type)];
        } else {
            opmap_const_iterator match = operators.find(opkey_type(name, type));
            if (match == operators.end()) {
                SiteOperator op = make_site_term(name, parms);
                match = register_operator(op, type, parms);
            }
            return match->second;
        }
    }

    table_ptr operators_table() const
    {
        return tag_handler;
    }

    initializer_ptr initializer(Lattice const& lat, BaseParameters & p_) const;
    boost::ptr_vector<measurement<Matrix, SymmGroup> > measurements () const;

private:

    template <class SiteOp>
    std::string simplify_name(const SiteOp &op) const
    {
        std::string term = op.term();
        std::string arg = "("+op.site()+")";
        boost::algorithm::replace_all(term,arg,"");
        return term;
    }

    bool fermionic (alps::SiteBasisDescriptor<I> const& b1, SiteOperator const& op1,
                    alps::SiteBasisDescriptor<I> const& b2, SiteOperator const& op2) const
    {
        bool is_ferm1 = safe_is_fermionic(b1, op1);
        bool is_ferm2 = safe_is_fermionic(b2, op2);
        return is_ferm1 || is_ferm2;
    }

    inline op_t convert_matrix (const alps_matrix& m, int type) const
    {
        op_t newm;
        for (int i=0; i<m.shape()[0]; ++i) {
            for (int j=0; j<m.shape()[1]; ++j) {
                if (m[i][j] != 0.) {
                    charge c_i = symm_basis[type].charge(i);
                    size_t bsize_i = symm_basis[type].block_size(i);
                    charge c_j = symm_basis[type].charge(j);
                    size_t bsize_j = symm_basis[type].block_size(j);

                    if (!newm.has_block(c_i, c_j))
                        newm.insert_block(Matrix(bsize_i, bsize_j, 0), c_i, c_j);
                    // Notation: going from state i to state j
                    newm(symm_basis[type].coords(i), symm_basis[type].coords(j)) = m[i][j];
                }
            }
        }
        return newm;
    }

    alps::SiteOperator make_site_term(std::string x, alps::Parameters const & parms) const
    {
        if (x[x.size()-1]!=')')
            x += "(i)";
        alps::SiteOperator op(x,"i");
        model.substitute_operators(op, parms);
        return op;
    }

    opmap_const_iterator register_operator(SiteOperator const& op, int type, alps::Parameters const& p) const
    {
        alps::SiteBasisDescriptor<I> const& b = basis_descriptors[type];
        alps_matrix m = alps::get_matrix(value_type(), op, b, p, true);
        tag_detail::operator_kind kind = safe_is_fermionic(b, op) ? tag_detail::fermionic : tag_detail::bosonic;
        tag_type mytag = tag_handler->register_op(convert_matrix(m, type), kind);

        opmap_const_iterator match;
        boost::tie(match, boost::tuples::ignore) = operators.insert( std::make_pair(opkey_type(simplify_name(op), type), mytag) );
        return match;
    }

    std::pair<meas_operators_type, short> operators_for_meas(std::string const& ops, bool repeat_one=false) const
    {
        meas_operators_type ret;

        int ntypes = alps::maximum_vertex_type(lattice.graph())+1;
        int f_ops = 0;

        boost::char_separator<char> sep(":");
        tokenizer corr_tokens(ops, sep);
        std::vector<std::string> opnames;
        short op_types = 0;
        /// get list of all local operators
        for (tokenizer::iterator it=corr_tokens.begin(); it != corr_tokens.end(); it++)
        {
            if (model.has_bond_operator(*it)) {
                /// extract all site operators in the bond term
                BondOperator bondop = model.get_bond_operator(*it);

                typedef std::vector<boost::tuple<alps::expression::Term<value_type>,alps::SiteOperator,alps::SiteOperator > > V;

                alps::SiteBasisDescriptor<I> const& b1 = basis_descriptors[0];
                alps::SiteBasisDescriptor<I> const& b2 = basis_descriptors[0];

                V  bond_terms = bondop.template templated_split<value_type>(b1,b2);
                if (std::distance(bond_terms.begin(), bond_terms.end()) != 1) throw std::runtime_error("Can only measure BONDOPERATOR with a single term.");

                SiteOperator op1 = boost::get<1>(*bond_terms.begin());
                SiteOperator op2 = boost::get<2>(*bond_terms.begin());

                opnames.push_back(simplify_name(op1));
                opnames.push_back(simplify_name(op2));
                op_types |= 1<<1; // flag that at least one bond term is in the list
            } else {
                opnames.push_back(*it);
                op_types |= 1<<0; // flag that at least one site term is in the list
            }
        }

        if (op_types == 3) throw std::runtime_error("Can only have either site operators or bond operators.");


        /// get matrices for the operators
        for (std::vector<std::string>::const_iterator it2 = opnames.begin(); it2 != opnames.end(); ++it2)
        {
            enum {uknown, bosonic, fermionic} kind = uknown;
            std::vector<op_t> tops(ntypes);
            for (int type=0; type<ntypes; ++type) {
                alps::SiteBasisDescriptor<I> const& b = basis_descriptors[type];
                SiteOperator op;
                if (b.has_operator(*it2)) {
                    op = make_site_term(*it2, parms);
                } else if (model.has_site_operator(*it2)) {
                    op = model.get_site_operator(*it2);
                }
                if (op.term() != "") {
                    bool is_ferm = safe_is_fermionic(b, op);
                    if (kind == uknown)
                        kind = is_ferm ? fermionic : bosonic;
                    else if ((is_ferm && kind==bosonic) || (!is_ferm && kind==fermionic))
                        throw std::runtime_error("Model is inconsitent. On some site the operator " + *it2 + "fermionic, on others is bosonic.");

                    tops[type] = this->get_operator(*it2, type);
                }
            }

            if (kind == fermionic) ++f_ops;
            ret.push_back( std::make_pair(tops, (kind==fermionic)) );
        }

        /// repeat last site term in case only one in the input
        if (repeat_one && op_types == 1 && ret.size() == 1) {
            ret.push_back(ret[0]);
            if (ret[1].second) ++f_ops;
        }
        /// repeat last bond term (=two site terms) in case only one in the input
        if (repeat_one && op_types == 2 && ret.size() == 2) {
            ret.push_back(ret[0]);
            ret.push_back(ret[1]);
            if (ret[2].second) ++f_ops;
            if (ret[3].second) ++f_ops;
        }

        if (f_ops % 2 != 0)
            throw std::runtime_error("Number of fermionic operators has to be even.");

        return std::make_pair(ret, op_types);
    }

    void generate_terms()
    {
        this->terms_.clear();
        this->terms_.reserve(expression_terms.size());

        alps::Parameters parms_with_defaults(parms);
        parms_with_defaults.copy_undefined(model.model().default_parameters());

        // typedef typename boost::container::flat_map<expression_type, value_type>::iterator coeff_iterator;
        // for(coeff_iterator it = expression_coeff.begin(); it != expression_coeff.end(); ++it)
        //     it->second = alps::partial_evaluate<value_type>(it->first, parms_with_defaults);

        typedef typename std::vector<expression_term>::const_iterator terms_iterator;
        for(terms_iterator it = expression_terms.begin(); it != expression_terms.end(); ++it) {

            value_type val = 0.;
            if (lattice.inhomogeneous_sites() && it->size() == 1) {
                alps::Parameters p(parms_with_defaults);
                alps::throw_if_xyz_defined(p, lattice.graph()); // check whether x, y, or z is set
                p << coordinate_as_parameter(lattice.graph(), lattice.site(it->position(0)));

                val = alps::evaluate<value_type>(it->coeff, p);
            } else if(lattice.inhomogeneous_bonds() && it->size() == 2) {
                alps::Parameters p(parms_with_defaults);
                alps::throw_if_xyz_defined(p, lattice.graph()); // check whether x, y, or z is set
                p << coordinate_as_parameter(lattice.graph(), lattice.site(it->position(0)), lattice.site(it->position(1)));

                val = alps::evaluate<value_type>(it->coeff, p);
            } else {
                val = alps::evaluate<value_type>(it->coeff, parms_with_defaults);
            }

            // value_type const& val = expression_coeff[it->coeff];
            if ( alps::numeric::is_nonzero(val) ) {
                value_term term;
                term.is_fermionic = it->is_fermionic;
                term.insert(term.end(), it->begin(), it->end());
                term.coeff = val;
                this->terms_.push_back(term);
            }
        }
    }


    alps::Parameters parms;
    Lattice raw_lattice;
    graph_type const& lattice;
    alps::model_helper<I> model;
    mutable table_ptr tag_handler;

    std::set<std::string> all_qn;
    qn_map_type all_conserved_qn;
    std::vector<symmetric_basis_descriptor<SymmGroup> > symm_basis;
    std::vector<alps::SiteBasisDescriptor<I> > basis_descriptors;
    std::vector<alps::site_basis<I> > site_bases;

    mutable opmap_type operators; // key=<name,type>
    std::vector<expression_term> expression_terms;
    boost::container::flat_map<expression_type, value_type> expression_coeff;
};

// Initial states
template <class Matrix, class SymmGroup>
typename ALPSModel<Matrix, SymmGroup>::initializer_ptr ALPSModel<Matrix, SymmGroup>::initializer(Lattice const& lat, BaseParameters & p_) const
{
    if ( p_["init_type"] == "local_quantumnumbers" ) {
        int max_site_type = 0;
        std::vector<int> site_types(lat.size(), 0);
        for (int p = 0; p < lat.size(); ++p) {
            site_types[p] = lat.get_prop<int>("type", p);
            max_site_type = std::max(site_types[p], max_site_type);
        }

        maquis::cout << "site_types: ";
        std::copy(site_types.begin(), site_types.end(), maquis::ostream_iterator<int>(maquis::cout, " "));
        maquis::cout << std::endl;

        std::vector<Index<SymmGroup> > phys_bases(symm_basis.size());
        for (int type = 0; type < phys_bases.size(); ++type) {
            phys_bases[type] = symm_basis[type].phys_dim();
            maquis::cout << "phys["<< type <<"]: " << phys_bases[type] << std::endl;
        }

        // TODO: avoid QN of size=1
        std::map<std::string, std::vector<double> > initial_local_charges;
        for(std::set<std::string>::const_iterator it = all_qn.begin(); it != all_qn.end(); ++it) {
            const std::string pname = "initial_local_" + *it;
            if (!p_.defined(pname))
                throw std::runtime_error(pname + " required for local_quantumnumbers initial state.");
            initial_local_charges[*it] = p_[pname].as<std::vector<double> >();
            if (initial_local_charges[*it].size() != lat.size())
                throw std::runtime_error(pname + " does not match the lattice size.");
        }

        std::vector<boost::tuple<charge, size_t> > state(lat.size());
        for (size_t p=0; p<lat.size(); ++p) {
            const int type = site_types[p];
            alps::SiteBasisDescriptor<I> const& b = basis_descriptors[type];
            alps::site_state<I> local_state;
            for (size_t i=0; i<b.size(); ++i)
                local_state.push_back( initial_local_charges[b[i].name()][p] );
            state[p] = symm_basis[type].coords(site_bases[type].index(local_state));
        }

        return initializer_ptr(new basis_mps_init_generic<Matrix, SymmGroup>(state, phys_bases, this->total_quantum_numbers(p_), site_types));
    } else {
        return base::initializer(lat, p_);
    }
}


// Loading Measurements
template <class Matrix, class SymmGroup>
boost::ptr_vector<measurement<Matrix, SymmGroup> >
ALPSModel<Matrix, SymmGroup>::measurements () const
{
    boost::ptr_vector<measurement<Matrix, SymmGroup> > meas;

    int ntypes = alps::maximum_vertex_type(lattice.graph())+1;

    std::vector<op_t> identitities(ntypes), fillings(ntypes);
    for (int type=0; type<ntypes; ++type) {
        identitities[type] = this->identity_matrix(type);
        fillings[type]     = this->filling_matrix(type);
    }

    {
        std::regex average_expr("^MEASURE_AVERAGE\\[(.*)]$");
        std::regex locale_expr("^MEASURE_LOCAL\\[(.*)]$");
        std::smatch what;
        for (auto&& it : parms.get_range()) {
            std::string lhs = it.first;
            std::string obsname;

            enum {not_found, is_local, is_average} meas_found = not_found;
            if (std::regex_match(lhs, what, average_expr)) {
                meas_found = is_average;
                obsname = what.str(1);
            }
            if (std::regex_match(lhs, what, locale_expr)) {
                meas_found = is_local;
                obsname = what.str(1);
            }

            if (meas_found != not_found) {

                if (model.has_bond_operator(it.second) {
                    BondOperator bondop = model.get_bond_operator(it.second);

                    typedef std::vector<op_t> op_vec;
                    typedef std::vector<std::pair<op_vec, bool> > bond_element;
                    typedef std::vector<boost::tuple<alps::expression::Term<value_type>,alps::SiteOperator,alps::SiteOperator > > V;

                    std::vector<op_t> tops1(ntypes), tops2(ntypes);

                    std::vector<bond_element> operators;
                    std::set<int> used1, used2;
                    for (int type1=0; type1<ntypes; ++type1)
                        for (int type2=0; type2<ntypes; ++type2) {
                            if (used1.find(type1) != used1.end() && used2.find(type2) != used2.end())
                                continue;

                            alps::SiteBasisDescriptor<I> const& b1 = basis_descriptors[type1];
                            alps::SiteBasisDescriptor<I> const& b2 = basis_descriptors[type2];

                            V  ops = bondop.template templated_split<value_type>(b1,b2);
                            if (operators.size() < ops.size()) operators.resize(ops.size(), bond_element(2, std::make_pair(op_vec(ntypes), false)) );
                            int num_done = 0;
                            for (typename V::iterator tit=ops.begin(); tit!=ops.end();++tit) {
                                SiteOperator op1 = boost::get<1>(*tit);
                                SiteOperator op2 = boost::get<2>(*tit);

                                if (!b1.has_operator(simplify_name(op1)) || !b2.has_operator(simplify_name(op2)))
                                    continue;

                                unsigned ii = std::distance(ops.begin(), tit);
                                {
                                    operators[ii][0].second = safe_is_fermionic(b1, op1);
                                    op_t & m = operators[ii][0].first[type1];
                                    if (operators[ii][0].second)
                                        gemm(fillings[type1], this->get_operator(simplify_name(op1), type1), m); // Note inverse notation because of notation in operator.
                                    else
                                        m = this->get_operator(simplify_name(op1), type1);
                                    m = boost::get<0>(*tit).value() * m;
                                }
                                {
                                    operators[ii][1].second = safe_is_fermionic(b2, op2);
                                    op_t & m = operators[ii][1].first[type2];
                                    m = this->get_operator(simplify_name(op2), type2);
                                }

                                num_done += 1;
                            }

                            if (num_done == ops.size()) { used1.insert(type1); used2.insert(type2); }
                        }

                    if (meas_found == is_average)
                        meas.push_back( new measurements::average<Matrix, SymmGroup>(obsname, raw_lattice, identitities, fillings, operators) );
                    else
                        meas.push_back( new measurements::local<Matrix, SymmGroup>(obsname, raw_lattice, identitities, fillings, operators) );
                } else {
                    std::vector<op_t> tops(ntypes);
                    bool measurement_not_found = true;
                    for (int type=0; type<ntypes; ++type) {
                        alps::SiteBasisDescriptor<I> const& b = basis_descriptors[type];
                        if (b.has_operator(it.second)) {
                            SiteOperator op = make_site_term(it.second, parms);
                            if (safe_is_fermionic(b, op))
                                throw std::runtime_error("Cannot measure local fermionic operators.");

                            tops[type] = this->get_operator(it.second, type);
                            measurement_not_found = false;
                        } else if (model.has_site_operator(it.second)) {
                            SiteOperator op = model.get_site_operator(it.second);

                            if (safe_is_fermionic(b, op))
                                throw std::runtime_error("Cannot measure local fermionic operators.");

                            tops[type] = this->get_operator(it.second, type);
                            measurement_not_found = false;
                        }
                    }
                    if (measurement_not_found)
                        throw std::runtime_error("Operator "+it.second+" not found.");

                    if (meas_found == is_average)
                        meas.push_back( new measurements::average<Matrix, SymmGroup>(obsname, raw_lattice, identitities, fillings, tops) );
                    else
                        meas.push_back( new measurements::local<Matrix, SymmGroup>(obsname, raw_lattice, identitities, fillings, tops) );
                }
            }
        }
    }

    { // Example: MEASURE_LOCAL_AT[Custom correlation] = "bdag:b|(1,2),(3,4),(5,6)"
        std::regex expression("^MEASURE_LOCAL_AT\\[(.*)]$");
        std::smatch what;
        for (auto&& it: parms.get_range()) {
            std::string lhs = it.first;
            std::string value = it.second;
            if (std::regex_match(lhs, what, expression)) {
                int f_ops = 0;

                std::string name = what.str(1);

                boost::char_separator<char> part_sep("|");
                tokenizer part_tokens(value, part_sep);
                std::vector<std::string> parts;
                std::copy( part_tokens.begin(), part_tokens.end(), std::back_inserter(parts) );

                if (parts.size() != 2)
                    throw std::runtime_error("MEASURE_LOCAL_AT must contain a `|` delimiter.");

                /// parse operators
                meas_operators_type operators;
                boost::tie(operators, boost::tuples::ignore) = operators_for_meas(parts[0], false);

                /// parse positions
                std::vector<std::vector<pos_t> > positions;
                std::regex pos_re("\\(([^(^)]*)\\)");
                boost::sregex_token_iterator it_pos(parts[1].begin(), parts[1].end(), pos_re, 1);
                boost::sregex_token_iterator it_pos_end;
                for (; it_pos != it_pos_end; ++it_pos)
                {
                    boost::char_separator<char> int_sep(", ");
                    std::string raw = *it_pos;
                    tokenizer int_tokens(raw, int_sep);

                    std::vector<pos_t> pos;
                    BOOST_FOREACH(std::string t, int_tokens) {
                      pos.push_back(boost::lexical_cast<std::size_t, std::string>(t));
                    }
                    positions.push_back(pos);
                }
                if (f_ops % 2 != 0)
                    throw std::runtime_error("Number of fermionic operators has to be even.");

                meas.push_back( new measurements::local_at<Matrix, SymmGroup>(name, raw_lattice, positions, identitities, fillings, operators) );
            }
        }
    }

    {
        std::regex expression("^MEASURE_MPS_BONDS\\[(.*)]$");
        std::smatch what;
        for (auto&& it: parms.get_range()) {
            std::string lhs = it.first;
            std::string value;

            if (std::regex_match(lhs, what, expression)) {
                throw std::runtime_error("MEASURE_MPS_BONDS not yet implemented in new version.");
//                mterm_t term;
//                term.name = what.str(1);
//                term.type = mterm_t::MPSBonds;
//
//                fill_meas_with_operators(term, it.second, true);
//                meas.add_term(term);
            }
        }
    }

    {
        std::regex expression("^MEASURE_CORRELATIONS\\[(.*)]$");
        std::regex expression_half("^MEASURE_HALF_CORRELATIONS\\[(.*)]$");
        std::regex expression_nn("^MEASURE_NN_CORRELATIONS\\[(.*)]$");
        std::regex expression_halfnn("^MEASURE_HALF_NN_CORRELATIONS\\[(.*)]$");
        std::smatch what;
        for (auto&& it : parms.get_range()) {
            std::string lhs = it.first;

            std::string name, value;
            bool half_only=true, nearest_neighbors_only=false;
            if (std::regex_match(lhs, what, expression)) {
                value = it.second;
                name = what.str(1);
                half_only = false;
                nearest_neighbors_only = false;
            }
            if (std::regex_match(lhs, what, expression_half)) {
                value = it.second;
                name = what.str(1);
                half_only = true;
                nearest_neighbors_only = false;
            }
            if (std::regex_match(lhs, what, expression_nn)) {
                value = it.second;
                name = what.str(1);
                half_only = false;
                nearest_neighbors_only = true;
            }
            if (std::regex_match(lhs, what, expression_halfnn)) {
                value = it.second;
                name = what.str(1);
                half_only = true;
                nearest_neighbors_only = true;
            }
            if (!name.empty()) {
                /// split op1:op2:...@p1,p2,p3,... into {op1:op2:...}, {p1,p2,p3,...}
                std::vector<std::string> value_split;
                boost::split( value_split, value, boost::is_any_of("@"));

                meas_operators_type operators;
                short ops_type;
                boost::tie(operators, ops_type) = operators_for_meas(value_split[0], true);

                if (ops_type == 2) nearest_neighbors_only = true;

                /// parse positions p1,p2,p3,... (or `space`)
                std::vector<pos_t> positions;
                if (value_split.size() > 1) {
                    boost::char_separator<char> pos_sep(", ");
                    tokenizer pos_tokens(value_split[1], pos_sep);
                    BOOST_FOREACH(std::string t, pos_tokens) {
                      positions.push_back(boost::lexical_cast<std::size_t, std::string>(t));
                    }
                }

                meas.push_back( new measurements::correlations<Matrix, SymmGroup>(name, raw_lattice, identitities, fillings, operators,
                                                                                  half_only, nearest_neighbors_only,
                                                                                  positions) );
            }
        }
    }


    return meas;
}

#endif
