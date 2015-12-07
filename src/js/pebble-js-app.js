/* -*-coding: utf-8 -*-
 * vim: sw=2 ts=2 expandtab ai
 *
 * *********************************
 * * Pebble Talk2Web App           *
 * * by bahbka <bahbka@bahbka.com> *
 * *********************************/

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

function doRequest(url) {
  console.log("request url: " + url);

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

      url = configData.serverURL + '?text=' + e.payload['keyText'];
      if (configData.sendAccountToken)
        url += '&token=' + Pebble.getAccountToken();

      if (configData.sendCoordinates) {
        try {
          navigator.geolocation.getCurrentPosition(
            function(position) {
              url += '&lat=' + position.coords.latitude + '&lon=' + position.coords.longitude;
              doRequest(url);
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
              doRequest(url);
            },
            {
              enableHighAccuracy: false, // TODO config
              maximumAge: 600000,
              timeout: 10000
            } // 10 minutes
          );
        } catch(e) {
          console.log("geolocation failed");
          doRequest(url);
        }
      } else {
        doRequest(url);
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

  sendMessageToPebble({
    keyStatus: 255,
  });

  Pebble.openURL(url);
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
