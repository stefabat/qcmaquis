/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */
#ifndef MPS_INIT_DEAS_HPP
#define MPS_INIT_DEAS_HPP

#include "dmrg/models/lattice/lattice.h"
#include "alps/numeric/matrix.hpp"
#include "dmrg/models/chem/util.h"
#include "dmrg/utils/DmrgParameters.h"
#include "dmrg/mp_tensors/charge_detail.h"
#include "dmrg/models/chem/cideas/ci_generator.hpp"

template<class Matrix, class SymmGroup, class=void>
struct deas_mps_init : public mps_initializer<Matrix,SymmGroup>
{

    typedef Lattice::pos_t pos_t;
    typedef std::size_t size_t;
    typedef typename SymmGroup::charge charge;
    typedef std::vector<Index<SymmGroup> > index_vec;
    typedef std::vector<typename SymmGroup::subcharge> site_vec;
    typedef typename Matrix::value_type value_type;
    typedef typename maquis::traits::real_type<value_type>::type real_type;

    deas_mps_init(DmrgParameters parms_,
                  const Matrix& s1_,
                  std::vector<Index<SymmGroup> > const& phys_dims_,
                  typename SymmGroup::charge right_end_,
                  std::vector<int> const& site_type)
    : parms(parms_)
    , s1(s1_)
    , phys_dims(phys_dims_)
    , site_types(site_type)
    , di(parms, phys_dims_, right_end_, site_type)
    , right_end(right_end_)
    , total_dets()
    {
        using entanglement_detail::comp;
        //using entanglement_detail::mpair;
        typedef std::pair<real_type, int> mpair;

        int L = parms["L"];
        cas_vector.resize(L);

        Determinant<SymmGroup> hf_unordered(parms.get<std::vector<int> >("hf_occ"));

        std::vector<int> order(hf_unordered.size());
        if (!parms.is_set("orbital_order"))
            for (int p = 0; p < hf_unordered.size(); ++p)
                order[p] = p+1;
        else
            order = parms["orbital_order"].template as<std::vector<int> >();

        std::transform(order.begin(), order.end(), order.begin(), boost::lambda::_1-1);
        std::vector<int> inv_order(L);
        for (int p = 0; p < order.size(); ++p)
            inv_order[p] = std::distance(order.begin(), std::find(order.begin(), order.end(), p));

        std::copy(order.begin(), order.end(), std::ostream_iterator<int>(std::cout, " "));
        maquis::cout << std::endl;

        for (int i = 0; i < order.size(); ++i)
            hf_occ.push_back(hf_unordered[order[i]]);

        std::cout << "hf_occ: ";
        std::copy(hf_occ.begin(), hf_occ.end(), std::ostream_iterator<int>(std::cout, " "));
        maquis::cout << std::endl;

        std::vector<mpair> casv_sort(L);
        for(int i = 0; i < L; i++){
            casv_sort[i].first = s1(0,i);
            casv_sort[i].second = inv_order[i];
        }

        std::sort(casv_sort.begin(),casv_sort.end(), comp<mpair>);
        for(int i = 0; i < L; i++)
            maquis::cout << casv_sort[i].first << " " << casv_sort[i].second << std::endl;

        ///////////////////////////////////////////////////////////////////////
        for(int i = 0; i < L; i++){
            cas_vector[i] = casv_sort[i].second;
        }

        std::cout << "CAS vector: ";
        for(int i =0; i < L; i++)
            std::cout << cas_vector[i] << " ";
        std::cout <<std::endl;
    }

