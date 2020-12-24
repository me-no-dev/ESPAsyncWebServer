const webpack = require('webpack');
const ExtractTextPlugin = require('extract-text-webpack-plugin');

const extractSass = new ExtractTextPlugin({
	filename: 'notifications.css',
	disable: process.env.NODE_ENV === 'development'
});

module.exports = {
	entry: ['./src/index.js', './src/style.scss'],
	output: {
		path: __dirname + '/dist',
		filename: 'notifications.js'
	},
	module: {
		rules: [
			{
				test: /\.js$/,
				loader: 'babel-loader',
				query: {
					presets: ['babel-preset-es2015', 'es2015-ie']
				}
			},
			{
				test: /\.scss$/,
				use: extractSass.extract({
					use: [{
						loader: 'css-loader'
					}, {
						loader: 'sass-loader'
					}],
					// use style-loader in development
					fallback: 'style-loader'
				})
			}
		],
	},
	plugins: [
		extractSass
	]
};