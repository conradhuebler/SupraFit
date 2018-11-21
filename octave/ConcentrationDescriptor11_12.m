function [bc50] = ConcentrationDescriptor11_12(logK11, logK12)
  %this function calculates all concentrational descriptors for any given logK21 and logK11 pairs
  
  global b11;
  global b12;
  
  lower_limit = 0;
  upper_limit = 1;
  
  b11 = 10^(logK11);
  b12 = 10^(logK12 + logK11);
    
  B = quad("BFunction11_12", lower_limit, upper_limit);
  AB = quad("ABFunction11_12", lower_limit, upper_limit);
  AB2 = quad("AB2Function11_12", lower_limit, upper_limit);
  printf("The normal way round ... \n");
  printf("B in Solution: %d\n", B*10^6);
  printf("AB in Solution: %d\n", AB*10^6);
  printf("A2B in Solution: %d\n", AB2*10^6);
 B0 = B + AB + 2 *AB2;
   printf("Lets calculate the relative concentrations from the approximated soluation\n");
  printf("B in Solution: %d\n", B/B0);
  printf("AB in Solution: %d\n", AB/B0);
  printf("A2B in Solution: %d\n", 2*AB2/B0);
  printf("Lets do it the inverse way \n");
  
  

  
endfunction
