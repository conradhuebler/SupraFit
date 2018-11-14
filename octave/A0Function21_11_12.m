function [A0] = A0Function21_11_12(x)
    
  global b12;
  global b11;
  global b21;
  
  [A B A2B AB AB2] = Solve_211112_bc(x, b21, b11, b12);
  
  A0 = A + AB + A2B + 2*AB2;
endfunction
