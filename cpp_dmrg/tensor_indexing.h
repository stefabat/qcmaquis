#ifndef TENSOR_INDEXING_H
#define TENSOR_INDEXING_H

// I think these are better placed in the global namespace
// lookup would save us for T==Index, but not for T==std::size_t, which we also need

template<class T> boost::array<T,1> _(const T &a)
{ 
    boost::array<T,1> r = {a};
    return r;
}

inline boost::array<Index, 2> operator,(const Index a, const Index b)
{
	boost::array<Index, 2> ret;
	ret[0] = a;
	ret[1] = b;
	return ret;
}

template<class T, int in>
boost::array<T, in+1> operator,(const boost::array<T, in> a, const T b)
{
	boost::array<T, in+1> ret;
	for (int i = 0; i < in; i++)
		ret[i] = a[i];
	ret[in] = b;
	return ret;
}

template<class T, int in>
boost::array<T, in+1> operator,(const T a, const boost::array<T, in> b)
{
	boost::array<T, in+1> ret;
	ret[0] = a;
	for (int i = 0; i < in; i++)
		ret[i+1] = b[i];
	return ret;
}

namespace tensor {
    enum Index { /* s1, s2, ... */ };

    template<class SymmGroup>
    class DIndex
    {
    public:
        typedef typename SymmGroup::charge charge;
        typedef typename SymmGroup::charge_v charge_v;
        typedef std::vector<std::size_t> dim_v;

        charge_v charges;
        dim_v dims;

        DIndex();
        // construct with only singlet charge
        DIndex(std::size_t);
        // construct with equal dimension in each sector
        DIndex(int, const charge_v &_ch);
        // provide everything
        DIndex(const std::vector<int> &_dims, const charge_v &_ch);

        bool operator==(const DIndex<SymmGroup> &o);
    };
} // namespace tensor

#endif