    void operator()(MPS<Matrix, SymmGroup> & mps)
    {
        pos_t L = mps.length();

        charge doubly_occ = phys_dims[0].begin()->first, empty = phys_dims[0].rbegin()->first;

        std::vector<int> ci_level(parms.get<std::vector<int> >("ci_level"));
        if(std::find(ci_level.begin(), ci_level.end(), 0) == ci_level.end())
           ci_level.push_back(0);

        std::vector<std::pair<int,int> > hf_occ_orb = get_orb(hf_occ);
        int m_value = parms.get<int>("init_bond_dimension");
        if (hf_occ.size() != L)
            throw std::runtime_error("HF occupation vector length != MPS length\n");

        // initialize objects required
        //idea: include current charges in rows_to_fill, should not affect 2U1
        std::vector<std::vector< int > > rows_to_fill;
        std::vector<std::map<charge, std::map<std::string, int> > > str_to_col_map(L);

        //another loop has to be inserted here where determinants are generated until m is reached at some site; correct numbers have to be checked here
        std::vector<Determinant<SymmGroup> > det_list_new, deas_dets;
        std::vector<int> sum_size(L);
        bool keep_running = true;
        int det_nr = 0;
        int num_runs = std::min(L,15);
        //main loop
        for(int run = 0; run < num_runs; ++run)
        {
            std::vector<std::vector<charge> >  determinants;
            std::vector<Determinant<SymmGroup> > new_det_list;

            //generate deas determinants
            deas_dets = generate_deas(hf_occ, cas_vector, run, deas_dets);

            size_t loop_start = pow(4, run) - 1;
            size_t loop_end = pow(4, run + 1);
            #pragma omp parallel for
            for(int i = loop_start; i < loop_end; ++i){
                if(!deas_dets[i].ci_check(ci_level, hf_occ_orb))
                    #pragma omp critical
                    new_det_list.push_back(deas_dets[i]);
            }
            //convert to charge_vec -> determinants
            std::cout << "size of new_det_list: "<< new_det_list.size() << std::endl;
            determinants = get_charge_determinants(new_det_list, det_list_new, phys_dims, site_types, right_end);
            std::cout << "size of determinants: "<< determinants.size() << std::endl;

            for(int d = 0; d < determinants.size(); ++d)
            {
                rows_to_fill.push_back(std::vector<int>(L));
                charge accumulated_charge = right_end;
                for(int s = L - 1; s > 0; --s)
                {
                    charge site_charge = determinants[d][s];
                    accumulated_charge = SymmGroup::fuse(accumulated_charge, -site_charge);
                    if(ChargeDetailClass<SymmGroup>::physical(accumulated_charge))
                    {
                        std::string str = det_string(s, det_list_new[det_nr]);
                        std::map<std::string, int> & str_map = str_to_col_map[s-1][accumulated_charge];

                        if (str_map[str])
                            rows_to_fill[det_nr][s] = str_map[str] - 1;

                        else
                        {
                           //get largest element in map
                           int max_value = str_map.size();
                           str_map[str] = max_value;
                           rows_to_fill[det_nr][s] = max_value - 1;
                           sum_size[s-1] += 1;
                        }
                    }
                    if(sum_size[s-1] >= m_value){
                        keep_running = false;
                        break;
                    }
                }
                det_nr ++;
                total_dets.push_back(determinants[d]);
                if(keep_running == false)
                    break;
            }
            if(keep_running == false)
                break;
        }
        //this now calls a function which is part of this structure
        std::cout <<"THE NUMBER OF SUITABLE DETS: " << det_nr <<std::endl;
        std::cout << "sum of sector sizes: " ;
        for (int i = 0; i < L; ++i)
            std::cout << sum_size[i] << " ";
        std::cout << std::endl;

        init_sect(mps, str_to_col_map, true, 0);

        //this here is absolutely necessary
        for(pos_t i = 0; i < L; ++i)
            mps[i].multiply_by_scalar(0.0);

        //fill loop
        for(int d = 0; d < total_dets.size(); ++d)
        {
            charge accumulated_charge = right_end;
            int prev_row = 0;
            for(int s = L - 1; s > 0; --s)
            {
                charge site_charge = total_dets[d][s];
                charge search_charge = SymmGroup::fuse(accumulated_charge, -site_charge);
                if (ChargeDetailClass<SymmGroup>::physical(search_charge) && mps[s].row_dim().has(search_charge))
                {
                    int nrows_fill = mps[s].row_dim().size_of_block(search_charge);
                    //get current matrix
                    size_t max_pos = mps[s].data().left_basis().position(accumulated_charge);
                    Matrix & m_insert = mps[s].data()[max_pos];

                    int nrows = m_insert.num_rows(), off = 0;

                    //get additional offsets for subsectors
                    if(site_charge == phys_dims[site_types[s]][1].first)
                    {
                        if(mps[s].row_dim().has(SymmGroup::fuse(accumulated_charge, -doubly_occ)))
                            off =  mps[s].row_dim().size_of_block(SymmGroup::fuse(accumulated_charge, -doubly_occ));
                    }
                    else if(site_charge == phys_dims[site_types[s]][2].first)
                    {
                        if(mps[s].row_dim().has(accumulated_charge))
                            off = nrows - nrows_fill - mps[s].row_dim().size_of_block(accumulated_charge);
                        else
                            off = nrows - nrows_fill;
                    }
                    else if(site_charge == phys_dims[site_types[s]][3].first){
                        off = nrows - nrows_fill;
                    }
                    //actual insertion
                    m_insert(off+rows_to_fill[d][s],prev_row) = 1;
                    prev_row = rows_to_fill[d][s];
                    accumulated_charge = search_charge;
                }
            }
       }
       std::cout << "fill worked" << std::endl;

       //first site needs to be filled as well
       for(int d = 0; d < total_dets.size(); d++){
          charge first_charge = total_dets[d][0];
          if (ChargeDetailClass<SymmGroup>::physical(first_charge))
          {
             size_t first_pos = mps[0].data().left_basis().position(first_charge);
             Matrix & m_first = mps[0].data()[first_pos];
             m_first(0,0) = 1;
          }
       }

    }//end of main initialization function


