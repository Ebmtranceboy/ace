#include <stdio.h>
#include <math.h>
#define TAYLOR_BOUND 500 

double PI;
double coef_inverf[TAYLOR_BOUND];

void
make_coef_inverf(){
	coef_inverf[0] = 1.0;
	double sum,den;
	for(int n=1; n<TAYLOR_BOUND; n++){
		sum = 0.0;
		for(int k=0; k<n; k++){
			den = (k+1)*(2*k+1);
			sum +=  coef_inverf[k] * coef_inverf[n-1-k] / den;
		}
		coef_inverf[n] = sum;
	}
}

double
inverf(double y){
	if (y<0) return -inverf(-y);
	int n,k = 0;
	double x = sqrt(PI) * y / 2.0;
	double val, sum = x;
	double xn = x ,x2 = x*x;
	double prec = 1.0;
	while(prec >= 1e-13 && k<TAYLOR_BOUND){
		val = sum;
		k++;
		n=2*k+1;
		xn *= x2;
		sum += coef_inverf[k] * xn / n;
		prec = fabs(val-sum);
	}
	return sum;
}

double 
normalDistribution(double x){
	return (1.0 + erf(x/sqrt(2.0)))/2.0;
}

double 
invNormalDistribution(double y){
	return sqrt(2.0) * inverf(2.0*y-1.0);
}

// X ~ L=N(m,s2)

// P_L(X<=x0)
double 
normalms2(double m, double s2, double x0){
	return normalDistribution((x0-m)/sqrt(s2));
}

// P_L(a<=X<=b)
double 
probams2(double m, double s2,double a, double b){
	return normalms2(m,s2,b)-normalms2(m,s2,a);
}

//h>0 such that P_L(m-h<=X<=m+h)=p
double 
radius(double s2, double p){
	return sqrt(s2)*invNormalDistribution((1+p)/2);
}

//a>0 such that P_L(X<=x1)=p 
double 
bound(double m, double s2, double p){
	return m+sqrt(2.0*s2)*inverf(2*p-1);
}

void
init(){
	PI = acos(-1.0);
	make_coef_inverf();
}

void demo(){
	printf("%0.15lf\n",erf(1.5));
	printf("%0.15lf\n",inverf(0.9661051464753108));
	printf("%0.15lf\n",normalDistribution(1.5));
	printf("%0.15lf\n",invNormalDistribution(0.9331927987311419));
	printf("%0.15lf\n",radius(2,0.68));
	printf("%0.15lf\n",probams2(15,2,15-1.406375825644,15+1.406375825644));
}

void interact(){
	printf("Menu: (L=N(m,s2))\n");
	printf("1) P_L(X<=x0) \n");
	printf("2) P_L(a<=X<=b)\n");
	printf("3) h such that P_L(m-h<=X<=m+h)=p \n");
	printf("4) x1 such that P_L(X<=x1)=p \n\n Choix:");
	int rep;
	scanf("%d",&rep);
	printf("\n");
	double m,s2,x0,a,b,p;
	switch(rep){
		case 1:
			printf("m:");scanf("%lf",&m);
			printf("s2:");scanf("%lf",&s2);
			printf("x0:");scanf("%lf",&x0);
			printf("\nP_L(X<= %lf) = %lf\n",x0,normalms2(m,s2,x0));
			break;
		case 2:
		 	printf("m:");scanf("%lf",&m);
			printf("s2:");scanf("%lf",&s2);
			printf("a:");scanf("%lf",&a);
			printf("b:");scanf("%lf",&b);
			printf("\nP_L(%lf<=X<=%lf) = %lf\n",a,b,probams2(m,s2,a,b));
		 	break;
		 case 3:
		 	printf("s2:");scanf("%lf",&s2);
			printf("p:");scanf("%lf",&p);
			printf("\nP_L(m-h<=X<=m+h)=%lf ==> h = %lf\n",p,radius(s2,p));
			break;
		case 4:
		 	printf("m:");scanf("%lf",&m);
			printf("s2:");scanf("%lf",&s2);
			printf("p:");scanf("%lf",&p);
			printf("\nP_L(X<=x1)=%lf ==> x1 = %lf\n",p,bound(m,s2,p));
			break;
	}
}

int 
main (){
	init();
	interact();
	return 0;
}