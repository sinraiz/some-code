var async = require('async');
var bcrypt = require('bcryptjs');
var emailValidator = require('email-validator');
var helper = requireLib('helper.js');

/**
 * Module exports the factory
 * @returns {Object} User object
 */
module.exports = function () {
    return new User();
};


/**
 * Creates a User object
 * @class
 */
function User() {

    // The user identifier
    this._id = 0;
    // User's email address, used as login
    this._email = null;
    // User's full name
    this._full_name = null;
    // The hash value of the user's password
    this._pwd_hash = null;
    // The current role id
    this._role = 0;
    // The preferred number of working hours
    this._working_hours = 0;
}

/**
 * Fill all the fields from the given user object
 * @param {Object} copy The User object to copy
 */
User.prototype.copy = function (obj) {
    if (obj === null || typeof obj !== 'object') {
        return;
    }
    if (typeof (obj._id) !== 'undefined') {
        this._id = obj._id;
    }
    if (typeof (obj._email) !== 'undefined') {
        this._email = obj._email;
    }
    if (typeof (obj._full_name) !== 'undefined') {
        this._full_name = obj._full_name;
    }
    if (typeof (obj._pwd_hash) !== 'undefined') {
        this._pwd_hash = obj._pwd_hash;
    }
    if (typeof (obj._role) !== 'undefined') {
        this._role = obj._role;
    }
    if (typeof (obj._working_hours) !== 'undefined') {
        this._working_hours = obj._working_hours;
    }
}


/**
 * Get the user's internal Id.
 * @returns {Number} The user's id
 */
User.prototype.getId = function () {
    return this._id;
}

/**
 * Validates the Id and assigns it to user
 * Throws an error if the id has wrong format
 * @param {Number} id - The new id value
 */
User.prototype.setId = function (id) {

    if (!id) {
        throw { name: 'no_id', message: 'The id is missing' };
    }

    if (!helper.isInt(id)) {
        throw { name: 'bad_id', message: 'The id has incorrect format' };
    }

    this._id = id;
}

/**
 * Get the user's name.
 * @returns {String} The user's name
 */
User.prototype.getName = function () {
    return this._full_name;
}

/**
 * Validates the name and assigns it to the _full_name field.
 * Throws an error if the name has wrong format
 * @param {String} full_name - The new user name
 */
User.prototype.setName = function (full_name) {

    if (!full_name) {
        throw { name: 'no_name', message: 'The name is missing' };
    }

    this._full_name = full_name;
}

/**
 * Get the user's email.
 * @returns {String} The email address (lowercase)
 */
User.prototype.getEmail = function () {
    if (this._email) {
        return this._email.toLowerCase();
    }
    return null;
}

/**
 * Validates the email and assigns it to the _email field.
 * Throws an error if the email has wrong format
 * @param {String} email - The email address
 */
User.prototype.setEmail = function (email) {
    
    if (!email) {
        throw { name: 'no_email', message: 'The email is missing' };
    }

    if (!helper.isEmail(email)) {
        throw { name: 'bad_email', message: 'The email is incorrect' };
    }

    this._email = email;
}

/**
 * Get the user's role as Enum.
 * @returns {Enum} The current role
 */
User.prototype.getRole = function () {

    var role = helper.enRoles.getByValue('value', this._role);
    return role;
}

/**
 * Validates the role and assigns its value to the _role field.
 * Throws an error if the email has wrong format
 * @param {Enum} role - The role to assign
 */
User.prototype.setRole = function (role) {

    var roleEnum = helper.enRoles.getByValue('value', role.value);
    if (!roleEnum) {
        throw { name: 'bad_role', message: 'The role is incorrect' };
    }
    this._role = roleEnum.value;
}

/**
 * Get the user's role as integer.
 * @returns {Number} The current role
 */
User.prototype.getRoleId = function () {
    
    return this._role;
}

/**
 * Validates the role id and assigns it to the _role field.
 * Throws an error if the email has wrong format
 * @param {Enum} role - The role to assign
 */
User.prototype.setRoleId = function (role) {

    // Try to find the enum value by id
    var roleEnum = helper.enRoles.getByValue('value', role);
    if (!roleEnum) {
        throw { name: 'bad_role', message: 'The role is incorrect' };
    }
    this._role = roleEnum.value;
}
/**
 * Get the user's preferred working hours
 * @returns {Number} The user's working hours
 */
User.prototype.getWorkingHours = function () {
    return this._working_hours;
}

/**
 * Validates the number of working hours and assigns it to user
 * Throws an error if the hours have wrong format
 * @param {Number} id - The new id value
 */
User.prototype.setWorkingHours = function (working_hours) {

    if (!working_hours) {
        this._working_hours = 0; // Will work as null
        return;
    }

    if (!helper.isFloat(working_hours)) {
        throw { name: 'bad_working_hours', message: 'The working hours have incorrect format' };
    }

    if (working_hours < 0) {
        throw { name: 'bad_working_hours', message: 'The working hours has incorrect format' };
    }

    if (working_hours >= 24) {
        throw { name: 'bad_working_hours', message: 'The working hours has incorrect format' };
    }

    this._working_hours = working_hours;
}

/**
 * Get the user's password hash.
 * @returns {String} The current password's hash (BCrypt)
 */
User.prototype.getPasswordHash = function () {
    return this._pwd_hash;
}

/**
 * Assigns the password hash to the _pwd_hash field.
 * @param {String} pwd_hash - The BCrypt hash of the password
 */
User.prototype.setPasswordHash = function (pwd_hash) {
    if (!pwd_hash) {
        throw { name: 'no_password', message: 'Password hash is empty' };
    }

    this._pwd_hash = pwd_hash;
}

/**
 * Generates the hash from a given password and
 * assigns it to the _pwd_hash field.
 * Throws an error if the password has wrong format
 * @param {String} password - Plain text password
 */
User.prototype.setPassword = function (password)
{
    if (!password) {
        throw { name: 'no_password', message: 'Password is empty' };
    }

    if (!helper.isPasswordGood(password)) {
        throw { name: 'bad_password', message: 'Password has wrong format' };
    }

    // Generate the salt for BCrypt
    var passwordSalt = bcrypt.genSaltSync(2);

    // Hash the password with salt and assign it
    this._pwd_hash = bcrypt.hashSync(password, passwordSalt);
    
}

/**
 * Returns the shorter version of user's data, without the
 * password hash and fields with no underscores/
 * @returns {Object} The public presentation of user's data
 */
User.prototype.toJson = function () {

    return {
        id: this._id,
        email: this._email,
        name: this._full_name,
        working_hours: this._working_hours,
        role: this._role
    };
}

/**
 * Checks if the given password matches the user's
 * @param {String} password - The password to check
 * @param {Function} cb - (err, isMatch) Callback triggered upon the completion. 
 *                        isMatch identifies if the password matched
 */
User.prototype.checkPassword = function (password, cb) {

    if (!password) {
        return cb({ name: 'no_password', message: 'Password is empty' }, false);
    }

    if (!helper.isPasswordGood(password)) {
        return cb({ name: 'bad_password', message: 'Password has wrong format' }, false);
    }

    // the bcrypt will validate the password and the callback
    return bcrypt.compare(password, this._pwd_hash, cb);
}