function [B0] = IBOFunction(x)
    
  global b12;
  global b11;
  global b21;
  
  [A B A2B AB AB2] = Solve_211112_bc(x);
  
  B0 = 1/(B + AB + A2B + 2*AB2);
endfunction
