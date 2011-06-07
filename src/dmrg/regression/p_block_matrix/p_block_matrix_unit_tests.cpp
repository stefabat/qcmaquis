#include "utils/zout.hpp"
#include <vector>
#include "p_dense_matrix/p_dense_matrix.h"
#include "p_dense_matrix/p_dense_matrix_algorithms.hpp"
#include "p_dense_matrix/concept/matrix_interface.hpp"
#include "p_dense_matrix/concept/resizable_matrix_interface.hpp"

#define BOOST_TEST_MODULE p_dense_matrix
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <boost/lambda/lambda.hpp>
#include <complex>
#include <numeric>

#define M_SIZE 6
typedef blas::p_dense_matrix<double> Matrix;
using namespace blas;
#include <alps/hdf5.hpp>

#include "block_matrix/block_matrix.h"
#include "block_matrix/block_matrix_algorithms.h"

typedef boost::mpl::list<double> test_types;
typedef ambient::dim2 dim;

struct caveats {
    caveats(){ ambient::init();     }
   ~caveats(){ ambient::finalize(); }
};

BOOST_GLOBAL_FIXTURE( caveats );

BOOST_AUTO_TEST_CASE_TEMPLATE( p_block_matrix_test, T, test_types )
{
    ambient::layout >> dim(1,1), dim(1,1), dim(10,1);
    typedef U1 grp;
    typedef blas::p_dense_matrix<double> Matrix;

    Matrix d(0,0);
    Matrix e(0,0);
    //ambient::playout();

    resize(d, 4, 4);
    resize(e, 4, 4);
    block_matrix<Matrix, grp> m1, m2;
    
    m1.insert_block(d, -1, 1);
    m1.insert_block(e, 0, 1);
    m1.generate(double());
    std::cout << m1;
    resize(d, 2, 2);

    printf("m2:\n");
    m2.reserve(2,2,6,6);
    m2.allocate_blocks();
    m2.generate(double());

    std::cout << m2;
    resize(d, 4, 4);
}

