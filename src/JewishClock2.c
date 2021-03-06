//#ifndef RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49
//#define RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49 ""
//#endif
//
//#ifndef RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30
//#define RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30 ""
//#endif
#include <pebble.h>
#include "hebrewdate.h"
#include "hdate_sun_time.h"
#include "xprintf.h"
#include "my_math.h"

static Window *window;  // Main Window

const bool withoutSunGraph = false;

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
static TextLayer *sunriseLabelLayer;   char zmanHourLabelString[]= "Hour #     ";
static TextLayer *sunsetLabelLayer;   char nextHourLabelString[]= "Next In:   ";
static Layer *sunGraphLayer;
static TextLayer *currentZmanLayer;    char currentZmanString[]=   "Mincha Gedola";
static TextLayer *EndOfZmanLayer;      char endOfZmanString[]=     "00:00";
static TextLayer *zmanHourLayer;       char zmanHourString[]=      "011";
static TextLayer *nextHourLayer;       char nextHourString[]=      "01:07:00";
static TextLayer *alertLayer;          char alertString[]=    "SUNSET IN 000mn";
static TextLayer *sunriseLayer;        char sunriseString[]=       "00:00";
static TextLayer *hatsotLayer;         char hatsotString[]=       "00:00";
static TextLayer *sunsetLayer;         char sunsetString[]=        "00:00";
//static TextLayer *hatsotLayer;         char hatsotString[]=        "00:00";

// Fonts
GFont tinyFont, smallFont, mediumFont, mediumBoldFont, largeFont, moonFont;

// Positions
const int screenWidth = 144;
const int screenHeight = 168;
const int screenMiddleX = 72;
const int screenMiddleY = 84;

static int dayY;
static int monthY;
static int moonY;
static int timeY;

static int lineY;

static int zmanHourY;

static int sunY;
//static int sunSize;

static int sunLabelY;
static int sunHourY;

// Parameters
static int kSHOW_ALERTS = 1; // 0 to disable alerts
static int MINCHA_ALERT = 18;
static int kBackgroundColor = GColorBlack;
static int kTextColor = GColorWhite;

// Global variables
int Jlatitude, Jlongitude;
int Jtimezone;
struct tm *currentPblTime  ;   // Keep current time so its available in all functions
int hebrewDayNumber;        // Current hebrew day
char *timeFormat;           // Format string to use for times (must change according to 24h or 12h option)
int currentTime, sunriseTime, sunsetTime, hatsotTime, timeUntilNextHour;    // Zmanim as minutes from midnight
int zmanHourNumber;         // current zman hour number
float zmanHourDuration;     // zman hour duration

static const int sunSize = 58;
static const int sunRadius = 27;
// Sun path
GPath *sun_path;
//GPathInfo sun_path_info = {
//    6,
//    (GPoint []) {
//        {0, 0},
//        {27, 10}, //replaced by sunrise angle
//        {27, 27}, //bottom right
//        {-27, 27}, //bottom left
//        {-27, 10}, //replaced by sunset angle
//        {0, 0}
//    }
//};

GPathInfo sun_path_info = {
    6,
    (GPoint []) {
        {0, 0},
        {-73, +84}, //replaced by sunrise angle
        {-73, +84}, //bottom left
        {+73, +84}, //bottom right
        {+73, +84}, //replaced by sunset angle
        { 0, 0}
    }
};

// Battery and phone
GColor background_color = GColorWhite;
GColor foreground_color = GColorBlack;
GCompOp compositing_mode = GCompOpAssign;
BitmapLayer *layer_batt_img;
BitmapLayer *layer_conn_img;
GBitmap *img_battery_full;
GBitmap *img_battery_half;
GBitmap *img_battery_low;
GBitmap *img_battery_charge;
GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;
TextLayer *layer_batt_text;
int charge_percent = 0;

// AppMessage and AppSync
//static AppSync sync;
static uint8_t sync_buffer[120];

enum JewishClockKey {
    LATITUDE_KEY = 0x0,
    LONGITUDE_KEY = 0x1,
    TIMEZONE_KEY = 0x2,
};

// Storage Keys
const uint32_t STORAGE_LATITUDE = 0x1000;
const uint32_t STORAGE_LONGITUDE = 0x1001;
const uint32_t STORAGE_TIMEZONE = 0x1002;

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
        }
        tuple = dict_read_next(iter);
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RECEIVED DATA lat=%i lon=%i, tz=%i", Jlatitude, Jlongitude, Jtimezone);
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

