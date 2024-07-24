/**
 * @file
 * @copyright This code is licensed under the 3-clause BSD license.
 *            Copyright ETH Zurich, Laboratory of Physical Chemistry, Reiher Group.
 *            See LICENSE.txt for details.
 */
#ifndef CI_GENERATOR_HPP
#define CI_GENERATOR_HPP

#include <math.h>
#include <algorithm>
#include <alps/numeric/matrix.hpp>

#include "dmrg/models/lattice/lattice.h"
#include "dmrg/models/measurements/chementropy.h"
#include "determinant.hpp"


//function to get orb under consideration
std::vector<int>  el_casv(const int &side, const int &left, std::vector<int> &casv){
   int len = casv.size();
   std::vector<int> casv_el;
   for(int i = 0; i<len; i++){
   //distuinguish left (side=0) and right(=1) part
      if(side==0){
         if(casv[i]>left-1){casv_el.push_back(casv[i]);}
      } else{
         if(casv[i]<left){casv_el.push_back(casv[i]);}
      }
   }
  return casv_el;
}

//function to copy determinants
template <class SymmGroup>
std::vector<Determinant<SymmGroup> > copy_det(std::vector<Determinant<SymmGroup> > dets){
   int end = dets.size();
   for(int i= 0; i<3; i++){    //number of copies
      for(int j =0;j<end;j++){ //number of already existing determinants
         dets.push_back(dets[j]);
      }
   }
   return dets;
}

//function to actually perform deas
template <class SymmGroup>
std::vector<Determinant<SymmGroup> > deas(const int &pos, const int &act_orb, std::vector<Determinant<SymmGroup> > dets)
{
   std::vector<int> orb_list;
   for(int i=0;i<4;i++){orb_list.push_back(i+1);}
   orb_list.erase(orb_list.begin()+dets[0][act_orb]-1);
   for(int i=0; i<3; i++){ 					//loop over possibilities
      for(int j=0; j<pow(4,pos); j++){				//loop over blocks
         dets[pow(4,pos)*(i+1)+j][act_orb] = orb_list[i];
      }
   }
   return dets;
}

//some functions to get info on left part
std::vector<int> get_half_filled(int nelec_left, int left){
   std::vector<int> half_filled;
   if(nelec_left>left){nelec_left=left-(nelec_left-left);}
   if(nelec_left>8){
     if(nelec_left%2==0){nelec_left=8;}
     else{nelec_left=7;}
   }
   for(int i = nelec_left; i>=0; i-=2){
      half_filled.push_back(i);
   }
   return half_filled;
}
//function to reduce the symmetry vector - here actually no pair is required since later only first entry is used
std::vector<std::pair<int,int> > reduce_symvec(const std::vector<int> &symvec_left){
   std::vector<std::pair<int,int> > symvec_red;
   int size  = 0;
   for(int i = 0; i<8; i++){
      size = 0;
      for(int j = 0; j<symvec_left.size(); j++){
         if(symvec_left[j]==i){size++;}
      }
      if(size>0){symvec_red.push_back(std::make_pair(i,size));}
   }
   return symvec_red;
}


//function to determine occupied orbitals in det
std::vector<std::pair<int, int> > get_orb(std::vector<int> hf_occ){
    std::vector<std::pair<int, int> > occ_orb;
    for(int i = 0; i < hf_occ.size(); i++)
        if(hf_occ[i] != 1)
            occ_orb.push_back(std::make_pair(i, hf_occ[i]));

   return occ_orb;
}


template <class SymmGroup>
std::vector<Determinant<SymmGroup> > generate_deas(Determinant<SymmGroup> const & hf_occ,
                                                   std::vector<int> const & casv,
                                                   int &run,
                                                   std::vector<Determinant<SymmGroup> > &deas_dets)
{
    alps::numeric::matrix<int> prd(8,8,0);
    prd(0,1)=prd(2,3)=prd(4,5)=prd(6,7)=1;
    prd(0,2)=prd(1,3)=prd(4,6)=prd(5,7)=2;
    prd(0,3)=prd(1,2)=prd(4,7)=prd(5,6)=3;
    prd(0,4)=prd(1,5)=prd(2,6)=prd(3,7)=4;
    prd(0,5)=prd(1,4)=prd(2,7)=prd(3,6)=5;
    prd(0,6)=prd(1,7)=prd(2,4)=prd(3,5)=6;
    prd(0,7)=prd(1,6)=prd(2,5)=prd(3,4)=7;
    for(int i=0;i<8; i++){
       for(int j=0; j<i;j++){
          prd(i,j)=prd(j,i);
       }
    }

    for (int i=0; i < hf_occ.size(); ++i)
        maquis::cout << hf_occ[i] << " ";
    maquis::cout << std::endl;
    for (int i=0; i < casv.size(); ++i)
        maquis::cout << casv[i] << " ";
    maquis::cout << std::endl;

    int act_orb = casv[0];
    if(run == 0){
        deas_dets.push_back(hf_occ);
        maquis::cout << deas_dets.size() << std::endl;
        int count = 0;
        for(int i = 1; i < 5; i++){
            if(hf_occ[act_orb] != i){
                deas_dets.push_back(hf_occ);
                maquis::cout << deas_dets.size() << std::endl;
                count++;
                deas_dets[count][act_orb] = i;
            }
        }
    }
    else if (run != 0){

        assert(deas_dets.size() == pow(4,run));
        std::vector<Determinant<SymmGroup> > new_deas;

        deas_dets = copy_det(deas_dets);
        act_orb = casv[run];
        deas_dets = deas(run,act_orb,deas_dets);
    }

    //insert ci check here
    return deas_dets;
}

#endif