/**
 * The Enum class.
 * Can be handy to easily to declare enums and access their
 * values and keys
 *
 * Define the enums example:
 *
 * var enSHAPES = defineEnum({
 *     CIRCLE : {
 *         value : 1,
 *         string : 'Circle'
 *     },
 *     SQUARE : {
 *         value : 2,
 *         string : 'Square'
 *     }
 * });
 *
 * Use the above enum as follows:
 *   enSHAPES.CIRCLE.string
 *   enSHAPES.SQUARE.value
 *   enSHAPES.getByName('SQUARE').string
 *   enSHAPES.getByValue('value', 1).string
 */

/**
 * Module exports the enum definition proc
 */
module.exports = {

   /**
    * Define a new enum
    * @returns {Object} Enum object instance
    */
    defineEnum: function(definition) {
        var k;
        var e = new Enum();
        for (k in definition) {
            var j = definition[k];
            e[k] = j;
            e.addEnum(j)
        }
        return e;
    }
};

function Enum() {

    this._enums = [];
    this._lookups = {};
}



Enum.prototype.getEnums = function () {

    return _enums;
}

Enum.prototype.forEach = function (callback) {

    var length = this._enums.length;
    for (var i = 0; i < length; ++i) {
        callback(this._enums[i]);
    }
}

Enum.prototype.addEnum = function (e) {

    this._enums.push(e);
}

Enum.prototype.getByName = function (name) {

    return this[name];
}

Enum.prototype.getByValue = function (field, value) {

    var lookup = this._lookups[field];
    if (lookup) {
        return lookup[value];
    } else {
        this._lookups[field] = (lookup = {});
        var k = this._enums.length - 1;
        for (; k >= 0; --k) {
            var m = this._enums[k];
            var j = m[field];
            lookup[j] = m;
            //if (j == value) {
            //    return m;
            //}
        }
        return lookup[value];
    }
    return null;
}