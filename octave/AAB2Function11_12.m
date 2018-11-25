function [AAB2] = AAB2Function11_12(x)
  
  global b11;
  global b12;
  alpha = x./(1-x);
  %B = -b11/2/b12 + sqrt((-b11/2/b12)^2 + alpha/b12);
  %A = 1 / (b11 + 2.0 * b12*B);
  %AB2 = A*B*B*b12;
  %A = AFunction11_12(x);
  %AB = ABFunction11_12(x);
  %AB2 = (x*sqrt(1-x))/((1-x)*sqrt(
  %AB2 = alpha*A-AB;
  
  %AB2 = alpha/(2*sqrt(alpha/b12+b11^2/4/b12^2)*b12)-b11/2/b12+b11^2/(4*sqrt(alpha/b12+b11^2/4/b12^2)*b12^2);
  A = AFunction11_12(x);
  B = BFunction11_12(x);%(1-b11*A)/(2*b12*A);
  AAB2 = A*B*b11+2*A*B*B*b12; %(1-b11*AFunction11_12(x)^2)/(b12*AFunction11_12(x));
  endfunction