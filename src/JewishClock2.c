#include <pebble.h>
#include "hebrewdate.h"
#include "hdate_sun_time.h"
#include "xprintf.h"
#include "my_math.h"

static Window *window;  // Main Window

// ******************************************
// Layers - top to bottom, left to right
// ******************************************
TextLayer *dayLayer;            char dayString[]=           "17";
TextLayer *hDayLayer;           char hDayString[]=          "13";
TextLayer *moonLayer;           char moonString[]=          "G";
TextLayer *monthLayer;          char monthString[]=         "May";
TextLayer *hMonthLayer;
TextLayer *timeLayer;           char timeString[]=          "00:00";
Layer *lineLayer;
TextLayer *zmanHourLabelLayer;  char zmanHourLabelString[]= "Hour #";
TextLayer *nextHourLabelLayer;  char nextHourLabelString[]= "Next In:";
Layer *sunGraphLayer;
//TextLayer *currentZmanLayer;    char currentZmanString[]=   "Mincha Gedola";
//TextLayer *EndOfZmanLayer;      char endOfZmanString[]=     "00:00";
TextLayer *zmanHourLayer;       char zmanHourString[]=      "11";
TextLayer *nextHourLayer;       char nextHourString[]=      "01:07:00";
TextLayer *alertLayer;          char alertString[]=    "SUNSET IN 000mn";
TextLayer *sunriseLayer;        char sunriseString[]=       "00:00";
TextLayer *sunsetLayer;         char sunsetString[]=        "00:00";
TextLayer *hatsotLayer;         char hatsotString[]=        "00:00";

// Constants
const int sunY = 104;
const int sunSize = 38;
const int MINCHA_ALERT = 18;
const int kBackgroundColor = GColorBlack;
const int kTextColor = GColorWhite;

// Global variables
double Jlatitude, Jlongitude;
int Jtimezone;
bool Jdst;
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

// AppMessage and AppSync
static AppSync sync;
static uint8_t sync_buffer[64];

enum JewishClockKey {
    LATITUDE_KEY = 0x1,
    LONGITUDE_KEY = 0x2,
    TIMEZONE_KEY = 0x3,
    DST_KEY = 0x4,
};

// Some function definitions
void updateWatch(bool forceUpdate);
void doEveryDay();
void doEveryHour();
void doEveryMinute();
void updateTime();
void updateDate();
void updateHebrewDate();
void updateMoonAndSun();
void updateZmanim();
void checkAlerts();


static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch (key) {
      case LATITUDE_KEY: {
          int newLat = new_tuple->value->int32;
          Jlatitude = ((float)newLat)/1000.0;
      break;
      }
      case LONGITUDE_KEY: {
          int newLon = new_tuple->value->int32;
          Jlongitude = ((float)newLon)/1000.0;
          break;
      }
      case TIMEZONE_KEY: {
          int newTz = new_tuple->value->int32;
          Jtimezone = newTz;
          break;
      }
      case DST_KEY: {
          int newDst = new_tuple->value->int32;
          Jdst = (newDst != 0);
          break;
      }
    }
    updateWatch(true);
}

static void send_cmd(void) {
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
    // ****************** warning tm_idst is not implemented yet, currently using a compile flag, find another way! ********************
    if (Jdst)  // Currently using DST flag in config.h
    {
        *time += 60;
    }
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
    xsprintf(zmanHourString, "%d", zmanHourNumber);
    xsprintf(nextHourString, "%d:%02d",nextHour, nextMinute);
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
        xsprintf(alertString, "MINCHA-G", MINCHA_ALERT);
    }
    if(currentTime == (sunsetTime - ((int)(zmanHourDuration*60.0*2.5)))) {  // 2,5 hours before sunset
        mustAlert = -1;
        xsprintf(alertString, "MINCHA-K", MINCHA_ALERT);
    }
    if(currentTime == (sunsetTime - MINCHA_ALERT)) { // this is the minute for the alert
        mustAlert = -1;
        xsprintf(alertString, "SUNSET-%dmn", MINCHA_ALERT);
    }
    if(currentTime == (sunsetTime - ((int)(zmanHourDuration*60.0*1.25)))) {  // 1 and a quarter hour before sunset
        mustAlert = -1;
        xsprintf(alertString, "PLAG", MINCHA_ALERT);
    }
    if(currentTime == (sunsetTime)) {
        mustAlert = -1;
        xsprintf(alertString, "SUNSET NOW!", MINCHA_ALERT);
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
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "SUN CALCULATION with lat=%i lon=%i, timezone=%i, dst=%i",(int)(Jlatitude*1000.0), (int)(Jlongitude*1000.0), Jtimezone, Jdst);
    hdate_get_utc_sun_time(currentPblTime->tm_mday, (currentPblTime->tm_mon)+1, currentPblTime->tm_year, Jlatitude, Jlongitude, &sunriseTime, &sunsetTime);
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

