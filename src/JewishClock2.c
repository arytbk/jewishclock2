#define USE_WEATHER
#define UPDATE_MINUTES 15

#include <pebble.h>
#include "hebrewdate.h"
#include "hdate_sun_time.h"
#include "xprintf.h"
#include "my_math.h"

static Window *window;  // Main Window

// ******************************************
// Layers - top to bottom, left to right
// ******************************************
static TextLayer *dayLayer;            char dayString[]=           "17";
static TextLayer *hDayLayer;           char hDayString[]=          "13";
static TextLayer *moonLayer;           char moonString[]=          "G";
static TextLayer *monthLayer;          char monthString[]=         "May";
static TextLayer *hMonthLayer;
static TextLayer *timeLayer;           char timeString[]=          "00:00";
static Layer *lineLayer;
static TextLayer *zmanHourLabelLayer;  char zmanHourLabelString[]= "Hour #";
static TextLayer *nextHourLabelLayer;  char nextHourLabelString[]= "Next In:";
static Layer *sunGraphLayer;
static TextLayer *currentZmanLayer;    char currentZmanString[]=   "Mincha Gedola";
static TextLayer *EndOfZmanLayer;      char endOfZmanString[]=     "00:00";
static TextLayer *zmanHourLayer;       char zmanHourString[]=      "11";
static TextLayer *nextHourLayer;       char nextHourString[]=      "01:07:00";
static TextLayer *alertLayer;          char alertString[]=    "SUNSET IN 000mn";
static TextLayer *sunriseLayer;        char sunriseString[]=       "00:00";
static TextLayer *sunsetLayer;         char sunsetString[]=        "00:00";
static TextLayer *hatsotLayer;         char hatsotString[]=        "00:00";
static TextLayer *temperature_layer;    char temperatureString[]=   "999 oC";
static TextLayer *temp_min_layer;    char tempMinString[]=   "999 oC";
static TextLayer *temp_max_layer;    char tempMaxString[]=   "999 oC";
static TextLayer *citylayer;
static BitmapLayer *icon_layer_white;
static BitmapLayer *icon_layer_black;
static GBitmap *icon_white = NULL;
static GBitmap *icon_black = NULL;

// Fonts
GFont tinyFont, smallFont, mediumFont, mediumBoldFont, largeFont, moonFont;

// Constants
const int sunY = 104;
const int sunSize = 38;
const int MINCHA_ALERT = 18;
const int kBackgroundColor = GColorBlack;
const int kTextColor = GColorWhite;

// Global variables
int Jlatitude, Jlongitude;
int Jtimezone;
int Jtemperature, JtempMin, JtempMax;
//int Jdst;
char *Jcity;
struct tm *currentPblTime  ;   // Keep current time so its available in all functions
int hebrewDayNumber;        // Current hebrew day
char *timeFormat;           // Format string to use for times (must change according to 24h or 12h option)
int currentTime, sunriseTime, sunsetTime, hatsotTime, timeUntilNextHour;    // Zmanim as minutes from midnight
int zmanHourNumber;         // current zman hour number
float zmanHourDuration;     // zman hour duration

// Sun path
GPath *sun_path;
GPathInfo sun_path_info = {
    5,
    (GPoint []) {
        {0, 0},
        {-73, +84}, //replaced by sunrise angle
        {-73, +84}, //bottom left
        {+73, +84}, //bottom right
        {+73, +84}, //replaced by sunset angle
    }
};

// Weather Icons
static const uint32_t WEATHER_ICONS_WHITE[] = {
    RESOURCE_ID_IMAGE_SUN_WHITE, //0
    RESOURCE_ID_IMAGE_CLOUD_WHITE, //1
    RESOURCE_ID_IMAGE_RAIN_WHITE, //2
    RESOURCE_ID_IMAGE_SNOW_WHITE //3
};

static const uint32_t WEATHER_ICONS_BLACK[] = {
    RESOURCE_ID_IMAGE_SUN_BLACK, //0
    RESOURCE_ID_IMAGE_CLOUD_BLACK, //1
    RESOURCE_ID_IMAGE_RAIN_BLACK, //2
    RESOURCE_ID_IMAGE_SNOW_BLACK //3
};

// AppMessage and AppSync
//static AppSync sync;
static uint8_t sync_buffer[120];

