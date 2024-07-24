/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#include "dmrg/utils/archive.h"
#include "dmrg/utils/logger.h"
#include "dmrg/utils/parallel/params.hpp"
#include "utils.hpp"
#include "random.hpp"

// Init Comparison object
double cmp_with_prefactor::prefactor = 1.;

// Init random
dmrg_random::engine_t dmrg_random::engine = dmrg_random::engine_t(42);

// Init logger
namespace storage {
    Logger<storage::archive> log;
}

// Init parallel params 
namespace parallel {
    parameters& params = parameters::instance();
    int groups_granularity;
}
