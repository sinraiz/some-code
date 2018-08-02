/**
 * Helper module contains a collection of
 * smaller routines and utilities used by 
 * other modules.
 */

var async = require('async');
var bcrypt = require('bcryptjs');
var emailValidator = require('email-validator');
var enums = requireLib('enum.js');

/**
 * Module exports the helper instance
 */
module.exports = {
   
    /**
     * Validate the email
     * @param {String} value
     * @returns {Boolean}
     */
    isEmail: function (value) {
        return emailValidator.validate(value);
    },
    
    /**
     * Validate the password
     * @param {String} value
     * @returns {Boolean}
     */
    isPasswordGood: function (value) {
        return value.length >= 6;
    },
    
    /**
     * Validate the given param is an integer
     * @param {String} value
     * @returns {Boolean}
     */
    isInt: function (n) {
        return !isNaN(n) &&
            parseInt(Number(n)) == n &&
            !isNaN(parseInt(n, 10));
    },

    isFloat: function isFloat(val) {
        var floatRegex = /^-?\d+(?:[.,]\d*?)?$/;
        if (!floatRegex.test(val))
            return false;

        val = parseFloat(val);
        if (isNaN(val))
            return false;
        return true;
    },

    /**
     * Validate the given param is a valid date object
     * @param {Date} value
     * @returns {Boolean}
     */
    isDate: function (value) {
        var dateWrapper = new Date(value);
        return !isNaN(dateWrapper.getDate());
    },
    
    /**
     * Converts the given JS date to string compatible with PostgreSQL
     * @param {Date} value
     * @returns {String} The date in PostgreSQL format
     */
    dateToPostgres: function (date) {
            var a = new Date(date);
            // In DB we need to store the day as it was given by user and 
            // we won't convert it to the viewer's timezone on client
            var year = a.getFullYear();
            var month = a.getMonth() + 1;
            var date = a.getDate();
            var hour = a.getHours();
            var min = a.getMinutes();
            var sec = a.getSeconds();
            var time = year + '-' + month + '-' + date;
            return time;
    },

    /**
     * Converts the given JS date and time to string compatible with PostgreSQL
     * @param {Date} value
     * @returns {String} The date and time string in PostgreSQL format
     */
    dateTimeToPostgres: function (date) {
        var a = new Date(date);
        // In DB we need to store the UTC and convert it to user's timezone on client
        var year = a.getUTCFullYear();
        var month = a.getUTCMonth() + 1;
        var date = a.getUTCDate();
        var hour = a.getUTCHours();
        var min = a.getUTCMinutes();
        var sec = a.getUTCSeconds();
        var time = year + '-' + month + '-' + date + ' ' + hour + ':' + min + ':' + sec;
        return time;
    },

    timestampToString: function (timestamp) {
        var d = new Date(timestamp * 1000),	// Convert the passed timestamp to milliseconds
            yyyy = d.getFullYear(),
            mm = ('0' + (d.getMonth() + 1)).slice(-2),	// Months are zero based. Add leading 0.
            dd = ('0' + d.getDate()).slice(-2),			// Add leading 0.
            hh = d.getHours(),
            h = hh,
            min = ('0' + d.getMinutes()).slice(-2),		// Add leading 0.
            ampm = 'AM',
            time;

        if (hh > 12) {
            h = hh - 12;
            ampm = 'PM';
        } else if (hh === 12) {
            h = 12;
            ampm = 'PM';
        } else if (hh == 0) {
            h = 12;
        }

        // ie: 2013-02-18, 8:35 AM	
        time = yyyy + '-' + mm + '-' + dd + ', ' + h + ':' + min + ' ' + ampm;

        return time;
    },
  
    /**
     * Define the enum with the available user roles
     * @returns {Enum}
     */
    enRoles: enums.defineEnum({
            USER : {
                value : 1,
                string : 'Regular User'
            },
            MANAGER : {
                value : 2,
                string : 'User Manager'
            },
            ADMIN : {
                value : 3,
                string : 'Administrator'
            }
        }),
};
