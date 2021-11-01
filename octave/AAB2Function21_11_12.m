function [AAB2] = AAB2Function21_11_12(x)
  
  global b21;
  global b11;
  global b12;
  
  A = AFunction21_11_12(x);
  B = BFunction21_11_12(x);
  AAB2 = A*A*B*b21 + A*B*b11 + 2*A*B*B*b12;
  endfunction