    //function to get string of left or right part from det
    std::string det_string(int s, Determinant<SymmGroup> det){
       std::string str;
       char c;
       int L = det.size();
       if(s > L/2){
          for(int i = s; i<det.size(); i++){
             c = boost::lexical_cast<char>(det[i]);
             str.push_back(c);
          }
       }else{
          for(int i = 0; i<s; i++){
             c = boost::lexical_cast<char>(det[i]);
             str.push_back(c);
          }
       }
    return str;
    }

    //function to initalize sectors -> copied from mps-initializers

    void init_sect(MPS<Matrix, SymmGroup> & mps,
                   const std::vector<std::map<charge, std::map<std::string, int> > > & str_to_col_map,
                   bool fillrand = true,
                   typename Matrix::value_type val = 0)
    {
        parallel::scheduler_balanced scheduler(mps.length());
        std::size_t L = mps.length();

        std::vector<Index<SymmGroup> > allowed = allowed_sectors(site_types, phys_dims, right_end, 5);
        maquis::cout << "allowed sectors created" << std::endl;
        allowed = adapt_allowed(allowed, str_to_col_map);
        maquis::cout << "size of sectors succesfully adapted" << std::endl;

        omp_for(size_t i, parallel::range<size_t>(0,L), {
            parallel::guard proc(scheduler(i));
            mps[i] = MPSTensor<Matrix, SymmGroup>(phys_dims[site_types[i]], allowed[i], allowed[i+1], fillrand, val);
            mps[i].divide_by_scalar(mps[i].scalar_norm());
        });
    }

    std::vector<Index<SymmGroup> > adapt_allowed(std::vector<Index<SymmGroup> > allowed,
                                                     const std::vector<std::map<charge, std::map<std::string, int > > > &str_to_col_map)
    {
        std::vector<Index<SymmGroup> > adapted = allowed;
        pos_t L = str_to_col_map.size();
        for(pos_t i = 1; i < L+1; ++i)
        {
            for(typename Index<SymmGroup>::iterator it = adapted[i].begin();
                     it != adapted[i].end(); ++it)
            {
                if (str_to_col_map[i-1].count(it->first) != 0)
                    it->second = str_to_col_map[i-1].at(it->first).size();
            }
        }
        return adapted;
    }

private:
    DmrgParameters parms;
    Matrix s1;
    std::vector<Index<SymmGroup> > phys_dims;
    std::vector<typename SymmGroup::subcharge> site_types;
    default_mps_init<Matrix, SymmGroup> di;
    std::vector<std::vector<charge> >  total_dets;
    charge right_end;
    Determinant<SymmGroup> hf_occ;
    std::vector<int> cas_vector;