// ************* Update watch as needed, calls required update functions at the right times, and once at startup
void updateWatch(bool forceUpdate) {
    static int currentYDay = -1;
    static int currentHour = -1;
    if(forceUpdate) {
        currentYDay = currentHour = -1;
    }
    
    currentTime = (currentPblTime->tm_hour * 60) + currentPblTime->tm_min;

    // call update functions as required, daily first, then hourly, then minute
    if(currentPblTime->tm_yday != currentYDay) {  // Day has changed, or app just started
        currentYDay = currentPblTime->tm_yday;
        doEveryDay();
    }
    if(currentPblTime->tm_hour != currentHour) {  // Hour has changed, or app just started
        currentHour = currentPblTime->tm_hour;
        doEveryHour();
    }
    doEveryMinute();
}

// ************* TICK HANDLER *****************
// Handles the system minute-tick, calls appropriate functions above
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    currentPblTime = tick_time;
    updateWatch(false);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);

    Tuplet initial_values[] = {
      TupletInteger(LATITUDE_KEY, 0),
      TupletInteger(LONGITUDE_KEY, 0),
        TupletInteger(TIMEZONE_KEY, 0),
        TupletInteger(DST_KEY, 0)
    };

    const int inbound_size = 64;
    const int outbound_size = 64;
    app_message_open(inbound_size, outbound_size);
    
    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
        sync_tuple_changed_callback, sync_error_callback, NULL);

    send_cmd();
    
    // Define fonts
    GFont tinyFont = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    GFont smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    GFont mediumFont = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    GFont mediumBoldFont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    GFont largeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
    GFont moonFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30));
    
    // ******************************************
    // Init Layers - top to bottom, left to right
    // ******************************************
    
    // Gregorian Day
    initTextLayer(&dayLayer, 0, 0, 144, 25, kTextColor, GColorClear, GTextAlignmentLeft, mediumFont);
    
    // Hebrew Day
    initTextLayer(&hDayLayer, 0, 0, 144, 25, kTextColor, GColorClear, GTextAlignmentRight, mediumFont);
    
    //  Moon phase
    initTextLayer(&moonLayer, 56, 9, 32, 32, kTextColor, GColorClear, GTextAlignmentCenter, moonFont);
    
    
    // Gregorian Month
    initTextLayer(&monthLayer, 0, 25, 144, 15, kTextColor, GColorClear, GTextAlignmentLeft, smallFont);
    
    // Hebrew Month
    initTextLayer(&hMonthLayer, 0, 25, 144, 15, kTextColor, GColorClear, GTextAlignmentRight, smallFont);
    
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
    layer_add_child(window_layer, sunGraphLayer);
    
    // Optional Alert message
    initTextLayer(&alertLayer, 0, 102, 144, 36, kBackgroundColor, kTextColor, GTextAlignmentCenter, mediumBoldFont);
    layer_remove_from_parent(text_layer_get_layer(alertLayer));  // don't show now!
    
    // Zman hour number
    initTextLayer(&zmanHourLayer, 0, 108, 144, 25, kTextColor, GColorClear, GTextAlignmentLeft, mediumFont);
    
    // Next zman hour
    initTextLayer(&nextHourLayer, 0, 108, 144, 25, kTextColor, GColorClear, GTextAlignmentRight, mediumFont);
    
    //  Sunrise hour
    initTextLayer(&sunriseLayer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    
    // Hatsot hour
    initTextLayer(&hatsotLayer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentCenter, tinyFont);
    
    //  Sunset hour
    initTextLayer(&sunsetLayer, 0, 145, 144, 30, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
    
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
}

static void window_unload(Window *window) {
    app_sync_deinit(&sync);

    text_layer_destroy(dayLayer);
    text_layer_destroy(hDayLayer);
    text_layer_destroy(moonLayer);
    text_layer_destroy(monthLayer);
    text_layer_destroy(hMonthLayer);
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
    
    layer_destroy(lineLayer);
    layer_destroy(sunGraphLayer);
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

