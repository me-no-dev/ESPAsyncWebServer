const { partial, append, isString, createElement, createParagraph } = require('../src/helpers');

const addNumbers = (x, y) => x + y;

const sum = (...numbers) => numbers.reduce((total, current) => total + current, 0);

describe('Helpers', () => {
	beforeEach(() => {
		document.body.innerHTML = '';
	});

	describe('Partial', () => {
		it('should return a partially applied function', () => {
			expect(typeof partial(addNumbers, 10)).toEqual('function');
		});

		it('should execute function when partially applied function is called', () => {
			expect(partial(addNumbers, 20)(10)).toEqual(30);
		});

		it('should gather argument', () => {
			expect(partial(sum, 5, 10)(15, 20, 25)).toEqual(75);
		});
	});

	describe('Append', () => {
		const container = document.createElement('div');
		document.body.appendChild(container);

		const elementToAppend = document.createElement('h1');
		elementToAppend.classList.add('heading');
		elementToAppend.innerText = 'working';

		append(container, elementToAppend);

		const element = document.querySelector('.heading');
		expect(element);

		expect(element.innerText).toEqual('working');
	});

	describe('Is string', () => {
		expect(isString(1)).toEqual(false);
		expect(isString(null)).toEqual(false);
		expect(isString(undefined)).toEqual(false);
		expect(isString({})).toEqual(false);

		expect(isString('')).toEqual(true);
		expect(isString('a')).toEqual(true);
		expect(isString('1')).toEqual(true);
		expect(isString('some string')).toEqual(true);
	});

	describe('Create element', () => {
		it('should create an element', () => {
			expect(createElement('p')).toEqual(document.createElement('p'));
			expect(createElement('h1')).toEqual(document.createElement('h1'));
			expect(createElement('ul')).toEqual(document.createElement('ul'));
			expect(createElement('li')).toEqual(document.createElement('li'));
			expect(createElement('div')).toEqual(document.createElement('div'));
			expect(createElement('span')).toEqual(document.createElement('span'));
		});

		it('should add class names', () => {
			expect(createElement('div', 'someclass1', 'someclass2').classList.contains('someclass2'));
			expect(createElement('p', 'para', 'test').classList.contains('para'));

			const mockUl = document.createElement('ul');
			mockUl.classList.add('nav');
			mockUl.classList.add('foo');

			expect(createElement('ul', 'nav', 'foo').classList).toEqual(mockUl.classList);
		});
	});

	describe('Create paragraph', () => {
		it('should create a paragraph', () => {
			const p = document.createElement('p');
			p.innerText = 'Some text';
			expect(createParagraph()('Some text')).toEqual(p);
		});

		it('should add class names', () => {
			const p = document.createElement('p');
			p.classList.add('body-text');
			p.classList.add('para');

			expect(createParagraph('body-text', 'para')('')).toEqual(p);
		});

		it('should set inner text', () => {
			const p = document.createElement('p');
			p.innerText = 'Hello world!';
			p.classList.add('text');

			expect(createParagraph('text')('Hello world!')).toEqual(p);
		});

		it('should append to DOM', () => {
			append(document.body, createParagraph('text')('hello'));
			expect(document.querySelector('.text').innerText).toEqual('hello');
		});
	});
});
