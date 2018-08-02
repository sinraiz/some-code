/**
 * Background worker
 */

var async = require('async');

/**
 * Module exports the start function
 * @param {Object} config - configuration parameters
 * @param {Object} db - database driver instance
 * @param {Object} enums - enum map
 * @param {Object} billing - billing service
 * @param {Object} common - service with common actions
 */
module.exports = function (config, db, enums, billing, common) {

    var tick = function () {
        async.series([
            function (acb) {
                finishSessions(config, db, enums, common, acb);
            },
            function (acb) {
                expireSessions(db, enums, acb);
            },
            function (acb) {
                performMaintenanceFees(db, billing, acb);
            }
        ], function (err) {
            if (!err) return;
            console.log(new Date());
            console.log('Background Worker');
            console.log(err);
            console.log();
        });
    };

    setInterval(tick, config.system.backgroudWorkerInterval * 1000);
};

function finishSessions(config, db, enums, common, cb) {

    var dbc = null;

    async.waterfall([
        function (acb) {
            db.getConnection(acb);
        },
        function (connection, acb) {
            dbc = connection;

            var sql = ''
                + ' SELECT id'
                + ' FROM v_session'
                + ' WHERE status_id = ?'
                + '   AND ('
                + '     activity_exp_time < 0'
                + '     OR inactive_time > ? AND NOT activity_is_resumable'
                + '     OR inactive_time > ?'
                + '   )';

            var params = [
                enums.session_status.in_progress.id,
                config.system.sessionInactiveTime,
                config.system.sessionAutoFinishTime
            ];

            dbc.queryVector(sql, params, acb);
        },
        function (ids, acb) {
            async.eachSeries(ids, function (id, acbInner) {
                common.finishSession(dbc, id, true, acbInner);
            }, acb);
        },
        function (acb) {
            dbc.commit(acb);
            dbc = null;
        },
        function () {
            cb();
        }
    ], function (err) {
        if (dbc) dbc.rollback();
        cb(err);
    });
}

function expireSessions(db, enums, cb) {

    var sql = ''
        + ' UPDATE session'
        + ' SET status_id = ?'
        + ' WHERE status_id = ? AND exp_on < now()';

    var params = [
        enums.session_status.expired.id,
        enums.session_status.pending.id
    ];

    db.execute(sql, params, cb);
}

function performMaintenanceFees(db, billing, cb) {

    var dbc = null;

    async.waterfall([
        function (acb) {
            db.getConnection(acb);
        },
        function (connection, acb) {
            dbc = connection;

            var sql = ''
                + ' SELECT id'
                + ' FROM v_company'
                + ' WHERE credits > 0 AND closing_time < 0';

            dbc.queryVector(sql, acb);
        },
        function (ids, acb) {
            async.eachSeries(ids, function (id, acbInner) {
                billing.performMaintenanceFee(dbc, id, acbInner);
            }, acb);
        },
        function (acb) {
            dbc.commit(acb);
            dbc = null;
        },
        function () {
            cb();
        }
    ], function (err) {
        if (dbc) dbc.rollback();
        cb(err);
    });
}