enum JewishClockKey {
    LATITUDE_KEY = 0x0,
    LONGITUDE_KEY = 0x1,
    TIMEZONE_KEY = 0x2,
    TEMPERATURE_KEY = 0x3,
    TEMP_MIN_KEY = 0x4,
    TEMP_MAX_KEY = 0x5,
    ICON_KEY = 0x6,
    CITY_KEY = 0x7
};

// Storage Keys
const uint32_t STORAGE_LATITUDE = 0x1000;
const uint32_t STORAGE_LONGITUDE = 0x1001;
const uint32_t STORAGE_TIMEZONE = 0x1002;
const uint32_t STORAGE_ICON = 0x1003;
const uint32_t STORAGE_CITY = 0x1004;

// Some function definitions
void updateWatch();
void doEveryDay();
void doEveryHour();
void doEveryMinute();
void updateTime();
void updateDate();
void updateHebrewDate();
void updateMoonAndSun();
void updateZmanim();
void checkAlerts();

static void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *tuple = dict_read_first(iter);
    while (tuple) {
        switch (tuple->key) {
            case LATITUDE_KEY: {
                int newLat = tuple->value->int32;
                Jlatitude = newLat;
                break;
            }
            case LONGITUDE_KEY: {
                int newLon = tuple->value->int32;
                Jlongitude = newLon;
                break;
            }
            case TIMEZONE_KEY: {
                int newTz = tuple->value->int32;
                Jtimezone = newTz;
                break;
            }
            case ICON_KEY: {
                if (icon_white) {
                    gbitmap_destroy(icon_white);
                }
                if (icon_black) {
                    gbitmap_destroy(icon_black);
                }
                int index = tuple->value->uint8;
                if ((index<0) || (index>3)) {
                    index = 1;
                }
                icon_white = gbitmap_create_with_resource(WEATHER_ICONS_WHITE[index]);
                icon_black = gbitmap_create_with_resource(WEATHER_ICONS_BLACK[index]);
                
                // Use GCompOpOr to display the white portions of the image
                bitmap_layer_set_bitmap(icon_layer_white, icon_white);
                bitmap_layer_set_compositing_mode(icon_layer_white, GCompOpOr);
                
                // Use GCompOpClear to display the black portions of the image
                bitmap_layer_set_bitmap(icon_layer_black, icon_black);
                bitmap_layer_set_compositing_mode(icon_layer_black, GCompOpClear);
                break;
            }
            case TEMPERATURE_KEY: {
                Jtemperature = tuple->value->int32;
                snprintf(temperatureString, 10, "%i °C", Jtemperature);
                text_layer_set_text(temperature_layer, temperatureString);
                break;
            }
            case TEMP_MIN_KEY: {
                JtempMin = tuple->value->int32;
                snprintf(tempMinString, 10, "%i °C", JtempMin);
                text_layer_set_text(temp_min_layer, tempMinString);
                break;
            }
            case TEMP_MAX_KEY: {
                JtempMax = tuple->value->int32;
                snprintf(tempMaxString, 10, "%i °C", JtempMax);
                text_layer_set_text(temp_max_layer, tempMaxString);
                break;
            }
            case CITY_KEY: {
                strcpy(Jcity, tuple->value->cstring);
                text_layer_set_text(citylayer, Jcity);
                break;
            }
        }
        tuple = dict_read_next(iter);
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RECEIVED DATA lat=%i lon=%i, tz=%i, temp=%i, min=%i, max=%i, city=%s", Jlatitude, Jlongitude, Jtimezone, Jtemperature, JtempMin, JtempMax, Jcity);
    updateWatch();
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

static void app_message_init(void) {
    // Register message handlers
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    // Init buffers
    const uint32_t inbound_size = app_message_inbox_size_maximum();
    const uint32_t outbound_size = 32;
    app_message_open(inbound_size, outbound_size);
}

//static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
//  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
//}

//static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
//    int keyNumber = key;
//    APP_LOG(APP_LOG_LEVEL_DEBUG,"KEY UPDATED: %i", keyNumber);
//    if(key != CITY_KEY) {
//        int value = new_tuple->value->int32;
//        APP_LOG(APP_LOG_LEVEL_DEBUG, "WITH VALUE: %i", value);
//    }
//    switch (key) {
//      case LATITUDE_KEY: {
//          int newLat = new_tuple->value->int32;
//          Jlatitude = newLat;
//          updateWatch();
//      break;
//      }
//      case LONGITUDE_KEY: {
//          int newLon = new_tuple->value->int32;
//          Jlongitude = newLon;
//          updateWatch();
//          break;
//      }
//      case TIMEZONE_KEY: {
//          int newTz = new_tuple->value->int32;
//          Jtimezone = newTz;
//          updateWatch();
//          break;
//      }
//        case ICON_KEY: {
//            if (icon_white) {
//                gbitmap_destroy(icon_white);
//            }
//            if (icon_black) {
//                gbitmap_destroy(icon_black);
//            }
//            int index = new_tuple->value->uint8;
//            if ((index<0) || (index>3)) {
//                index = 1;
//            }
//            icon_white = gbitmap_create_with_resource(WEATHER_ICONS_WHITE[index]);
//            icon_black = gbitmap_create_with_resource(WEATHER_ICONS_BLACK[index]);
//            
//            // Use GCompOpOr to display the white portions of the image
//            bitmap_layer_set_bitmap(icon_layer_white, icon_white);
//            bitmap_layer_set_compositing_mode(icon_layer_white, GCompOpOr);
//            
//            // Use GCompOpClear to display the black portions of the image
//            bitmap_layer_set_bitmap(icon_layer_black, icon_black);
//            bitmap_layer_set_compositing_mode(icon_layer_black, GCompOpClear);
//            break;
//        }
//        case TEMPERATURE_KEY: {
//            Jtemperature = new_tuple->value->int32;
//            snprintf(temperatureString, 10, "%i °C", Jtemperature);
//            text_layer_set_text(temperature_layer, temperatureString);
//            break;
//        }
//        case TEMP_MIN_KEY: {
//            JtempMin = new_tuple->value->int32;
//            snprintf(tempMinString, 10, "%i °C", JtempMin);
//            text_layer_set_text(temp_min_layer, tempMinString);
//            break;
//        }
//        case TEMP_MAX_KEY: {
//            JtempMax = new_tuple->value->int32;
//            snprintf(tempMaxString, 10, "%i °C", JtempMax);
//            text_layer_set_text(temp_max_layer, tempMaxString);
//            break;
//        }
//        case CITY_KEY: {
//            strcpy(Jcity, new_tuple->value->cstring);
//            text_layer_set_text(citylayer, Jcity);
//            break;
//        }
//    }
//    APP_LOG(APP_LOG_LEVEL_DEBUG, "RECEIVED SYNC DATA lat=%i lon=%i, tz=%i, temp=%i, min=%i, max=%i, city=%s", Jlatitude, Jlongitude, Jtimezone, Jtemperature, JtempMin, JtempMax, Jcity);
//}

static void send_cmd(void) {    // send a dummy integer with value 1 to initiate data fetch from the .js
  Tuplet value = TupletInteger(1, 1);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    return;
  }
  dict_write_tuplet(iter, &value);
  dict_write_end(iter);
  app_message_outbox_send();
}

