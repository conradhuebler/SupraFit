function [AB] = ABFunction21_11_12(x)
    
  global b11;
  global b21;
  global b12;
  
  [A B A2B AB AB2] = Solve_211112_bc(x, b21, b11, b12, false);
  
endfunction