void handle_battery(BatteryChargeState charge_state) {
    static char battery_text[] = "+100% ";
    
    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, img_battery_charge);
        
        snprintf(battery_text, sizeof(battery_text), "+%d%%", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
        if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_low);
        } else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_half);
        } else {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_full);
        }
        
        if (charge_state.charge_percent < charge_percent) {
            if (charge_state.charge_percent==20){
                vibes_double_pulse();
            } else if(charge_state.charge_percent==10){
                vibes_long_pulse();
            }
        }
    }
    charge_percent = charge_state.charge_percent;
    
    text_layer_set_text(layer_batt_text, battery_text);
}

void handle_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
        vibes_long_pulse();
    }
}

void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
        handle_battery(battery_state_service_peek());
    }
}

void force_update(void) {
    handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());
}

// Draw line
void lineLayerUpdate(Layer *me, GContext* ctx) {
    (void)me;
    graphics_context_set_stroke_color(ctx, kTextColor);
    graphics_draw_line(ctx, GPoint(0, lineY-1), GPoint(screenWidth, lineY-1));
    graphics_draw_line(ctx, GPoint(0, lineY), GPoint(screenWidth, lineY));
}

// Draw sun graph
void sunGraphLayerUpdate(Layer *me, GContext* ctx)
{
    (void)me;
    
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    
    // Fill layer with black
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, sunRadius, sunRadius), 0, 0);
    
    GPoint sunCenter = GPoint(sunRadius+1, sunRadius+1);
    
    // Fill white filled circle
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, sunCenter, sunRadius);
    
    // Must fill night part with black
    if(sun_path != NULL) {
        gpath_destroy(sun_path);
        sun_path=NULL;
    }
    sun_path = gpath_create(&sun_path_info);
//    APP_LOG(APP_LOG_LEVEL_DEBUG,"SUN_PATH: %i", (int)sun_path->num_points);
//    for (int k=0; k<(int)(sun_path->num_points); k++) {
//        GPoint p = sun_path->points[k];
//        APP_LOG(APP_LOG_LEVEL_DEBUG, "POINT[%i] = (%i,%i)", k, p.x, p.y);
//    }

    gpath_move_to(sun_path, sunCenter);
    graphics_context_set_fill_color(ctx, GColorBlack);
    gpath_draw_filled(ctx, sun_path);     // ******** BUG in 2.0 firmware, wil not fill the path! *********************
    graphics_context_set_stroke_color(ctx, GColorWhite);
    gpath_draw_outline(ctx, sun_path);

//    GPoint p1 = sun_path->points[1];
//    GPoint p2 = sun_path->points[4];
//    int x1 = p1.x;
//    int x2 = p2.x;
//    GPoint pp = GPoint(x1, sun_path->points[1].y);
//    graphics_context_set_stroke_color(ctx, GColorBlack);
//    graphics_draw_line(ctx, sunCenter, p1 );
//    graphics_draw_line(ctx, sunCenter, p2 );
//    for (int x=x1; x<x2; x++) {
//        pp.x = x;
//        graphics_draw_line(ctx, sunCenter, pp );
//    }
    
    // Draw white circle
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, sunCenter, sunRadius);
    
    // Draw hand/needle at current time
    // Black if day, white if night
    if((currentTime >= sunriseTime) && (currentTime <= sunsetTime)) { // Day
        graphics_context_set_stroke_color(ctx, GColorBlack);
    } else {  // night
        graphics_context_set_stroke_color(ctx, GColorWhite);
    }
    float angle = (18.0 - minutes2Hours(currentTime))/24.0 * 2.0 * M_PI;
    GPoint toPoint = GPoint(sunCenter.x + my_cos(angle)*sunRadius, sunCenter.y - my_sin(angle)*sunRadius);
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
    snprintf(zmanHourString, 3, "%i", zmanHourNumber);
    
    snprintf(nextHourString, 6, "%i:%02i",nextHour, nextMinute);
    text_layer_set_text(zmanHourLayer, zmanHourString);
    text_layer_set_text(nextHourLayer, nextHourString);
}

// **** Check if alert needed and show/vibrate if needed
void checkAlerts() {
    if (!kSHOW_ALERTS) {
        return; // alerts are disabled
    }
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
//    float rise2 = minutes2Hours(sunriseTime);
//    sun_path_info.points[1].y = (int16_t)(my_sin((rise2-6.0)/6.0 * M_PI * 2) * sunRadius);
    float rise2 = minutes2Hours(sunriseTime)+12.0f;
    sun_path_info.points[1].x = (int16_t)(my_sin(rise2/24 * M_PI * 2) * 120);
    sun_path_info.points[1].y = -(int16_t)(my_cos(rise2/24 * M_PI * 2) * 120);
//    float set2 =  minutes2Hours(sunsetTime);
//    sun_path_info.points[4].y = (int16_t)(my_sin((18.0-set2)/6.0 * M_PI * 2) * sunRadius);
    float set2 =  minutes2Hours(sunsetTime)+12.0f;
    sun_path_info.points[4].x = (int16_t)(my_sin(set2/24 * M_PI * 2) * 120);
    sun_path_info.points[4].y = -(int16_t)(my_cos(set2/24 * M_PI * 2) * 120);
}