// ******************** Utility functions ****************

// Initializes a text layer
void initTextLayer(TextLayer **theLayer, int x, int y, int w, int h, GColor textColor, GColor backgroundColor, GTextAlignment alignment, GFont theFont) {
    *theLayer =  text_layer_create(GRect(x, y, w, h));
    text_layer_set_text_color(*theLayer, textColor);
    text_layer_set_background_color(*theLayer, backgroundColor);
    text_layer_set_text_alignment(*theLayer, alignment);
    text_layer_set_font(*theLayer, theFont);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(*theLayer));
}

void adjustTimezone(int* time)  // time as minutes since midnight
{
//    if (Jdst != 0)
//    {
//        *time += 60;
//    }
    *time += (Jtimezone);
    if (*time >= (24*60)) *time -= (24*60);
    if (*time < 0) *time += (24*60);
}

//return julian day number for time
int tm2jd(struct tm *time)
{
    int y,m;
    y = time->tm_year + 1900;
    m = time->tm_mon + 1;
    return time->tm_mday-32075+1461*(y+4800+(m-14)/12)/4+367*(m-2-(m-14)/12*12)/12-3*((y+4900+(m-14)/12)/100)/4;
}

int moon_phase(int jdn)
{
    double jd;
    jd = jdn-2451550.1;
    jd /= 29.530588853;
    jd -= (int)jd;
    return (int)(jd*27 + 0.5); /* scale fraction from 0-27 and round by adding 0.5 */
}

