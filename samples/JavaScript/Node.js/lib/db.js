/**
 * Database Driver
 */

var pg = require('pg');
var util = require('util');

// From PG docs
// https://github.com/brianc/node-postgres/wiki/pg
// -----------------------------------------------
// By default fields with type int8 are returned as strings because JavaScript
// cannot represent 64-bit numbers. 
// -----------------------------------------------
// Yes, we want int8 to be numbers:
pg.defaults.parseInt8 = true;

/**
 * Module exports the factory
 * @param {String} connectionString
 * @param {Number} poolSize
 * @returns {Object} database driver instance
 */
module.exports = function (connectionString, poolSize) {
    return new DB(connectionString, poolSize);
};

/**
 * IQuery is an abstract class
 * @class
 */
function IQuery() { }

/**
 * Run a query and return rows
 * @param {String} sql
 * @param {Array} [params]
 * @param {Function} [cb] - (err, rows)
 */
IQuery.prototype.query = function (sql, params, cb) {
    // Normalize the input:
    var input = normalizeInput(sql, params, cb);
    sql = input.sql;
    params = input.params;
    cb = input.cb;

    // Replace ? with $i:
    var paramIndex = 0;

    sql = sql.replace(/\?/g, function () {
        paramIndex++;
        return '$' + paramIndex;
    });

    // Replace all empty strings with nulls (like Oracle does):
    for (var i = 0; i < params.length; i++) {
        if (params[i] === '') params[i] = null;
        if (typeof params[i] === 'undefined') params[i] = null;
    }

    // Run the query:
    this._query(sql, params, function (err, result) {
        if (err) return cb(err);
        cb(null, result.rows);
    });
};

/**
 * Run a query and return the vector
 * @param {String} sql
 * @param {Array} [params]
 * @param {Function} [cb] - (err, vector)
 */
IQuery.prototype.queryVector = function (sql, params, cb) {
    // Normalize the input:
    var input = normalizeInput(sql, params, cb);
    sql = input.sql;
    params = input.params;
    cb = input.cb;

    // Run the query and return the vector:
    this.query(sql, params, function (err, rows) {
        if (err) return cb(err);
        var vector = [];

        for (var i = 0; i < rows.length; i++) {
            var row = rows[i];
            vector.push(row[Object.keys(row)[0]]);
        }

        cb(null, vector);
    });
};

/**
 * Run a query and return the first row
 * @param {String} sql
 * @param {Array} [params]
 * @param {Function} [cb] - (err, row)
 */
IQuery.prototype.queryRow = function (sql, params, cb) {
    // Normalize the input:
    var input = normalizeInput(sql, params, cb);
    sql = input.sql;
    params = input.params;
    cb = input.cb;

    // Run the query and return only the first row:
    this.query(sql, params, function (err, rows) {
        if (err) return cb(err);
        cb(null, rows.length > 0 ? rows[0] : null);
    });
};

/**
 * Run a query and return the first field of the first row
 * @param {String} sql
 * @param {Array} [params]
 * @param {Function} [cb] - (err, field)
 */
IQuery.prototype.queryValue = function (sql, params, cb) {
    // Normalize the input:
    var input = normalizeInput(sql, params, cb);
    sql = input.sql;
    params = input.params;
    cb = input.cb;

    // Run the query and return only the first field of the first row:
    this.queryRow(sql, params, function (err, row) {
        if (err) return cb(err);
        if (!row) return cb(null, null);
        cb(null, row[Object.keys(row)[0]]);
    });
};

/**
 * Run a query and return nothing
 * @param {String} sql
 * @param {Array} [params]
 * @param {Function} [cb] - (err)
 */
IQuery.prototype.execute = function (sql, params, cb) {
    // Normalize the input:
    var input = normalizeInput(sql, params, cb);
    sql = input.sql;
    params = input.params;
    cb = input.cb;

    // Run the query and return nothing:
    this.query(sql, params, function (err, rows) {
        cb(err);
    });
};

/**
 * Insert a new row
 * @param {String} table
 * @param {Object} obj
 * @param {Boolean} [returnId = false]
 * @param {Function} [cb] - (err, id) or (err)
 */
IQuery.prototype.insert = function (table, obj, returnId, cb) {
    // Normalize input:
    if (typeof returnId === 'function') {
        cb = returnId;
        returnId = false;
    }

    if (!cb) cb = function () { };

    // Generate SQL:
    var fields = Object.keys(obj);
    var values = getValues(obj, fields);

    var sql = 'INSERT INTO '
        + table
        + ' ('
        + fields.join(',')
        + ') VALUES ('
        + getArrayOf('?', fields.length).join(',')
        + ')';

    if (returnId) sql += ' RETURNING id';

    // Run query and return id or nothing:
    this.query(sql, values, function (err, rows) {
        if (err) return cb(err);
        if (returnId) return cb(null, rows[0].id);
        cb(null);
    });
};

