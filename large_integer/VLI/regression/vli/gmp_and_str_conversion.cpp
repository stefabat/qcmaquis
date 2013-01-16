#include <gmpxx.h>
#include <regression/vli/test_header.hpp>

using namespace vli::test;

VLI_FUZZABLE_TEST( gmp_and_str_conversion_mpz )
{
    integer_type a;
    init(a);
    mpz_class agmp(a);
    BOOST_CHECK_EQUAL(a.get_str(),agmp.get_str());
}

VLI_FUZZABLE_TEST( gmp_and_str_conversion_mpq )
{
    integer_type a;
    init(a);
    mpq_class agmp(a);
    BOOST_CHECK_EQUAL(a.get_str(),agmp.get_str());
}

VLI_FUZZABLE_TEST( gmp_and_str_conversion_negative_mpz )
{
    integer_type a;
    init(a);
    negate_inplace(a);
    mpz_class agmp(a);
    BOOST_CHECK_EQUAL(a.get_str(),agmp.get_str());
}

VLI_FUZZABLE_TEST( gmp_and_str_conversionnegative_mpq )
{
    integer_type a;
    init(a);
    negate_inplace(a);
    mpq_class agmp(a);
    BOOST_CHECK_EQUAL(a.get_str(),agmp.get_str());
}
