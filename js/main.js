// -*-coding: utf-8 -*-
// vim: sw=2 ts=2 expandtab ai

(function() {
  loadOptions();
  submitHandler();
})();

function submitHandler() {
  var $submitButton = $('#submitButton');
  $submitButton.on('click', function() {
    console.log('Submit');

    var return_to = getQueryParam('return_to', 'pebblejs://close#');
    document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
  });
}

function loadOptions() {
  var $serverURL = $('#serverURL');
  var $startImmediately = $('#startImmediately');
  var $enableConfirmationDialog = $('#enableConfirmationDialog');
  var $enableErrorDialog = $('#enableErrorDialog');
  var $sendAccountToken = $('#sendAccountToken');
  var $sendCoordinates = $('#sendCoordinates');
  var $enableEval = $('#enableEval');

  if (localStorage.serverURL) {
    $serverURL[0].value = localStorage.serverURL;
    $startImmediately[0].checked = localStorage.startImmediately === 'true';
    $enableConfirmationDialog[0].checked = localStorage.enableConfirmationDialog === 'true';
    $enableErrorDialog[0].checked = localStorage.enableErrorDialog === 'true';
    $sendAccountToken[0].checked = localStorage.sendAccountToken === 'true';
    $sendCoordinates[0].checked = localStorage.sendCoordinates === 'true';
    $enableEval[0].checked = localStorage.enableEval === 'true';
  }
}

function getAndStoreConfigData() {
  var $serverURL = $('#serverURL');
  var $startImmediately = $('#startImmediately');
  var $enableConfirmationDialog = $('#enableConfirmationDialog');
  var $enableErrorDialog = $('#enableErrorDialog');
  var $sendAccountToken = $('#sendAccountToken');
  var $sendCoordinates = $('#sendCoordinates');
  var $enableEval = $('#enableEval');

  var options = {
    serverURL: $serverURL.val(),
    startImmediately: $startImmediately[0].checked,
    enableConfirmationDialog: $enableConfirmationDialog[0].checked,
    enableErrorDialog: $enableErrorDialog[0].checked,
    sendAccountToken: $sendAccountToken[0].checked,
    sendCoordinates: $sendCoordinates[0].checked,
    enableEval: $enableEval[0].checked
  };

  // Save for next launch
  localStorage.serverURL = options.serverURL;
  localStorage.startImmediately = options.startImmediately;
  localStorage.enableConfirmationDialog = options.enableConfirmationDialog;
  localStorage.enableErrorDialog = options.enableErrorDialog;
  localStorage.sendAccountToken = options.sendAccountToken;
  localStorage.sendCoordinates = options.sendCoordinates;
  localStorage.enableEval = options.enableEval;

  console.log('Got options: ' + JSON.stringify(options));
  return options;
}

function getQueryParam(variable, defaultValue) {
  var query = location.search.substring(1);
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    if (pair[0] === variable) {
      return decodeURIComponent(pair[1]);
    }
  }
  return defaultValue || false;
}
