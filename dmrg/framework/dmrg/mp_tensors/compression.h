/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "dmrg/mp_tensors/mps.h"

#include "dmrg/mp_tensors/reshapes.h"
#include "dmrg/block_matrix/indexing.h"

struct compression {

  template<class Matrix, class SymmGroup>
  static truncation_results
  replace_two_sites_l2r(MPS<Matrix, SymmGroup> & mps, std::size_t Mmax, double cutoff,
                        block_matrix<Matrix, SymmGroup> const & t, std::size_t p, bool verbose=false)
  {
    block_matrix<Matrix, SymmGroup> u, v;
    typedef typename alps::numeric::associated_real_diagonal_matrix<Matrix>::type dmt;
    block_matrix<dmt, SymmGroup> s;
    truncation_results trunc = svd_truncate(t, u, v, s, cutoff, Mmax, verbose);
    mps[p].replace_left_paired(u, Lnorm);
    gemm(s, v, u);
    mps[p+1].replace_right_paired(u);
    return trunc;
  }

  template<class Matrix, class SymmGroup>
  static truncation_results
  replace_two_sites_r2l(MPS<Matrix, SymmGroup> & mps, std::size_t Mmax, double cutoff,
                        block_matrix<Matrix, SymmGroup> const & t, std::size_t p, bool verbose=false)
  {
    block_matrix<Matrix, SymmGroup> u, v;
    typedef typename alps::numeric::associated_real_diagonal_matrix<Matrix>::type dmt;
    block_matrix<dmt, SymmGroup> s;
    truncation_results trunc = svd_truncate(t, u, v, s, cutoff, Mmax, verbose);
    mps[p+1].replace_right_paired(v, Rnorm);
    gemm(u, s, v);
    mps[p].replace_left_paired(v);
    return trunc;
  }

template<class Matrix, class SymmGroup>
static void compress_two_sites(MPS<Matrix, SymmGroup> & mps, std::size_t Mmax, double cutoff,
                               std::size_t p, bool verbose=false)
{
  block_matrix<Matrix, SymmGroup> t;
  mps[p].make_left_paired();
  mps[p+1].make_right_paired();
  gemm(mps[p].data(), mps[p+1].data(), t);
  replace_two_sites_l2r(mps, Mmax, cutoff, t, p, verbose);
}


template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>
static l2r_compress(MPS<Matrix, SymmGroup> mps, std::size_t Mmax, double cutoff, double & ttrace, bool verbose = false)
{
  int L = mps.length();
  auto initialNorm = norm(mps);
  block_matrix<Matrix, SymmGroup> t;
  mps[0] /= std::sqrt(initialNorm);
  mps.canonize(1);
  for (int p = 1; p < L; ++p) {
    compress_two_sites(mps, Mmax, cutoff, p-1, verbose);
    t = mps[p].leftNormalizeAndReturn(DefaultSolver());
    if (p+1 < L)
      mps[p+1].multiply_from_left(t);
    else
      ttrace = trace(t);
  }
  mps[L-1] *= std::sqrt(initialNorm);
  return mps;
}
    
template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>
static l2r_compress(MPS<Matrix, SymmGroup> mps, std::size_t Mmax, double cutoff, bool verbose = false)
{
  int L = mps.length();
  auto initialNorm = norm(mps);
  mps.canonize(0);
  mps[0] /= std::sqrt(initialNorm);
  for (int p = 1; p < L; ++p)
    compress_two_sites(mps, Mmax, cutoff, p-1, verbose);
  mps[L-1] *= std::sqrt(initialNorm);
  return mps;
}

template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>
static r2l_compress(MPS<Matrix, SymmGroup> mps, std::size_t Mmax, double cutoff,
                    bool verbose = false)
{
  std::size_t L = mps.length();
  mps.canonize(L-1);
  auto initialNorm = overlap(mps, mps);
  mps[L-1] /= std::sqrt(initialNorm);
  for (std::size_t p = L-1; p > 0; --p)
      compress_two_sites(mps, Mmax, cutoff, p-1, verbose);
  mps[0] *= std::sqrt(initialNorm);
  return mps;
}

/*
template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>
static l2r_compress(MPS<Matrix, SymmGroup> mps, std::size_t Mmax, double cutoff, bool verbose = false)
{
  std::size_t L = mps.length();
  block_matrix<Matrix, SymmGroup> t;
  mps.canonize(1);
  if (verbose)
      maquis::cout << "Compressing @ ";
  for (std::size_t p = 1; p < L; ++p)
  {
      if (verbose) {
          maquis::cout << p << " ";
          maquis::cout.flush();
      }
      compress_two_sites(mps, Mmax, cutoff, p-1);
      t = mps[p].leftNormalizeAndReturn(DefaultSolver());
      if (p+1 < L)
          mps[p+1].multiply_from_left(t);
      // else
      //     maquis::cout << "Norm reduction: " << trace(t) << std::endl;
  }
  return mps;
}
*/

/*
template<class Matrix, class SymmGroup>
MPS<Matrix, SymmGroup>
static r2l_compress(MPS<Matrix, SymmGroup> mps,
                    std::size_t Mmax, double cutoff,
                    bool verbose = false)
{
    std::size_t L = mps.length();
    block_matrix<Matrix, SymmGroup> t;
    mps.canonize(L-1);
    if (verbose) maquis::cout << "Compressing @ ";
    for (std::size_t p = L-1; p > 0; --p)
    {
        if (verbose) {
            maquis::cout << p << " ";
            maquis::cout.flush();
        }
        compress_two_sites(mps, Mmax, cutoff, p-1);
        t = mps[p-1].rightNormalizeAndReturn(DefaultSolver());
        if (p > 1)
            mps[p-2].multiply_from_right(t);
        // else
        //     maquis::cout << "Norm reduction: " << trace(t) << std::endl;
    }
    return mps;
}
*/

};

#endif
