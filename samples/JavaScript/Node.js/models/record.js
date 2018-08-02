var helper = requireLib('helper.js');

/**
 * Module exports the factory
 * @returns {Object} Record object
 */
module.exports = function () {
    return new Record();
};


/**
 * Creates a Work record object. This object is a unit of
 * done by the users.
 * @class
 */
function Record() {
    // The work record's identifier
    this._id = 0;
    // Automatically generated datetime value ofrecord's creation
    this._dt_created = null;
    // Who did the work (user identifier)
    this._user_id = 0;
    // When the work was being done
    this._dt_day = null;
    // For how long it was being done (sec)
    this._duration_sec = 0;
    // The description of work
    this._note = null;

    /*---------- The calculated fields for fetched records --------------*/

    // If the work was under the user's hours
    this._is_under_hours = 0;

    // The username fetched from DB's view
    this._user_name = null;
}

/**
 * Fill all the fields from the given Record object
 * @param {Object} copy The Record object to copy
 */
Record.prototype.copy = function (obj) {
    if (obj === null || typeof obj !== 'object') {
        return;
    }
    if (typeof (obj._dt_created) !== 'undefined') {
        this._dt_created = obj._dt_created;
    }
    if (typeof (obj._user_id) !== 'undefined') {
        this._user_id = obj._user_id;
    }
    if (typeof (obj._dt_day) !== 'undefined') {
        this._dt_day = obj._dt_day;
    }
    if (typeof (obj._duration_sec) !== 'undefined') {
        this._duration_sec = obj._duration_sec;
    }
    if (typeof (obj._note) !== 'undefined') {
        this._note = obj._note;
    }

    // ----------------------------------------------------------

    if (typeof (obj._is_under_hours) !== 'undefined') {
        this._is_under_hours = obj._is_under_hours;
    }
    if (typeof (obj._user_name) !== 'undefined') {
        this._user_name = obj._user_name;
    }
}

/**
 * Get the record's internal Id.
 * @returns {Number} The record's id
 */
Record.prototype.getId = function () {
    return this._id;
}

/**
 * Validates the Id and assigns it to record
 * Throws an error if the id has wrong format
 * @param {Number} id - The new id value
 */
Record.prototype.setId = function (id) {

    if (!id) {
        throw { name: 'no_id', message: 'The id is missing' };
    }

    if (!helper.isInt(id)) {
        throw { name: 'bad_id', message: 'The id has incorrect format' };
    }

    this._id = id;
}

/**
 * Get the record's creation time.
 * @returns {Date} The record's creation time in the system
 */
Record.prototype.getCreated = function () {
    return this._dt_created;
}

/**
 * Get the record's creation time in the DB format.
 * @returns {Date} The record's creation time in the DB format.
 */
Record.prototype.getCreatedDB = function () {
    return helper.dateToPostgres(this._dt_created);
}

/**
 * Get the record's creator user Id.
 * @returns {Number} The record's user id
 */
Record.prototype.getUserId = function () {
    return this._user_id;
}

/**
 * Validates the user Id format and assigns it to record
 * Throws an error if the user id has wrong format
 * @param {Number} userId - The record's user id value
 */
Record.prototype.setUserId = function (userId) {

    if (!userId) {
        throw { name: 'no_userid', message: 'The user id is missing' };
    }

    if (!helper.isInt(userId)) {
        throw { name: 'bad_userid', message: 'The user id has incorrect format' };
    }

    this._user_id = userId;
}

/**
 * Get the record's duration in seconds
 * @returns {Number} The record work duration
 */
Record.prototype.getDuration = function () {
    return this._duration_sec;
}

/**
 * Validates the duration of work and assigns it to _duration_sec
 * Throws an error if the duration has wrong format
 * @param {Number} duration - The duration of work
 */
Record.prototype.setDuration = function (duration) {

    if (!duration) {
        throw { name: 'no_duration', message: 'The duration is missing' };
    }

    if (!helper.isInt(duration)) {
        throw { name: 'bad_duration', message: 'The duration has incorrect format' };
    }

    if (duration <= 0) {
        throw { name: 'bad_duration', message: 'The duration has incorrect format' };
    }

    if (duration > 24*60*60) {
        throw { name: 'bad_duration', message: 'The duration has incorrect format' };
    }

    this._duration_sec = duration;
}

/**
 * Get the note identified with the record.
 * @returns {String} The description
 */
Record.prototype.getNote = function () {

    return this._note;
}

/**
 * Validates the note not to be empty and assigns it to the _note field.
 * Throws an error if the note has wrong format
 * @param {String} note - The note with the recor's description
 */
Record.prototype.setNote = function (note) {

    if (!note) {
        throw { name: 'no_note', message: 'The note is missing' };
    }

    this._note = note;
}

/**
 * When the work was done (day)
 * @returns {Date} The work time
 */
Record.prototype.getDay = function () {
    return this._dt_day;
}

/**
 * When the work was done (day) in the DB format
 * @returns {Date} The work time in the DB format
 */
Record.prototype.getDayDB = function () {
    if (this._dt_day) {
        return helper.dateToPostgres(this._dt_day);
    }
    else
        return null;
}

/**
 * Assigns the day when the work was done
 * The passed value is validated to be a valid date object
 * @returns {Date} day - The day when the work was done
 */
Record.prototype.setDay = function (day) {

    if (!day) {
        throw { name: 'no_date', message: 'The date is missing' };
    }

    if (!helper.isDate(day)) {
        throw { name: 'bad_date', message: 'The day has incorrect format' };
    }
    this._dt_day = day;
}

/**
 * Returns the shorter version of record's data, without the
 * fields with underscores
 * @returns {Object} The public presentation of records's data
 */
Record.prototype.toJson = function () {

    return {
        id: this._id,
        created: this._dt_created,
        user_id: this._user_id,
        when: this._dt_day,
        duration: this._duration_sec,
        note: this._note,

        // The below fields are used only when records are fetched, otherwise they're ignored
        is_under_hours: this._is_under_hours,
        user_name: this._user_name
    };
}

