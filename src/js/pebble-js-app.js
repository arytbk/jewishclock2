function fetchTimezone(latitude, longitude) {
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "https://maps.googleapis.com/maps/api/timezone/json?location=" + latitude + "," + longitude + "&timestamp=" + Math.round(new Date().getTime() / 1000) + "&sensor=true");
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                console.log("PARSING Timezone *****");
                var dst = parseInt(response.dstOffset);
                var timezone = parseInt(response.rawOffset)/60;   // minutes
                console.log(dst);
                console.log(timezone);
                var bigLat = Math.round(parseFloat(latitude)*1000);
                var bigLon = Math.round(parseFloat(longitude)*1000);
                console.log(bigLat);
                console.log(bigLon);
                Pebble.sendAppMessage({
                                      "latitude":bigLat,
                                      "longitude":bigLon,
                                      "timezone":timezone,
                                      "dst":dst});
            }
        }
    }
    req.send(null);
}

function iconFromWeatherId(weatherId) {
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

function fetchWeather(latitude, longitude) {
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
             "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var temperature, icon, city;
                if (response && response.list && response.list.length > 0) {
                    var weatherResult = response.list[0];
                    temperature = Math.round(weatherResult.main.temp - 273.15);
                    icon = iconFromWeatherId(weatherResult.weather[0].id);
                    city = weatherResult.name;
                    console.log("temp = " + temperature);
                    console.log("City = " + city);
                    console.log("Icon = " + icon);
                    Pebble.sendAppMessage({
//                        "temperature":(temperature + "\u00B0C"),
                          "temperature":temperature + "°C",
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


function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchTimezone(coordinates.latitude, coordinates.longitude);
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "city":"Loc Unavailable",
    "temperature":"N/A"
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
                          console.log(e.payload.temperature);
                          console.log("message!");
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     console.log("webview closed");
                                     console.log(e.type);
                                     console.log(e.response);
                                     });


