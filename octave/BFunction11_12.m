function [B] = BFunction11_12(x)
  
  global b11;
  global b12;
  alpha = x./(1-x);
  B = -b11/2/b12 + sqrt((-b11/2/b12)^2 + alpha/b12);
  %A = AFunction11_12(x);
  %B = (1-b11*A)/(2*b12*A);
  endfunction