[![Build Status](https://travis-ci.org/JamieLivingstone/Notifications.svg?branch=master)](https://travis-ci.org/JamieLivingstone/Notifications)

# Notifications
**Notifications** is a Javascript library for notifications heavily inspired by toastr but does not require any dependencies such as jQuery.

Works on browsers: IE9+, Safari, Chrome, FireFox, opera, edge

## npm Installation
Do either
```
npm i styled-notifications
```
or add the following to your `package.json`:
```
"dependencies": {
  "styled-notifications": "^1.0.1"
},
```

## Installation
Download files from the dist folder and then:
1. Link to notifications.css `<link href="notifications.css" rel="stylesheet"/>`

2. Link to notifications.js `<script src="notifications.js"></script>`

## Usage
### Custom options
- closeOnClick <bool> - Close the notification dialog when a click is invoked.
- displayCloseButton <bool> - Display a close button in the top right hand corner of the notification.
- positionClass <string> - Set the position of the notification dialog. Accepted positions: ('nfc-top-right', 'nfc-bottom-right', 'nfc-bottom-left', 'nfc-top-left').
- onClick <function(event)> - Call a callback function when a click is invoked on a notification.
- showDuration <integer> - Milliseconds the notification should be visible (0 for a notification that will remain open until clicked)
- theme <string> - Set the position of the notification dialog. Accepted positions: ('success', 'info', 'warning', 'error', 'A custom clasName').
```
const defaultOptions = {
		closeOnClick: true,
		displayCloseButton: false,
		positionClass: 'nfc-top-right',
		onclick: false,
		showDuration: 3500,
		theme: 'success'
};
```

## Example

### Success notification
```
// Create a success notification instance
const successNotification = window.createNotification({
	theme: 'success',
	showDuration: 5000
});
  
// Invoke success notification
successNotification({ 
    message: 'Simple success notification' 
});

// Use the same instance but pass a title
successNotification({ 
    title: 'Working',
    message: 'Simple success notification' 
});
```

### Information notification
```
// Only running it once? Invoke immediately like this
window.createNotification({
    theme: 'success',
    showDuration: 5000
})({
    message: 'I have some information for you...'
});
```

### Todo
~~1. Add to NPM~~
2. Improve documentation
3. Further device testing
4. Add contributor instructions