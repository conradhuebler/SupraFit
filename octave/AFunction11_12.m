function [A] = AFunction11_12(x)
  
  global b11;
  global b12;
 alpha = x./(1-x);
 B = -b11/2/b12 + sqrt((-b11/2/b12)^2 + alpha/b12);
 
 % k = 4*b12/b11/b11;
 % A = sqrt(x-1)/(b11*sqrt(x-1+k*x));
 A = 1/(b11+2*b12*B);
 
 %A = sqrt(x-1)/(sqrt((b11^2*(x-1)+4*b12)));
 %A = sqrt(x-1)/(sqrt(4*x*b12-b11*b11*(1-x)));
 %A = 1/(2*sqrt((alpha/b12)+(b11^2/4/b12^2))*b12);
 %A = 1/(sqrt(4*b12*alpha+b11*b11));
 %A = sqrt(1-x)/sqrt((4*b12*x + b11*b11*(1-x)));
  endfunction