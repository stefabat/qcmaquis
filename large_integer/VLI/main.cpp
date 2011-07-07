#include <iostream>
#include <cstdio>

#include "gpu/GpuManager.h"
#include "gpu/GpuManager.hpp"

#include "monome/vector_polynomial_gpu.h"
#include "vli_cpu/vli_number_cpu.hpp"
#include "vli_gpu/vli_number_gpu.hpp"
#include "monome/monome.h"
#include "monome/polynome_gpu.h"
#include "monome/polynome.h"

using vli::vli_cpu;
using vli::vli_gpu;
using vli::monomial;
using vli::polynomial;
using vli::polynomial_gpu;
using vli::vector_polynomial_gpu;

int main (int argc, char * const argv[]) 
{
	gpu::gpu_manager* GPU;
	GPU->instance();
    /*
    polynomial<vli_cpu<int,8>, 2> pa;
    
    for(int i=0; i<6; i++){
        pa(0,0)[i] = 22*i;
        pa(0,1)[i] = 22*i;
        pa(1,0)[i] = 22*i;
        pa(1,1)[i] = 22*i;        
    }
 
    polynomial_gpu<vli_gpu<int,8>, 2> pagpu(pa);
 
    vector_polynomial_gpu< polynomial_gpu<vli_gpu<int, 8>,2> > Va(4);
    vector_polynomial_gpu< polynomial_gpu<vli_gpu<int, 8>,2> > Vb(4);
    vector_polynomial_gpu< polynomial_gpu<vli_gpu<int, 8>,2> > Vc(4);

    Va[0]=pa;   
    Va[2]=pa;   
    Vb[0]=pa;   
    Vb[2]=pa;   
    
    std::cout << Vc << std::endl;
    
    Vc = inner_product(Va,Vb);
    
    std::cout << Vc << std::endl;
*/
    
  
    vli::polynomial<vli_cpu<int,8>,2> pa;
    vli::polynomial<vli_cpu<int,8>,2> pb;
    vli::polynomial<vli_cpu<int,8>,2> pc;
    
    vli::polynomial_gpu<vli_gpu<int,8>,2> pagpu;
    vli::polynomial_gpu<vli_gpu<int,8>,2> pbgpu;
    vli::polynomial_gpu<vli_gpu<int,8>,2> pcgpu;
    
    
    for(int i=0; i <2; i++){     
        
        pa(0,0)[i] = 255;        
        pa(0,1)[i] = 255;
        pa(1,0)[i] = 255;
        pa(1,1)[i] = 255;
        
        pb(0,0)[i] = 255;
        pb(0,1)[i] = 255;
        pb(1,0)[i] = 255;
        pb(1,1)[i] = 255;
        
        pagpu(0,0)[i] = 255;        
        pagpu(0,1)[i] = 255;
        pagpu(1,0)[i] = 255;
        pagpu(1,1)[i] = 255;
        
        pbgpu(0,0) = pb(0,0);
        pbgpu(0,1) = pb(0,1);
        pbgpu(1,0) = pb(1,0);
        pbgpu(1,1) = pb(1,1); 
    }
    
    pc = pa*pb;
    pcgpu = pagpu*pbgpu;
    
    printf("GPU \n");
    std::cout << pcgpu << std::endl;
    printf("--------------------------- \n");
    printf("CPU \n");
    std::cout << pc << std::endl;

    
	GPU->instance().destructor();
}
