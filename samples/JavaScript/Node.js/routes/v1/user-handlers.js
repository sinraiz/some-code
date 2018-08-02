/**
 * The user management controller for API.v1
 */
var async = require('async');
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
        // Middleware to parse the token and identify the role
        checkToken: function (req, res, next) {
            var users = new Users(db);
            users.get(req.user.id, function (err, userData) {

                if (err || userData == null) {
                    return res.sendStatus(403);
                }

                req.aux = {
                    role: userData.getRole()
                };

                // If everything is fine the proceed to the installed
                // route handler
                next();
            });
        },

        // Add a new user with the given details
        addUser: function (req, res, next) {

            var idUser = req.params.id;
            var callerId = req.user.id;

            // Only admin and managers can add new users
            if (req.aux.role != helper.enRoles.ADMIN &&
                req.aux.role != helper.enRoles.MANAGER) {
                return res.sendStatus(403);
            }

            var user = new User();
            var users = new Users(db);

            // Validate the input and assign to the 
            // user object
            try {
                // Mandatory fields
                user.setName(req.body.name);
                user.setEmail(req.body.email);
                user.setPassword(req.body.password);

                // Optional fields
                if (req.body.working_hours) {
                    user.setWorkingHours(req.body.working_hours);
                }

                if (req.body.role) {
                    user.setRoleId(req.body.role);
                }
                else {
                    // Set the default role
                    user.setRole(helper.enRoles.USER);
                }
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
                    res.send(usr.toJson());
                }
            ], function (err) {
                // report the error as 422 with content set to error code
                return res.status(422).send(err.name);
            });
        }, 

        // Get all the users
        getAllUsers: function (req, res, next) {

            var users = new Users(db);

            var idUser = req.params.id;
            var callerId = req.user.id;

            // Only admin and managers can fetch other users
            if (req.aux.role != helper.enRoles.ADMIN &&
                req.aux.role != helper.enRoles.MANAGER) {
                return res.sendStatus(403);
            }

            users.getAll(function (err, userRecords) {

                if (err) {
                    return res.sendStatus(422);
                }

                if (userRecords == null) {
                    return res.status(422).send('user_not_found');
                }

                // Prepare the result
                var resJson = [];
                userRecords.map(function (record) {
                    resJson.push(record.toJson());
                });

                // Send the user data
                return res.send(resJson);
            });
        },

        // Get user details provided the id
        getUser: function (req, res, next) {
            
            var users = new Users(db);

            var idUser = req.params.id;
            var callerId = req.user.id;

            // Only admin and managers can fetch users other than themselves
            if (callerId != idUser &&
                req.aux.role != helper.enRoles.ADMIN &&
                req.aux.role != helper.enRoles.MANAGER) {
                return res.sendStatus(403);
            }

            users.get(idUser, function (err, userData) {

                
                if (err) {
                    return res.sendStatus(422);
                }

                if (userData == null) {
                    return res.status(422).send('user_not_found');
                }

                // Send the user data
                return res.send(userData.toJson());
            });
        },

        // Modify the user details provided the id
        updateUser: function (req, res, next) {

            var idUser = req.params.id;
            var callerId = req.user.id;

            var user = new User();
            var users = new Users(db);


            // Only admin and managers can manipulate users other than themselves
            if (callerId != idUser &&
                req.aux.role != helper.enRoles.ADMIN &&
                req.aux.role != helper.enRoles.MANAGER) {
                return res.sendStatus(403);
            } 

            // Assign the new user details from request's body

            // Validate the input and assign the values to the 
            // user object
            try { 
                if (req.body.name) {
                    user.setName(req.body.name);
                }
                if (req.body.email) {
                    user.setEmail(req.body.email);
                }
                if (req.body.working_hours) {
                    user.setWorkingHours(req.body.working_hours);
                }
                if (req.body.role) {
                    // The caller wants to change the role. Make sure he's
                    // not doing it for himself
                    if (callerId == idUser) {
                        return res.sendStatus(403);
                    }
                    user.setRoleId(req.body.role); 
                }
                if (req.body.password) {
                    user.setPassword(req.body.password);
                }
            }
            catch (err) {
                // The validation failed
                return res.status(422).send(err.name);
            }

            users.update(idUser, user, function (err, userData) {

                if (err) {
                    return res.status(422).send(err.name);
                }

                if (userData == null) {
                    return res.status(422).send('user_not_found');
                }

                // Send back the updated user data
                return res.send(userData.toJson());
            });
        },

        // Delete the user based on id
        deleteUser: function (req, res, next) {

            var users = new Users(db);

            var idUser = req.params.id;
            var callerId = req.user.id;

            // Only admins and managers can delete other users
            // and user cannot delete himself
            if (callerId == idUser ||
                req.aux.role != helper.enRoles.ADMIN &&
                req.aux.role != helper.enRoles.MANAGER) {
                return res.sendStatus(403);
            }
            
            users.delete(idUser, function (err, userData) {

                if (err) {
                    return res.status(422).send(err.name);
                }
                return res.send('OK');
            });
        }

        }; // The END of handlers object  
    } // module exports 