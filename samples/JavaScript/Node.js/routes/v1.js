/**
 * The root controller for API.v1
 */

var express = require('express');
var expressJwt = require('express-jwt');
var packageJSON = require('./../package.json');

/**
 * Module exports the factory
 * @param {Object} routeContext
 * @returns {Object} Express router
 */
module.exports = function (routeContext) {

    // Extract objects from the route context:
    var config = routeContext.config;

    // Create Express router:
    var router = express.Router();

    // Version info:
    router.get('/version', function (req, res) {
        res.send({version: packageJSON.version});
    });
    
    var handlers = {
        auth: require('./v1/auth-handlers.js')(routeContext),
        users: require('./v1/user-handlers.js')(routeContext),
        work: require('./v1/work-handlers.js')(routeContext),
    };
    
    // Mount authorization handlers:
    router.route("/auth/signup").post(handlers.auth.signup);
    router.route("/auth/signin").post(handlers.auth.signin);
    router.route("/auth/signin-by-token").post(handlers.auth.signinByToken);
    router.route("/auth/password").put(handlers.auth.changePassword);
    
    // All other controllers need JWT for all the routes:
    router.use(expressJwt({secret: config.auth.secret}));

    // The middleware used to fetch the role
    // id for the users with valid tokens
    router.use(handlers.users.checkToken);    
    
    // Mount all the controllers the require auth
    
    // User handling:
    router.route("/users")
        .post(handlers.users.addUser)
        .get(handlers.users.getAllUsers);
    router.route("/users/:id")
        .get(handlers.users.getUser)
        .put(handlers.users.updateUser)
        .delete(handlers.users.deleteUser);    
    
    // Work records handling:
    router.route("/work")
        .post(handlers.work.addRecord)
        .get(handlers.work.getAllRecords);        
    router.route("/work/:id")
        .get(handlers.work.getRecord)
        .put(handlers.work.updateRecord)
        .delete(handlers.work.deleteRecord);

    // Return Express router:
    return router;
};
