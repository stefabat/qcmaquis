/*
*Very Large Integer Library, License - Version 1.0 - May 3rd, 2012
*
*Timothee Ewart - University of Geneva,
*Andreas Hehn - Swiss Federal Institute of technology Zurich.
*
*Permission is hereby granted, free of charge, to any person or organization
*obtaining a copy of the software and accompanying documentation covered by
*this license (the "Software") to use, reproduce, display, distribute,
*execute, and transmit the Software, and to prepare derivative works of the
*Software, and to permit third-parties to whom the Software is furnished to
*do so, all subject to the following:
*
*The copyright notices in the Software and this entire statement, including
*the above license grant, this restriction and the following disclaimer,
*must be included in all copies of the Software, in whole or in part, and
*all derivative works of the Software, unless such copies or derivative
*works are solely in the form of machine-executable object code generated by
*a source language processor.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
*SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
*FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
*ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*DEALINGS IN THE SOFTWARE.
*/

#ifndef VLI_VLI_HPP
#define VLI_VLI_HPP
#include "vli/function_hooks/vli_number_cpu_function_hooks.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp> // for the type boost::uint64_t
#include <boost/operators.hpp>

//#include <initializer_list>
#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <boost/swap.hpp>


/*! \namespace vli
    \brief the name space of the library
    
    This name space contains a lot of free functions for the main functionnalities of the library : arithmetic operations between integer, monomial and polynomial, as inner profucts.
*/
namespace vli {
    /* \cond I do not need this part in the doc*/
    struct integer_division_by_zero_error : public std::runtime_error { integer_division_by_zero_error(): std::runtime_error("Divide by zero."){}; };

    template<std::size_t NumBits> class integer;

    template <std::size_t NumBits>
    void swap(integer<NumBits>& integer_a, integer<NumBits>& integer_b){
        boost::swap(integer_a.data_,integer_b.data_);
    }

    // Karatsuba stuff
    struct copy_msb_tag{}; // Most Significant Bits
    struct copy_lsb_tag{}; // Least Significant Bits
    struct copy_right_shift_tag{}; // for the shift copy
    /* \endcond */
    
