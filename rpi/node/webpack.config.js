const path = require('path');

module.exports = {
  context: __dirname + "/app",

  entry: {
    javascript: "./js/app.js",
    html: "./index.html",
  },

  proxy: {
    'api/**': {
      target: 'http://localhost:3000',
      secure: false
    }
  },

  historyApiFallback: false,

  resolve: {
    extensions: ['', '.js', '.jsx', '.json'],
    root: path.resolve(__dirname, './app'),
  },

  module: {
    loaders: [
      {
        test: /\.html$/,
        loader: "file?name=[name].[ext]",
      },
      {
        test: /\.jsx?$/,
        exclude: /node_modules/,
        loaders: ["react-hot", "babel-loader"]
      }
    ]
  },

  output: {
    filename: "app.js",
    path: __dirname + "/dist",
  }
};
