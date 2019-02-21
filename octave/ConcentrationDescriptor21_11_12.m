function [rB rA2B rAB rAB2] = ConcentrationDescriptor21_11_12(logK21, logK11, logK12, upper_limit = 0.9999)
  %this function calculates all concentrational descriptors for any given logK21 and logK11 pairs
   printf("\n\n\n");

  global b11;
  global b21;
  global b12;
  
  lower_limit = 0;
  
  b11 = 10^(logK11);
  b21 = 10^(logK21 + logK11);
  b12 = 10^(logK11 + logK12);
  
  bc50 = -b11/2/b21 + sqrt((-b11/2/b21)^2 + 1/b21);
  printf("The BC50 is as follows: %d mu M\n", bc50*10^6);
    
  A =  quad("AFunction21_11_12", lower_limit, upper_limit);
  %B = quad("BFunction21_11_12", lower_limit, upper_limit);
  %A2B = quad("A2BFunction21_11_12", lower_limit, upper_limit);
  %AB = quad("ABFunction21_11_12", lower_limit, upper_limit);
  %AB2 = quad("AB2Function21_11_12", lower_limit, upper_limit);
  B = (1-(b11*A+b21*A*A))/(2*b12*A);
  AB = b11 *A* B;
  AB2 = b12*A*B*B;
  A2B = b21* A*A*B;
  A0 = A + AB + 2*A2B + AB2
  B0 = B + AB + A2B + 2*AB2

%  printf("The normal way round ... \n");
%  printf("A0 in Solution: %d\n", A0*10^6);
%  printf("B0 in Solution: %d\n", B0*10^6);

  
%  printf("A in Solution: %d\n", A*10^6);
%  printf("B in Solution: %d\n", B*10^6);

  
%  printf("A2B in Solution: %d\n", A2B*10^6);
%  printf("AB in Solution: %d\n", AB*10^6);
%  printf("A2B in Solution: %d\n", A2B*10^6);  
 
  
%  printf("B in Solution: %d\n", B/B0);
%  printf("AB in Solution: %d\n", AB/B0);
%  printf("A2B in Solution: %d\n", A2B/B0);
%  printf("AB2 in Solution: %d\n", 2*AB2/B0);

  B = quad("BFunction21_11_12", lower_limit, upper_limit);
  A2B = quad("A2BFunction21_11_12", lower_limit, upper_limit);
  AB = quad("ABFunction21_11_12", lower_limit, upper_limit);
  AB2 = quad("AB2Function21_11_12", lower_limit, upper_limit);
  AAB2 = quad("AAB2Function21_11_12", lower_limit, upper_limit)

  %B = AAB2;
  %(AAB2-AB-A2B)/2
  printf("The normal way round ... \n");
  printf("A in Solution: %d\n", A*10^6);
  printf("B in Solution: %d\n", B*10^6);
  printf("A2B in Solution: %d\n", A2B*10^6);
  printf("AB in Solution: %d\n", AB*10^6);
  printf("AB2 in Solution: %d\n", AB2*10^6);
  
  B0 = B +   A2B + AB + 2*AB2
  A0 = A + 2*A2B + AB +   AB2
   
  printf("Lets calculate the relative concentrations from the approximated soluation\n");
  printf("B in Solution: %d\n", B/B0);
  printf("A2B in Solution: %d\n", A2B/B0);
  printf("AB in Solution: %d\n", AB/B0);
  printf("AB2 in Solution: %d\n", 2*AB2/B0);
  %printf("Lets do it the inverse way \n");

  %rB = B/B0;
  %rA2B = A2B/B0;
  %rAB = AB/B0;
  %rAB2 = AB2/B0;
  
endfunction
