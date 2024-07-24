/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */


#include "BaseParameters.h"
#include <alps/parameter.h>
#include <boost/serialization/list.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

    class BaseParameters::Impl : public alps::Parameters { typedef alps::Parameters base; using base::base; };

    BaseParameters::BaseParameters() : impl_(new Impl()) {}
    BaseParameters::BaseParameters(const BaseParameters& p) : defaults(p.defaults), descriptions(p.descriptions), impl_(new Impl(*(p.impl_))) {}

    BaseParameters::BaseParameters(std::istream& param_file)
    {
        try {
            impl_ = std::unique_ptr<Impl>(new Impl(param_file));
        } catch (std::exception & e) {
            maquis::cerr << "Exception thrown when parsing parameters:" << std::endl;
            maquis::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    BaseParameters& BaseParameters::operator=(const BaseParameters& rhs)
    {
        BaseParameters tmp(rhs);
        swap(*this, tmp);
        return *this;
    }

    BaseParameters& BaseParameters::operator=(BaseParameters&& rhs)
    {
        swap(*this, rhs);
        return *this;
    }

    BaseParameters::~BaseParameters() = default;

    template <class Stream>
    void BaseParameters::print_description(Stream& os) const
    {
        typedef std::map<std::string, std::string>::const_iterator map_iterator;

        std::size_t column_length = 0;
        for (map_iterator it = descriptions.begin(); it != descriptions.end(); ++it)
            column_length = std::max(column_length, it->first.size());
        column_length += 2;

        for (map_iterator it = descriptions.begin(); it != descriptions.end(); ++it) {
            os << std::setw(column_length) << std::left << it->first << it->second << std::endl;
            map_iterator matched = defaults.find(it->first);
            if (matched != defaults.end())
                os << std::setw(column_length) << "" << "(default: " << matched->second << ")" << std::endl;
        }
    }

    bool BaseParameters::is_set(std::string const & key) const
    {
        return impl_->defined(key);
    }

    bool BaseParameters::defined(std::string const & key) const
    {
        return impl_->defined(key);
    }

    bool BaseParameters::empty() const
    {
        return impl_->empty();
    }

    parameters::proxy BaseParameters::operator[](std::string const& key)
    {
        if (!is_set(key)) {
            std::map<std::string, std::string>::const_iterator match = defaults.find(key);
            if (match != defaults.end())
                impl_->operator[](key) = match->second;
            else
                boost::throw_exception(std::runtime_error("parameter " + key + " not defined"));
        }
        return parameters::proxy(impl_->operator[](key));
    }

    template<class T>
    T BaseParameters::get(std::string const & key)
    {
        parameters::proxy const& p = (*this)[key];
        return p.as<T>();
    }

    template<class T>
    void BaseParameters::set(std::string const & key, const T& value)
    {
        impl_->operator[](key) = boost::lexical_cast<std::string>(value);
    }

    // overload instead of specialization for char*
    void BaseParameters::set(std::string const & key, const char value[])
    {
        impl_->operator[](key) = std::string(value);
    }

    void BaseParameters::erase(std::string const & key)
    {
        if (this->is_set(key)) // if parameter is not set, do nothing
            impl_->erase(key);
    }

    void BaseParameters::erase_regex(std::string const & regex)
    {
        std::vector<std::string> keys_toerase;

        std::regex expression(regex);
        for (auto&& k : *impl_)
            if(std::regex_search(k.key(), expression))
                keys_toerase.push_back(k.key());

        for (auto&& k_erase : keys_toerase)
                impl_->erase(k_erase);
    }

    BaseParameters BaseParameters::measurements() const
    {
        BaseParameters p;
        const std::string regex = "^MEASURE";
        std::regex expression(regex);

        for (auto&& k : *impl_)
            if(std::regex_search(k.key(), expression))
                p.set(k.key(), k.value());

        return p;
    }

    BaseParameters BaseParameters::twou1_measurements() const
    {
        // Get all measurements
        BaseParameters p = measurements();

        // For now, the SU2-enabled measurements are hard-coded
        // and set to 1/2-RDM (this also matches the corresponding
        // transition DMs)
        const std::string regex = "1rdm|2rdm";
        // Erase SU2-enabled measurements
        p.erase_regex(regex);

        return p;
    }

    BaseParameters BaseParameters::iteration_params(std::string const & var, std::size_t val)
    {
        BaseParameters p;

        std::regex expression("^(.*)\\[" + var + "\\]$");
        std::smatch what;
        for (alps::Parameters::const_iterator it=impl_->begin();it != impl_->end();++it) {
            std::string key = it->key();
            if (std::regex_match(key, what, expression)) {
                std::vector<std::string> v = (*this)[key]; // use std::strign instead of value type, because value type is some alps internal type that can anyway be constructed from string.
                if (val < v.size())
                    p.set(what.str(1), v[val]);
                else
                    p.set(what.str(1), *(v.rbegin()));
            }
        }
        p.set(var, val);
        return p;
    }


    BaseParameters & BaseParameters::operator<<(BaseParameters const& p)
    {
        for (alps::Parameters::const_iterator it=p.impl_->begin(); it!=p.impl_->end(); ++it)
            impl_->operator[](it->key()) = it->value();
        defaults.insert(p.defaults.begin(), p.defaults.end());

        return *this;
    }

    template<class T>
    BaseParameters& BaseParameters::add(std::string const & key, const T& value)
    {
        impl_->operator[](key) = boost::lexical_cast<std::string>(value);
        return *this;
    }


    void BaseParameters::add_option(std::string const & name,
                                    std::string const & desc,
                                    parameters::value const & val)
    {
        if (!val.empty())
            defaults[name] = val.get();
        descriptions[name] = desc;
    }

    // for now, an implementation that copies the whole range. consider using references
    std::list<typename BaseParameters::value_type> BaseParameters::get_range() const
    {
        std::list<typename BaseParameters::value_type> ret;

        for (auto&& l: *impl_)
            ret.push_back(std::make_pair(l.key(), l.value()));

        return ret;
    }

    template<class Archive>
    void BaseParameters::load(Archive& ar)
    {
        impl_->load(ar);
    }

    template<class Archive>
    void BaseParameters::save(Archive& ar) const
    {
        impl_->save(ar);
    }

    template void BaseParameters::load<alps::hdf5::archive>(alps::hdf5::archive& ar);
    template void BaseParameters::save<alps::hdf5::archive>(alps::hdf5::archive& ar) const;

    #define INSTANTIATE_TEMPLATE_FUNCTIONS(r,d,T) \
    template T BaseParameters::get<T>(std::string const & key); \
    template void BaseParameters::set<T>(std::string const & key, T const & value); \
    template BaseParameters& BaseParameters::add<T>(std::string const & key, T const & value);

    #define BASE_PARAMETERS_TYPES (double) (int) (unsigned long) (std::string)

    BOOST_PP_SEQ_FOR_EACH(INSTANTIATE_TEMPLATE_FUNCTIONS, _, BASE_PARAMETERS_TYPES)

    #undef BASE_PARAMETERS_TYPES
    #undef INSTANTIATE_TEMPLATE_FUNCTIONS

    template std::vector<int> BaseParameters::get<std::vector<int> >(std::string const & key);
    template std::vector<unsigned long> BaseParameters::get<std::vector<unsigned long> >(std::string const & key);

    template void BaseParameters::print_description(std::ostream& os) const;

    std::ostream& operator<<(std::ostream& os, const BaseParameters& p)
    {
        os << *p.impl_;
        return os;
    }