int hours2Minutes(float theTime) {
    int hours = (int)theTime;
    int minutes = (int)((theTime - hours)*60.0);
    return (hours * 60) + minutes;
}

float minutes2Hours(int theTime) {
    return ((float)(theTime))/60.0;
}

void displayTime(int theTime, TextLayer *theLayer, char *theString, int maxSize){
    struct tm thePblTime;
    thePblTime.tm_hour = theTime / 60;
    thePblTime.tm_min = theTime % 60;
    strftime(theString, maxSize, timeFormat, &thePblTime);
    text_layer_set_text(theLayer, theString);
}

// Draw line
void lineLayerUpdate(Layer *me, GContext* ctx) {
    (void)me;
    graphics_context_set_stroke_color(ctx, kTextColor);
    graphics_draw_line(ctx, GPoint(0, 97), GPoint(144, 97));
    graphics_draw_line(ctx, GPoint(0, 98), GPoint(144, 98));
}

// Draw sun graph
void sunGraphLayerUpdate(Layer *me, GContext* ctx)
{
    (void)me;
    
    // Fill layer with black
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, sunSize+2, sunSize+2), 0, 0);
    
    GPoint sunCenter = GPoint(sunSize/2, sunSize/2);
    // Draw white filled circle
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, sunCenter, sunSize/2);
    
    // Must fill night part with black
    graphics_context_set_fill_color(ctx, GColorBlack);
    sun_path = gpath_create(&sun_path_info);
    gpath_move_to(sun_path, sunCenter);
    gpath_draw_filled(ctx, sun_path);
    
    // Draw white circle
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, sunCenter, sunSize/2);
    
    // Draw hand/needle at current time
    // Black if day, white if night
    if((currentTime >= sunriseTime) && (currentTime <= sunsetTime)) { // Day
        graphics_context_set_stroke_color(ctx, GColorBlack);
    } else {  // night
        graphics_context_set_stroke_color(ctx, GColorWhite);
    }
    float angle = (18.0 - minutes2Hours(currentTime))/24.0 * 2.0 * M_PI;
    GPoint toPoint = GPoint(sunCenter.x + my_cos(angle)*sunSize/2, sunCenter.y - my_sin(angle)*sunSize/2);
    graphics_draw_line(ctx, sunCenter, toPoint);
}

// Update time
void updateTime() {
    displayTime(currentTime, timeLayer, timeString, sizeof(timeString));
}

// Update zmanim
void updateZmanim() {
    zmanHourDuration = (minutes2Hours(sunsetTime) - minutes2Hours(sunriseTime)) / 12.0; // in hours
    int startTime = sunriseTime;
    int theTime = currentTime;
    if((currentTime < sunriseTime) || (currentTime > sunsetTime)) { // Night
        startTime = sunsetTime;
        zmanHourDuration = 2.0 - zmanHourDuration; // Day hour + Night hour = 2
        if(currentTime < sunriseTime) theTime += 24*60;
    }
    float zh = ((float)(theTime - startTime))/60.0/zmanHourDuration;
    zmanHourNumber = (int)zh + 1;
    float hoursUntilNext = ((float)zmanHourNumber)*zmanHourDuration;
    timeUntilNextHour = startTime + ((int)(hoursUntilNext * 60.0)) - theTime;
    
    int nextHour = timeUntilNextHour / 60;
    int nextMinute = timeUntilNextHour % 60;
    snprintf(zmanHourString, 2, "%i", zmanHourNumber);
    snprintf(nextHourString, 6, "%i:%02i",nextHour, nextMinute);
    text_layer_set_text(zmanHourLayer, zmanHourString);
    text_layer_set_text(nextHourLayer, nextHourString);
}

