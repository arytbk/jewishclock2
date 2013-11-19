// Extracted from http://libhdate.sourceforge.net
// Adaptation for Pebble use by Ary Tebeka contact@arytbk.net

/*  libhdate - Hebrew calendar library: http://libhdate.sourceforge.net
 *
 *  Copyright (C) 2011-2012 Boruch Baum  <boruch-baum@users.sourceforge.net>
 *                2004-2007 Yaacov Zamir <kzamir@walla.co.il>
 *                1984-2003 Amos Shapir
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hebrewdate.h"

#define HOUR 1080
#define DAY  (24*HOUR)
#define WEEK (7*DAY)
#define M(h,p) ((h)*HOUR+p)
#define MONTH (DAY+M(12,793))	/* Tikun for regular month */

// Hebrew month names in english
char *hebrewMonthNames[14] = {
  "Tishrei", "Cheshvan", "Kislev", "Tevet",
  "Sh'vat", "Adar", "Nisan", "Iyyar",
  "Sivan", "Tammuz", "Av", "Elul", "Adar I",
  "Adar II"};

/**
 @brief Compute Julian day from Gregorian day, month and year
 Algorithm from the wikipedia's julian_day
 
 @author Yaacov Zamir
 
 @param day Day of month 1..31
 @param month Month 1..12
 @param year Year in 4 digits e.g. 2001
 @return The julian day number
 */
int
hdate_gdate_to_jd (int day, int month, int year)
{
	int a;
	int y;
	int m;
	int jdn;
	
	a = (14 - month) / 12;
	y = year + 4800 - a;
	m = month + 12 * a - 3;
	
	jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
	
	return jdn;
}

/**
 @brief Converting from the Julian day to the Hebrew day
 
 @author Amos Shapir 1984 (rev. 1985, 1992) Yaacov Zamir 2003-2008
 
 @param jd Julian day
 @param day Return Day of month 1..31
 @param month Return Month 1..14 (13 - Adar 1, 14 - Adar 2)
 @param year Return Year in 4 digits e.g. 2001
 */
void
hdate_jd_to_hdate (int jd, int *day, int *month, int *year, int *jd_tishrey1, int *jd_tishrey1_next_year)
{
	int days;
	int size_of_year;
	int internal_jd_tishrey1, internal_jd_tishrey1_next_year;
	
	/* calculate Gregorian date */
	hdate_jd_to_gdate (jd, day, month, year);
  
	/* Guess Hebrew year is Gregorian year + 3760 */
	*year = *year + 3760;
  
	internal_jd_tishrey1 = hdate_days_from_3744 (*year) + 1715119;
	internal_jd_tishrey1_next_year = hdate_days_from_3744 (*year + 1) + 1715119;
	
	/* Check if computed year was underestimated */
	if (internal_jd_tishrey1_next_year <= jd)
	{
		*year = *year + 1;
		internal_jd_tishrey1 = internal_jd_tishrey1_next_year;
		internal_jd_tishrey1_next_year = hdate_days_from_3744 (*year + 1) + 1715119;
	}
  
	size_of_year = internal_jd_tishrey1_next_year - internal_jd_tishrey1;
	
	/* days into this year, first month 0..29 */
	days = jd - internal_jd_tishrey1;
	
	/* last 8 months allways have 236 days */
	if (days >= (size_of_year - 236)) /* in last 8 months */
	{
		days = days - (size_of_year - 236);
		*month = days * 2 / 59;
		*day = days - (*month * 59 + 1) / 2 + 1;
		
		*month = *month + 4 + 1;
		
		/* if leap */
		if (size_of_year > 355 && *month <=6)
			*month = *month + 8;
	}
	else /* in 4-5 first months */
	{
		/* Special cases for this year */
		if (size_of_year % 10 > 4 && days == 59) /* long Heshvan (day 30 of Heshvan) */
    {
      *month = 1;
      *day = 30;
    }
		else if (size_of_year % 10 > 4 && days > 59) /* long Heshvan */
    {
      *month = (days - 1) * 2 / 59;
      *day = days - (*month * 59 + 1) / 2;
    }
		else if (size_of_year % 10 < 4 && days > 87) /* short kislev */
    {
      *month = (days + 1) * 2 / 59;
      *day = days - (*month * 59 + 1) / 2 + 2;
    }
		else /* regular months */
    {
      *month = days * 2 / 59;
      *day = days - (*month * 59 + 1) / 2 + 1;
    }
    
		*month = *month + 1;
	}
	
	/* return the 1 of tishrey julians */
	if (jd_tishrey1 && jd_tishrey1_next_year)
	{
		*jd_tishrey1 = internal_jd_tishrey1;
		*jd_tishrey1_next_year = internal_jd_tishrey1_next_year;
	}
	
	return;
}

