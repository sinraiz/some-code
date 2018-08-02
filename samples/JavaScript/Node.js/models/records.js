var async = require('async');

var helper = requireLib('helper.js');
var Record = require("./record.js");

/**
 * Module exports the factory
 * @param {String} database - The database connection
 * @returns {Object} Records collection object
 */
module.exports = function (database) {
    return new RecordsModel(database);
};


/**
 * Creates a Records model instance
 * @class
 * @param {String} connectionString
 * @param {Number} poolSize
 */
function RecordsModel(database) {
    this.db = database
}

/**
 * Adds a new record to the DB.
 * @param {Object} user - The Record object containing the details of the new work record.
 * @param {Function} cb - (err, rec) Callback trigger upon the completion
 */
RecordsModel.prototype.add = function (record, cb) {

    var recordId = 0;
    var addedRecord = null;
    var dtCreated = Date.now(); 

    var db = this.db;
    var dbc = null;
    var self = this;

    async.waterfall([
        function (acb) {
            // Begin a new transaction:
            db.getConnection(acb);
        },
        function (connection, acb) {
            dbc = connection;

            // Create a new Db record:
            dbc.insert('work', {
                dt_created: helper.dateTimeToPostgres(dtCreated),// UTC
                user_id: record.getUserId(),
                dt_day: record.getDayDB(),
                duration_sec: record.getDuration(),
                note: record.getNote()
            }, true, acb);
        },
        function (id, acb) {
            recordId = id;
            // Commit the transaction:
            dbc.commit(acb);
            dbc = null;
        },
        function (acb) {
            self.get(recordId, acb);
        },
        function (record) {
            // Return the result
            cb(null, record);
        }], function (err) {
            if (dbc) {
                dbc.rollback();
            }

            if (err.code === '23503') {
                err.name = 'unknown_user';
            }
            else
            if (err.code === '23505') {
                err.name = 'duplicate_entry';
            }

            cb(err, null);
        });
}

/**
 * Get a work record from the DB based on id
 * @param {Number} userId - The id of the record to be fetched
 * @param {Function} cb - (err, rec) Callback trigger upon the completion. rec will
 *                        be either null if the record is not found or the actual record
 */
RecordsModel.prototype.get = function (recId, cb) {

    var db = this.db;

    async.waterfall([
        function (acb) {
            var sql = ''
                + ' SELECT'
                + '   id,'
                + '   extract(epoch from dt_created at time zone \'utc\') as dt_created,'
                + '   user_id,'
                + '   extract(epoch from dt_day at time zone \'utc\') as dt_day,'
                + '   duration_sec,'
                + '   note,'
                + '   is_under_hours,'
                + '   user_name'
                + ' FROM v_work w'
                + ' WHERE w.id = ?';
            db.queryRow(sql, [recId], acb);
        },
        function (row, acb) {
            // check what we've got
            if (!row) {
                return cb(null, null); // not found
            }            

            // Prepare the record 
            var rec = null;
            try {
                rec = new Record();
                rec.setId(row.id);
                rec._dt_created = new Date(row.dt_created*1000); // UTC as string, the clients convert to local
                rec.setUserId(row.user_id);
                rec.setDay(new Date(row.dt_day * 1000)); // UTC as string, the clients convert to local
                rec.setDuration(row.duration_sec);
                rec.setNote(row.note);

                // The fielda that are not supposed to be
                // accessed from outside the model because
                // they're aggregates and is used only at 
                // the clients' side
                rec._is_under_hours = row.is_under_hours;
                rec._user_name = row.user_name;
            }
            catch (err) {
                // Something is wrong with the data
                return cb({ name: 'bad_format' }, null);
            }

            // The user record was found
            return cb(null, rec);
        }], function (err) {

            cb(err, null);
        });
}

/**
 * Get all the work records from DB
 * @param {Number} userId - Optional user id to filter to fetch only the records from this user
 * @param {Date} from - Optional filter for the starting date for the selection
 * @param {Date} to - Optional filter for the ending date for the selection
 * @param {Function} cb - (err, recs) Callback trigger upon the completion. recs will
 *                        contain the array of the actual work records
 */