// **** Check if alert needed and show/vibrate if needed
void checkAlerts() {
    static int mustRemove = 0;
    int mustAlert = 0;
    
    if(mustRemove) {
        layer_remove_from_parent(text_layer_get_layer(alertLayer));
        mustRemove=0;
    }
    if(currentTime == (hatsotTime + ((int)(zmanHourDuration*60.0*0.5)))) {  // half an hour after midday
        mustAlert = -1;
        snprintf(alertString, 12, "MINCHA-G");
    }
    if(currentTime == (sunsetTime - ((int)(zmanHourDuration*60.0*2.5)))) {  // 2,5 hours before sunset
        mustAlert = -1;
        snprintf(alertString, 12, "MINCHA-K");
    }
    if(currentTime == (sunsetTime - MINCHA_ALERT)) { // this is the minute for the alert
        mustAlert = -1;
        snprintf(alertString, 12, "SUNSET-%imn", MINCHA_ALERT);
    }
    if(currentTime == (sunsetTime - ((int)(zmanHourDuration*60.0*1.25)))) {  // 1 and a quarter hour before sunset
        mustAlert = -1;
        snprintf(alertString, 12, "PLAG");
    }
    if(currentTime == (sunsetTime)) {
        mustAlert = -1;
        snprintf(alertString, 12, "SUNSET NOW!");
    }
    if(mustAlert) {
        mustAlert = -1;
        text_layer_set_text(alertLayer, alertString);
        layer_add_child(window_get_root_layer(window), text_layer_get_layer(alertLayer));  // show message
        vibes_short_pulse();
        mustRemove=-1;
    }
}

// Update MOON phase and Sun info
void updateMoonAndSun() {
    // ******************* MOON
    
    // Moonphase font:
    // A-Z phases on white background
    // a-z phases on black background
    // 0 = new moon
    
    float jDay = (float) (hebrewDayNumber - 1);     // 0 to 29
    float mPhase = jDay * 26.0 / 29.0;  // 0 to 26
    int moonphase_number = (int)mPhase;
    // correct for southern hemisphere
    if ((moonphase_number > 0) && (Jlatitude < 0)) {
        moonphase_number = 26 - moonphase_number;
    }
    
    char moonChar;
    
    if(moonphase_number < 1) {    // new moon
        moonChar = '0';
#ifndef REVERSED
        moonChar = 'N';
#endif
    } else {
        int offset = moonphase_number - 1;
#ifndef REVERSED
        // Black background we must use the opposite phase direction...
        if(offset >= 13) {
            offset -= 13;
        } else {
            offset += 13;
        }
#endif
        moonChar = 'A' + offset;
    }
    moonString[0] = moonChar;
    
    text_layer_set_text(
                        moonLayer, moonString);
    
    // ******************* SUN TIMES
    //  sunriseTime = hours2Minutes(calcSunRise(currentPblTime.tm_year, currentPblTime.tm_mon+1, currentPblTime.tm_mday, LATITUDE, LONGITUDE, 91.0f));
    //  sunsetTime = hours2Minutes(calcSunSet(currentPblTime.tm_year, currentPblTime.tm_mon+1, currentPblTime.tm_mday, LATITUDE, LONGITUDE, 91.0f));
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "SUN CALCULATION with lat=%i lon=%i, timezone=%i", Jlatitude, Jlongitude, Jtimezone);
    double Dlat=((double)Jlatitude)/1000.0;
    double Dlong = ((double)Jlongitude)/1000.0;
    hdate_get_utc_sun_time(currentPblTime->tm_mday, (currentPblTime->tm_mon)+1, currentPblTime->tm_year, Dlat, Dlong, &sunriseTime, &sunsetTime);
	hatsotTime = (sunriseTime+sunsetTime)/2;
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "UTC Sunrise=%i, UTC Sunset = %i", sunriseTime, sunsetTime);
    
    adjustTimezone(&sunriseTime);
    adjustTimezone(&sunsetTime);
    adjustTimezone(&hatsotTime);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "LOCAL Sunrise=%i, LOCAL Sunset = %i", sunriseTime, sunsetTime);
    
    displayTime(sunriseTime, sunriseLayer, sunriseString, sizeof(sunriseString));
    displayTime(hatsotTime, hatsotLayer, hatsotString, sizeof(hatsotString));
    displayTime(sunsetTime, sunsetLayer, sunsetString, sizeof(sunsetString));
    
    // SUN GRAPHIC
    float rise2 = minutes2Hours(sunriseTime)+12.0f;
    sun_path_info.points[1].x = (int16_t)(my_sin(rise2/24 * M_PI * 2) * 120);
    sun_path_info.points[1].y = -(int16_t)(my_cos(rise2/24 * M_PI * 2) * 120);
    float set2 =  minutes2Hours(sunsetTime)+12.0f;
    sun_path_info.points[4].x = (int16_t)(my_sin(set2/24 * M_PI * 2) * 120);
    sun_path_info.points[4].y = -(int16_t)(my_cos(set2/24 * M_PI * 2) * 120);
}