    /*! \class integer
        \brief This class models the integer<NumBits> number it is templated over the total number of bit
     
        The four basics operators are included : +,-,* and / as bit operations and equality and inequality operators. All theses operators
        conserve the number of bit. The class derived from the boost::operators package.  Per exmaple for a given operator integer<NumBits> += integer<NumBits>  the complentary operation
        integer<NumBits> + integer<NumBits> is generated automatically.
     */
    template<std::size_t NumBits>
    class integer
        :boost::equality_comparable<integer<NumBits> >, // generate != operator
         boost::less_than_comparable<integer<NumBits> >, // generate <= >= > < whatever the paire integer/integer
         boost::less_than_comparable<integer<NumBits>, long int>, // generate <= >= > < whatever the paire integer/int
         boost::addable<integer<NumBits> >, // generate integer<nbits> = VLIVLI<nbits> + integer<integer<nbits>
         boost::subtractable<integer<NumBits> >, // generate integer<nbits> = VLIVLI<nbits> - integer<integer<nbits>
         boost::multipliable<integer<NumBits> >, //  generate integer<nbits> = VLIVLI<nbits> * integer<integer<nbits>
         boost::left_shiftable<integer<NumBits>, long int>, // enerate integer<nbits> = VLIVLI<nbits> << int
         boost::right_shiftable<integer<NumBits>, long int>, //enerate integer<nbits> = VLIVLI<nbits> >> int
         boost::modable<integer<NumBits> >,
         boost::dividable<integer<NumBits> >
    {
    public:
        /*! \brief The value type of the integer number: a 64-bit unsigned integer */
        typedef boost::uint64_t      value_type;
        
         /* \cond I do not need this part in the doc*/
        typedef std::size_t          size_type;
        /* \endcond */
        
        /*! \brief The size of the integer [bit] */
        static const std::size_t numbits = NumBits;

        /*! \brief The  number of word of the integer, we consider the world equal to 64 bits for the CPU version*/
        static const std::size_t numwords = (NumBits+63)/64;
        
   //     integer(std::initializer_list<value_type> i) : data_{i} {}
        /**
         \brief Default constructor, the integer number is equal to 0, every entries of the container are set up to 0
        */
        integer();
        /**
         \brief Constructor, the integer<NumBits> number is equal to the parameter.
         \param 64-bit int
         \note  If the parameter is negative, the integer number is initialized with the two complementary method. Copy constructor and destructor are generated automatically by the compiler.
         */
        explicit integer(long int num);
        /* \cond I do not need this part in the doc*/
        integer(integer<2*NumBits> const&, copy_lsb_tag);
        integer(integer<2*NumBits> const&, copy_msb_tag);
        integer(integer<NumBits/2> const&, copy_right_shift_tag);
        integer(integer<NumBits/2> const&, integer<NumBits/2> const&, copy_right_shift_tag);//just  coherence previous one
        /* \endcond */
#if defined __GNU_MP_VERSION
        // TODO find a better solution for this.
        operator mpz_class() const;
        operator mpq_class() const;
#endif //__GNU_MP_VERSION
        /* \cond I do not need this part in the doc*/
        friend void swap<> (integer& integer_a, integer& integer_b);
        /* \endcond */
        /**
         \fn value_type& operator[](size_type i)
         \brief Give a write acces to the element of the integer number
         \param i unsigned 64-bit int
         */
        value_type& operator[](size_type i);
        /**
         \fn value_type& operator[](size_type i) const
         \brief Give a read acces to the element of the integer number
         \param i unsigned 64-bit int
         */
        const value_type& operator[](size_type i) const;
        // c - negative number
        /* \cond I do not need this part in the doc*/
        void negate();
        bool is_negative() const;
        /* \endcond */
        // c - basic operator

        /**
         \fn integer<NumBits>& operator >>= (integer<NumBits> const& integer_a)
         \brief Perform a right bit shift operation on the integer<NumBits> number, it conserves the number of bits
         \param a 64-bit int
         */
        integer& operator >>= (long int const a); // bit shift
        
        /**
         \fn integer<NumBits>& operator <<= (integer<NumBits> const& integer_a)
         \brief Perform a left bit shift operation on the integer<NumBits> number, it conserves the number of bits
         \param a 64-bit int
         */
        integer& operator <<= (long int const a); // bit shift

        /**
         \fn integer<NumBits>& operator |= (integer<NumBits> const& integer_a)
         \brief Perform a bit | operation between two integer<NumBits> numbers, it conserves the number of bits
         \param integer_a integer<NumBits> number
         */
        integer& operator |= (integer const& integer_a); // bit shift

        /**
         \fn integer<NumBits>& operator ^= (integer<NumBits> const& integer_a)
         \brief Perform a bit ^ operation between two integer<NumBits> numbers, it conserves the number of bits
         \param integer_a integer<NumBits> number
         */
        integer& operator ^= (integer const& integer_a);

        /**
         \fn integer<NumBits>& operator &= (integer<NumBits> const& integer_a)
         \brief Perform a bit & operation between two integer<NumBits> numbers, it conserves the number of bits
         \param integer_a integer<NumBits> number
         */
        integer& operator &= (integer const& integer_a);

        /**
         \fn integer<NumBits>& operator += (integer<NumBits> const& integer_a)
         \brief Perform an addition between two integer<NumBits> numbers, it conserves the number of bits
         \param integer_a integer<NumBits> number
         */
        integer& operator += (integer const& integer_a);

        /**
         \fn integer<NumBits>& operator += (long int const a)
         \brief Perform an addition between a integer<NumBits> number and a 64-bit long integer, it conserves the number of bits
         \param a 64-bit int
        */
        integer& operator += (long int const a);

        /**
         \fn integer<NumBits>& operator -= (integer<NumBits> const& integer_a)
         \brief Perform a substraction between two integer<NumBits> numbers, it conserves the number of bits
         \param integer_a integer<NumBits> number
         */
        integer& operator -= (integer const& integer_a);
        
        /**
         \fn integer<NumBits>& operator -= (long int const a)
         \brief Perform a substraction between a integer<NumBits> number and a signed 64-bit int, it conserves the number of bits
         \param a 64-bit int
         */
        integer& operator -= (long int a);

        /**
         \fn integer<NumBits>& operator *= (integer<NumBits> const& integer_a)
         \brief Perform a multiplication between two integer<NumBits> numbers, it conserves the number of bits
         \param a integer<NumBits> number
         */
        integer& operator *= (integer const& integer_a);
        
        /**
         \fn integer<NumBits>& operator *= (long int const a)
         \brief Perform a multiplication between a integer number<NumBits> and a  signed 64-bit int, it conserves the number of bits
         \param a 64-bit int
         */
        integer& operator *= (long int a);

        /**
         \fn integer<NumBits>& operator /= (integer<NumBits> a)
         \brief Perform a division between two integer<NumBits> numbers. It return the quotient of the euclidian division, it conserves the number of bits
         \param a integer<NumBits> number
         \note this operator is slow
         */
        integer& operator /= (integer integer_a);

        /**
         \fn integer<NumBits>& operator %= (integer<NumBits> a)
         \brief Perform a division between two integer numbers. It return the rest of the euclidian division followinf the GMP convention (for negative number it exists two possibilities), it conserves the number of bits. The ASM solver is generic with  /= 
         \param integer_a integer<NumBits> number
         \note this operator is slow
         */
        integer& operator %= (integer integer_a);

        /* \cond I do not need this part in the doc*/
        integer operator -() const;
        /* \endcond */
        
        /**
         \fn integer<NumBits>& operator == (integer<NumBits> const& integer_a) const
         \brief Test the equality between two integer<NumBits> numbers. The complementary operator != is generated automatically by the boost operator package
         \param a integer<NumBits> number
         */
        bool operator == (integer const& integer_a) const; // need by boost::equality_comparable

        /**
         \fn integer<NumBits>& operator < (integer<NumBits> const& integer_a) const
         \brief Test the inequality < between two integer<NumBits> numbers. The complementary operator <=, > and >= are generated automatically by the boost operator package
         \param a integer number
         */
        bool operator < (integer const& integer_a) const; // need by less_than_comparable<T>

        /**
         \fn integer<NumBits>& operator < (long int) const
         \brief Test the inequality < between a signed 64-bit int and a integer<NumBits> number. The complementary operator <= is generated automatically by the boost operator package
         \param a signed int 64-bit
         */
        bool operator < (long int a) const; // need by less_than_comparable<T,U>

        /**
         \fn integer& operator > (long int) const
         \brief Test the inequality > between a signed 64-bit int and a integer<NumBits> number. The complementary operator >= is generated automatically by the boost operator package
         \param a signed int 64-bit
         */
        bool operator > (long int a) const; // need by less_than_comparable<T,U>
 
        /* \cond I do not need this part in the doc*/
        bool is_zero() const;
        void print_raw(std::ostream& os) const;
        void print(std::ostream& os) const;

        std::string get_str() const;
        size_type order_of_magnitude_base10(integer const& value) const;
        std::string get_str_helper_inplace(integer& value, size_type ten_exp) const;
        /* \endcond */

    private:
        value_type data_[numwords]; /*!< The container of the integer number a 64-bit unsigned integer */
    };
    
