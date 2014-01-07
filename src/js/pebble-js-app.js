function fetchWeather(latitude, longitude) {
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
             "lat=" + latitude + "&lon=" + longitude + "&cnt=1" + "&APPID=bdf1d15ddb26525960d3b8ea3ca0bc1c", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var temperature, temp_min, temp_max, icon, city;
                if (response && response.list && response.list.length > 0) {
                    var weatherResult = response.list[0];
                    temp_min = Math.round(weatherResult.main.temp_min - 273.15);
                    temp_max = Math.round(weatherResult.main.temp_max - 273.15);
                    temperature = Math.round(weatherResult.main.temp - 273.15);
                    icon = iconFromWeatherId(weatherResult.weather[0].id);
                    city = weatherResult.name.substring(0,20);  // limit to 20 chars
                    var bigLat = Math.round(parseFloat(latitude)*1000);
                    var bigLon = Math.round(parseFloat(longitude)*1000);
                    console.log("lat = " + bigLat);
                    console.log("lon = " + bigLon);
                    console.log("temp = " + temperature);
                    console.log("min = " + temp_min);
                    console.log("max = " + temp_max);
                    console.log("city = " + city);
                    console.log("icon = " + icon);
                    Pebble.sendAppMessage({
                                          "lat":bigLat,
                                          "lon":bigLon,
                                          "temp":temperature,
                                          "min":temp_min,
                                          "max":temp_max,
                                          "icon":icon,
                                          "city":city
                                          });
                }
            } else {
                console.log("Error");
            }
        }
    }
    req.send(null);
}

//function fetchWeather(latitude, longitude) {
//    var response;
//    var req = new XMLHttpRequest();
//    var myQuery = "https://api.forecast.io/forecast/d4e632cffbd68b4db2029e30b7708ccb/" +latitude + "," + longitude + "?units=si&exclude=hourly,flags";
//    console.log("QUERY: " + myQuery);
//    req.open('GET', myQuery, true);
//    req.onload = function(e) {
//        if (req.readyState == 4) {
//            if(req.status == 200) {
//                console.log("RECEIVED RESPONSE:\n");
//                console.log(req.responseText);
//                console.log("END OF RESPONSE\n");
//                response = JSON.parse(req.responseText);
//                console.log("PARSED JSON COUNT: " + response.count);
//                var temperature, temp_min, temp_max, icon, city;
//                if (response && response.list && response.list.length > 0) {
//                    console.log("PARSING WEATHER IN JS");
//                    var weatherResult = response.data;
//                    var current = weatherResult.current_condition[0];
//                    var forecast = weatherResult.weather[0];
//                    var nearest = weatherResult.nearest_area;
//                    
//                    temp_min = Math.round(forecast.tempMinC);
//                    temp_max = Math.round(forecast.tempMaxC);
//                    temperature = Math.round(current.temp_C);
//                    icon = iconFromWeatherId(current.weatherCode);
//                    var place = nearest.areaName[0].value.substring(0,12) + "-" + nearest.country[0].value;
//                    city = place.substring(0,20);  // limit to 20 chars
//                    var bigLat = Math.round(parseFloat(latitude)*1000);
//                    var bigLon = Math.round(parseFloat(longitude)*1000);
//                    console.log("lat = " + bigLat);
//                    console.log("lon = " + bigLon);
//                    console.log("temp = " + temperature);
//                    console.log("min = " + temp_min);
//                    console.log("max = " + temp_max);
//                    console.log("city = " + city);
//                    console.log("icon = " + icon);
//                    Pebble.sendAppMessage({
//                                          "lat":bigLat,
//                                          "lon":bigLon,
//                                          "temp":temperature,
//                                          "min":temp_min,
//                                          "max":temp_max,
//                                          "icon":icon,
//                                          "city":city
//                                          });
//                }
//
//            } else {
//                console.log("Error");
//            }
//        }
//    }
//    req.send(null);
//}

