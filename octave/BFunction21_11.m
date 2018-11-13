function [B] = BFunction21_11(x)
  
  global b11;
  global b21;
  
  A = -b11/2/b21 + sqrt((-b11/2/b21)^2 + 1/b21);
  
  alpha = x./(1-x);
  B = alpha./(2.*b21 * A + b11);
endfunction