    /* \cond */
    template <std::size_t NumBits>
    bool is_zero(integer<NumBits> const& v);

    template <std::size_t NumBits>
    void negate_inplace(integer<NumBits>& v);
    /* \endcond */

    /**
     \brief Addition between an integer<NumBits> and a signed 64-bit int
     \return integer<NumBits>
     \param integer_a integer<NumBits> numer
     \param b long int
     This operator performs an addition between an integer and as signed 64 bits integer, the return has the same size than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     */
    template <std::size_t NumBits>
    const integer<NumBits> operator + (integer<NumBits> integer_a, long int b);

    /**
     \brief Addition between a signed 64-bit integer and an integer<NumBits>
     \return integer number<NumBits>
     \param a long int
     \param integer_b integer<NumBits> 
     This operator performs an addition between an integer and as signed 64 bits integer, the return has the same size than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     */
    template <std::size_t NumBits>
    const integer<NumBits> operator + (long int b, integer<NumBits> const& integer_b);
    
    /**
     \brief Addition between an integer<NumBits> number and a signed 64-bit int
     \return integer larger (64-bit more)
     \param integer_a an integer<NumBits> number
     \param integer_b an integer<NumBits> number
     This operator performs an addition between an integer<NumBits> and as signed 64 bits int, the return has the same size than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     */
    template <std::size_t NumBits> //extented arithmetic
    const integer<NumBits+64> plus_extend(integer<NumBits> const& integer_a, integer<NumBits> const& integer_b);

