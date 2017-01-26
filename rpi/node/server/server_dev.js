var fork = require('child_process').fork;

var server = fork("./server/server.js");
var webpack = fork(
  "./node_modules/webpack-dev-server/bin/webpack-dev-server.js",
  ['--hot', '--inline', '--history-api-fallback']
);

console.log('dev server started');
