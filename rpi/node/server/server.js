const Hapi = require( 'hapi' );
const server = new Hapi.Server();
const PORT = process.env.PORT || 3000;

server.connection({ port: PORT, host: 'localhost' });

server.register(require('inert'), (err) => {
  if (err) {
    throw err;
  }

  server.route({
    method: 'GET',
    path: '/{param*}',
    handler: {
      directory: {
        path: 'dist'
      }
    }
  });
});

server.route({
  method: 'GET',
  path: '/api/test',
  handler: function (req, reply) {
    reply('hello world from API');
  }
});

server.route({
  method: 'GET',
  path: '/api/test.html',
  handler: function (req, reply) {
    reply('hello world from API.html');
  }
});

server.start((err) => {
    if (err) {
        throw err;
    }
    console.log(`Server running at: ${server.info.uri}`);
});
