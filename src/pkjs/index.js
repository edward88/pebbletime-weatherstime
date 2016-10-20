var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig);

var conditionscode = {
    Clear         : 0,
    Clouds        : 1,
    Drizzle       : 2,
    Rain          : 3,
    Thunderstorm  : 4,
    Snow          : 5,
    Atmosphere    : 6,
    Unknown       : 99,
};

var xhrRequest = function (url, type, callback, customHeaderKey, customHeaderValue) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  
  if (customHeaderKey && customHeaderValue) {
    try {xhr.setRequestHeader(customHeaderKey, customHeaderValue);} catch(e){}
  }
  
  xhr.send();
  
};

function locationSuccessWU(pos) {
  var API_KEY = ""; // API Key for Wunderground.com

  // Construct URL
  var url = 'http://api.wunderground.com/api/' + API_KEY + '/conditions/q/'
      + pos.coords.latitude + ',' + pos.coords.longitude + '.json';

  // Send request to Wunderground
  xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      var temperature = Math.round(json.current_observation.temp_c);
      // Conditions
      var conditions = json.current_observation.weather;
      var code = json.current_observation.icon;
        if(code === 'clear'){
          code = conditionscode.Clear;
        }
        else if(code === 'mostlyysunny' || code === 'partlycloudy' || code === 'partlysunny' || code === 'mostlycloudy' || code === 'cloudy'){
          code = conditionscode.Clouds;
        }
        else if(code === 'rain'){
          code = conditionscode.Rain;
        }
        else if(code === 'tstorm'){
          code = conditionscode.Thunderstorm;
        }
        else if(code === 'snow' || code === 'sleet' || code === 'flurries'){
          code = conditionscode.Snow;
        }
        else if(code === 'fog' || code === 'hazy'){
          code = conditionscode.Atmosphere;
        }
        else {
          code = conditionscode.Unknown;
        }

      var description = json.current_observation.weather;
      var location = json.current_observation.display_location.city;
      var is_day = json.current_observation.icon_url.indexOf("nt_") == -1 ? 1 : 0;

      // Assemble dictionary using our keys
      var dictionary = {
        'TEMPERATURE': temperature,
        'CONDITIONS': conditions,
        'CODE': code,
        'DESCRIPTION': description,
        'LOCATION': location,
        'ISDAY':is_day
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Wunderground Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }
  );
}

function getLocationName(pos) {

  url = 'http://nominatim.openstreetmap.org/reverse?format=json&lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude;
  xhrRequest(url, 'GET',
    function(response) {
      var json = JSON.parse(response);
      var location = json.address.city;

      // Assemble dictionary using our keys
      var dictionary = {
        'LOCATION': location
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Location info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending location info to Pebble!');
        }
      );

    }, 'User-Agent', 'Pebble Generic Weather'
  );
}

function locationSuccessDarkSky(pos) {
  var API_KEY = ""; // API Key for Dark Sky
  
  getLocationName(pos);

  // Construct URL
  var url = 'https://api.darksky.net/forecast/' + API_KEY + '/' +
      pos.coords.latitude + ',' + pos.coords.longitude + '?units=si';

  // Send request to DarkSky
  xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.currently.temperature);
      // Conditions
      var conditions = json.currently.summary;
      var code = json.currently.icon;
      if(code === 'clear-day' || code === 'clear-night'){
        code = conditionscode.Clear;
      }
      else if(code === 'partly-cloudy-day' || code === 'partly-cloudy-night' || code === 'cloudy'){
        code = conditionscode.Clouds;
      }
      else if(code === 'rain'){
        code = conditionscode.Rain;
      }
      else if(code === 'thunderstorm'){
        code = conditionscode.Thunderstorm;
      }
      else if(code === 'snow' || code === 'sleet'){
        code = conditionscode.Snow;
      }
      else if(code === 'fog'){
        code = conditionscode.Atmosphere;
      }
      else {
        code = conditionscode.Unknown;
      }

      var description = json.currently.summary;
      var is_day = (json.currently.time > json.daily.data[0].sunriseTime && json.currently.time < json.daily.data[0].sunsetTime) ? 1 : 0;

      // Assemble dictionary using our keys
      var dictionary = {
        'TEMPERATURE': temperature,
        'CONDITIONS': conditions,
        'CODE': code,
        'DESCRIPTION': description,
        'ISDAY':is_day
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Dark Sky Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }      
  );
}