/**
 * Update a row by id
 * @param {String} table
 * @param {Number} id
 * @param {Object} obj
 * @param {Function} [cb] - (err)
 */
IQuery.prototype.update = function (table, id, obj, cb) {
    // Ignore empty updates:
    if (Object.keys(obj).length === 0)
        return process.nextTick(function () { cb(); });

    // Generate SQL:
    var pairs = [];
    var values = [];

    for (var field in obj) {
        pairs.push(field + ' = ?');
        values.push(obj[field]);
    }

    var sql = 'UPDATE '
        + table
        + ' SET '
        + pairs.join(',')
        + ' WHERE id = ?';

    values.push(id);

    // Run query and return nothing:
    this.execute(sql, values, cb);
};

/**
 * Delete a row by id
 * @param {String} table
 * @param {Number} id
 * @param {Function} [cb] - (err)
 */
IQuery.prototype.delete = function (table, id, cb) {
    this.execute('DELETE FROM ' + table + ' WHERE id = ?', [id], cb);
};

/**
 * An abstract method for run queries
 * @abstract
 * @param {String} sql
 * @param {Array} [params]
 * @param {Function} [cb] - (err, rows)
 */
IQuery.prototype._query = function (sql, params, cb) { };

/**
 * Creates a DB instance
 * @class
 * @param {String} connectionString
 * @param {Number} poolSize
 */
function DB(connectionString, poolSize) {
    this._connectionString = connectionString;
    this._poolSize = poolSize;
}

util.inherits(DB, IQuery);

/**
 * Acquire a connection from the pool and begin a transaction
 * @param {Function} cb - (err, con)
 */
DB.prototype.getConnection = function (cb) {
    this._getConnection(function (err, client, done) {
        if (err) return cb(err);

        client.query('BEGIN', function (err) {
            if (err) {
                done(err);
                return cb(err);
            }

            cb(null, new Connection(client, done));
        });
    });
};

/**
 * Overrides IQuery abstract method
 */
DB.prototype._query = function (sql, params, cb) {
    this._getConnection(function (err, client, done) {
        if (err) return cb(err);

        query(client, sql, params, function (err, result) {
            done(err);
            cb(err, result);
        });
    });
};

/**
 * Get a connection from the pool
 * @param {Function} [cb] - (err, con)
 */
DB.prototype._getConnection = function (cb) {
    pg.defaults.poolSize = this._poolSize;
    pg.connect(this._connectionString, cb);
};

/**
 * Creates a Connection instance
 * @class
 * @param {Object} client
 * @param {Function} done
 */
function Connection(client, done) {
    this._client = client;
    this._done = done;
}

util.inherits(Connection, IQuery);

/**
 * Commit the transaction and release the connection
 * @param {Function} [cb] - (err)
 */
Connection.prototype.commit = function (cb) {
    var that = this;
    if (!cb) cb = function () { };

    this.execute('COMMIT', function (err) {
        that._done(err);
        if (err) return cb(err);
        cb();
    });
};

/**
 * Rollback the transaction and release the connection
 * @param {Function} [cb] - (err)
 */
Connection.prototype.rollback = function (cb) {
    var that = this;
    if (!cb) cb = function () { };

    this.execute('ROLLBACK', function (err) {
        that._done(err);
        if (err) return cb(err);
        cb();
    });
};

/**
 * Overrides IQuery abstract method
 */
Connection.prototype._query = function (sql, params, cb) {
    query(this._client, sql, params, cb)
};

function normalizeInput(sql, params, cb) {
    if (!params) params = [];

    if (typeof params === 'function') {
        cb = params;
        params = [];
    }

    if (!cb) cb = function () { };
    return { sql: sql, params: params, cb: cb };
}

function getValues(obj, keys) {
    var values = [];

    for (var i = 0; i < keys.length; i++)
        values.push(obj[keys[i]]);

    return values;
}

function getArrayOf(elem, count) {
    var arr = [];

    for (var i = 0; i < count; i++)
        arr.push(elem);

    return arr;
}

function query(client, sql, params, cb) {
    if (!client.queryRegistry)
        client.queryRegistry = { next: 1, map: {} };

    var queryName = client.queryRegistry.map[sql];

    if (queryName) {
        client.query({ name: queryName, values: params }, cb);
    } else {
        queryName = 'q' + client.queryRegistry.next;
        client.queryRegistry.next++;
        client.queryRegistry.map[sql] = queryName;
        client.query({ text: sql, values: params, name: queryName }, cb);
    }
} 