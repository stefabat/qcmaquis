/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#include "dmrg/mp_tensors/mps.h"
#include "contractions.h"
#include "dmrg/utils/archive.h"

#include <limits>

template<class Matrix, class SymmGroup>
std::string MPS<Matrix, SymmGroup>::description() const
{
    std::ostringstream oss;
    for (int i = 0; i < length(); ++i)
    {
        oss << "MPS site " << i << std::endl;
        oss << (*this)[i].row_dim() << std::endl;
        oss << "Sum: " << (*this)[i].row_dim().sum_of_sizes() << std::endl;
        oss << (*this)[i].col_dim() << std::endl;
        oss << "Sum: " << (*this)[i].col_dim().sum_of_sizes() << std::endl;
    }
    return oss.str();
}

template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>::MPS()
    : canonized_i(std::numeric_limits<size_t>::max()) { }

template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>::MPS(size_t L): data_(L), canonized_i(std::numeric_limits<size_t>::max()) { }

template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>::MPS(std::initializer_list<MPSTensor<Matrix, SymmGroup> > l)
    : data_{l}, canonized_i(std::numeric_limits<size_t>::max()) { }

template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>::MPS(size_t L, mps_initializer<Matrix, SymmGroup> & init)
    : data_(L), canonized_i(std::numeric_limits<size_t>::max())
{
    init(*this);
    // MD: this is actually important
    //     it turned out, this is also quite dangerous: if a block is 1x2,
    //     normalize_left will resize it to 1x1
    //     init() should take care of it, in case needed. Otherwise some
    //     adhoc states will be broken (e.g. identity MPS)
    // for (int i = 0; i < L; ++i)
    //     (*this)[i].normalize_left(DefaultSolver());
    this->normalize_left();
}

template<class Matrix, class SymmGroup>
typename MPS<Matrix, SymmGroup>::value_type const & MPS<Matrix, SymmGroup>::operator[](size_t i) const
{
    return data_[i];
}

