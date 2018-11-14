function [a] = Solve_211112_a(a0, b, b21, b11, b12)
  x1 = 2 * b21 * b;
  x2 = b12 * b * b + b11 * b + 1;
  x3 = -a0;
  
  a1 = roots([x1, x2, x3]);
  
  if(max(a1) > a0)
    a = min(a1);
  else
    a = max(a1);
  end
  
endfunction
