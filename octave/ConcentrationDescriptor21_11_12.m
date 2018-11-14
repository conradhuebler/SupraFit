function [bc50] = ConcentrationDescriptor21_11_12(logK21, logK11, logK12)
  %this function calculates all concentrational descriptors for any given logK21 and logK11 pairs
  
  global b11;
  global b21;
  global b21;
  
  lower_limit = 0;
  upper_limit = 0.9999;
  
  b11 = 10^(logK11);
  b21 = 10^(logK21 + logK11);
  b12 = 10^(logK11 + logK12);
  
  bc50 = -b11/2/b21 + sqrt((-b11/2/b21)^2 + 1/b21);
  printf("The BC50 is as follows: %d mu M\n", bc50*10^6);
    
  B = quadcc("BFunction21_11", lower_limit, upper_limit);
  AB = quadcc("ABFunction21_11", lower_limit, upper_limit);
  A2B = quadcc("A2BFunction21_11", lower_limit, upper_limit);
  printf("The normal way round ... \n");
  printf("B in Solution: %d\n", B*10^6);
  printf("AB in Solution: %d\n", AB*10^6);
  printf("A2B in Solution: %d\n", A2B*10^6);

  printf("Lets do it the inverse way \n");
  
  factor = 2;
  
  B = 1/factor/quadcc("IBFunction21_11", lower_limit, upper_limit);
  AB = 1/factor/quadcc("IABFunction21_11", lower_limit, upper_limit);
  A2B = 1/factor/quadcc("IA2BFunction21_11", lower_limit, upper_limit);
  
  printf("B in Solution: %d\n", B*10^6);
  printf("AB in Solution: %d\n", AB*10^6);
  printf("A2B in Solution: %d\n", A2B*10^6);
  
  printf("Lets calculate the relative concentrations\n");
  qut = (1 + b11*bc50 +b21*bc50^2);
  
  printf("B in Solution: %d\n", 1/qut);
  printf("AB in Solution: %d\n", b11*bc50/qut);
  printf("A2B in Solution: %d\n", b21*bc50*bc50/qut);

  
endfunction