template<class Matrix, class SymmGroup>
typename MPS<Matrix, SymmGroup>::value_type& MPS<Matrix, SymmGroup>::operator[](size_t i)
{
    if (i != canonized_i)
        canonized_i=std::numeric_limits<size_t>::max();
    return data_[i];
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::resize(size_t L)
{
    // if canonized_i < L and L < current L, we could conserve canonized_i
    canonized_i=std::numeric_limits<size_t>::max();
    data_.resize(L);
}

template<class Matrix, class SymmGroup>
size_t MPS<Matrix, SymmGroup>::canonization(bool search) const
{
    if (!search)
        return canonized_i;

    size_t center = ((*this)[0].isleftnormalized()) ? 1 : 0;
    for (size_t i=1; i<length(); ++i) {
        if (!(*this)[i].isnormalized() && center != i) {
            canonized_i = std::numeric_limits<size_t>::max();
            return canonized_i;
        } else if ((*this)[i].isleftnormalized() && center == i)
            center = i+1;
        else if ((*this)[i].isleftnormalized()) {
            canonized_i = std::numeric_limits<size_t>::max();
            return canonized_i;
        }
    }
    if (center == length())
        center = length()-1;

    canonized_i = center;
    return canonized_i;
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::normalize_left()
{
    parallel::scheduler_balanced scheduler(length());
    canonize(length()-1);
    // now state is: A A A A A A M
    parallel::guard proc(scheduler(length()-1));
    auto normalizationFactor = (*this)[length()-1].leftNormalizeAndReturn(DefaultSolver());
    if (maquis::real(normalizationFactor.trace()) < 0.)
        this->operator[](0) *= -1.;
    // now state is: A A A A A A A
    canonized_i = length()-1;
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::normalize_right()
{
    parallel::scheduler_balanced scheduler(length());
    canonize(0);
    // now state is: M B B B B B B
    parallel::guard proc(scheduler(0));
    auto normalizationFactor = (*this)[0].rightNormalizeAndReturn(DefaultSolver());
    if (maquis::real(normalizationFactor.trace()) < 0)
        this->operator[](0) *= -1.;
    // now state is: B B B B B B B
    canonized_i = 0;
}

// input:  M  M  M  M  M  M  M
//  (idx)        c
// output: A  A  M  B  B  B  B
template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::canonize(std::size_t center, DecompMethod method)
{
    if (canonized_i == center)
        return;

    if (canonized_i < center)
        move_normalization_l2r(canonized_i, center, method);
    else if (canonized_i < length())
        move_normalization_r2l(canonized_i, center, method);
    else {
        move_normalization_l2r(0, center, method);
        move_normalization_r2l(length()-1, center, method);
    }
    canonized_i = center;
}

// input:  M  M  M  M  M  M  M
//  (idx)     p1       p2
// output: M  A  A  A  M  M  M
template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::move_normalization_l2r(size_t p1, size_t p2, DecompMethod method)
{
    parallel::scheduler_balanced scheduler(length());

    size_t tmp_i = canonized_i;
    for (int i = p1; i < std::min(p2, length()); ++i)
    {
        if ((*this)[i].isleftnormalized())
            continue;
        block_matrix<Matrix, SymmGroup> t;
        {
            parallel::guard proc(scheduler(i));
            t = (*this)[i].leftNormalizeAndReturn(method);
        }
        if (i < length()-1) {
            parallel::guard proc(scheduler(i+1));
            (*this)[i+1].multiply_from_left(t);
            //(*this)[i+1].divide_by_scalar((*this)[i+1].scalar_norm());
        }
    }
    if (tmp_i == p1)
        canonized_i = p2;
    else
        canonized_i = std::numeric_limits<size_t>::max();
}

// input:  M  M  M  M  M  M  M
//  (idx)     p2       p1
// output: M  M  B  B  B  M  M
template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::move_normalization_r2l(size_t p1, size_t p2, DecompMethod method)
{
    parallel::scheduler_balanced scheduler(length());

    size_t tmp_i = canonized_i;
    for (int i = p1; i > static_cast<int>(std::max(p2, size_t(0))); --i)
    {
        if ((*this)[i].isrightnormalized())
            continue;
        block_matrix<Matrix, SymmGroup> t;
        {
            parallel::guard proc(scheduler(i));
            t = (*this)[i].rightNormalizeAndReturn(method);
        }
        if (i > 0) {
            parallel::guard proc(scheduler(i-1));
            (*this)[i-1].multiply_from_right(t);
            //(*this)[i-1].divide_by_scalar((*this)[i-1].scalar_norm());
        }
    }
    if (tmp_i == p1)
        canonized_i = p2;
    else
        canonized_i = std::numeric_limits<size_t>::max();
}

template<class Matrix, class SymmGroup>
template<class OtherMatrix>
truncation_results
MPS<Matrix, SymmGroup>::grow_l2r_sweep(MPOTensor<Matrix, SymmGroup> const & mpo, Boundary<OtherMatrix, SymmGroup> const & left,
                                       Boundary<OtherMatrix, SymmGroup> const & right, std::size_t l, double alpha,
                                       double cutoff, std::size_t Mmax, bool perturbDM, bool verbose)
{ // canonized_i invalided through (*this)[]
    using Contractor = typename contraction::Engine<Matrix, OtherMatrix, SymmGroup>;
    MPSTensor<Matrix, SymmGroup> new_mps;
    truncation_results trunc;
    boost::tie(new_mps, trunc) = Contractor::predict_new_state_l2r_sweep((*this)[l], mpo, left, right, alpha, cutoff, Mmax, perturbDM, verbose);
    (*this)[l+1] = contraction::Engine<Matrix, OtherMatrix, SymmGroup>::predict_lanczos_l2r_sweep((*this)[l+1], (*this)[l], new_mps);
    (*this)[l] = new_mps;
    return trunc;
}

template<class Matrix, class SymmGroup>
template<class OtherMatrix>
truncation_results
MPS<Matrix, SymmGroup>::grow_r2l_sweep(MPOTensor<Matrix, SymmGroup> const & mpo, Boundary<OtherMatrix, SymmGroup> const & left,
                                       Boundary<OtherMatrix, SymmGroup> const & right, std::size_t l, double alpha,
                                       double cutoff, std::size_t Mmax, bool perturbDM, bool verbose)
{ // canonized_i invalided through (*this)[]
    using Contractor = typename contraction::Engine<Matrix, OtherMatrix, SymmGroup>;
    MPSTensor<Matrix, SymmGroup> new_mps;
    truncation_results trunc;
    boost::tie(new_mps, trunc) = Contractor::predict_new_state_r2l_sweep((*this)[l], mpo, left, right, alpha, cutoff, Mmax, perturbDM, verbose);
    (*this)[l-1] = contraction::Engine<Matrix, OtherMatrix, SymmGroup>::predict_lanczos_r2l_sweep((*this)[l-1], (*this)[l], new_mps);
    (*this)[l] = new_mps;
    return trunc;
}

template<class Matrix, class SymmGroup>
Boundary<Matrix, SymmGroup>
MPS<Matrix, SymmGroup>::left_boundary() const
{
    Index<SymmGroup> i = (*this)[0].row_dim();
    Boundary<Matrix, SymmGroup> ret(i, i, 1);

    for(std::size_t k(0); k < ret[0].n_blocks(); ++k)
       maquis::dmrg::detail::left_right_boundary_init(ret[0][k]);

    ret[0].index_sizes();
    return ret;
}

template<class Matrix, class SymmGroup>
Boundary<Matrix, SymmGroup>
MPS<Matrix, SymmGroup>::right_boundary() const
{
    Index<SymmGroup> i = (*this)[length()-1].col_dim();
    Boundary<Matrix, SymmGroup> ret(i, i, 1);

    for(std::size_t k(0); k < ret[0].n_blocks(); ++k)
        maquis::dmrg::detail::left_right_boundary_init(ret[0][k]);

    ret[0].index_sizes();
    return ret;
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::apply(typename operator_selector<Matrix, SymmGroup>::type const& op, typename MPS<Matrix, SymmGroup>::size_type p)
{
    typedef typename SymmGroup::charge charge;
    using std::size_t;

    /// Compute (and check) charge difference
    charge diff = SymmGroup::IdentityCharge;
    if (op.n_blocks() > 0)
        diff = SymmGroup::fuse(op.basis().right_charge(0), -op.basis().left_charge(0));
    for (size_t n=0; n< op.n_blocks(); ++n) {
        if ( SymmGroup::fuse(op.basis().right_charge(n), -op.basis().left_charge(n)) != diff )
            throw std::runtime_error("Operator not allowed. All non-zero blocks have to provide same `diff`.");
    }

    /// Apply operator
    (*this)[p] = contraction::multiply_with_op((*this)[p], op);

    /// Propagate charge difference
    for (size_t i=p+1; i<length(); ++i) {
        (*this)[i].shift_aux_charges(diff);
    }
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::apply(typename operator_selector<Matrix, SymmGroup>::type const& fill,
                                   typename operator_selector<Matrix, SymmGroup>::type const& op, typename MPS<Matrix, SymmGroup>::size_type p)
{
    for (size_t i=0; i<p; ++i) {
        (*this)[i] = contraction::multiply_with_op((*this)[i], fill);
    }
    apply(op, p);
}

template<class Matrix, class SymmGroup>
template <class Archive>
void MPS<Matrix, SymmGroup>::serialize(Archive & ar, const unsigned int version)
{
    ar & canonized_i & data_;
}

template<class Matrix, class SymmGroup>
void load(std::string const& dirname, MPS<Matrix, SymmGroup> & mps)
{
    /// get size of MPS
    std::size_t L = 0;
    while (boost::filesystem::exists( dirname + "/mps" + boost::lexical_cast<std::string>(++L) + ".h5" ));

    /// load tensors
    MPS<Matrix, SymmGroup> tmp(L);
    size_t loop_max = tmp.length();
    parallel::scheduler_balanced scheduler(loop_max);
    for(size_t k = 0; k < loop_max; ++k){
        parallel::guard proc(scheduler(k));
        std::string fname = dirname+"/mps"+boost::lexical_cast<std::string>((size_t)k)+".h5";
        storage::archive ar(fname);
        ar["/tensor"] >> tmp[k];
    }
    swap(mps, tmp);
}

template<class Matrix, class SymmGroup>
void save(std::string const& dirname, MPS<Matrix, SymmGroup> const& mps)
{
    /// create chkp dir
    if(parallel::master() && !boost::filesystem::exists(dirname))
        boost::filesystem::create_directory(dirname);

    parallel::scheduler_balanced scheduler(mps.length());
    size_t loop_max = mps.length();

    for(size_t k = 0; k < loop_max; ++k){
        parallel::guard proc(scheduler(k));
        mps[k].make_left_paired();
        storage::migrate(mps[k]);
    }
    parallel::sync();

    for(size_t k = 0; k < loop_max; ++k){
        parallel::guard proc(scheduler(k));
        if(!parallel::local()) continue;
        const std::string fname = dirname+"/mps"+boost::lexical_cast<std::string>((size_t)k)+".h5.new";
        storage::archive ar(fname, "w");
        ar["/tensor"] << mps[k];
    }

    parallel::sync(); // be sure that chkp is in valid state before overwriting the old one.

    omp_for(size_t k, parallel::range<size_t>(0,loop_max), {
        parallel::guard proc(scheduler(k));
        if(!parallel::local()) continue;
        const std::string fname = dirname+"/mps"+boost::lexical_cast<std::string>((size_t)k)+".h5";
        boost::filesystem::rename(fname+".new", fname);
    });
}

template <class Matrix, class SymmGroup>
void check_equal_mps (MPS<Matrix, SymmGroup> const & mps1, MPS<Matrix, SymmGroup> const & mps2)
{
    // Length
    if (mps1.length() != mps2.length())
        throw std::runtime_error("Length doesn't match.");

    for (int i=0; i<mps1.length(); ++i)
        try {
            mps1[i].check_equal(mps2[i]);
        } catch (std::exception & e) {
            maquis::cerr << "Problem on site " << i << ":" << e.what() << std::endl;
            exit(1);
        }
}

template <class Matrix, class SymmGroup>
void clean_mps(MPS<Matrix, SymmGroup> & mps)
{
    // ensure consistent indices across bonds by removing blocks without connection across bonds

    bool again;
    do {
        again = false;
        for (size_t p = 0; p < mps.length()-1; ++p)
        {
            mps[p].make_left_paired();
            mps[p+1].make_right_paired();
            block_matrix<Matrix, SymmGroup> bm1 = mps[p].data(), bm2 = mps[p+1].data();

            for (size_t b = 0; b < bm1.n_blocks(); ++b)
            {
                typename SymmGroup::charge c = bm1.basis().right_charge(b);
                if (! bm2.basis().has(c, c)) {
                    bm1.remove_block(b--);
                    again = true;
                }
            }
            for (size_t b = 0; b < bm2.n_blocks(); ++b)
            {
                typename SymmGroup::charge c = bm2.basis().left_charge(b);
                if (! bm1.basis().has(c, c)) {
                    bm2.remove_block(b--);
                    again = true;
                }
            }

            assert(bm1.right_basis() == bm2.left_basis());

            mps[p].replace_left_paired(bm1);
            mps[p+1].replace_right_paired(bm2);
        }
    } while (again);
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::setComplexPartToZero() {
    for (auto& mpsTensor: data_)
        mpsTensor.setComplexPartToZero();
}

template<class Matrix, class SymmGroup>
void MPS<Matrix, SymmGroup>::scaleByScalar(scalar_type scalingFactor) {
    this->operator[](this->length()-1) *= scalingFactor;
}
