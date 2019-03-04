function [A B A2B AB AB2] = Solve_211112(A0, B0, logK21, logK11, logK12)
  
  b11 = 10^(logK11);
  b21 = 10^(logK21 + logK11);
  b12 = 10^(logK11 + logK12);
  
  epsilon = 1e-12;
  
  a = min(A0, B0)/b11*10;
  b = B0;
  a_1 = 0;
  b_1 = 0;
  
  for(i = 0:1:1000)

  a_1 = a;
  b_1 = b;
  b = Solve_211112_b(a, B0, b21, b11, b12);
  if(b < 0)
    b = -1 * b;
  end
  
  a = Solve_211112_a(A0, b, b21, b11, b12);
  
  if(a < 0)
    a = -1 * a;
  end

  if(abs(b21 * a_1 * a_1 * b_1 - b21 * a * a * b) < epsilon && abs(b12 * a_1 * b_1 * b_1 - b12 * a * b * b) < epsilon && abs(b11 * a_1 * b_1 - b11 * a * b) < epsilon)
    break;
  end
end

A = a;
B = b;
AB = b11*A*B;
A2B = b21*A*A*B;
AB2 = b12*A*B*B;

printf("Solution is as follows\n");
printf("A = %f ... ", A);
printf("B = %f ... ", B);
printf("AB = %f ... ", AB);
printf("A2B = %f ... ", A2B);
printf("AB2 = %f ... \n", AB2);

printf("Checking results:\n");
printf("A + AB + 2*A2B +   AB2 = %f\n", A + AB + 2*A2B +   AB2);
printf("B + AB +   A2B + 2*AB2 = %f\n", B + AB +   A2B + 2*AB2);
printf("AB/A/B = %f\n", AB/A/B);
printf("A2B/A/AB = %f\n", A2B/A/AB);
printf("AB2/AB/B = %f\n", AB2/AB/B);

printf("Ratio B/B0 = %f ...\n", B/B0);
printf("Ratio A2B/B0 = %f ...\n", A2B/B0);
printf("Ratio AB/B0 = %f ...\n", AB/B0);
printf("Ratio AB2/B0 = %f ...\n", 2*AB2/B0);


endfunction