    Determinant<SymmGroup> str_from_det(std::vector<typename SymmGroup::charge> const &charge_vec, std::vector<Index<SymmGroup> > const &phys_dims, std::vector<typename SymmGroup::subcharge> const &site_types)
    {
        Determinant<SymmGroup> det(charge_vec.size());
        for (int i = 0; i < charge_vec.size(); ++i)
        {
        // TODO: maybe it should work like this?
        // for (int j = 0; j < 3; j++)
        //     if (charge_vec[i] == phys_dims[site_types[i]][j].first)
        //         det[i] = 4-j;
            if (charge_vec[i] == phys_dims[site_types[i]][0].first)
            {
                det[i] = 4;
            }else if (charge_vec[i] == phys_dims[site_types[i]][1].first)
            {
                det[i] = 3;
            }else if (charge_vec[i] == phys_dims[site_types[i]][2].first) // singly-occ (2)
            {
                det[i] = 2;
            }else if (charge_vec[i] == phys_dims[site_types[i]][3].first)
            {
                det[i] = 1;
            }
        }
        return det;
}


    std::vector<std::vector<typename SymmGroup::charge> >
    get_charge_determinants(std::vector<Determinant<SymmGroup> > const & det_list,
                            std::vector<Determinant<SymmGroup> > &det_list_new,
                            std::vector<Index<SymmGroup> > const &phys_dims,
                            std::vector<typename SymmGroup::subcharge> site_types,
                            typename SymmGroup::charge right_end)
    {
        // convert determinant strings into charge vectors
        // and generate all SU2 possibilites at singly occupied sites

        typedef typename SymmGroup::charge charge;
        std::vector<std::vector<charge> > determinants;
        std::vector< std::vector<std::vector< charge > > > dummy_dets;
        // convert det_list to vec<vec<charge>>
        for (size_t i = 0; i < det_list.size(); ++i)
            dummy_dets.push_back(det_list[i].charge_det(phys_dims, site_types));
        std::cout << "size of dummy_dets: "<< dummy_dets.size() <<std::endl;
        int L = dummy_dets[0].size();
        #pragma omp parallel for
        for (int i = 0; i<dummy_dets.size(); ++i)
        {
            std::vector<std::vector<charge> > single_det(1);
            for (int j = 0; j < det_list[i].size(); ++j)
            {
                if (dummy_dets[i][j].size() != 1)
                {
                    int times = single_det.size();
                    for (int k = 0; k < times; k++)
                        single_det.push_back(single_det[k]);

                    for (int k = 0; k < single_det.size(); ++k)
                    {
                        if (k < single_det.size()/2) //works only if there are two options, like (1,1) and (1,-1)
                            single_det[k].push_back(dummy_dets[i][j][0]);
                        else
                            single_det[k].push_back(dummy_dets[i][j][1]);
                    }
                }
                else{
                    for (int k = 0; k < single_det.size(); ++k)
                        single_det[k].push_back(dummy_dets[i][j][0]);
                }
            }
            for (int m = 0; m < single_det.size(); ++m)
            {
                bool valid = true;
                charge accumulated_charge = single_det[m][0];
                for (int l = 1; l < L; ++l)
                {
                    accumulated_charge = SymmGroup::fuse(accumulated_charge, single_det[m][l]);
                    if (!ChargeDetailClass<SymmGroup>::physical(accumulated_charge))
                        valid = false;
                }
                //letzte Bedingung kann später geloescht werden
                if(valid && accumulated_charge == right_end
                                && std::find(determinants.begin(),
                                determinants.end(), single_det[m]) == determinants.end())
                {
                    #pragma omp critical
                    {
                    determinants.push_back(single_det[m]);
                    Determinant<SymmGroup> det_str = str_from_det(single_det[m], phys_dims, site_types);
                    det_list_new.push_back(det_str);
                    }
                }
            }
            single_det.clear();
        }
        return determinants;
    }
};

#endif