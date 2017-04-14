#include <stdio.h>
#include <math.h>

void
dump_row(size_t i, double *R){
   printf("R[%2zu] = ", i);
   for(size_t j = 0; j <= i; ++j){
      printf("%f ", R[j]);
   }
   printf("\n");
}

double
romberg(double (*f/* function to integrate */)(double), double /*lower limit*/ a, double /*upper limit*/ b, size_t max_steps, double /*desired accuracy*/ acc){
   double R1[max_steps], R2[max_steps]; //buffers
   double *Rp = &R1[0], *Rc = &R2[0]; //Rp is previous row, Rc is current row
   double h = (b-a); //step size
   Rp[0] = (f(a) + f(b))*h*.5; //first trapezoidal step

   dump_row(0, Rp);

   for(size_t i = 1; i < max_steps; ++i){
      h /= 2.;
      double c = 0;
      size_t ep = 1 << (i-1); //2^(n-1)
      for(size_t j = 1; j <= ep; ++j){
         c += f(a+(2*j-1)*h);
      }
      Rc[0] = h*c + .5*Rp[0]; //R(i,0)

      for(size_t j = 1; j <= i; ++j){
         double n_k = pow(4, j);
         Rc[j] = (n_k*Rc[j-1] - Rp[j-1])/(n_k-1); //compute R(i,j)
      }

      //Dump ith column of R, R[i,i] is the best estimate so far
//      dump_row(i, Rc);

      if(i > 1 && fabs(Rp[i-1]-Rc[i]) < acc){
         return Rc[i-1];
      }

      //swap Rn and Rc as we only need the last row
      double *rt = Rp;
      Rp = Rc;
      Rc = rt;
   }
   return Rp[max_steps-1]; //return our best guess
}

#define PI 3.141592653589793

double 
inverse(double x){ return 1.0/x;}

double 
gaussian(double x){ return exp(-x*x/2.0)/sqrt(2.0*PI);}

double 
diff(double (*f)(double), double x, size_t max_steps, double acc){
  double dx=0.01;
  double df,df0;
  for(size_t i=0; i<max_steps;i++){
     dx/=2.0;
     df= (f(x+dx)-f(x-dx))/dx/2.0;
     if(fabs(df-df0)<acc) return df;
     df0=df;
  }
  return df;
}

double curvilinear_integral(double (*f)(double,double,double), double (*x)(double),double (*y)(double),double (*z)(double), double t0, double t1, size_t max_steps, double acc){
  
  speed
}

int
main(){
       
        printf("\nDerivation:");
	printf("Expected : %0.18lf        Computed : %0.18lf\n"
	      ,-0.25,             diff(inverse,2, 100, 1e-8));

        printf("\nIntegration:");
	printf("Expected : %0.18lf        Computed : %0.18lf\n"
	      , log(3.14),             romberg(inverse,1.0, 3.14, 100, 1e-8));
	printf("Expected : %0.18lf        Computed : %0.18lf\n"
	      , 1.0,             romberg(gaussian,-9999.9, 9999.9, 100, 1e-8));
	return 0;
}