// Update Hebrew Date
void updateHebrewDate() {
    int julianDay = hdate_gdate_to_jd(currentPblTime->tm_mday, currentPblTime->tm_mon + 1, currentPblTime->tm_year + 1900);
    // Convert julian day to hebrew date
    int hDay, hMonth, hYear, hDayTishrey, hNextTishrey;
    hdate_jd_to_hdate(julianDay, &hDay, &hMonth, &hYear, &hDayTishrey, &hNextTishrey);
    hebrewDayNumber = hDay;
    char *hebrewMonthName = hdate_get_month_string(hMonth);
    xsprintf(hDayString, "%d",hDay);
    text_layer_set_text(hDayLayer, hDayString);
    text_layer_set_text(hMonthLayer, hebrewMonthName);
}

// Update Gregorian Date
void updateDate() {
    static char *monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    xsprintf(dayString, "%d", currentPblTime->tm_mday);
    xsprintf(monthString, "%s", monthNames[currentPblTime->tm_mon]);
    text_layer_set_text(dayLayer, dayString);
    text_layer_set_text(monthLayer, monthString);
}

// Called once per day at midnight, and once at startup
void doEveryDay() {
    updateDate();
    updateHebrewDate();
    updateMoonAndSun();
}

// Called once per hour when minute becomes 0, and once at startup
void doEveryHour() {
}

// Called once per minute, and once at startup
void doEveryMinute() {
    updateTime();
    updateZmanim();
    // Must update Sun Graph rendering
    layer_mark_dirty(sunGraphLayer);
    checkAlerts();
}

// ************* Update watch with current data
void updateWatch() {
    time_t unixTime;
    time(&unixTime);
    currentPblTime = localtime(&unixTime);
    currentTime = (currentPblTime->tm_hour * 60) + currentPblTime->tm_min;
    
    doEveryDay();
    doEveryHour();
    doEveryMinute();
}

// ************* TICK HANDLER *****************
// Handles the system minute-tick, calls appropriate functions above
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    static int updateCounter = UPDATE_MINUTES;
    currentPblTime = tick_time;
    currentTime = (currentPblTime->tm_hour * 60) + currentPblTime->tm_min;
    
    static int currentYDay = -1;
    static int currentHour = -1;
    
    // call update functions as required, daily first, then hourly, then minute
    if(currentPblTime->tm_yday != currentYDay) {  // Day has changed, or app just started
        currentYDay = currentPblTime->tm_yday;
        doEveryDay();
    }
    if(currentPblTime->tm_hour != currentHour) {  // Hour has changed, or app just started
        currentHour = currentPblTime->tm_hour;
        doEveryHour();
        send_cmd(); // fetch new data from phone once every hour
    }
    doEveryMinute();
//    if(++updateCounter >= UPDATE_MINUTES) { // auto-update data from phone every few minutes
//        updateCounter=0;
//        send_cmd();
//    }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);

    // Default Values
    Jcity = "Unknow";
    Jlatitude = Jlongitude = Jtimezone = 0;
    Jtemperature = JtempMax = JtempMin = 0;
    
    // Try to load values from storage
    if(persist_exists(STORAGE_LATITUDE)) {
        Jlatitude = persist_read_int(STORAGE_LATITUDE);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Restored Latitude: %i", Jlatitude);
    }
    if(persist_exists(STORAGE_LONGITUDE)) {
        Jlongitude = persist_read_int(STORAGE_LONGITUDE);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Restored Longitude: %i", Jlongitude);
    }
    if(persist_exists(STORAGE_TIMEZONE)) {
        Jtimezone = persist_read_int(STORAGE_TIMEZONE);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Restored Timezone: %i", Jtimezone);
    }
