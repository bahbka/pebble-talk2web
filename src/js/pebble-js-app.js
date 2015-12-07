/* -*-coding: utf-8 -*-
 * vim: sw=2 ts=2 expandtab ai
 *
 * *********************************
 * * Pebble Talk2Web App           *
 * * by bahbka <bahbka@bahbka.com> *
 * *********************************/


// global variables for eval()
var text = "";
var token = "";
var lat = "";
var lon = "";

function sendMessageToPebble(message) {
  var transactionId = Pebble.sendAppMessage(message,
    function(e) {
      console.log('ok message ' + e.data.transactionId);
    },
    function(e) {
      console.log('fail message ' + e.data.transactionId + ', error: ' + e.error.message);
    }
  );
}

function doRequest() {
  if (configData.enableEval) {
    url_template = configData.serverURL;
  } else {
    url_template = "'" + configData.serverURL + "'+'?text='+text+'&token='+token+'&lat='+lat+'&lon='+lon";
  }

  console.log("url template: " + url_template);

  try {
    url = eval(url_template);
    console.log("request url : " + url);

  } catch(e) {
    console.log("eval error: " + e)
    sendMessageToPebble({keyStatus: 102, keyText: "url eval() error: " + e});
    return;
  }

  message = {
    keyStatus: 100,
    keyText: "unknown error"
  };

  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        try {
          message = JSON.parse(req.responseText);
        } catch(e) {
          message = {
            keyStatus: 0,
            keyText: req.responseText
          };
        }
      } else {
        //Pebble.showSimpleNotificationOnPebble("ERROR", req.status);
        message = {
          keyStatus: 101,
          keyText: "server error " + req.status.toString()
        };
      }
    }
    sendMessageToPebble(message);
  };
  req.send(null);
}

Pebble.addEventListener('appmessage',
  function(e) {
    //console.log('received message: ' + e.payload[0]);

    try {
      configData = JSON.parse(localStorage.getItem("pebble-talk2web-config"));
      console.log('current config: ' + JSON.stringify(configData));

      text = e.payload['keyText'];
      token = "";
      lat = "";
      lon = "";

      if (configData.sendAccountToken)
        token = Pebble.getAccountToken();

      if (configData.sendCoordinates) {
        try {
          navigator.geolocation.getCurrentPosition(
            function(position) {
              lat = position.coords.latitude;
              lon = position.coords.longitude;
              doRequest();
            },
            function(error) {
              //error error.message
              /*
                TODO inform user about error
                PERMISSION_DENIED (numeric value 1)
                POSITION_UNAVAILABLE (numeric value 2)
                TIMEOUT (numeric value 3)
              */
              console.log("geolocation error");
              doRequest();
            },
            {
              enableHighAccuracy: false, // TODO config
              maximumAge: 600000,
              timeout: 10000
            } // 10 minutes
          );
        } catch(e) {
          console.log("geolocation failed");
          doRequest();
        }
      } else {
        doRequest();
      }

    } catch(e) {
      console.log(e);
      sendMessageToPebble({
        keyStatus: 102,
        keyText: "sending failed"
      });
    }
  }
);

Pebble.addEventListener('showConfiguration', function(e) {
  var url = 'http://bahbka.github.io/pebble-talk2web/';
  console.log('showing configuration page: ' + url);

  Pebble.openURL(url);

  sendMessageToPebble({
    keyStatus: 255,
  });
});

Pebble.addEventListener('webviewclosed', function(e) {
  //console.log('Configuration window returned: ' + e.response);

  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('configuration page returned: ' + JSON.stringify(configData));

  if (configData.serverURL) {
    localStorage.setItem("pebble-talk2web-config", JSON.stringify(configData));

    sendMessageToPebble({
      keyStartImmediately: configData.startImmediately,
      keyEnableConfirmationDialog: configData.enableConfirmationDialog,
      keyEnableErrorDialog: configData.enableErrorDialog
    });
  }
});

Pebble.addEventListener('ready', function(e) {
  console.log('ready');
});
