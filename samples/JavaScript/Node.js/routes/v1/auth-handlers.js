/**
 * The authorization controller for API.v1
 */

var async = require('async');
var express = require('express');
var jwt = require('jsonwebtoken');
var helper = requireLib('helper.js');
var User = require("../../models/user.js");
var Users = require("../../models/users.js");

/**
 * Module exports the factory
 * @param {Object} routeContext
 * @returns {Object} The controller with all the route handlers
 */
module.exports = function (routeContext) {
    
    // Extract objects from the route context:
    var config = routeContext.config;
    var db = routeContext.db;
    
    // The route handlers:    
    return {

        // Sign up provided username (email0 and password:
        signup: function (req, res, next) {
                        
            var user = new User();
            var users = new Users(db);
            
            // Validate the input and assign to the 
            // user object
            try {
                user.setName(req.body.name);
                user.setEmail(req.body.email);
                user.setPassword(req.body.password);
                user.setRole(helper.enRoles.USER);
            }
            catch (err) {
                // The validation failed
                return res.status(422).send(err.name);
            }
                        
            async.waterfall([
                function (acb) {                    
                    users.add(user, acb)
                },
                function (usr, acb) {
                    var json = getAuthResponse(config, db, usr, acb);
                    res.send(json);
                }
            ], function (err) {
                // report the error as 422 with content set to error code
                return res.status(422).send(err.name);
            });
        }, 
        
        // Sign in by login/password:
        signin: function (req, res, next) {
            
            var tmpUser = new User();

            var users = new Users(db);
            
            // Validate the input and assign to the 
            // user object
            try {
                tmpUser.setEmail(req.body.email);
                tmpUser.setPassword(req.body.password);
            }
            catch (err) {
                // The validation failed
                return res.status(422).send(err.name);
            }
            
            var password = req.body.password;
            var user = null;

            // Find the user by his email and validate the password
            async.waterfall([
                function (acb) {
                    users.findbyEmail(tmpUser.getEmail(), acb);
                },
                function (usrFound, acb) {
                    // Retain the found user record
                    user = usrFound;
                    
                    // Check if it was found
                    if (!usrFound) {
                        return res.status(401).send("auth_error");
                    }
                    
                    // Compare the hashes
                    user.checkPassword(password, acb);
                },
                function (isMatch, acb) {
                    if (!isMatch) {
                        return res.status(401).send("auth_error");
                    }
                    var json = getAuthResponse(config, db, user, acb);
                    res.send(json);
                }
            ], function (err) {
                // report the error as 401 with content set to error code
                return res.status(401).send(err.name);
            })
        },
        
        // Sign in by token:
        signinByToken: function (req, res, next) {
            
            // Check for the manadatory token
            if (!req.body.token) {
                return res.sendStatus(401);
            }

            var users = new Users(db);
            
            // Async verification
            async.waterfall([
                function (acb) {
                    // Check the token validity
                    jwt.verify(req.body.token, config.auth.secret, acb);
                },
                function (userId, acb) {
                    if (!userId) {
                        // Token invalid
                        return res.sendStatus(401);
                    }

                    // Attempt to find user by that Id
                    users.get(userId.id, acb);
                },
                function (usrFound, acb) {                    
                    // Check if it was found
                    if (!usrFound) {
                        return res.status(401).send('user_not_found');
                    }

                    var json = getAuthResponse(config, db, usrFound, acb);
                    res.send(json);                    
                }], function (err) {
                if (err.name === 'JsonWebTokenError' || err.name === 'TokenExpiredError') {
                    return res.sendStatus(401);
                }
                
                next(err);
            });
        },
        
        // Set password:
        changePassword: function (req, res, next) {
            // Make sure the token is given in the body because 
            // it can be used via the password recovery service 
            // and thus the caller may not have the auth header
            if (!req.body.token) {
                return res.status(422).send('no_token');
            }
                        
            var tmpUser = new User();
            
            var users = new Users(db);
            
            // Validate the input and assign to the 
            // user object
            try {
                tmpUser.setPassword(req.body.password);
            }
            catch (err) {
                // The validation failed
                return res.status(422).send(err.name);
            }
                        
            async.waterfall([
                function (acb) {
                    jwt.verify(req.body.token, config.auth.secret, acb);
                },
                function (user, acb) {
                    // Grab the user id from the given token
                    var userId = user.id;
                    
                    // Call the update with only one field set (password)
                    users.update(userId, tmpUser, acb);
                },
                function (user, acb) {
                    var json = getAuthResponse(config, db, user, acb);
                    res.send(json); 
                }
            ], function (err) {
                if (err.name === 'JsonWebTokenError' || err.name === 'TokenExpiredError') {
                    return res.status(422).send('bad_token');
                }
                else
                if (err.name === 'user_not_found') {
                    return res.status(422).send('user_not_found');
                }
                
                next(err);
            });
        },
        
        
    }; // The END of handlers object  
} // module exports 

function getAuthResponse(config, db, userData, callback) 
{
    return {
        token: jwt.sign({ id: userData.getId() },
                   config.auth.secret,
                   { expiresIn: config.auth.expiresIn }),
        user: userData.toJson()
    };    
}