//    if(persist_exists(STORAGE_DST)) {
//        Jdst = persist_read_int(STORAGE_DST);
//        APP_LOG(APP_LOG_LEVEL_DEBUG, "Restored DST: %i", Jdst);
//    }
    if(persist_exists(STORAGE_CITY)) {
        persist_read_string(STORAGE_CITY, Jcity, 22);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Restored City: %s", Jcity);
    }
    
//    Tuplet initial_values[] = {
//      TupletInteger(LATITUDE_KEY, Jlatitude),
//      TupletInteger(LONGITUDE_KEY, Jlongitude),
//        TupletInteger(TIMEZONE_KEY, Jtimezone),
////        TupletInteger(DST_KEY, Jdst),
//        TupletInteger(TEMPERATURE_KEY, 0),
//        TupletCString(CITY_KEY, Jcity),
//        TupletInteger(ICON_KEY, (uint8_t)1),
//    };

//    const int inbound_size = 120;
//    const int outbound_size = 120;
//    app_message_open(inbound_size, outbound_size);
    
//    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
//        sync_tuple_changed_callback, sync_error_callback, NULL);
//    send_cmd();
  
    app_message_init(); // initialises communication between this watchapp and the .js in the phone
    
    // Define fonts
    tinyFont = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    mediumFont = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    mediumBoldFont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    largeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
    moonFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30));
    
    // ******************************************
    // Init Layers - top to bottom, left to right
    // ******************************************
    
    // Gregorian Day
    initTextLayer(&dayLayer, 0, 0, 144, 25, kTextColor, GColorClear, GTextAlignmentLeft, mediumFont);
    
    // Hebrew Day
    initTextLayer(&hDayLayer, 0, 0, 144, 25, kTextColor, GColorClear, GTextAlignmentRight, mediumFont);
    
    //  Moon phase
    initTextLayer(&moonLayer, 56, 7, 32, 32, kTextColor, GColorClear, GTextAlignmentCenter, moonFont);
    
    
    // Gregorian Month
    initTextLayer(&monthLayer, 0, 25, 144, 15, kTextColor, GColorClear, GTextAlignmentLeft, smallFont);
    
    // Hebrew Month
    initTextLayer(&hMonthLayer, 0, 25, 144, 15, kTextColor, GColorClear, GTextAlignmentRight, smallFont);
    
    // City
    initTextLayer(&citylayer, 0, 32, 144, 30, kTextColor, GColorClear, GTextAlignmentCenter, tinyFont);
    text_layer_set_text(citylayer, Jcity);
    
    //  Time
    initTextLayer(&timeLayer, 0, 40, 144, 50, kTextColor, GColorClear, GTextAlignmentCenter, largeFont);
    
    // Line
    lineLayer = layer_create(layer_get_frame(window_layer));
    layer_set_update_proc(lineLayer, &lineLayerUpdate);
    layer_add_child(window_layer, lineLayer);
    
    // Zman hours labels
    initTextLayer(&zmanHourLabelLayer, 0, 100, 144, 15, kTextColor, GColorClear, GTextAlignmentLeft, smallFont);
    text_layer_set_text(zmanHourLabelLayer, zmanHourLabelString);
    initTextLayer(&nextHourLabelLayer, 0, 100, 144, 15, kTextColor, GColorClear, GTextAlignmentRight, smallFont);
    text_layer_set_text(nextHourLabelLayer, nextHourLabelString);
    
    // Sun Graph
    sunGraphLayer = layer_create(GRect(72-sunSize/2, sunY, sunSize+2, sunSize+2));
    layer_set_update_proc(sunGraphLayer, &sunGraphLayerUpdate);
    layer_set_clips(sunGraphLayer, true);
#ifndef USE_WEATHER
    layer_add_child(window_layer, sunGraphLayer);   // Show sun graph instead of weather icon
#endif
    
    // Weather Icon
    icon_layer_white = bitmap_layer_create(GRect(32, 82, 80, 80));
    icon_layer_black = bitmap_layer_create(GRect(32, 82, 80, 80));
#ifdef USE_WEATHER
    layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer_black));
    layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer_white));
