var async = require('async');

var helper = requireLib('helper.js');
var User = require("./user.js");

/**
 * Module exports the factory
 * @param {String} database - The database connection
 * @returns {Object} Users collection object
 */
module.exports = function (database) {
    return new UsersModel(database);
};


/**
 * Creates a Users model instance
 * @class
 * @param {String} connectionString
 * @param {Number} poolSize
 */
function UsersModel(database) {
    this.db = database
}

/**
 * Adds a new user to the DB.
 * @param {Object} user - The User object containing the details of the new record.
 * @param {Function} cb - (err, usr) Callback trigger upon the completion
 */
UsersModel.prototype.add = function (user, cb) {

    var userId = 0;
    var addedUser = null;

    var db = this.db;
    var dbc = null;

    async.waterfall([
        function (acb) {
            // Begin a new transaction:
            db.getConnection(acb);
        },
        function (connection, acb) {
            dbc = connection;

            // Create a new Db record:
            dbc.insert('users', {
                full_name: user.getName(),                 
                role_id: user.getRole().value,        
                email: user.getEmail(),       
                pwd_hash: user.getPasswordHash()                                
            }, true, acb);
        },
        function (id, acb) {
            // Retain the user id
            userId = id;

            // Commit the transaction:
            dbc.commit(acb);
            dbc = null;
        },
        function (acb) {
            // the resulting user record
            addedUser = new User();
            addedUser.copy(user);
            addedUser.setId(userId);

            // Return the result
            cb(null, addedUser);

        }], function (err) {
        if (dbc) {
            dbc.rollback();
        }
        if (err.code === '23505') {
            err.name = 'email_in_use';
        }

        cb(err, null);
    });
}

/**
 * Get all the users from DB
 * @param {Function} cb - (err, usr) Callback trigger upon the completion. usr will
 *                        contain the array of the actual user records
 */
UsersModel.prototype.getAll = function (cb) {

    var db = this.db;

    async.waterfall([
        function (acb) {
            var sql = ''
                + ' SELECT'
                + '   id,'
                + '   role_id,'
                + '   email,'
                + '   pwd_hash,'
                + '   max_hours,'
                + '   full_name'
                + ' FROM users u'
                + ' ORDER BY id';
            db.query(sql, null, acb);
        },
        function ( results, acb) {
            // check what we've got
            if (!results) {
                return cb(null, null); // not found
            }

            // Run through the record set, parse each entry
            // and push into the array of User objects
            var resUsers = [];
            results.map(function (row) {
                try {
                    var user = new User();
                    user.setId(row.id);
                    user.setRoleId(row.role_id);
                    user.setEmail(row.email);
                    user.setPasswordHash(row.pwd_hash);
                    user.setWorkingHours(row.max_hours);
                    user.setName(row.full_name);

                    // Add to the resulting collection
                    resUsers.push(user);
                }
                catch (err) {
                    // Something is wrong with the data
                    return cb({ name: 'bad_format' }, null);
                }
            });                      

            // Return all the user records
            return cb(null, resUsers);
        }], function (err) {

            cb(err, null);
        });
}

/**
 * Get a user from the DB based on id
 * @param {Number} userId - The id of the user to be fetched
 * @param {Function} cb - (err, usr) Callback trigger upon the completion. usr will
 *                        be either null if the user is not found or the actual record
 */
UsersModel.prototype.get = function (userId, cb) {
    
    var db = this.db;

    async.waterfall([
        function (acb) {
            var sql = ''
                + ' SELECT'
                + '   id,'
                + '   role_id,'
                + '   email,'
                + '   pwd_hash,'
                + '   max_hours,'
                + '   full_name'
                + ' FROM users u'
                + ' WHERE u.id = ?';
            db.queryRow(sql, [userId], acb);
        },
        function (row, acb) {
            // check what we've got
            if (!row) {
                return cb(null, null); // not found
            }

            // Get the role
            var role = helper.enRoles.getByValue('value', row.role_id);
            if (role == null) {
                // Need to set it with a default value
                role = helper.enRoles.USER;
            }

            // Prepare the record
            var user = null;
            try {
                user = new User();
                user.setId(row.id);
                user.setRole(role);
                user.setEmail(row.email);
                user.setPasswordHash(row.pwd_hash);
                user.setWorkingHours(row.max_hours);
                user.setName(row.full_name);
            }
            catch (err) {
                // Something is wrong with the data
                return cb({ name: 'bad_format' }, null);
            }

            // The user record was found
            return cb(null, user);
        }], function (err) {

        cb(err, null);
    });
}

