function [AB] = ABFunction11_12(x)
  
  global b11;
  global b12;
  %alpha = x./(1-x);
  %B = -b11/2/b12 + sqrt((-b11/2/b12)^2 + alpha/b12);
  %A = 1 / (b11 + 2.0 * b12 * B);
  %AB = A*B*b11;
  B = BFunction11_12(x);
  A = AFunction11_12(x);
  AB = A*B*b11;%b11/2/b12-b11*b11/(2*b12)*AFunction11_12(x);
  endfunction