//function fetchWeather(latitude, longitude) {
//    var response;
//    var req = new XMLHttpRequest();
//    req.open('GET', "http://api.worldweatheronline.com/free/v1/weather.ashx?" +
//             "q=" + latitude + "," + longitude + "&cnt=1" + "&format=json&extra=localObsTime,isDayTime,utcDateTime&num_of_days=1&includelocation=yes&key=hu455zpvu682zrn9hm54z4m3", true);
//    req.onload = function(e) {
//        if (req.readyState == 4) {
//            if(req.status == 200) {
//                console.log("RECEIVED RESPONSE:\n");
//                console.log(req.responseText);
//                console.log("END OF RESPONSE\n");
//                response = JSON.parse(req.responseText);
//                console.log("PARSED JSON COUNT: " + response.count);
//                var temperature, temp_min, temp_max, icon, city;
//                if (response && response.list && response.list.length > 0) {
//                    console.log("PARSING WEATHER IN JS");
//                    var weatherResult = response.data;
//                    var current = weatherResult.current_condition[0];
//                    var forecast = weatherResult.weather[0];
//                    var nearest = weatherResult.nearest_area;
//                    
//                    temp_min = Math.round(forecast.tempMinC);
//                    temp_max = Math.round(forecast.tempMaxC);
//                    temperature = Math.round(current.temp_C);
//                    icon = iconFromWeatherId(current.weatherCode);
//                    var place = nearest.areaName[0].value.substring(0,12) + "-" + nearest.country[0].value;
//                    city = place.substring(0,20);  // limit to 20 chars
//                    var bigLat = Math.round(parseFloat(latitude)*1000);
//                    var bigLon = Math.round(parseFloat(longitude)*1000);
//                    console.log("lat = " + bigLat);
//                    console.log("lon = " + bigLon);
//                    console.log("temp = " + temperature);
//                    console.log("min = " + temp_min);
//                    console.log("max = " + temp_max);
//                    console.log("city = " + city);
//                    console.log("icon = " + icon);
//                    Pebble.sendAppMessage({
//                                          "lat":bigLat,
//                                          "lon":bigLon,
//                                          "temp":temperature,
//                                          "min":temp_min,
//                                          "max":temp_max,
//                                          "icon":icon,
//                                          "city":city
//                                          });
//                }
//            } else {
//                console.log("Error");
//            }
//        }
//    }
//    req.send(null);
//}

function fetchTimezone(latitude, longitude) {
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "https://maps.googleapis.com/maps/api/timezone/json?location=" + latitude + "," + longitude + "&timestamp=" + Math.round(new Date().getTime() / 1000) + "&sensor=true");
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var dst = parseInt(response.dstOffset)/60;   // minutes
                var timezone = parseInt(response.rawOffset)/60;   // minutes
                var tz = timezone + dst*60; // combined time offset from utc
                var bigLat = Math.round(parseFloat(latitude)*1000);
                var bigLon = Math.round(parseFloat(longitude)*1000);
                console.log("lat = " + bigLat);
                console.log("lon = " + bigLon);
                console.log("tz = " + tz);
                Pebble.sendAppMessage({
                                      "lat":bigLat,
                                      "lon":bigLon,
                                      "tz":tz    // time offset in minutes
                                      });
            }
        }
    }
    req.send(null);
}

function iconFromWeatherId(weatherId) {
//    if(weatherId <= 115) return 0;  // Sun
//    if(weatherId <= 143) return 1;  // Clouds
//    if((weatherId == 176) || ((weatherId >= 263) && (weatherId <= 314)) || ((weatherId >= 353) && (weatherId <= 389))) return 2;    // Rain
//    return 3;   // Snow
    if (weatherId < 600) {
        return 2;
    } else if (weatherId < 700) {
        return 3;
    } else if (weatherId > 800) {
        return 1;
    } else {
        return 0;
    }
}




function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchTimezone(coordinates.latitude, coordinates.longitude);
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "city":"Unavailable",
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          console.log("Message Received!");
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     console.log("webview closed");
                                     console.log(e.type);
                                     console.log(e.response);
                                     });


