/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#ifndef SYMMETRY_Z2_H
#define SYMMETRY_Z2_H

#include <iostream>

#include <boost/functional/hash.hpp>
 
#include <alps/hdf5.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>

class Ztwo {
	public:
		typedef enum { Plus = 0, Minus = 1 } charge;
        typedef int subcharge; // used if charge is site-dependent
		
		static const charge IdentityCharge = Plus;
        static const bool finite = true;
		
		static inline charge fuse(charge a, charge b)
		{
			if (a == b)
				return Plus;
			else
				return Minus;
		}

        static subcharge particleNumber(charge a) { return 1; }
		
		template<int R>
		static charge fuse(boost::array<charge, R> v)
		{
			// this operation actually could be rearranged into a tree
			for (int i = 1; i < R; i++)
				v[0] = fuse(v[0], v[i]);
			return v[0];
		}
};

inline void save(alps::hdf5::archive & ar,
                 std::string const & p,
                 Ztwo::charge const & v,
                 std::vector<std::size_t> size = std::vector<std::size_t>(),
                 std::vector<std::size_t> chunk = std::vector<std::size_t>(),
                 std::vector<std::size_t> offset = std::vector<std::size_t>())
{
    ar[p] << static_cast<int>(v);
}

inline void load(alps::hdf5::archive & ar,
                 std::string const & p,
                 Ztwo::charge & v,
                 std::vector<std::size_t> size = std::vector<std::size_t>(),
                 std::vector<std::size_t> chunk = std::vector<std::size_t>(),
                 std::vector<std::size_t> offset = std::vector<std::size_t>())
{
    int t;
    ar[p] >> t;
    v = (t == 0 ? Ztwo::Plus : Ztwo::Minus);
}

template <class Archive>
inline void serialize(Archive & ar, Ztwo::charge & c, const unsigned int version)
{
    ar & c;
}

inline Ztwo::charge operator-(Ztwo::charge a) { return a; }

inline std::ostream& operator<<(std::ostream& ost, Ztwo::charge c)
{
	if (c == Ztwo::Plus)
		ost << "Plus";
	else if (c == Ztwo::Minus)
		ost << "Minus";
	else
		ost << "???";
	return ost;
}
inline std::ostream& operator<<(std::ostream& ost, const std::vector<Ztwo::charge> &c)
{
	ost << "[ ";
	for (std::vector<Ztwo::charge>::const_iterator it = c.begin();
		 it != c.end();
		 it++)
		ost << ", " << *it;
	ost << " ]";
	return ost;
}

#endif
