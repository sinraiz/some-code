/**
 * The photos - controller for API.v1
 */

var async = require('async');
var express = require('express');
var helper = requireLib('helper.js');

/**
 * Module exports the factory
 * @param {Object} routeContext
 * @returns {Object} Express router
 */
module.exports = function (routeContext) {

    // Extract objects from the route context:
    var db = routeContext.db;
    var enums = routeContext.enums;
    var media = routeContext.media;
    var common = routeContext.common;

    // Create Express router:
    var router = express.Router({mergeParams: true});

    // Middleware for checking photos:
    router.use('/:id', function (req, res, next) {

        var sql = ''
            + ' SELECT ca.role_id'
            + ' FROM company_photo p'
            + ' LEFT JOIN v_company_account_active ca ON ca.company_id = p.company_id AND ca.account_id = ?'
            + ' WHERE p.id = ?';

        db.queryRow(sql, [req.user.id, req.params.id], function (err, row) {
            if (err) return next(err);
            if (!row) return res.sendStatus(404);
            if (!row.role_id) return res.sendStatus(403);

            req.aux = {
                role: enums.company_account_role[row.role_id].title
            };

            next();
        });
    });

    // Get the photo:
    router.get('/:id', function (req, res, next) {

        var sql = ''
            + ' SELECT'
            + '   id,'
            + '   title,'
            + '   media_bucket,'
            + '   media_object'
            + ' FROM v_company_photo'
            + ' WHERE id = ?';

        db.queryRow(sql, [req.params.id], function (err, row) {
            if (err) return next(err);
            if (!row) return res.sendStatus(404);

            res.send({
                id: row.id,
                title: row.title,
                media_key: media.key(row.media_bucket, row.media_object)
            });
        });
    });

    // Update the photo:
    router.put('/:id', function (req, res, next) {

        if (req.aux.role !== 'admin') {
            return res.sendStatus(403);
        }

        async.waterfall([
            function (acb) {
                common.validatePhoto(req.body, acb);
            },
            function (valres, acb) {
                if (valres !== true) return res.status(422).send(valres);
                var update = helper.filterProps(req.body, ['title', 'media_id']);
                db.update('company_photo', req.params.id, update, acb);
            },
            function () {
                res.sendStatus(200);
            }
        ], next);
    });

    // Delete the photo:
    router.delete('/:id', function (req, res, next) {

        if (req.aux.role !== 'admin') {
            return res.sendStatus(403);
        }

        db.delete('company_photo', req.params.id, function (err) {
            if (err) return next(err);
            res.sendStatus(200);
        });
    });

    // Return Express router:
    return router;
};
