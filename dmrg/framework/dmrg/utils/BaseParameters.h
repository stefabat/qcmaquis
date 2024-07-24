/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */

#if !defined(BASEPARAMETERS_H) && !defined(DMRGPARAMETERS_H)
#define BASEPARAMETERS_H

#include "utils/io.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <memory>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

#include "dmrg/utils/parameter_proxy.h"

namespace parameters {
    class value {
    public:
        value () : val_(""), empty_(true) { }

        template <class T>
        value (const T & val)
        : val_(boost::lexical_cast<std::string>(val))
        , empty_(false)
        { }

        std::string get() const {return val_;}
        bool empty() const {return empty_;}
    private:
        std::string val_;
        bool empty_;

    };
}

class BaseParameters
{
    public:

        typedef std::map<std::string, std::string> map_t;
        typedef typename map_t::value_type value_type;

        BaseParameters();
        BaseParameters(const BaseParameters & p);
        BaseParameters(std::istream& param_file);

        ~BaseParameters();

        // BaseParameters(alps::Parameters const& p);

        std::list<value_type> get_range() const;

        template <class Stream>
        void print_description(Stream& os) const;

        bool is_set (std::string const & key) const;
        bool defined (std::string const & key) const;
        bool empty () const;

        parameters::proxy operator[](std::string const& key);

        template<class T> T get(std::string const & key);

        template<class T> void set(std::string const & key, T const & value);
        void set(std::string const & key, const char value[]);
        void erase(std::string const & key);

        // Erase all keys containing a substring
        void erase_regex(std::string const & regex);

        // Erase all measurements
        void erase_measurements() { erase_regex("^MEASURE"); }

        // Return all meaurements
        BaseParameters measurements() const;

        // Return measurements that require 2U1 group (not SU2)
        BaseParameters twou1_measurements() const;

        // Parameters that correspond to iterations
        BaseParameters iteration_params(std::string const & var, std::size_t val);

        // Append parameters to another parameters object
        BaseParameters & operator<<(BaseParameters const& p);

        template<class T> BaseParameters & add(std::string const & key, T const & value);

        friend void swap(BaseParameters& lhs, BaseParameters& rhs)
        {
            using std::swap;
            swap(lhs.defaults, rhs.defaults);
            swap(lhs.descriptions, rhs.descriptions);
            lhs.impl_.swap(rhs.impl_);
        }

        BaseParameters& operator=(const BaseParameters& rhs);
        BaseParameters& operator=(BaseParameters&& rhs);

        // for Boost::serialization
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        template<class Archive>
        void load(Archive& ar);

        template<class Archive>
        void save(Archive& ar) const;


    protected:
        friend class boost::serialization::access;

        friend std::ostream& operator<<(std::ostream& os, const BaseParameters& p);

        void add_option(std::string const & name,
                        std::string const & desc,
                        parameters::value const & val = parameters::value());

        map_t defaults;
        map_t descriptions;

        class Impl;
        std::unique_ptr<Impl> impl_;
};


#endif