/**
 * Find a user record provided the email address
 * @param {Number} email - The email of the user to be fetched
 * @param {Function} cb - (err, usr) Callback trigger upon the completion. usr will
 *                        be either null if the user is not found or the actual record
 */
UsersModel.prototype.findbyEmail = function (email, cb) {

    var db = this.db;

    async.waterfall([
        function (acb) {
            var sql = ''
                + ' SELECT'
                + '   id,'
                + '   role_id,'
                + '   email,'
                + '   pwd_hash,'
                + '   max_hours,'
                + '   full_name'
                + ' FROM users u'
                + ' WHERE u.email = ?';
            db.queryRow(sql, [email.toLowerCase()], acb);
        },
        function (row, acb) {
            // check what we've got
            if (!row) {
                return cb(null, null); // not found
            }

            // Get the role
            var role = helper.enRoles.getByValue('value', row.role_id);
            if (role == null) {
                // Need to set it with a default value
                role = helper.enRoles.USER;
            }

            // Prepare the record
            var user = null;
            try {
                user = new User();
                user.setId(row.id);
                user.setRole(role);
                user.setEmail(row.email);
                user.setPasswordHash(row.pwd_hash);
                user.setWorkingHours(row.max_hours);;
                user.setName(row.full_name);
            }
            catch (err) {
                // Something is wrong with the data
                return cb({ name: 'bad_format' }, null);
            }

            // The user record was found
            return cb(null, user);
        }], function (err) {

            cb(err, null);
        });
}

/**
 * Update the user record in the DB based on his id
 * @param {Number} id - The id of the user to be updated
 * @param {Object} user - The user record with new data
 * @param {Function} cb - (err, usr) Callback trigger upon the completion. The usr will
 *                        be either null if the user is not found or the actual record
 *                        with new fields
 */
UsersModel.prototype.update = function (id, user, cb) {

    var db = this.db;
    var dbc = null;
    var self = this;

    // Collect the fields to be updated
    var updateData = {};
    if (user.getPasswordHash()) {
        updateData.pwd_hash = user.getPasswordHash();
    }
    if (user.getEmail()) {
        updateData.email = user.getEmail();
    }
    if (user.getName()) {
        updateData.full_name = user.getName();
    }
    if (user.getRoleId() > 0) {
        updateData.role_id = user.getRoleId();
    }
    if (user.getWorkingHours() > 0) {
        updateData.max_hours = user.getWorkingHours();
    }

    async.waterfall([
        function (acb) {
            // Begin a new transaction
            // We have select and update , nust be atomic
            db.getConnection(acb);
        },
        function (connection, acb) {
            dbc = connection; 

            // Make sure the user exists
            var sql = ''
                + ' SELECT id'
                + ' FROM users u'
                + ' WHERE u.id = ?';
            dbc.queryRow(sql, [id], acb);
        },
        function (row, acb) {
            // check what we've got
            if (!row) {
                return cb({ name: 'user_not_found' }, null); // not found
            }
            // The user exists, so we can issue the update
            dbc.update('users', id, updateData, acb);
        },
        function (acb) {
            // Commit the transaction:
            dbc.commit(acb);
            dbc = null;
        },
        function (acb) {
            self.get(id, acb);
        },
        function (updatedUser, acb) {

            // Return the updated user record
            return cb(null, updatedUser);

        }], function (err) {
            if (dbc) {
                dbc.rollback();
            }

            if (err.code === '23505') {
                err.name = 'email_in_use';
            }
            cb(err, null);
        });
}

/**
 * Delete a user from the DB based on his id
 * @param {Number} userId - The id of the user to be deleted
 * @param {Function} cb - (err) Callback triggered upon the completion. If err is null we're ok
 */
UsersModel.prototype.delete = function (userId, cb) {
    
    this.db.delete('users', userId,
        function (err) {

        if (err && err.code === '23503') {
            err.name = 'user_has_records';
            }

        cb(err);
    });
}


/**
 * Deletes all the user records (required for unit testing)
 * @param {Function} cb - (err) Callback triggered upon the completion. If err is null we're ok
 */
UsersModel.prototype._deleteAll = function (cb) {

    this.db.query('DELETE FROM users', null, function (err) {
        cb(err);
    });
}

/**
 * Deletes all the user records (required for unit testing)
 * @param {Function} cb - (err) Callback triggered upon the completion. If err is null we're ok
 */
UsersModel.prototype._deleteAllButAdmin = function (cb) {

    this.db.query('DELETE FROM users WHERE email NOT ilike \'%admin%\'', null, function (err) {
        cb(err);
    });
}