function locationSuccessYahoo(pos){
  // Construct URL
  var url = 'https://query.yahooapis.com/v1/public/yql?q=select astronomy, location.city, item.condition from weather.forecast where woeid in '+
            '(select woeid from geo.places(1) where text=\'(' + pos.coords.latitude+','+pos.coords.longitude+')\') and u=\'c\'&format=json';

  // Send request to Yahoo!
  xhrRequest(encodeURI(url), 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      // Get the temperture
      var temperature = Math.round(json.query.results.channel.item.condition.temp);
      // Get the conditions
      var conditions = parseDescription(json.query.results.channel.item.condition.text);
      var code = parseInt(json.query.results.channel.item.condition.code);
      
      switch (code) {
        case 3:
        case 4:
        case 37:
        case 38:
        case 39:
        case 45:
        case 47:
          code = conditionscode.Thunderstorm;
          break;
        case 8:
        case 9:
          code = conditionscode.Drizzle;
          break;
        case 10:
        case 11:
        case 12:
        case 35:
        case 40:
          code = conditionscode.Rain;
          break;
        case 5:
        case 6:
        case 7:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 41:
        case 42:
        case 43:
        case 46:
          code = conditionscode.Snow;
          break;
        case 0:
        case 1:
        case 2:
        case 19:
        case 21:
        case 22:
        case 23:
        case 24:
          code = conditionscode.Atmosphere;
          break;
        case 31:
        case 32:
        case 33:
        case 34:
          code = conditionscode.Clear;
          break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
          code = conditionscode.Clouds;
          break;
        default:
          code = conditionscode.Clouds;
      }
      
      var description = parseDescription(json.query.results.channel.item.condition.text);
      var location = json.query.results.channel.location.city;
      
      var current_time = new Date();
      var sunrise_time = TimeParse(json.query.results.channel.astronomy.sunrise);
      var sunset_time = TimeParse(json.query.results.channel.astronomy.sunset);
      var is_day = (current_time >  sunrise_time && current_time < sunset_time) ? 1 : 0;
      
      // Assemble dictionary using our keys
      var dictionary = {
        'TEMPERATURE': temperature,
        'CONDITIONS': conditions,
        'CODE': code,
        'DESCRIPTION': description,
        'LOCATION': location,
        'ISDAY':is_day
      };
      
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
        console.log('Yahoo! Weather info sent to Pebble successfully!');
        },
        function(e) {
        console.log('Error sending weather info to Pebble!');
        }
      );
    }
  );
}

function TimeParse(str) {
  var buff = str.split(" ");
  if(buff.length == 2) {
    var time = buff[0].split(":");
    if(buff[1] == "pm" && parseInt(time[0]) != 12) {
      time[0] = parseInt(time[0]) + 12;
    }
  }
  
  var date = new Date();
  date.setHours(time[0]);
  date.setMinutes(time[1]);
  
  return date;
}

function locationSuccessOpenWeatherMap(pos) {

  var API_KEY = ""; // API KEY for OpenWeatherMap.org
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + API_KEY;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      // Conditions
      var conditions = json.weather[0].main;      
      var code = json.weather[0].id;

      switch (code) {
        case 200:
        case 201:
        case 202:
        case 210:
        case 211:
        case 212:
        case 221:
        case 230:
        case 231:
        case 232:
          code = conditionscode.Thunderstorm;
          break;
        case 300:
        case 301:
        case 302:
        case 310:
        case 311:
        case 312:
        case 313:
        case 314:
        case 321:
          code = conditionscode.Drizzle;
          break;
        case 500:
        case 501:
        case 502:
        case 503:
        case 504:
        case 511:
        case 520:
        case 521:
        case 522:
        case 531:
          code = conditionscode.Rain;
          break;
        case 600:
        case 601:
        case 602:
        case 611:
        case 612:
        case 615:
        case 616:
        case 620:
        case 621:
        case 622:
          code = conditionscode.Snow;
          break;
        case 701:
        case 711:
        case 721:
        case 731:
        case 741:
        case 751:
        case 761:
        case 762:
        case 771:
        case 781:
          code = conditionscode.Atmosphere;
          break;
        case 800:
          code = conditionscode.Clear;
          break;
        case 801:
        case 802:
        case 803:
        case 804:
          code = conditionscode.Clouds;
          break;
        default:
          code = conditionscode.Clouds;
      }

      var description = parseDescription(json.weather[0].description);
      var location = json.name;
      var is_day = (json.dt > json.sys.sunrise && json.dt < json.sys.sunset) ? 1 : 0;
      
      // Assemble dictionary using our keys
      var dictionary = {
        'TEMPERATURE': temperature,
        'CONDITIONS': conditions,
        'CODE': code,
        'DESCRIPTION': description,
        'LOCATION': location,
        'ISDAY':is_day
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('OpenWeatherMap Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }
  );
}

function parseDescription(desc) {
  return desc.replace("and" , "&").replace("Severe", "Sv").replace("Mixed", "").replace("Isolated", "Iso").replace("Scattered", "Sc").trim();
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeatherWU() {
  navigator.geolocation.getCurrentPosition(
    locationSuccessWU,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function getWeatherDarkSky() {
  navigator.geolocation.getCurrentPosition(
    locationSuccessDarkSky,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function getWeatherOpenWeatherMap() {
  navigator.geolocation.getCurrentPosition(
    locationSuccessOpenWeatherMap,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function getWeatherYahoo() {
  navigator.geolocation.getCurrentPosition(
    locationSuccessYahoo,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
    
    var dictionary = {'JSREADY' : true};
    
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log('Ready status sent to Pebble successfully!');
      },
      function(e) {
        console.log('Error sending ready state to Pebble!');
      }
    );
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    // Get the dictionary from the message
    var dict = e.payload;
    console.log('AppMessage received!');
    
    switch (dict.WeatherSource) {
      case 1:
        getWeatherWU();
        break;
      case 2:
        getWeatherDarkSky();
        break;
      case 3:
        getWeatherYahoo();
        break;
      case 0:
      default:
        getWeatherOpenWeatherMap();
        break;
    }
  }                     
);

