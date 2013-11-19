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

/**
 @brief Compute Julian day from Gregorian date
 
 @author Yaacov Zamir (algorithm from Henry F. Fliegel and Thomas C. Van Flandern ,1968)
 
 @param day Day of month 1..31
 @param month Month 1..12
 @param year Year in 4 digits e.g. 2001
 @return the julian day number
 */
int
hdate_gdate_to_jd (int day, int month, int year);

/**
 @brief Converting from the Julian day to the Hebrew day
 
 @author Yaacov Zamir 2005
 
 @param jd Julian day
 @param day return Day of month 1..31
 @param month return Month 1..14 (13 - Adar 1, 14 - Adar 2)
 @param year return Year in 4 digits e.g. 2001
 @param jd_tishrey1 return the julian number of 1 Tishrey this year
 @param jd_tishrey1_next_year return the julian number of 1 Tishrey next year
 */
void
hdate_jd_to_hdate (int jd, int *day, int *month, int *year, int *jd_tishrey1, int *jd_tishrey1_next_year);

/**
 @brief Converting from the Julian day to the Gregorian date
 
 @author Yaacov Zamir (Algorithm, Henry F. Fliegel and Thomas C. Van Flandern ,1968)
 
 @param jd Julian day
 @param day return Day of month 1..31
 @param month return Month 1..12
 @param year return Year in 4 digits e.g. 2001
 */
void
hdate_jd_to_gdate (int jd, int *day, int *month, int *year);

/**
 @brief Days since Tishrey 3744
 
 @author Amos Shapir 1984 (rev. 1985, 1992) Yaacov Zamir 2003-2005
 
 @param hebrew_year The Hebrew year
 @return Number of days since 3,1,3744
 */
int
hdate_days_from_3744 (int hebrew_year);

/**
 @brief Return a static string, with name of month.
 
 @param month The number of the month 1..12 (1 - jan).
 @param short_form A short flag.
 @warning DEPRECATION: This function is now just a wrapper for
 hdate_string, and is subject to deprecation.
 [deprecation date 2011-12-28]
 */
char *
hdate_get_month_string (int month);
