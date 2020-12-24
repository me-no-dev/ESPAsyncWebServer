require('../src/index');

describe('Notifications', () => {
	beforeEach(() => {
		document.body.innerHTML = '';
	});

	it('should display a console warning if no title or message is passed', () => {
		jest.spyOn(global.console, 'warn');
		window.createNotification()();
		expect(console.warn).toBeCalled();
	});

	it('should render a default notification', () => {
		const notification = window.createNotification();

		const title = 'I am a title';

		// Should initially not contain any notifications
		expect(document.querySelectorAll('.ncf').length).toEqual(0);

		// Create a notification instance with a title
		notification({ title });

		// Should be one notification with the title passed in
		expect(document.querySelectorAll('.ncf').length).toEqual(1);
		expect(document.querySelector('.ncf-title').innerText).toEqual(title);

		// Create a second instance so there should now be two instances
		notification({ title });
		expect(document.querySelectorAll('.ncf').length).toEqual(2);
	});

	it('should close on click if the option is enabled', () => {
		const notification = window.createNotification({
			closeOnClick: true
		});

		// Create a notification with a generic body
		notification({ message: 'some text' });

		// Should be one notification instance
		expect(document.querySelectorAll('.ncf').length).toEqual(1);

		// Click the notification
		document.querySelector('.ncf').click();

		expect(document.querySelectorAll('.ncf').length).toEqual(0);
	});

	it('should not close on click if the option is disabled', () => {
		const notification = window.createNotification({
			closeOnClick: false
		});

		// Create a notification with a generic body
		notification({ message: 'some text' });

		// Should be one notification instance
		expect(document.querySelectorAll('.ncf').length).toEqual(1);

		// Click the notification
		document.querySelector('.ncf').click();

		expect(document.querySelectorAll('.ncf').length).toEqual(1);
	});

	it('should set position class if valid', () => {
		const validPositions = [
			'nfc-top-left',
			'nfc-top-right',
			'nfc-bottom-left',
			'nfc-bottom-right'
		];

		validPositions.forEach(position => {
			const notification = window.createNotification({
				positionClass: position
			});

			notification({ title: 'title here' });

			const className = `.${position}`;

			expect(document.querySelectorAll(className).length).toEqual(1);

			const container = document.querySelector(className);
			expect(container.querySelectorAll('.ncf').length).toEqual(1);
		});
	});

	it('should revert to default to default position and warn if class is invalid', () => {
		const notification = window.createNotification({
			positionClass: 'invalid-name'
		});

		jest.spyOn(global.console, 'warn');

		notification({ message: 'test' });

		expect(console.warn).toBeCalled();

		expect(document.querySelectorAll('.nfc-top-right').length).toEqual(1);
	});

	it('should allow a custom onclick callback', () => {
		let a = 'not clicked';

		const notification = window.createNotification({
			onclick: () => {
				a = 'clicked';
			}
		});

		notification({ message: 'click test' });

		expect(a).toEqual('not clicked');

		// Click the notification
		document.querySelector('.ncf').click();

		expect(a).toEqual('clicked');
	});

	it('should show for correct duration', () => {
		const notification = window.createNotification({
			showDuration: 500
		});

		notification({ message: 'test' });

		expect(document.querySelectorAll('.ncf').length).toEqual(1);

		// Should exist after 400ms
		setTimeout(() => {
			expect(document.querySelectorAll('.ncf').length).toEqual(1);
		}, 400);

		// Should delete after 500ms
		setTimeout(() => {
			expect(document.querySelectorAll('.ncf').length).toEqual(0);
		});
	}, 501);
});
