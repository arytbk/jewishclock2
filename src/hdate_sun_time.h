//
//  hdate_sun_time.h
//  TBK_Jewish_Clock
//
//  Created by Ary Tebeka on 08/07/2013.
//
//

#ifndef TBK_Jewish_Clock_hdate_sun_time_h
#define TBK_Jewish_Clock_hdate_sun_time_h

/**
 @brief utc sunrise/set time for a gregorian date
 
 @parm day this day of month
 @parm month this month
 @parm year this year
 @parm longitude longitude to use in calculations
 @parm latitude latitude to use in calculations
 @parm sunrise return the utc sunrise in minutes
 @parm sunset return the utc sunset in minutes
 */
void
hdate_get_utc_sun_time (int day, int month, int year, double latitude, double longitude, int *sunrise, int *sunset);

/**
 @brief utc sunrise/set time for a gregorian date
 
 @parm day this day of month
 @parm month this month
 @parm year this year
 @parm longitude longitude to use in calculations
 @parm latitude latitude to use in calculations
 @parm sun_hour return the length of shaa zaminit in minutes
 @parm first_light return the utc alut ha-shachar in minutes
 @parm talit return the utc tphilin and talit in minutes
 @parm sunrise return the utc sunrise in minutes
 @parm midday return the utc midday in minutes
 @parm sunset return the utc sunset in minutes
 @parm first_stars return the utc tzeit hacochavim in minutes
 @parm three_stars return the utc shlosha cochavim in minutes
 */
void
hdate_get_utc_sun_time_full (int day, int month, int year, double latitude, double longitude,
                             int *sun_hour, int *first_light, int *talit, int *sunrise,
                             int *midday, int *sunset, int *first_stars, int *three_stars);

#endif
