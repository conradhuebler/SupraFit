function [b] = Solve_211112_b(a, b0, b21, b11, b12)
  x1 = 2 * b12 * a;
  x2 = b21 * a * a + b11 * a + 1;
  x3 = -b0;
  b1 = roots([x1, x2, x3]);
    
  if(max(b1) > b0)
    b = min(b1);
  else
    b = max(b1);
  end
endfunction
