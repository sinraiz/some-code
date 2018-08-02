/**
 * The entry point of the application.
 * Recrumatic API is the RESTful service.
 */

// External dependencies:
var bodyParser = require('body-parser');
var express = require('express');
var url = require('url');

// Register requireLib() in the GLOBAL scope:
GLOBAL.requireLib = function (name) {
    return require('./lib/' + name);
};

// Internal dependencies:
var config = require('./config.js');
var db = requireLib('./db.js')(config.db.connectionString, config.db.poolSize); 

// Let's create the Express app:
var app = express();

// view engine setup
app.set('view engine', 'jade');

// We need to parse only JSON:
app.use(bodyParser.json({ limit: '5mb' }));
app.use(bodyParser.urlencoded({ extended: false }));

// Prepare route context:
var routeContext = {
    app: app,
    config: config,
    db: db
};

// Mount API v1:
var v1 = require('./routes/v1.js')(routeContext);
app.use('/v1', v1);

// Setup the error handlers:

// The development error handler with stacktrace
if (app.get('env') === 'development') {
    app.use(function (err, req, res, next) {
        res.status(err.status || 500);
        res.render('error', {
            message: err.message,
            error: err
        });
    });
}
else {
    // production error handler with no stacktraces leaked to user
    app.use(function (err, req, res, next) {
        res.status(err.status || 500);
        res.render('error',
            {
                message: err.message,
                error: {}
            });
    });
}

// Start the app's Web server
const PORT = process.env.PORT || config.system.port;

var server = app.listen(PORT, config.system.hostname, function () {

    var address = server.address();
    var hostname = address.address;
    var port = address.port;
    console.log('Time Management API is listening at %s:%s', hostname, port);
    console.log();
}); 