    /**
     \brief substraction between an integer<NumBits> number and a signed 64-bit int
     \return integer number<NumBits> number
     \param integer_res integer number<NumBits> number
     \param b long int
     This operator performs an addition between an integer<NumBits> and as signed 64 bits int, the return has the same size than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     */
    template <std::size_t NumBits>
    const integer<NumBits> operator - (integer<NumBits> integer_a, long int b);

    
    /**
     \brief multiplication between an integer<NumBits> and a signed 64-bit int
     \return integer number<NumBits> number
     \param integer_a integer number<NumBits> number
     \param b long int
     This operator performs an addition between an integer<NumBits> and as signed 64 bits int, the return has the same size than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     */
    template <std::size_t NumBits>
    const integer<NumBits> operator * (integer<NumBits> integer_a, long int b);

    /**
     \brief multiplication between a signed 64-bit int and integer<NumBits> number
     \return integer number<NumBits> number
     \param a long int
     \param integer_b integer number<NumBits> number
     This operator performs an addition between an integer<NumBits> and as signed 64 bits int, the return has the same size than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     */
    template <std::size_t NumBits>
    const integer<NumBits> operator * (long int a, integer<NumBits> const& integer_b);
    
    /**
     \brief extended multiplication between two integer<NumBits>
     \return void
     \param integer_res integer number<2*umBits> number
     \param integer_a integer number<NumBits> number
     \param integer_b integer number<NumBits> number
     This operator performs an addition between two integer<NumBits>, the result is twice larger than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     \warning The size template parameter must be twice large for the ouput. This operation give a new type
     */
    template <std::size_t NumBits>
    void multiply_extend(integer<2*NumBits>& integer_res, integer<NumBits> const&  integer_a, integer<NumBits> const& integer_b); // C nt = non truncated

    /**
     \brief fuse multiply-add operation
     \return void
     \param integer_res integer number<2*NumBits> number
     \param integer_a integer number<NumBits> number
     \param integer_b integer number<NumBits> number
     This operator performs an addition between two integer<NumBits>, the result is twice larger than the input.
     The ASM solver is specific. This operator is not generated by BOOST::operator for performance issue.
     \warning The size template parameter must be twice large for the ouput. This operation give a new type
     */
    template <std::size_t NumBits>
    void multiply_add(integer<2*NumBits>& integer_res, integer<NumBits> const&  integer_a, integer<NumBits> const& integer_b); // C

    /**
         \brief  stream operator
          As classical iostream, the integer stream allows hexadecimal e.g. std::cout << or std::cout << std::hex <<
    */
    template <std::size_t NumBits>
    std::ostream& operator<< (std::ostream& os,  integer<NumBits> const& );
}

#include <vli/vli.ipp>
#include "vli/detail/cpu/x86_64/karatsuba_asm.h"

#endif //VLI_VLI_IPP
