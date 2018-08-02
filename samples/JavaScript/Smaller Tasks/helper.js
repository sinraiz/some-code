/**
 * Helper
 */

var async = require('async');
var bcrypt = require('bcrypt');
var emailValidator = require('email-validator');
var Readable = require('stream').Readable;

/**
 * Module exports the helper instance
 */
module.exports = {
    /**
     * Parse the boolean parameter (from URL)
     * @param {String} value
     * @returns {Boolean|null}
     */
    parseBoolean: function (value) {
        if (value === 'true') return true;
        if (value === 'false') return false;
        return null;
    },

    /**
     * Parse the search query (from URL)
     * @param {String} value
     * @returns {Boolean|null}
     */
    parseSearchQuery: function (value) {
        if (!value) return null;
        return '%' + value.trim().replace(/ /g, '%') + '%';
    },

    /**
     * Validate the email
     * @param {String} value
     * @returns {Boolean}
     */
    isEmail: function (value) {
        return emailValidator.validate(value);
    },

    /**
     * Validate the domain
     * @param {String} value
     * @returns {Boolean}
     */
    isDomain: function (value) {
        var subdomain = '([a-z0-9]+([a-z0-9-]+[a-z0-9])?)';
        var re = new RegExp('^' + subdomain + '(\\.' + subdomain + ')+$');
        return re.test(value);
    },

    /**
     * Validate the subdomain
     * @param {String} value
     * @returns {Boolean}
     */
    isSubdomain: function (value) {
        var re = /^[a-z0-9]+([a-z0-9-]+[a-z0-9])?$/;
        return re.test(value);
    },

    /**
     * Validate the URL
     * @param {String} value
     * @returns {Boolean}
     */
    isURL: function (value) {
        var re = /^https?:\/\/\S+\.\S+$/;
        return re.test(value);
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
     * Check the media type
     * @param {String} mediaType
     * @returns {Boolean}
     */
    isImage: function (mediaType) {
        return ['jpg', 'png', 'gif'].indexOf(mediaType) !== -1;
    },

    /**
     * Check the color
     * @param {String} value
     * @returns {Boolean}
     */
    isColor: function (value) {
        var re = /^[A-F0-9]{6}$/i;
        return re.test(value);
    },

    /**
     * Get a new object with subset of properties
     * @param {Object} obj - initial object
     * @param {Array} props - allowed properties
     * @returns {Object} new object
     */
    filterProps: function (obj, props) {

        var newObj = {};

        for (var i = 0; i < props.length; i++) {
            var prop = props[i];
            if (typeof obj[prop] !== 'undefined') newObj[prop] = obj[prop];
        }

        return newObj;
    },

    /**
     * Fisher–Yates shuffle
     * http://stackoverflow.com/a/2450976
     *
     * @param {Array} arr
     */
    shuffle: function (arr) {

        var currentIndex = arr.length;

        // While there remain elements to shuffle:
        while (currentIndex > 0) {

            // Pick a remaining element ...
            var randomIndex = Math.floor(Math.random() * currentIndex);
            currentIndex--;

            // ... and swap it with the current element:
            var temporaryValue = arr[currentIndex];
            arr[currentIndex] = arr[randomIndex];
            arr[randomIndex] = temporaryValue;
        }
    },

    /**
     * Rearrange the array
     * @param {Array} items
     * @param {*} item
     * @param {Number} newPos
     */
    rearrange: function (items, item, newPos) {

        if (newPos < 0 || newPos >= items.length)
            return;

        var oldPos = items.indexOf(item);

        if (oldPos === -1 || oldPos === newPos)
            return;

        var step = (newPos > oldPos) ? 1 : -1;

        for (var i = oldPos; i != newPos; i += step)
            items[i] = items[i + step];

        items[newPos] = item;
    },

    /**
     * Make a new array
     * @param {*} value
     * @param {Number} count
     * @returns {Array}
     */
    makeArrayOf: function (value, count) {

        var arr = [];

        for (var i = 0; i < count; i++)
            arr.push(value);

        return arr;
    },

    /**
     * Generate a sequence of numbers
     * @param {Number} count
     * @param {Number} [start = 1]
     * @param {Number} [step = 1]
     * @returns {Array}
     */
    generateSequence: function (count, start, step) {

        if (typeof start === 'undefined') start = 1;
        if (typeof step === 'undefined') step = 1;

        var arr = [];
        var val = start;

        for (var i = 0; i < count; i++) {
            arr.push(val);
            val += step;
        }

        return arr;
    },

    /**
     * Check uniqueness of items in the array
     * @param {Array} items
     * @param {String} [key]
     * @returns {Boolean}
     */
    areElementsUnique: function (items, key) {

        var map = {};

        for (var i = 0; i < items.length; i++) {
            var item = items[i];
            var value = key ? item[key] : item;
            if (map[value]) return false;
            map[value] = true;
        }

        return true;
    },

    /**
     * Extract unique items from the array
     * @param {Array} items
     * @returns {Array}
     */
    getUniqueElements: function (items) {

        var map = {};
        var uniqueItems = [];

        for (var i = 0; i < items.length; i++) {
            var item = items[i];

            if (!map[item]) {
                map[item] = true;
                uniqueItems.push(item);
            }
        }

        return uniqueItems;
    },

    /**
     * Validate in parallel
     * @param {Array} tasks - array of function (acb)
     * @param {Function} cb - function (err, valres)
     */
    validateParallel: function (tasks, cb) {

        async.parallel(tasks, function (err, results) {
            if (err) return cb(err);

            for (var i = 0; i < results.length; i++) {
                if (results[i] !== true)
                    return cb(null, results[i]);
            }

            cb(null, true);
        });
    },

    /**
     * Escape HTML
     * https://github.com/component/escape-html
     *
     * @param {String} title
     * @returns {String}
     */
    escape: function (html) {
      return String(html)
        .replace(/&/g, '&amp;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
    },

    /**
     * Parse tags from an array of strings
     * @param {Array} tags
     * @returns {Array}
     */
    parseTags: function (tags) {

        if (!tags) return [];

        return tags.map(function (tag) {
            var parts = tag.split(':');
            return {
                id: Number(parts[0]),
                title: parts.slice(2).join(':'),
                color: parts[1]
            };
        });
    },

    /**
     * Parse YouTube URL
     * @param {String} url
     * @returns {String}
     */
    parseYoutubeUrl: function (url) {
        // Examples:
        // https://youtu.be/P2HqXR_-B3mNM
        // https://www.youtube.com/watch?v=P2HqXR_-B3mNM
        var re1 = /https?:\/\/youtu\.be\/([a-z0-9_-]+)/i;
        var re2 = /https?:\/\/www\.youtube\.com\/watch\?v=([a-z0-9_-]+)/i;
        var matches = re1.exec(url);
        if (matches) return matches[1];
        matches = re2.exec(url);
        return matches ? matches[1] : null;
    },

    /**
     * Get a hash to the password
     * @param {String} password
     * @param {Function} cb - function (err, passwordHash)
     */
    getPasswordHash: function (password, cb) {
        async.waterfall([
            function (acb) {
                bcrypt.genSalt(acb);
            },
            function (passwordSalt, acb) {
                bcrypt.hash(password, passwordSalt, acb);
            }
        ], cb);
    },

    /**
     * Convert something to string,
     * keeping nulls and replacing empty strings with nulls
     *
     * @param {*} value
     * @returns {String}
     */
    toString: function (value) {
        if (typeof value === 'undefined') return null;
        if (value === null) return null;
        if (value === '') return null;
        return value.toString();
    },

    /**
     * Convert something to number, keeping nulls
     * @param {*} value
     * @returns {Number}
     */
    toNumber: function (value) {
        if (typeof value === 'undefined') return null;
        if (value === null) return null;
        return Number(value);
    },

    /**
     * Create a Readable Stream from the string
     * @param {String} str
     * @returns {Object}
     */
    stringToStream: function (str) {
        // From here:
        // http://stackoverflow.com/a/22085851
        var stream = new Readable();
        stream._read = function () {};
        stream.push(str);
        stream.push(null);
        return stream;
    },

    /**
     * Create a clone of the object
     * @param {Object} obj
     * @returns {Object}
     */
    clone: function (obj) {
        return JSON.parse(JSON.stringify(obj));
    }
};