/**
 @brief Converting from the Julian day to the Gregorian day
 Algorithm from 'Julian and Gregorian Day Numbers' by Peter Meyer
 
 @author Yaacov Zamir ( Algorithm, Henry F. Fliegel and Thomas C. Van Flandern ,1968)
 
 @param jd Julian day
 @param d Return Day of month 1..31
 @param m Return Month 1..12
 @param y Return Year in 4 digits e.g. 2001
 */
void
hdate_jd_to_gdate (int jd, int *d, int *m, int *y)
{
	int l, n, i, j;
	l = jd + 68569;
	n = (4 * l) / 146097;
	l = l - (146097 * n + 3) / 4;
	i = (4000 * (l + 1)) / 1461001;	/* that's 1,461,001 */
	l = l - (1461 * i) / 4 + 31;
	j = (80 * l) / 2447;
	*d = l - (2447 * j) / 80;
	l = j / 11;
	*m = j + 2 - (12 * l);
	*y = 100 * (n - 49) + i + l;	/* that's a lower-case L */
  
	return;
}

/**
 @brief Days since bet (?) Tishrey 3744
 
 @author Amos Shapir 1984 (rev. 1985, 1992) Yaacov Zamir 2003-2005
 
 @param hebrew_year The Hebrew year
 @return Number of days since 3,1,3744
 */
int
hdate_days_from_3744 (int hebrew_year)
{
	int years_from_3744;
	int molad_3744;
	int leap_months;
	int leap_left;
	int months;
	int parts;
	int days;
	int parts_left_in_week;
	int parts_left_in_day;
	int week_day;
  
	/* Start point for calculation is Molad new year 3744 (16BC) */
	years_from_3744 = hebrew_year - 3744;
	molad_3744 = M (1 + 6, 779);	/* Molad 3744 + 6 hours in parts */
  
	/* Time in months */
	leap_months = (years_from_3744 * 7 + 1) / 19;	/* Number of leap months */
	leap_left = (years_from_3744 * 7 + 1) % 19;	/* Months left of leap cycle */
	months = years_from_3744 * 12 + leap_months;	/* Total Number of months */
  
	/* Time in parts and days */
	parts = months * MONTH + molad_3744;	/* Molad This year + Molad 3744 - corections */
	days = months * 28 + parts / DAY - 2;	/* 28 days in month + corections */
  
	/* Time left for round date in corections */
	parts_left_in_week = parts % WEEK;	/* 28 % 7 = 0 so only corections counts */
	parts_left_in_day = parts % DAY;
	week_day = parts_left_in_week / DAY;
  
	/* Special cases of Molad Zaken */
	if ((leap_left < 12 && week_day == 3
	     && parts_left_in_day >= M (9 + 6, 204)) ||
	    (leap_left < 7 && week_day == 2
	     && parts_left_in_day >= M (15 + 6, 589)))
	{
		days++, week_day++;
	}
  
	/* ADU */
	if (week_day == 1 || week_day == 4 || week_day == 6)
	{
		days++;
	}
  
	return days;
}

/**
 @brief Return a static string, with name of month.
 
 @param month The number of the month 1..12 (1 - jan).
 @param short_form A short flag.
 @warning DEPRECATION: This function is now just a wrapper for
 hdate_string, and is subject to deprecation.
 [deprecation date 2011-12-28]
 */
char *
hdate_get_month_string (int month) {
  if((month >0) && (month <=14)) {
    return hebrewMonthNames[month-1];
  } else {
    return "";
  }
}