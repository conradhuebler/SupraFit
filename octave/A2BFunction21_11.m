function [A2B] = A2BFunction21_11(x)
    
  global b11;
  global b21;
  
  B = BFunction21_11(x);
  A = -b11/2/b21 + sqrt((-b11/2/b21)^2 + 1/b21);
  A2B = A*A*B*b21;
endfunction
