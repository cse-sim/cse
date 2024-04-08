// nummeth.unit.cpp - test uses of numerical-method functions

#include "gtest/gtest.h"

#include "cnglob.h"
#include "cvpak.h"
#include "nummeth.h"

constexpr double a = 2., xi = 3.;

// linear function
static double contin_func(void *no_obj, double &x) {

  double f = a * (x - xi);
  return f;
}

// inverted linear function
static double discontin_func(void *no_obj, double &x) {
  double f = DBL_MAX;
  if (abs(x - xi) > 0.0001) {
    f = 1. / (a * (x - xi));
  }
  return 1./ f;
}

TEST(nummeth, secant_test) {

  { // solution of linear function
    double x1 = 3.5, x2 = 4.5;
    double f1 = DBL_MIN, f2 = DBL_MIN;
    double fTarg = 2.;

    int ret = secant(contin_func, NULL, fTarg, .0001 * fTarg, x1, f1, // x1, f1
                     x2, f2);                                         // x2, f2

    EXPECT_EQ(ret, 0) << "secant solution of continuous function failed.";
    double xExpected =  fTarg / a + xi;
    EXPECT_NEAR(x1, xExpected, 1.e-12)
        << "expected solution of continuous function not found.";
  }

  { // solution of inverted linear function
    double x1 = 3.5, x2 = 4.5;
    double f1 = DBL_MIN, f2 = DBL_MIN;
    double fTarg = 2.;

    int ret =
        secant(discontin_func, NULL, fTarg, .0001 * fTarg, x1, f1, // x1, f1
               x2, f2);                                            // x2, f2

    EXPECT_EQ(ret, 0) << "secant solution of discontinuous function failed.";
    double xExpected =  fTarg / a + xi;
    EXPECT_NEAR(x1, xExpected, 1.e-12)
        << "expected solution of discontinuous function not found.";
  }
}
