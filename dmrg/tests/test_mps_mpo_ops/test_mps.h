/*****************************************************************************
*
* ALPS MPS DMRG Project
*
* Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
*               2011-2011 by Bela Bauer <bauerb@phys.ethz.ch>
*               2011-2013    Michele Dolfi <dolfim@phys.ethz.ch>
*               2014-2014    Sebastian Keller <sebkelle@phys.ethz.ch>
*               2018         Leon Freitag <lefreita@ethz.ch>
*
* This software is part of the ALPS Applications, published under the ALPS
* Application License; you can use, redistribute it and/or modify it under
* the terms of the license, either version 1 or (at your option) any later
* version.
*
* You should have received a copy of the ALPS Application License along with
* the ALPS Applications; see the file LICENSE.txt. If not, the license is also
* available from http://alps.comp-phys.org/.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#ifndef TEST_MPS_H
#define TEST_MPS_H

// Test MPS rotation: MPS based on OpenMOLCAS test qcmaquis:007
// CH2, triplet and singlet, 6 electrons in 6 orbitals/ANO-RCC-MB
// CH2 Triplet coordinates in Angstrom
// C 0.000000 0.000000 0.000000
// H 0.000000 0.000000 1.077500
// H 0.784304 0.000000 -0.738832

template <class grp>
    struct Fixture
    {
        // This MPS came out of the OpenMOLCAS DMRGSCF optimisation of the T1 state
        MPS<matrix, grp> mps =
        {
            MPSTensor<matrix,grp>(
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // phys_i
                { {{0,0,0},1} },                                        // left_i
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // right_i
                block_matrix<matrix,grp>(
                        { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // rows
                        { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // cols
                        {
                        matrix(1, 1, {-0.98761145122206129}),
                        matrix(1, 1, {-0.14162250971811657}),
                        matrix(1, 1, {-0.14162250971811657}),
                        matrix(1, 1, {-0.067577260644360676})
                        } // data
                    )
                ), // mps[0]
            MPSTensor<matrix,grp>(
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // phys_i
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // left_i
                { {{2,2,0},1}, {{2,1,0},2}, {{2,0,0},1}, {{1,2,0},2},
                {{1,1,0},4}, {{1,0,0},2}, {{0,2,0},1}, {{0,1,0},2},
                {{0,0,0},1} }, // right_i
                block_matrix<matrix,grp>(
                        { {{2,2,0},1}, {{2,1,0},2}, {{2,0,0},1}, {{1,2,0},2},
                        {{1,1,0},4}, {{1,0,0},2}, {{0,2,0},1}, {{0,1,0},2},
                        {{0,0,0},1} }, // rows
                        { {{2,2,0},1}, {{2,1,0},2}, {{2,0,0},1}, {{1,2,0},2},
                        {{1,1,0},4}, {{1,0,0},2}, {{0,2,0},1}, {{0,1,0},2},
                        {{0,0,0},1} }, // cols
                        {
                        matrix(1, 1, {0.99593907511899671}),
                        matrix(2, 2, {-0.86191004558575357, -4.6002047033774667e-08, // column-major order so this looks transposed!
                                        -1.1092845129179752e-07, 0.042738578511034929}),
                        matrix(1, 1, {0.16680163900950651}),
                        matrix(2, 2, {-0.86191004558575357, -4.6002047033774667e-08,
                                        -1.1092845129179752e-07, 0.042738578511034929}),
                        matrix(4, 4, {0, 0.11794657005665261, 0.11794657005665261, 0,
                                        -0.84624527865506782, -4.189254585009221e-07, 4.189254585009221e-07, -0.074475287186416703,
                                        9.9360377841658186e-07, -0.33609788758063924, 0.33609788758063924, 2.4892311701994896e-08,
                                        0.50863130999437023, 1.2245198253793157e-07, -1.2245198253793157e-07, -0.027059271174061181}),
                        matrix(2, 2, {-7.1104467702987106e-07, 0.05800608993260413,
                                        0.11784371620441166, 3.2890185339773664e-07}),
                        matrix(1, 1, {0.16680163900950651}),
                        matrix(2, 2, {-7.1104467702987106e-07, 0.05800608993260413,
                                        0.11784371620441166, 3.2890185339773664e-07}),
                        matrix(1, 1, {0.10618840519123873})
                        }
                    )
                ), // mps[1]
            MPSTensor<matrix,grp>(
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // phys_i
                { {{2,2,0},1}, {{2,1,0},2}, {{2,0,0},1}, {{1,2,0},2},
                    {{1,1,0},4}, {{1,0,0},2}, {{0,2,0},1}, {{0,1,0},2},
                    {{0,0,0},1} }, // left_i
                { {{3,3,0},1}, {{3,2,0},3}, {{3,1,0},3}, {{3,0,0},1},
                    {{2,3,0},3}, {{2,2,0},9}, {{2,1,0},9}, {{2,0,0},3},
                    {{1,3,0},3}, {{1,2,0},9}, {{1,1,0},9}, {{1,0,0},3},
                    {{0,3,0},1}, {{0,2,0},3}, {{0,1,0},3}, {{0,0,0},1} }, // right_i
                block_matrix<matrix,grp>(
                    { {{3,3,0},1}, {{3,2,0},3}, {{3,1,0},3}, {{3,0,0},1},
                        {{2,3,0},3}, {{2,2,0},9}, {{2,1,0},9}, {{2,0,0},3},
                        {{1,3,0},3}, {{1,2,0},9}, {{1,1,0},9}, {{1,0,0},3},
                        {{0,3,0},1}, {{0,2,0},3}, {{0,1,0},3}, {{0,0,0},1} }, // rows
                    { {{3,3,0},1}, {{3,2,0},3}, {{3,1,0},3}, {{3,0,0},1},
                        {{2,3,0},3}, {{2,2,0},9}, {{2,1,0},9}, {{2,0,0},3},
                        {{1,3,0},3}, {{1,2,0},9}, {{1,1,0},9}, {{1,0,0},3},
                        {{0,3,0},1}, {{0,2,0},3}, {{0,1,0},3}, {{0,0,0},1} }, // cols
                    {
                        matrix(1,1, {0.93746101075188237}),
                        matrix(3,3, {0.79945267548720589, -9.7525062984128917e-08, 0.057926481718290561,
                                        1.374306380734629e-08, 0.23669297798685096, -2.187286966261547e-09,
                                    -5.0831906442895123e-07, 7.8902345004036946e-13, 1.0804450563575665e-07}),
                        matrix(3,3, {-0.85922687394491437, -1.5357982317124378e-09, -0.28716727592789465,
                                        1.0497825555221065e-10, -1.1514782595191607e-07, -9.8378985436422006e-11,
                                    -2.09534742539695e-08, -2.7215438828174424e-12, 1.9637299590796334e-08}),
                        matrix(1,1,  {1.0737353814975803e-07}),
                        matrix(3,3,  {0.79945267548720589, -9.7525062984128917e-08, 0.057926481718290561,
                                        1.374306380734629e-08, 0.23669297798685096, -2.187286966261547e-09,
                                        -5.0831906442895123e-07, 7.8902345004036946e-13, 1.0804450563575665e-07}),
                        matrix(9,9, {-0.85922687394491437, 0, 0, 0, -1.0859733441781734e-09, -0.20305792814348275, -1.0859733441781734e-09, -0.20305792814348275, 0,
                                        1.0497825555221065e-10, 0, 0, 0, -8.142180856948818e-08, -6.9564447728346609e-11, -8.142180856948818e-08, -6.9564447728346609e-11, 0,
                                        -2.09534742539695e-08, 0, 0, 0, -1.9244221348369805e-12, 1.3885667704843903e-08, -1.9244221348369805e-12, 1.3885667704843903e-08, 0,
                                        0, -0.39681064213452655, 6.8080633851176515e-07, 0.84794529823348896, 0.28886598334105201, 4.2817845693649786e-07, -0.28886598334105201, -4.2817845693649786e-07, 0.34321756119334562,
                                        0, 0.83912626734996909, 8.254430520207203e-07, 0.34258013901559314, 0.23578179340809122, -1.157616224477544e-06, -0.23578179340809122, 1.157616224477544e-06, 0.00093361034266240114,
                                        0, -4.8545389808849599e-07, 0.91879363105015066, -8.3868839625076861e-07, -2.9106318717234916e-07, -0.65170998856387397, 2.9106318717234916e-07, 0.65170998856387397, 1.0280587058553529e-09,
                                        0, 0.11501955249657893, -8.9668297330238715e-08, 0.32788671562170779, -0.14998938126092645, -3.2228861992541279e-08, 0.14998938126092645, 3.2228861992541279e-08, 0.0034942947797437039,
                                        0, -8.0923733655903144e-09, -1.4892892294591776e-11, 4.5633750296962747e-08, 4.2246694355350358e-09, -2.6733445576739468e-11, -4.2246694355350358e-09, 2.6733445576739468e-11, -4.1436468860991808e-10,
                                        0, -3.7397425007307549e-12, 1.012558276189554e-08, 2.1119037180377538e-11, 1.9548766499490645e-12, 1.8153793205358114e-08, -1.9548766499490645e-12, -1.8153793205358114e-08, -1.9188030785163945e-13}),
                        matrix(9,9, {0, 0, 8.7670126781390194e-08, 0, 0, 0, 6.1992141154605362e-08, 0, 0,
                                    -0.59925391448108101, -3.6599855248668926e-06, -9.0903437305423485e-08, 0.0047392808856795247, 7.9852210437783579e-08, 0.17123436500604244, 1.2855687390366225e-07, 0.19422415837098939, -9.6397904068558764e-08,
                                    -3.1190945768740547e-06, 0.52804381338319217, -0.23716039421747409, 9.0024134722138928e-08, 0.11811361298160912, -5.4386498498047103e-08, 0.33539544596010162, -2.9604390857457332e-08, -0.10025785419636195,
                                        0.33147809043562992, 1.220040408989585e-06, 5.7658790327181543e-08, 0.053493585384599966, 6.5085433805182451e-08, 0.1169811753582989, -8.1541843270726768e-08, -0.0092491140256009824, 9.4309745306253951e-08,
                                    -2.4229784988507011e-06, 0.49540247422019706, -0.021539334937028712, 1.5554747080081057e-08, -0.061929958780955918, 1.5954487614707503e-07, 0.030461219592442641, -8.2904648990140911e-09, -0.04480997755944599,
                                    -4.7145353432787614e-07, -2.4627472762671679e-09, -5.9369273566404788e-11, 7.046033026727851e-08, -9.4113623618698699e-11, -9.7877571343695798e-08, 8.3960831865848135e-11, 3.3883009849175823e-09, -6.1083594464534834e-10,
                                        4.2806229038739586e-10, 4.4557669009716591e-07, 4.1962966985006567e-08, 1.6565585673707064e-10, 2.5514175193817664e-08, -3.627582736295522e-10, -5.9344597027610713e-08, 4.4353512854682538e-11, 6.6711532908839742e-08,
                                    -5.9065786653371098e-08, 2.0617513721752685e-10, 7.3439319173942326e-11, -8.7583942306661769e-10, 2.6512338840173733e-11, 6.8124302563031476e-09, -1.0385888118723572e-10, -1.5770515717879077e-09, -4.5079910574407511e-11,
                                    -1.3940919284824835e-10, -2.9437297127523975e-09, -2.9370132701690144e-09, 4.2922499075487994e-13, -8.9201431992455463e-10, 1.117237623199455e-11, 4.1535639995427754e-09, -3.2074527921788316e-12, 3.3036625227550927e-09}),
                        matrix(3,3, {-1.0008940439030065e-06, 0.16480431836511464, 0.30265854107565943,
                                        -1.5771456145433263e-09, -7.9213089659315704e-08, 4.9018653232918252e-09,
                                        7.1238111095068528e-08, -1.4601168409437267e-09, 9.0383468842654526e-11}),
                        matrix(3,3, {-0.85922687394491437, -1.5357982317124378e-09, -0.28716727592789465,
                                        1.0497825555221065e-10, -1.1514782595191607e-07, -9.8378985436422006e-11,
                                    -2.09534742539695e-08, -2.7215438828174424e-12, 1.9637299590796334e-08}),
                        matrix(9,9, {0, 0, 6.1992141154605362e-08, 8.7670126781390194e-08, 0, 0, 0, 0, 0,
                                    -0.59925391448108101, -3.6599855248668926e-06, -1.2855687390366225e-07, 9.0903437305423485e-08, 0.0047392808856795247, 7.9852210437783579e-08, 0.17123436500604244, 0.19422415837098939, -9.6397904068558764e-08,
                                    -3.1190945768740547e-06, 0.52804381338319217, -0.33539544596010162, 0.23716039421747409, 9.0024134722138928e-08, 0.11811361298160912, -5.4386498498047103e-08, -2.9604390857457332e-08, -0.10025785419636195,
                                        0.33147809043562992, 1.220040408989585e-06, 8.1541843270726768e-08, -5.7658790327181543e-08, 0.053493585384599966, 6.5085433805182451e-08, 0.1169811753582989, -0.0092491140256009824, 9.4309745306253951e-08,
                                    -2.4229784988507011e-06, 0.49540247422019706, -0.030461219592442641, 0.021539334937028712, 1.5554747080081057e-08, -0.061929958780955918, 1.5954487614707503e-07, -8.2904648990140911e-09, -0.04480997755944599,
                                    -4.7145353432787614e-07, -2.4627472762671679e-09, -8.3960831865848135e-11, 5.9369273566404788e-11, 7.046033026727851e-08, -9.4113623618698699e-11, -9.7877571343695798e-08, 3.3883009849175823e-09, -6.1083594464534834e-10,
                                        4.2806229038739586e-10, 4.4557669009716591e-07, 5.9344597027610713e-08, -4.1962966985006567e-08, 1.6565585673707064e-10, 2.5514175193817664e-08, -3.627582736295522e-10, 4.4353512854682538e-11, 6.6711532908839742e-08,
                                    -5.9065786653371098e-08, 2.0617513721752685e-10, 1.0385888118723572e-10, -7.3439319173942326e-11, -8.7583942306661769e-10, 2.6512338840173733e-11, 6.8124302563031476e-09, -1.5770515717879077e-09, -4.5079910574407511e-11,
                                    -1.3940919284824835e-10, -2.9437297127523975e-09, -4.1535639995427754e-09, 2.9370132701690144e-09, 4.2922499075487994e-13, -8.9201431992455463e-10, 1.117237623199455e-11, -3.2074527921788316e-12, 3.3036625227550927e-09}),
                        matrix(9,9, {0, -7.0773896569304195e-07, 0.11653425108479924, -7.0773896569304195e-07, 0.11653425108479924, 0.30265854107565943, 0, 0, 0,
                                        0, -1.1152103589622108e-09, -5.6012112856840121e-08, -1.1152103589622108e-09, -5.6012112856840121e-08, 4.9018653232918252e-09, 0, 0, 0,
                                        0, 5.0372951434243588e-08, -1.0324585195559889e-09, 5.0372951434243588e-08, -1.0324585195559889e-09, 9.0383468842654526e-11, 0, 0, 0,
                                    -0.55644183644827794, 0.27389920319338074, 8.7741153854754845e-07, -0.27389920319338074, -8.7741153854754845e-07, 0, 0.34967959917606711, 1.5716698734999902e-07, 0.013329297630283666,
                                    -5.3613621687186204e-07, -2.3537059230335696e-06, 0.44381986160905179, 2.3537059230335696e-06, -0.44381986160905179, 0, 9.2927923093060951e-08, -0.37152673951454696, 1.8934759330976894e-07,
                                        0.70226852569559983, 0.42089278624497417, 2.5913731742603447e-06, -0.42089278624497417, -2.5913731742603447e-06, 0, 0.0013320179531309511, 7.3021377996920168e-10, 0.020556824148241854,
                                    -0.24154099454843109, 0.077876576328423472, 5.1513823437729922e-07, -0.077876576328423472, -5.1513823437729922e-07, 0, -0.0041597431347802699, 2.8345340696502569e-08, 0.11185644787939245,
                                        2.0230041917455727e-10, -1.4305553144687009e-10, 2.796238142662273e-08, 1.4305553144687009e-10, -2.796238142662273e-08, 0, 3.4881002898898595e-12, 9.3499376156081464e-10, 1.9543250861864144e-11,
                                        1.9839352858865977e-08, -1.401346467695389e-08, -1.5145722932255893e-10, 1.401346467695389e-08, 1.5145722932255893e-10, 0, 3.4200227294491434e-10, -5.0614682457243745e-12, 1.9166869890467789e-09}),
                        matrix(3,3, {1.3705380501137873e-08, -1.4219204639230911e-06, 0.23373813806030871,
                                        0.02992207385222433, 0.12060048583230916, 7.7929037485471503e-07,
                                    -1.3817067429633793e-07, 2.6157854328475686e-08, 1.7591292429989698e-13}),
                        matrix(1,1, {1.0737353814975803e-07}),
                        matrix(3,3, {-1.0008940439030065e-06, 0.16480431836511464, 0.30265854107565943,
                                        -1.5771456145433263e-09, -7.9213089659315704e-08, 4.9018653232918252e-09,
                                        7.1238111095068528e-08, -1.4601168409437267e-09, 9.0383468842654526e-11}),
                        matrix(3,3, {1.3705380501137873e-08, -1.4219204639230911e-06, 0.23373813806030871,
                                        0.02992207385222433, 0.12060048583230916, 7.7929037485471503e-07,
                                    -1.3817067429633793e-07, 2.6157854328475686e-08, 1.7591292429989698e-13}),
                        matrix(1,1, {0.3714216174592247})
                        }
                    )
                ), // mps[2]
            MPSTensor<matrix,grp>(
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // phys_i
                { {{3,3,0},1}, {{3,2,0},3}, {{3,1,0},3}, {{3,0,0},1},
                    {{2,3,0},3}, {{2,2,0},9}, {{2,1,0},9}, {{2,0,0},3},
                    {{1,3,0},3}, {{1,2,0},9}, {{1,1,0},9}, {{1,0,0},3},
                    {{0,3,0},1}, {{0,2,0},3}, {{0,1,0},3}, {{0,0,0},1} }, // left_i
                { {{3,3,0},1}, {{3,2,0},2}, {{3,1,0},1},
                    {{2,3,0},2}, {{2,2,0},4}, {{2,1,0},2},
                    {{1,3,0},1}, {{1,2,0},2}, {{1,1,0},1} }, // right_i
                block_matrix<matrix,grp>(
                    { {{3,3,0},16}, {{3,2,0},24}, {{3,1,0},16},
                    {{2,3,0},24}, {{2,2,0},36}, {{2,1,0},24},
                    {{1,3,0},16}, {{1,2,0},24}, {{1,1,0},16} }, // rows
                    { {{3,3,0},1}, {{3,2,0},2}, {{3,1,0},1},
                    {{2,3,0},2}, {{2,2,0},4}, {{2,1,0},2},
                    {{1,3,0},1}, {{1,2,0},2}, {{1,1,0},1} },    // cols
                    {
                        matrix(16,1, {0, 0, 0, -0.99628435117267622, -0.065427488330118147, 8.0528171502727054e-08, -0.05600656550124207, -5.788858731223935e-09, 1.0956050688023146e-10, 4.4194222291240363e-07, 1.9980193220357173e-08, 0.70710678118640913, -4.4194222291240363e-07, -1.9980193220357173e-08, -0.70710678118640913, 1}),
                        matrix(24,2, {0, -0.99470911071239376, 1.7923205286660328e-07, -0.10273161668000159, -1.1353837801253027e-07, 3.4624652883283789e-08, 1.0500514096454037e-11, -1.6232367207383726e-07, -1.0032555322719643e-09, 1.0222049560320616e-09, -0.57735011279184834, -0.00042496184971529418, -1.094979247466835e-08, 2.0563439917320494e-08, 4.9535954856748179e-10, 6.7389931239756737e-08, 0.99998182167842575, -0.0060296196142152732, -1.4456161123455348e-09, 0.81649635974786805, 0.00060098681135852601, 0.9999999999998046, 5.8876478969960305e-09, -6.2500068559439785e-07, 0, -1.6792660347439182e-07, -0.97373048533852469, -3.2452221593337852e-07, 0.22770362737969352, -2.179217606083446e-09, 5.8734164655232282e-07, -2.1506366581361139e-09, 2.3115646039679236e-07, 4.8120424119209835e-08, -0.00042496184971520766, 0.57735011279184634, 3.8910688575168246e-11, 3.9687888707711403e-10, 5.9246005639076212e-08, 1.7722513627758232e-10, 0.0060296196142154085, 0.99998182167842664, -6.8052556416531944e-08, 0.00060098681135840361, -0.81649635974786527, -5.8876655495421221e-09, 0.99999999999999978, -2.8256256551274589e-08}),
                        matrix(16,1, {-0.99999999999999767, -6.741974297352024e-08, -1.0429489131737951e-09, 0.49999999999999983, 1.8689773938012472e-11, 1.2457164501252459e-07, -8.2439726647828279e-10, 2.3331572793762409e-07, 0.00074881461589754335, -0.26358729924963015, -0.0054120520854885976, 0.96462007269344663, -0.86602540378443837, -0.99999999999999678, -1.8318584729205164e-09, 8.3345693129013979e-08}),
                        matrix(24,2, {0, -0.99470911071239376, 1.7923205286660328e-07, -0.10273161668000159, -1.1353837801253027e-07, 3.4624652883283789e-08, 1.0500514096454037e-11, -1.6232367207383726e-07, -1.0032555322719643e-09, 1.4456161123455348e-09, -0.81649635974786805, -0.00060098681135852601, -1.0222049560320616e-09, 0.57735011279184834, 0.00042496184971529418, -1.094979247466835e-08, 2.0563439917320494e-08, 4.9535954856748179e-10, 6.7389931239756737e-08, 0.99998182167842575, -0.0060296196142152732, 0.9999999999998046, 5.8876478969960305e-09, -6.2500068559439785e-07, 0, -1.6792660347439182e-07, -0.97373048533852469, -3.2452221593337852e-07, 0.22770362737969352, -2.179217606083446e-09, 5.8734164655232282e-07, -2.1506366581361139e-09, 2.3115646039679236e-07, 6.8052556416531944e-08, -0.00060098681135840361, 0.81649635974786527, -4.8120424119209835e-08, 0.00042496184971520766, -0.57735011279184634, 3.8910688575168246e-11, 3.9687888707711403e-10, 5.9246005639076212e-08, 1.7722513627758232e-10, 0.0060296196142154085, 0.99998182167842664, -5.8876655495421221e-09, 0.99999999999999978, -2.8256256551274589e-08}),
                        matrix(36,4, {-0.99999999999999767, -6.741974297352024e-08, -1.0429489131737951e-09, 0, 0, 0, 0, 0, 0, 0.70710678118654724, 1.3215665890412225e-11, 8.8085454931919509e-08, -5.8293689751844699e-10, 1.6497913338216961e-07, 0.0005294918927527529, -0.18638436673406128, -0.0038268987297837842, 0.68208939467019658, -0.70710678118654724, 1.3215665890412225e-11, 8.8085454931919509e-08, -5.8293689751844699e-10, 1.6497913338216961e-07, 0.0005294918927527529, -0.18638436673406128, -0.0038268987297837842, 0.68208939467019658, -0.99999999999999678, -1.8318584729205164e-09, 8.3345693129013979e-08, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0.99064512431069807, 1.6330137343651072e-07, -0.057030988791132088, 0.12397461029146277, -7.9306275603593625e-11, 2.0889319808940142e-09, 0, -6.779023204519709e-08, -5.2668338259082041e-10, 8.9556100566237728e-07, -7.5856429884597752e-10, 0.70710363022088096, 0.002040741597526324, -0.00053987854369347385, 5.7039888067134764e-06, 0, 6.779023204519709e-08, 5.2668338259082041e-10, -8.9556100566237728e-07, 7.5856429884597752e-10, -0.70710363022088096, -0.002040741597526324, 0.00053987854369347385, -5.7039888067134764e-06, 0, 0, 0, -0.069362757902840563, 0.99502798776758894, -2.0371522666840116e-07, 0.071471052708437052, -2.6039103252081242e-08, -2.4785964448041671e-10, 0, 0, 0, -1.3225528688970736e-07, -0.99999999999996347, -1.0208597474589085e-07, 2.1344091480782455e-07, 1.7021751331339241e-09, -7.8388860249488337e-11, 0, 4.9658954223217554e-11, -2.3629834895190405e-07, 1.4090783755194479e-10, 9.3816378614093654e-07, 0.0019709410060079809, -0.68209727731117364, 0.0011338362460640925, -0.18638168925800053, 0, -4.9658954223217554e-11, 2.3629834895190405e-07, -1.4090783755194479e-10, -9.3816378614093654e-07, -0.0019709410060079809, 0.68209727731117364, -0.0011338362460640925, 0.18638168925800053, 0, 0, 0, -7.4008578444260882e-08, -2.1960542463633225e-07, -0.99999999999996181, 1.3522786739755615e-07, 8.5257598681548843e-10, 5.9241941862933688e-08, 0, 0, 0, 0.098504157815728882, 1.5368946355994861e-07, 0.32987946230382809, 0.93886978396533927, -3.045492996480201e-10, -7.1523001775380817e-08, 0, -1.0153358091786937e-07, -6.4856089600960975e-10, -1.3401179888363886e-07, -3.6435285262256271e-10, 0.00053959015171164005, 8.657145898888578e-05, 0.70709531026065309, 0.0039904329606807315, 0, 1.0153358091786937e-07, 6.4856089600960975e-10, 1.3401179888363886e-07, 3.6435285262256271e-10, -0.00053959015171164005, -8.657145898888578e-05, -0.70709531026065309, -0.0039904329606807315, 0, 0, 0, 0.051051928705441806, 0.075090261219055904, -1.5493775639168341e-07, -0.99586904422493183, 6.612611117602791e-08, -2.5400589239090402e-10}),
                        matrix(24,2, {2.2648006927322006e-06, -0.99999999999741951, 1.795212202138082e-07, 1.3079113834798221e-10, -0.010869683224165603, 0.57724793920804918, -1.5012010191212403e-08, 7.5135298263387308e-11, 3.5128859328673038e-08, -8.6948154554667715e-08, -0.0018996786974614242, -0.99999819560879089, -1.8496660168993222e-10, 0.015372053434314305, -0.81635186447994301, 0, -0.10273161667999042, -4.3149316619202534e-07, 0.99470911071138512, -5.0330431087148041e-07, -1.2695240556096221e-06, -3.6759857334905678e-09, 1.7472913464947176e-07, 1.8136310105333988e-09, 0.99999999999743583, 2.2648006928463975e-06, 6.3591192034362375e-10, -3.8929244217706105e-08, 0.57724793920804796, 0.010869683224165778, -3.6344185715772181e-12, 1.7023211130229836e-09, -5.7016464337541008e-10, 3.8037370364203626e-10, 0.99999819560879544, -0.0018996786974613687, 5.505426514561436e-08, -0.81635186447994124, -0.015372053434314555, 0, 1.298156214688068e-07, -0.22770362738010269, -5.7805758143896963e-07, -0.97373048533772899, 2.3662606047376383e-09, -1.2401792313522939e-06, -1.4325445302993135e-10, -7.3962787287251511e-08}),
                        matrix(16,1, {-0.99999999999999767, -6.741974297352024e-08, -1.0429489131737951e-09, 0.86602540378443837, -0.49999999999999983, 1.8689773938012472e-11, 1.2457164501252459e-07, -8.2439726647828279e-10, 2.3331572793762409e-07, 0.00074881461589754335, -0.26358729924963015, -0.0054120520854885976, 0.96462007269344663, -0.99999999999999678, -1.8318584729205164e-09, 8.3345693129013979e-08}),
                        matrix(24,2, {2.2648006927322006e-06, -0.99999999999741951, 1.795212202138082e-07, 1.8496660168993222e-10, -0.015372053434314305, 0.81635186447994301, -1.3079113834798221e-10, 0.010869683224165603, -0.57724793920804918, -1.5012010191212403e-08, 7.5135298263387308e-11, 3.5128859328673038e-08, -8.6948154554667715e-08, -0.0018996786974614242, -0.99999819560879089, 0, -0.10273161667999042, -4.3149316619202534e-07, 0.99470911071138512, -5.0330431087148041e-07, -1.2695240556096221e-06, -3.6759857334905678e-09, 1.7472913464947176e-07, 1.8136310105333988e-09, 0.99999999999743583, 2.2648006928463975e-06, 6.3591192034362375e-10, -5.505426514561436e-08, 0.81635186447994124, 0.015372053434314555, 3.8929244217706105e-08, -0.57724793920804796, -0.010869683224165778, -3.6344185715772181e-12, 1.7023211130229836e-09, -5.7016464337541008e-10, 3.8037370364203626e-10, 0.99999819560879544, -0.0018996786974613687, 0, 1.298156214688068e-07, -0.22770362738010269, -5.7805758143896963e-07, -0.97373048533772899, 2.3662606047376383e-09, -1.2401792313522939e-06, -1.4325445302993135e-10, -7.3962787287251511e-08}),
                        matrix(16,1, {-1, 4.4994511640537974e-10, -1.2694067116135686e-07, -0.70710678118653614, -4.4994511640537974e-10, 1.2694067116135686e-07, 0.70710678118653614, 0, 0, 0, -0.094441349907957314, -1.5225715590760371e-07, 0.94229878842521675, -0.32117569142122881, 7.7771401913730513e-10, 6.2443905003382943e-08})
                    }
                )
            ), // mps[3]
            MPSTensor<matrix,grp>(
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // phys_i
                { {{3,3,0},1}, {{3,2,0},2}, {{3,1,0},1},
                {{2,3,0},2}, {{2,2,0},4}, {{2,1,0},2},
                {{1,3,0},1}, {{1,2,0},2}, {{1,1,0},1} }, // left_i
                { {{3,3,0},1}, {{3,2,0},1}, {{2,3,0},1}, {{2,2,0},1} }, // right_i
                block_matrix<matrix,grp>(
                    { {{3,3,0},9}, {{3,2,0},9}, {{2,3,0},9}, {{2,2,0},9} }, // rows
                    { {{3,3,0},1}, {{3,2,0},1}, {{2,3,0},1}, {{2,2,0},1} }, // cols
                    {
                        matrix(9,1, {0, -0.8251218474325851, 9.6628628628918444e-07, -0.56495480959852429, 0.70710678118568282, 1.1059734144295836e-06, -0.70710678118568282, -1.1059734144295836e-06, -1}),
                        matrix(9,1, {-3.6753848541614702e-06, -0.99999999999324607, -0.57735026918962584, 8.7915142454668688e-07, -0.99999999999513089, -2.9943869936800525e-06, 0.81649658092772615, -1.5640826023103971e-06, 0.9999999999987772}),
                        matrix(9,1, {-3.6753848541614702e-06, -0.99999999999324607, -0.81649658092772615, 0.57735026918962584, 8.7915142454668688e-07, -0.99999999999513089, -2.9943869936800525e-06, -1.5640826023103971e-06, 0.9999999999987772}),
                        matrix(9,1, {-1, 0.70710678118177162, -2.5988895538479053e-06, -0.70710678118177162, 2.5988895538479053e-06, 0, -0.56495480959866662, -2.9674149535962617e-06, 0.82512184742771755})
                    }
                )
            ), // mps[4]
            MPSTensor<matrix,grp>(
                { {{1,1,0},1}, {{1,0,0},1}, {{0,1,0},1}, {{0,0,0},1} }, // phys_i
                { {{3,3,0},1}, {{3,2,0},1}, {{2,3,0},1}, {{2,2,0},1} }, // left_i
                { {{3,3,0},1} }, // right_i
                block_matrix<matrix,grp>(
                    { {{3,3,0},4} }, // rows
                    { {{3,3,0},1} }, // cols
                    {
                        matrix(4,1, {-1, 0.70710678118654757, -0.70710678118654757, -1})
                    }
                )
            )
        };
    };

#endif