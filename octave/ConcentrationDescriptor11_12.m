function [bc50] = ConcentrationDescriptor11_12(logK11, logK12)
  %this function calculates all concentrational descriptors for any given logK21 and logK11 pairs
  
  global b11;
  global b12;
  
  lower_limit = 0;
  upper_limit = 1;
  
  b11 = 10^(logK11);
  b12 = 10^(logK12 + logK11);
    
    
  A = quad("AFunction11_12", lower_limit, upper_limit);
  B = (1-b11*A)/(2*b12*A);
  AB = A*B*b11;
  AB2 = A*B*B*b12;

  printf("The normal way round ... \n");
  printf("A in Solution: %d\n", A*10^6);
  printf("B in Solution: %d\n", B*10^6);
  printf("AB in Solution: %d\n", AB*10^6);
  printf("AB2 in Solution: %d\n", AB2*10^6);
 B0 = B + AB + 2 *AB2;
   printf("Lets calculate the relative concentrations from the approximated soluation\n");
  printf("B in Solution: %d\n", B/B0);
  printf("AB in Solution: %d\n", AB/B0);
  printf("A2B in Solution: %d\n", 2*AB2/B0);
  printf("Lets do it the inverse way \n");
  
  %B = quad("BFunction11_12", lower_limit, upper_limit);
  AB = quad("ABFunction11_12", lower_limit, upper_limit);
  AB2 = quad("AB2Function11_12", lower_limit, upper_limit);
  AAB2 = quad("AAB2Function11_12", lower_limit, upper_limit)

  B = AAB2;
  (AAB2-AB)/2
  printf("The normal way round ... \n");
  %printf("A in Solution: %d\n", A*10^6);
  printf("B in Solution: %d\n", B*10^6);
  printf("AB in Solution: %d\n", AB*10^6);
  printf("AB2 in Solution: %d\n", AB2*10^6);
  
  B0 = B + AB + 2 *AB2;
   printf("Lets calculate the relative concentrations from the approximated soluation\n");
  printf("B in Solution: %d\n", B/B0);
  printf("AB in Solution: %d\n", AB/B0);
  printf("A2B in Solution: %d\n", 2*AB2/B0);
  printf("Lets do it the inverse way \n");
  
endfunction
