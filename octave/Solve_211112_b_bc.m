function [b] = Solve_211112_b_bc(a, x, b21, b11, b12)
  x1 = b12;
  x2 = 2. * b21. * a + b11 ;
  x3 = -x;
  b1 = roots([x1, x2, x3]);
    
  b = max(b1);

endfunction
