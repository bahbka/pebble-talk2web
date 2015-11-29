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
      console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
    },
    function(e) {
      console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
    }
  );
}

Pebble.addEventListener('appmessage',
  function(e) {
    //console.log('Received message: ' + e.payload[0]);    
    var req = new XMLHttpRequest();
    req.open('GET', localStorage.serverURL + '?text=' + e.payload[0] + '&token=' + Pebble.getAccountToken(), true);
    req.onload = function(e) {
      if (req.readyState == 4) {
        if(req.status == 200) {
          //Pebble.showSimpleNotificationOnPebble("Voice2Web OK", req.responseText);
          send_message_to_pebble();
        } else {
          //Pebble.showSimpleNotificationOnPebble("Voice2Web ERROR", req.status);
        }
      }
    };
    req.send(null);
  }
);

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  //var url = 'https://rawgit.com/pebble-examples/design-guides-slate-config/master/config/index.html';
  var url = 'http://bahbka.github.io/pebble-talk2web/';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  //console.log('Configuration window returned: ' + e.response);

  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  if (configData.serverURL) {
    sendMessageToPebble({
      keyStartImmediately: configData.startImmediately,
      keyEnableConfirmationDialog: configData.enableConfirmationDialog,
      keyEnableErrorDialog: configData.enableErrorDialog
    });
  }
});

Pebble.addEventListener('ready', function(e) {
  console.log('JavaScript app ready and running!');
  if (localStorage.serverURL) {
    sendMessageToPebble({
      keyStartImmediately: configData.startImmediately,
      keyEnableConfirmationDialog: configData.enableConfirmationDialog,
      keyEnableErrorDialog: configData.enableErrorDialog
    });
  }
});
