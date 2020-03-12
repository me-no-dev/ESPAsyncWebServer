'use strict';

// Polyfills
import './polyfills/classList';

import {
	append,
	createElement,
	createParagraph,
	isString
} from './helpers';

(function Notifications(window) {
	// Default notification options
	const defaultOptions = {
		closeOnClick: true,
		displayCloseButton: false,
		positionClass: 'nfc-top-right',
		onclick: false,
		showDuration: 3500,
		theme: 'success'
	};

	function configureOptions(options) {
		// Create a copy of options and merge with defaults
		options = Object.assign({}, defaultOptions, options);

		// Validate position class
		function validatePositionClass(className) {
			const validPositions = [
				'nfc-top-left',
				'nfc-top-right',
				'nfc-bottom-left',
				'nfc-bottom-right'
			];

			return validPositions.indexOf(className) > -1;
		}

		// Verify position, if invalid reset to default
		if (!validatePositionClass(options.positionClass)) {
			console.warn('An invalid notification position class has been specified.');
			options.positionClass = defaultOptions.positionClass;
		}

		// Verify onClick is a function
		if (options.onclick && typeof options.onclick !== 'function') {
			console.warn('Notification on click must be a function.');
			options.onclick = defaultOptions.onclick;
		}

		// Verify show duration
		if(typeof options.showDuration !== 'number') {
			options.showDuration = defaultOptions.showDuration;
		}

		// Verify theme
		if(!isString(options.theme) || options.theme.length === 0) {
			console.warn('Notification theme must be a string with length');
			options.theme = defaultOptions.theme;
		}

		return options;
	}

	// Create a new notification instance
	function createNotification(options) {
		// Validate options and set defaults
		options = configureOptions(options);

		// Return a notification function
		return function notification({ title, message } = {}) {
			const container = createNotificationContainer(options.positionClass);

			if(!title && !message) {
				return console.warn('Notification must contain a title or a message!');
			}

			// Create the notification wrapper
			const notificationEl = createElement('div', 'ncf', options.theme);

			// Close on click
			if(options.closeOnClick === true) {
				notificationEl.addEventListener('click', () => container.removeChild(notificationEl));
			}

			// Custom click callback
			if(options.onclick) {
				notificationEl.addEventListener('click', (e) => options.onclick(e));
			}

			// Display close button
			if(options.displayCloseButton) {
				const closeButton = createElement('button');
				closeButton.innerText = 'X';

				// Use the wrappers close on click to avoid useless event listeners
				if(options.closeOnClick === false){
					closeButton.addEventListener('click', () =>container.removeChild(notificationEl));
				}

				append(notificationEl, closeButton);
			}

			// Append title and message
			isString(title) && title.length && append(notificationEl, createParagraph('ncf-title')(title));
			isString(message) && message.length && append(notificationEl, createParagraph('nfc-message')(message));

			// Append to container
			append(container, notificationEl);

			// Remove element after duration
			if(options.showDuration && options.showDuration > 0) {
				const timeout = setTimeout(() => {
					container.removeChild(notificationEl);

					// Remove container if empty
					if(container.querySelectorAll('.ncf').length === 0) {
						document.body.removeChild(container);
					}
				}, options.showDuration);

				// If close on click is enabled and the user clicks, cancel timeout
				if(options.closeOnClick || options.displayCloseButton) {
					notificationEl.addEventListener('click', () => clearTimeout(timeout));
				}
			}
		};
	}

	function createNotificationContainer(position) {
		let container = document.querySelector(`.${position}`);

		if(!container) {
			container = createElement('div', 'ncf-container', position);
			append(document.body, container);
		}

		return container;
	}

	// Add Notifications to window to make globally accessible
	if (window.createNotification) {
		console.warn('Window already contains a create notification function. Have you included the script twice?');
	} else {
		window.createNotification = createNotification;
	}
})(window);
