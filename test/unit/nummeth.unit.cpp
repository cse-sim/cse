// nummeth.unit.cpp - test uses of numerical-method functions

#include "gtest/gtest.h"

#include "cnglob.h"
#include "cvpak.h"
#include "nummeth.h"

// linear function
static double linear_func(void *, double &x) {
  return x;
}

// inverse function
static double inverse_func(void *, double &x) {
  double f = DBL_MAX;
  if (abs(x) > 0.0001) {
    f = 1. / x;
  }
  return f;
}

TEST(nummeth, secant_test) {

  double eps = .0001;
  
  { // solution of linear function
    double x1 = 1., x2 = 3.;
    double f1 = DBL_MIN, f2 = DBL_MIN;
    double fTarg = 2.;

    int ret = secant(linear_func, NULL, fTarg, eps * fTarg, x1, f1, // x1, f1
                     x2, f2);                                         // x2, f2

    EXPECT_EQ(ret, 0) << "secant solution of linear function failed.";
    double xExpected =  fTarg;
    EXPECT_NEAR(x1, xExpected, eps)
        << "expected solution of linear function not found.";
  }

  { // solution of inverse function
    double x1 = 0.01, x2 = 1.0;
    double f1 = DBL_MIN, f2 = DBL_MIN;
    double fTarg = 2.;

    int ret =
        secant(inverse_func, NULL, fTarg, eps * fTarg, x1, f1, // x1, f1
               x2, f2);                                            // x2, f2

    EXPECT_EQ(ret, 0) << "secant solution of inverse function failed.";
    double xExpected =  1. / fTarg;
    EXPECT_NEAR(x1, xExpected, eps)
        << "expected solution of inverse function not found.";
  }
  
  { // solution of inverted inverse function
    double x1 = 0.01, x2 = 1.0;
    double f1 = DBL_MIN, f2 = DBL_MIN;
    double fTarg = 1. / 2.;
    
    
    
    int ret =
        secant([](void*, double& x){return 1./inverse_func(NULL, x);}, NULL, fTarg, eps * fTarg, x1, f1, // x1, f1
               x2, f2);                                            // x2, f2

    EXPECT_EQ(ret, 0) << "secant solution of inverted inverse function failed.";
    double xExpected =  fTarg;
    EXPECT_NEAR(x1, xExpected, eps)
        << "expected solution of inverted inverse function not found.";
  }

  { // solution of inverted inverse function (below "zero" tolerance)
    double x1 = 0.01, x2 = 1.0;
    double f1 = DBL_MIN, f2 = DBL_MIN;
    double fTarg = 0.00005;
    
    
    
    int ret =
        secant([](void*, double& x){return 1./inverse_func(NULL, x);}, NULL, fTarg, eps * fTarg, x1, f1, // x1, f1
               x2, f2);                                            // x2, f2
                                                                                                                 
    EXPECT_EQ(ret, -3) << "secant solution of inverted inverse function failed.";
    double xExpected = fTarg;
    EXPECT_NEAR(x1, xExpected, eps)
        << "expected solution of inverted inverse function not found.";
  }
  
  
}
