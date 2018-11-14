function [a] = Solve_211112_a_bc(b, b21, b11, b12)
  x1 = b21;
  x2 = 2 * b12 * b + b11;
  x3 = -1;
  
  a1 = roots([x1, x2, x3]);
  
  a = max(a1);
  
  
endfunction