#endif
    
    // Optional Alert message
    initTextLayer(&alertLayer, 0, 102, 144, 36, kBackgroundColor, kTextColor, GTextAlignmentCenter, mediumBoldFont);
    layer_remove_from_parent(text_layer_get_layer(alertLayer));  // don't show now!
    
    // Zman hour number and Next zman hour
#ifdef USE_WEATHER
    initTextLayer(&zmanHourLayer, 0, 108, 144, 25, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    initTextLayer(&nextHourLayer, 0, 108, 144, 25, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
#else
    initTextLayer(&zmanHourLayer, 0, 108, 144, 25, kTextColor, GColorClear, GTextAlignmentLeft, mediumFont);
    initTextLayer(&nextHourLayer, 0, 108, 144, 25, kTextColor, GColorClear, GTextAlignmentRight, mediumFont);
#endif
    
    // Hatsot hour
    initTextLayer(&hatsotLayer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentCenter, tinyFont);
#ifdef USE_WEATHER
    layer_remove_from_parent(text_layer_get_layer(hatsotLayer));
#endif
    
    // Temperature
    initTextLayer(&temperature_layer, 0, 138, 144, 30, kTextColor, GColorClear, GTextAlignmentCenter, mediumFont);
    initTextLayer(&temp_min_layer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    initTextLayer(&temp_max_layer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
#ifndef USE_WEATHER
    layer_remove_from_parent(text_layer_get_layer(temperature_layer));
    layer_remove_from_parent(text_layer_get_layer(temp_min_layer));
    layer_remove_from_parent(text_layer_get_layer(temp_max_layer));
#endif
    
    //  Sunrise and Sunset hour
#ifdef USE_WEATHER
    initTextLayer(&sunriseLayer, 0, 124, 144, 30, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    initTextLayer(&sunsetLayer, 0, 124, 144, 30, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
#else
    initTextLayer(&sunriseLayer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    initTextLayer(&sunsetLayer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
#endif
    
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
    updateWatch();
}

static void window_unload(Window *window) {
    // Save values in storage
    APP_LOG(APP_LOG_LEVEL_DEBUG, "**** SAVING DATA IN STORAGE ****");
    persist_write_int(STORAGE_LATITUDE, Jlatitude);
    persist_write_int(STORAGE_LONGITUDE, Jlongitude);
    persist_write_int(STORAGE_TIMEZONE, Jtimezone);
//    persist_write_int(STORAGE_DST, Jdst);
    persist_write_string(STORAGE_CITY, Jcity);
    
//    app_sync_deinit(&sync);
    app_message_deregister_callbacks();

    if (icon_white) {
        gbitmap_destroy(icon_white);
    }
    if (icon_black) {
        gbitmap_destroy(icon_black);
    }
    
    text_layer_destroy(dayLayer);
    text_layer_destroy(hDayLayer);
    text_layer_destroy(moonLayer);
    text_layer_destroy(monthLayer);
    text_layer_destroy(hMonthLayer);
    text_layer_destroy(citylayer);
    text_layer_destroy(timeLayer);
    text_layer_destroy(zmanHourLabelLayer);
    text_layer_destroy(nextHourLabelLayer);
//    text_layer_destroy(currentZmanLayer);
//    text_layer_destroy(EndOfZmanLayer);
    text_layer_destroy(zmanHourLayer);
    text_layer_destroy(nextHourLayer);
    text_layer_destroy(alertLayer);
    text_layer_destroy(sunriseLayer);
    text_layer_destroy(sunsetLayer);
    text_layer_destroy(hatsotLayer);
    text_layer_destroy(temperature_layer);
    text_layer_destroy(temp_min_layer);
    text_layer_destroy(temp_max_layer);
    
    layer_destroy(lineLayer);
    layer_destroy(sunGraphLayer);
    
    bitmap_layer_destroy(icon_layer_black);
    bitmap_layer_destroy(icon_layer_white);
}

static void init(void) {
    window = window_create();
    window_set_background_color(window, GColorBlack);
    window_set_fullscreen(window, true);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    
    // Init time format string
    timeFormat = clock_is_24h_style()?"%R":"%I:%M";
    
    // Init time struct
    time_t unixTime;
    time(&unixTime);
    currentPblTime = localtime(&unixTime);
    
    const bool animated = true;
    window_stack_push(window, animated);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}

