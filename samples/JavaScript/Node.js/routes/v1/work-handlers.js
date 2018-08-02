/**
 * The work management controller for API.v1
 */
var async = require('async');
var helper = requireLib('helper.js');
var Record = require("../../models/record.js");
var Records = require("../../models/records.js");


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
        // Add a new work record with the given details
        addRecord: function (req, res, next) {

            var callerId = req.user.id;
            var userId = req.body.user_id;

            if (!userId) {
                // If the user is undefined the work record
                // will be assigned to caller
                userId = callerId;
            }

            // Only admin can manipulate the work of others
            if (userId != callerId &&
                req.aux.role != helper.enRoles.ADMIN) {
                return res.sendStatus(403);
            }

            // Deal with the date
            if (!req.body.when) {
                return res.status(422).send('no_date');
            }
            var timestamp = Date.parse(req.body.when);
            if (isNaN(timestamp)) {
                return res.status(422).send('bad_date');
            }

            var record = new Record();
            var records = new Records(db);

            // Validate the input and assign to the 
            // user object
            try {
                record.setUserId(userId);
                record.setDay(timestamp);
                record.setDuration(req.body.duration);
                record.setNote(req.body.note);
            }
            catch (err) {
                // The validation failed
                return res.status(422).send(err.name);
            }

            async.waterfall([
                function (acb) {
                    records.add(record, acb)
                },
                function (rec, acb) {
                    res.send(rec.toJson());
                }
            ], function (err) {
                // report the error as 422 with content set to error code
                return res.status(422).send(err.name);
            });
        },

        // Get details of the work record by its id
        getRecord: function (req, res, next) {

            var records = new Records(db);

            var idRecord = req.params.id;
            var callerId = req.user.id;
            
            records.get(idRecord, function (err, rec) {


                if (err) {
                    return res.sendStatus(422);
                }

                if (rec == null) {
                    return res.status(422).send('rec_not_found');
                }

                // Only admin can fetch work of other users
                if (callerId != rec.getUserId() &&
                    req.aux.role != helper.enRoles.ADMIN) {
                    return res.sendStatus(403);
                }

                // Send the work record data
                return res.send(rec.toJson());
            });
        },

        // Modify the given work record by its id
        updateRecord: function (req, res, next) {

            var idRecord = req.params.id;
            var callerId = req.user.id;

            var record = new Record();
            var records = new Records(db);


            // Assign the new record details from request's body

            // Validate the input and assign the values to the 
            // work record object
            try {
                // Deal with the date
                if (req.body.when) {
                    var timestamp = Date.parse(req.body.when);
                    if (!isNaN(timestamp)) {
                        record.setDay(timestamp);
                    }
                }

                if (req.body.user_id) {
                    record.setUserId(req.body.user_id);
                }

                if (req.body.duration) {
                    record.setDuration(req.body.duration);
                }

                if (req.body.note) {
                    record.setNote(req.body.note);
                }

            }
            catch (err) {
                // The validation failed
                return res.status(422).send(err.name);
            }

            async.waterfall([
                function ( acb) {
                    // Get the current record to check the owner
                    records.get(idRecord, acb);
                },
                function (fetchedRecord, acb) {
                    // If the record with this ID exists at all
                    if (!fetchedRecord){
                        return res.status(422).send('rec_not_found');
                    }

                    // Only admin can manipulate the records created by others
                    if (callerId != fetchedRecord.getUserId() &&
                        req.aux.role != helper.enRoles.ADMIN) {
                        return res.sendStatus(403);
                    }

                    // Issue the update
                    records.update(idRecord, record, acb);
                },
                function (rec) {
                    // Send the updated work record data back
                    return res.send(rec.toJson());
                }
            ], function (err) {
                if (err.name === 'JsonWebTokenError' || err.name === 'TokenExpiredError') {
                    return res.status(422).send('bad_token');
                }
                else if (err.name === 'rec_not_found') {
                    return res.status(422).send('rec_not_found');
                }

                next(err);
            });
        },

        // Delete the work record based on its id
        deleteRecord: function (req, res, next) {

            var records = new Records(db);

            var idRecord = req.params.id;
            var callerId = req.user.id;

            async.waterfall([
                function (acb) {
                    // Get the current record to check the owner
                    records.get(idRecord, acb);
                },
                function (fetchedRecord) {
                    // If the record with this ID exists at all
                    if (!fetchedRecord) {
                        return res.status(422).send('rec_not_found');
                    }

                    // Only admin can manipulate the records created by others
                    if (callerId != fetchedRecord.getUserId() &&
                        req.aux.role != helper.enRoles.ADMIN) {
                        return res.sendStatus(403);
                    }

                    // Issue the delete
                    records.delete(idRecord, function (err, userData) {

                        if (err) {
                            return res.sendStatus(422);
                        }
                        return res.status(200).send('OK');
                    });
                }], function (err) {
                if (err.name === 'JsonWebTokenError' || err.name === 'TokenExpiredError') {
                    return res.status(422).send('bad_token');
                }
                else if (err.name === 'rec_not_found') {
                    return res.status(422).send('rec_not_found');
                }

                next(err);
            });
        },

        // Get all the records either from all the users
        // or only the current one
        getAllRecords: function (req, res, next) {

            var records = new Records(db);

            var idUser = req.params.id;
            var callerId = req.user.id;
            var userFilter = req.query.user_id;

            // Only admins can see the work records of others
            if (req.aux.role != helper.enRoles.ADMIN) {
                userFilter = callerId;
            }

            // Get the optional date filters from the request
            var aDtStart = null;
            var aDtFinish = null;

            // Grab the starting date parameter for fetching
            if (req.query.from)
                aDtStart = helper.dateToPostgres(req.query.from);

            // Grad the ending date parameter for fetching
            if (req.query.to)
                aDtFinish = helper.dateToPostgres(req.query.to);


            records.getAll(userFilter, aDtStart, aDtFinish, function (err, workRecords) {

                if (err) {
                    return res.sendStatus(422);
                }

                if (workRecords == null) {
                    return res.status(422).send('rec_not_found');
                }

                // Prepare the result
                var resJson = [];
                workRecords.map(function (record) {
                    resJson.push(record.toJson());
                });

                // Send the records' data
                return res.send(resJson);
            });
        },

    }; // The END of handlers object  
} // module exports 