RecordsModel.prototype.getAll = function (userId, from, to, cb) {

    var db = this.db;

    async.waterfall([
        function (acb) {
            var sql = ''
                + ' SELECT'
                + '   id,'
                + '   extract(epoch from dt_created at time zone \'utc\') as dt_created,'
                + '   user_id,'
                + '   extract(epoch from dt_day at time zone \'utc\') as dt_day,'
                + '   duration_sec,'
                + '   note,'
                + '   is_under_hours,'
                + '   user_name'
                + ' FROM v_work w'
                + ' WHERE (user_id = $1 OR $1 IS NULL)'
                + '       AND (date_trunc(\'day\', dt_day) >= $2 OR $2 IS NULL)'
                + '       AND  (date_trunc(\'day\', dt_day) <= $3 OR $3 IS NULL)'
                + ' ORDER BY dt_day';
            db.query(sql, [userId, from, to], acb);
        },
        function (results, acb) {
            // check what we've got
            if (!results) {
                return cb(null, null); // not found
            }

            // Run through the record set, parse each entry
            // and push into the array of User objects
            var recs = [];
            results.map(function (row) {
                try {
                    var rec = new Record();
                    rec.setId(row.id);
                    rec._dt_created = new Date(row.dt_created * 1000); // UTC as string, the clients convert to local
                    rec.setUserId(row.user_id);
                    rec.setDay(new Date(row.dt_day * 1000)); // UTC as string, the clients convert to local
                    rec.setDuration(row.duration_sec);
                    rec.setNote(row.note);

                    // The field that are not supposed to be
                    // accessed from outside the model because
                    // they're aggregates and are used only at 
                    // the clients
                    rec._is_under_hours = row.is_under_hours;
                    rec._user_name = row.user_name;

                    // Add to the resulting collection
                    recs.push(rec);
                }
                catch (err) {
                    // Something is wrong with the data
                    return cb({ name: 'bad_format' }, null);
                }
            });

            // Return all the user records
            return cb(null, recs);
        }], function (err) {
            cb(err, null);
        });
}

/**
 * Update the work record in the DB based on his id
 * @param {Number} id - The id of the work record to be updated
 * @param {Object} user - The work record with new data
 * @param {Function} cb - (err, rec) Callback trigger upon the completion. The rec will
 *                        be either null if the record is not found or the actual record
 *                        with new fields otherwise
 */
RecordsModel.prototype.update = function (id, rec, cb) {

    var db = this.db;
    var dbc = null;
    var self = this;

    // Collect the fields to be updated
    var updateData = {};
    if (rec.getUserId() > 0) {
        updateData.user_id = rec.getUserId();
    }
    if (rec.getDuration() > 0) {
        updateData.duration_sec = rec.getDuration();
    }
    if (rec.getNote()) {
        updateData.note = rec.getNote();
    }
    if (rec.getDay()) {
        updateData.dt_day = rec.getDayDB();
    }
    async.waterfall([
        function (acb) {
            // Begin a new transaction
            // We have select and update , nust be atomic
            db.getConnection(acb);
        },
        function (connection, acb) {
            dbc = connection;

            // Make sure the records exists within
            // this transaction's scope
            var sql = ''
                + ' SELECT id'
                + ' FROM work w'
                + ' WHERE w.id = ?';
            dbc.queryRow(sql, [id], acb);
        },
        function (row, acb) {
            // check what we've got
            if (!row) {
                return cb({ name: 'rec_not_found' }, null); // not found
            }
            // The user exists, so we can issue the update
            dbc.update('work', id, updateData, acb);
        },
        function (acb) {
            // Commit the transaction:
            dbc.commit(acb);
            dbc = null;
        },
        function (acb) {
            self.get(id, acb);
        },
        function (updatedRec, acb) {

            // Return the updated user record
            return cb(null, updatedRec);

        }], function (err) {
            if (dbc) {
                dbc.rollback();
            }

            if (err.code === '23503') {
                err.name = 'unknown_user';
            }
            else
            if (err.code === '23505') {
                err.name = 'duplicate_entry';
            }
            
            cb(err, null);
        });
}

/**
 * Delete the user record from the DB based on the id
 * @param {Number} userId - The id of the work record to be fetched
 * @param {Function} cb - (err) Callback triggered upon the completion. If err is null we're ok
 */
RecordsModel.prototype.delete = function (userId, cb) {

    this.db.delete('work', userId, function (err) {
        cb(err);
    });
}

/**
 * Deletes all the work records (required for unit testing)
 * @param {Function} cb - (err) Callback triggered upon the completion. If err is null we're ok
 */
RecordsModel.prototype._deleteAll = function (cb) {

    this.db.query('DELETE FROM work', null, function (err) {
        cb(err);
    });
}