// Update Hebrew Date
void updateHebrewDate() {
    int julianDay = hdate_gdate_to_jd(currentPblTime->tm_mday, currentPblTime->tm_mon + 1, currentPblTime->tm_year + 1900);
    // check if after sunset, add 1 day
    if(sunsetTime && (currentTime > sunsetTime)) {
        julianDay++;
    }
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
    updateMoonAndSun();
    updateDate();
}

// Called once per hour when minute becomes 0, and once at startup
void doEveryHour() {
}

// Called once per minute, and once at startup
void doEveryMinute() {
    updateTime();
    updateZmanim();
    updateHebrewDate();
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
//    static int updateCounter = UPDATE_MINUTES;
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
//        send_cmd(); // fetch new data from phone once every hour
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
    Jlatitude = Jlongitude = Jtimezone = 0;
    
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
    
    app_message_init(); // initialises communication between this watchapp and the .js in the phone
    
    // Define fonts
    tinyFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    mediumFont = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    mediumBoldFont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    largeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
    moonFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30));
    
    // ******************************************
    // Init Layers - top to bottom, left to right
    // ******************************************
    
    // Gregorian Day
    initTextLayer(&dayLayer, 0, 0, screenWidth, 24, kTextColor, GColorClear, GTextAlignmentLeft, mediumFont);
    
    // Hebrew Day
    initTextLayer(&hDayLayer, 0, 0, screenWidth, 24, kTextColor, GColorClear, GTextAlignmentRight, mediumFont);
    
    //  Moon phase
    initTextLayer(&moonLayer, screenMiddleX-16, moonY, 32, 32, kTextColor, GColorClear, GTextAlignmentCenter, moonFont);
    
    
    // Gregorian Month
    initTextLayer(&monthLayer, 0, monthY, screenWidth, 24, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    
    // Hebrew Month
    initTextLayer(&hMonthLayer, 0, monthY, screenWidth, 24, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
    
    //  Time
    initTextLayer(&timeLayer, 0, timeY, screenWidth, 50, kTextColor, GColorClear, GTextAlignmentCenter, largeFont);
    
    // Line
    lineLayer = layer_create(layer_get_frame(window_layer));
    layer_set_update_proc(lineLayer, &lineLayerUpdate);
    layer_add_child(window_layer, lineLayer);
    
    // Sunrise/set labels
    initTextLayer(&sunriseLabelLayer, 0, sunLabelY-1, screenWidth, 15, kTextColor, GColorClear, GTextAlignmentLeft, smallFont);
    text_layer_set_text(sunriseLabelLayer, zmanHourLabelString);
    initTextLayer(&sunsetLabelLayer, 0, sunLabelY-1, screenWidth, 15, kTextColor, GColorClear, GTextAlignmentRight, smallFont);
    text_layer_set_text(sunsetLabelLayer, nextHourLabelString);
    strcpy(zmanHourLabelString, "Sunrise");
    strcpy(nextHourLabelString, "Sunset");
    
    // Sun Graph
    sunGraphLayer = layer_create(GRect(72-sunRadius, sunY, sunSize, sunSize));
    layer_set_update_proc(sunGraphLayer, &sunGraphLayerUpdate);
    layer_set_clips(sunGraphLayer, true);
    if(!withoutSunGraph) {
        layer_add_child(window_layer, sunGraphLayer);
    }
    
    // Alert message
    initTextLayer(&alertLayer, 0, lineY + 2, screenWidth, 36, kBackgroundColor, kTextColor, GTextAlignmentCenter, mediumBoldFont);
    layer_remove_from_parent(text_layer_get_layer(alertLayer));  // don't show now!
    
    // Zman hour number and Next zman hour
    initTextLayer(&nextHourLayer, screenMiddleX-20, zmanHourY+3, 44, 17, kTextColor, GColorBlack, GTextAlignmentRight, smallFont);
    initTextLayer(&zmanHourLayer, screenMiddleX-32, zmanHourY, 30, 25, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
    
    // **** Test without zman hours
    if(1 || withoutSunGraph) {
        layer_remove_from_parent(text_layer_get_layer(nextHourLayer));
        layer_remove_from_parent(text_layer_get_layer(zmanHourLayer));
    }
    
    //  Sunrise and Sunset hour
    initTextLayer(&sunriseLayer, 0, sunHourY, screenWidth, 30, kTextColor, GColorClear, GTextAlignmentLeft, tinyFont);
    initTextLayer(&hatsotLayer, 0, sunHourY, screenWidth, 30, kTextColor, GColorClear, GTextAlignmentCenter, tinyFont);
    initTextLayer(&sunsetLayer, 0, sunHourY, screenWidth, 30, kTextColor, GColorClear, GTextAlignmentRight, tinyFont);
    
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
    updateWatch();
}

static void window_unload(Window *window) {
    // Save values in storage
    APP_LOG(APP_LOG_LEVEL_DEBUG, "**** SAVING DATA IN STORAGE ****");
    persist_write_int(STORAGE_LATITUDE, Jlatitude);
    persist_write_int(STORAGE_LONGITUDE, Jlongitude);
    persist_write_int(STORAGE_TIMEZONE, Jtimezone);
    app_message_deregister_callbacks();
    
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    app_focus_service_unsubscribe();

//    text_layer_destroy(dayLayer);
//    text_layer_destroy(hDayLayer);
//    text_layer_destroy(moonLayer);
//    text_layer_destroy(monthLayer);
//    text_layer_destroy(hMonthLayer);
//    text_layer_destroy(timeLayer);
//    text_layer_destroy(zmanHourLabelLayer);
//    text_layer_destroy(nextHourLabelLayer);
//    text_layer_destroy(zmanHourLayer);
//    text_layer_destroy(nextHourLayer);
//    text_layer_destroy(alertLayer);
//    text_layer_destroy(sunriseLayer);
//    text_layer_destroy(sunsetLayer);
//    text_layer_destroy(hatsotLayer);
//    layer_destroy(lineLayer);
//    layer_destroy(sunGraphLayer);
}

void set_style(void) {
    background_color  = GColorBlack;
    foreground_color  = GColorWhite;
    compositing_mode  = GCompOpAssignInverted;
    
    window_set_background_color(window, background_color);
    text_layer_set_text_color(layer_batt_text, foreground_color);
    bitmap_layer_set_compositing_mode(layer_batt_img, compositing_mode);
    bitmap_layer_set_compositing_mode(layer_conn_img, compositing_mode);
}

static void init(void) {
    // Positions
    dayY = 0;
    monthY = 19;
    moonY = 7;
    
    timeY = 30;
    lineY = screenMiddleY+2;
    sunY = lineY + 7;
    
    zmanHourY = sunY + sunRadius;
    
    sunLabelY = screenHeight - 28;
    sunHourY = screenHeight - 19;

    if(withoutSunGraph) {
        int offset = 18;
        timeY+=offset;
        lineY+=offset;
    }
    
    // Other params
    MINCHA_ALERT = 18;
    kBackgroundColor = GColorBlack;
    kTextColor = GColorWhite;
    
    window = window_create();
    window_set_background_color(window, GColorBlack);
    window_set_fullscreen(window, true);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    
    // Init time format string
    timeFormat = clock_is_24h_style()?"%R":"%l:%M";
    
    // Init time struct
    time_t unixTime;
    time(&unixTime);
    currentPblTime = localtime(&unixTime);
    
    const bool animated = true;
    window_stack_push(window, animated);
    
    // ********* battery and blutooth
    // resources
    img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECT);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
    img_battery_full   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_FULL);
    img_battery_half   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_HALF);
    img_battery_low    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_LOW);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);
    
    // layers
    layer_batt_text = text_layer_create(GRect(1,lineY+12,30,20));
    layer_batt_img  = bitmap_layer_create(GRect(4, lineY+2, 16, 16));
    layer_conn_img  = bitmap_layer_create(GRect(124, lineY+4, 20, 20));
    
    
    text_layer_set_background_color(layer_batt_text, GColorClear);
    text_layer_set_font(layer_batt_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(layer_batt_text, GTextAlignmentCenter);
    
    bitmap_layer_set_bitmap(layer_batt_img, img_battery_full);
    bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    
    // composing layers
    Layer *window_layer = window_get_root_layer(window);
    
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img));
    layer_add_child(window_layer, text_layer_get_layer(layer_batt_text));
    
    // style
    set_style();
    
    // handlers
    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
    app_focus_service_subscribe(&handle_appfocus);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    
    // draw first frame
    force_update();
    
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}

