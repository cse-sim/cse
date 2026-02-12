#include "gtest/gtest.h"

#include "cnglob.h"
#include "tdpak.h"

TEST(tdpak, basic_calendar_functions) 
{
	// Many functions to be added
	EXPECT_EQ(tddmon("Feb"), 2);

	EXPECT_EQ(tdIsLeapYear(-1), false);
	EXPECT_EQ(tdIsLeapYear(-2), false);
	EXPECT_EQ(tdIsLeapYear(-3), false);
	EXPECT_EQ(tdIsLeapYear(-4), false);
	EXPECT_EQ(tdIsLeapYear(-5), false);
	EXPECT_EQ(tdIsLeapYear(-6), false);
	EXPECT_EQ(tdIsLeapYear(-7), false);

	EXPECT_EQ(tdIsLeapYear(1600), true);
	EXPECT_EQ(tdIsLeapYear(1700), false);
	EXPECT_EQ(tdIsLeapYear(1800), false);
	EXPECT_EQ(tdIsLeapYear(1900), false);
	EXPECT_EQ(tdIsLeapYear(2000), true);
	EXPECT_EQ(tdIsLeapYear(2020), true);
	EXPECT_EQ(tdIsLeapYear(2021), false);
	EXPECT_EQ(tdIsLeapYear(2022), false);
	EXPECT_EQ(tdIsLeapYear(2023), false);
	EXPECT_EQ(tdIsLeapYear(2024), true);
	EXPECT_EQ(tdIsLeapYear(2025), false);

	EXPECT_EQ(tdLeapDay(2000, 1), 0);
	EXPECT_EQ(tdLeapDay(2000, 3), 1);
	EXPECT_EQ(tdLeapDay(2001, 1), 0);
	EXPECT_EQ(tdLeapDay(2001, 3), 0);

}


