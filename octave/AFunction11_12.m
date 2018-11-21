function [A] = AFunction11_12(x)
  
  global b11;
  global b12;
 % alpha = x./(1-x);
 % B = -b11/2/b12 + sqrt((-b11/2/b12)^2 + alpha/b12);
 
 k = 4*b12/b11/b11;
  A = sqrt(x-1)/(b11*sqrt(x-1+k*x));
  endfunction