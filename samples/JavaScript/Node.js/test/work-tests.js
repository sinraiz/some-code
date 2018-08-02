//Require the dev-dependencies
var chai = require('chai');
var chaiHttp = require('chai-http');
var should = chai.should();
var moment = require('moment');

// Inner app models
// Register requireLib() in the GLOBAL scope:
GLOBAL.requireLib = function (name) {
    return require('../lib/' + name);
};
var config = require('../config.js');
var db = require('../lib/db.js')(config.db.connectionString, config.db.poolSize);
var helper = require('../lib/helper.js')
var Users = require("../models/users.js");
var User = require("../models/user.js");
var Work = require("../models/record.js");
var Works = require("../models/records.js");

chai.use(chaiHttp);

var url = 'http://localhost:3000';

describe('Work Records', function () {

    this.timeout(15000);
    var adminAuthToken = null;
    var adminUserId = 0;

    // Clean up before we start
    before(function (done) {

        console.log('    Obtaining an auth token...');
        var authRequest = {
            email: 'admin@domain.com',
            password: 'password'
        };

        chai.request(url)
            .post('/v1/auth/signin')
            .send(authRequest)
            .end(function (err, res) {

                res.should.have.status(200);
                res.body.should.be.a('object');

                res.body.should.have.property('token');
                adminAuthToken = res.body.token;

                res.body.should.have.property('user');
                res.body.user.should.have.property('id');
                adminUserId = res.body.user.id;

                console.log('    Received admin token');


                var works = new Works(db);
                works._deleteAll(function (err) {
                    if (err) {
                        done(err);
                    }
                    else {
                        console.log('    Cleared records');

                        // Remove all users (leave only the system admin)
                        var users = new Users(db);
                        users._deleteAllButAdmin(function (err) {
                            if (err) {
                                done(err);
                            }
                            else {
                                console.log('    Cleared users');
                                done();
                            }
                        });
                    }
                });
            });
    });

    describe('Add Record', function () {

        it('should fail without auth token', function (done) {
            var addRecordRequest = {
                user_id: 1,
                when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail without duration', function (done) {
            var addRecordRequest = {
                user_id: 1,
                when: "2016-08-24 01:40:12",
                missing_duration: 7200,
                note: "Did something cool"
            };
             
            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_duration');

                    done();
                });
        });

        it('should fail without day specified', function (done) {
            var addRecordRequest = {
                user_id: 1,
                missing_when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_date');

                    done();
                });
        });

        it('should fail with incorrect day specified', function (done) {
            var addRecordRequest = {
                user_id: 1,
                when: "2016-08-34 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('bad_date');

                    done();
                });
        });

        it('should succeed without user ID', function (done) {
            var addRecordRequest = {
                missing_user_id: 1,
                when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(200);

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {

            // A regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("user_add_work@domain.com");
            user.setPassword("password");
            user.setWorkingHours(8);
            user.setRoleId(1);

            // Create a dummy user through model
            var users = new Users(db);
            users.add(user, function (err, usr) {
                should.not.exist(err);
                usr.should.be.a('object');

                // Try to login with it
                var authRequest = {
                    email: 'user_add_work@domain.com',
                    password: 'password'
                };

                chai.request(url)
                    .post('/v1/auth/signin')
                    .send(authRequest)
                    .end(function (err, res) {

                        res.should.have.status(200);
                        res.body.should.be.a('object');

                        res.body.should.have.property('token');
                        var userAuthToken = res.body.token;

                        // Try to add work for admin
                        var addRecordRequest = {
                            user_id: 1,
                            when: "2016-08-24 01:40:12",
                            duration: 7200,
                            note: "Work for admin"
                        };

                        chai.request(url)
                            .post('/v1/work')
                            .send(addRecordRequest)
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403);

                                done();
                            });
                    });
            });
        });

        it('should succeed to create for another user', function (done) {

            // A regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("user_add_work2@domain.com");
            user.setPassword("password");
            user.setWorkingHours(8);
            user.setRoleId(1);

            // Create a dummy user through model
            var users = new Users(db);
            users.add(user, function (err, usr) {
                should.not.exist(err);
                usr.should.be.a('object');

                // Get the new user id
                var userId = usr.getId(); 
               

                // Try to add work for admin
                var addRecordRequest = {
                    user_id: userId,
                    when: "2016-08-24 01:40:12",
                    duration: 7200,
                    note: "Work for another user"
                };

                chai.request(url)
                    .post('/v1/work')
                    .send(addRecordRequest)
                    .set('Authorization', 'Bearer ' + adminAuthToken)
                    .end(function (err, res) {

                        res.should.have.status(200);
                        res.body.should.be.a('object');

                        res.body.should.have.property('id');
                        res.body.id.should.be.a('number');

                        res.body.should.have.property('user_id');
                        res.body.user_id.should.be.a('number');
                        res.body.user_id.should.be.eql(userId);

                        res.body.should.have.property('duration');
                        res.body.duration.should.be.a('number');
                        res.body.duration.should.be.eql(7200);

                        res.body.should.have.property('note');
                        res.body.note.should.be.a('string');
                        res.body.note.should.be.eql("Work for another user");

                        res.body.should.have.property('when');
                        res.body.when.should.be.a('number');

                        // Switch to UTC
                        moment().utc();

                        // Parse as integer
                        var when = res.body.when/1000;
                        var recDate = moment.unix(when);
                        var recDateStr = recDate.format();

                        var origDate = moment("2016-08-24 01:40:12");
                        var origDateStr = origDate.format();


                        origDateStr.should.be.eql(recDateStr);

                        done();
                    });
            });
        });
    });

    describe('Update Record', function () {

        it('should fail without auth token', function (done) {
            var updateRecordRequest = {
                user_id: 1,
                when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .put('/v1/work/' + 0)
                .send(updateRecordRequest)
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail for invalid record', function (done) {
            var updateRecordRequest = {
                user_id: 1,
                when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .put('/v1/work/' + 0)
                .send(updateRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('rec_not_found');

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {

            // Add work for admin
            var record = new Work();
            record.setUserId(1);
            record.setDay(Date.now());
            record.setDuration(1);
            record.setNote("add");

            var records = new Works(db);
            records.add(record, function (err, rec) {

                should.not.exist(err);
                var recId = rec.getId();


                // A regular user
                var user = new User();
                user.setName("User name");
                user.setEmail("user_add_work3@domain.com");
                user.setPassword("password");
                user.setWorkingHours(8);
                user.setRoleId(1);

                // Create a dummy user through model
                var users = new Users(db);
                users.add(user, function (err, usr) {
                    should.not.exist(err);
                    usr.should.be.a('object');

                    // Try to login with it
                    var authRequest = {
                        email: 'user_add_work3@domain.com',
                        password: 'password'
                    };



                    chai.request(url)
                        .post('/v1/auth/signin')
                        .send(authRequest)
                        .end(function (err, res) {

                            res.should.have.status(200);
                            res.body.should.be.a('object');

                            res.body.should.have.property('token');
                            var userAuthToken = res.body.token;

                            // Try to update work for admin
                            var updateRecordRequest = {
                                user_id: 1,
                                when: "2016-08-24 01:40:12",
                                duration: 7200,
                                note: "Did something cool"
                            };

                            chai.request(url)
                                .put('/v1/work/' + recId)
                                .send(updateRecordRequest)
                                .set('Authorization', 'Bearer ' + userAuthToken)
                                .end(function (err, res) {

                                    res.should.have.status(403);

                                    done();
                                });
                        });
                });
            });

        });

        it('should succeed', function (done) {
            var addRecordRequest = {
                missing_user_id: 1,
                when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(200);

                    res.body.should.have.property('id');
                    res.body.id.should.be.a('number');

                    var recId = res.body.id;

                    // Now modify it
                    var updateRecordRequest = {
                        user_id: 1,
                        when: "2015-08-24 01:40:12",
                        duration: 200,
                        note: "cool"
                    };

                    chai.request(url)
                        .put('/v1/work/' + recId)
                        .send(updateRecordRequest)
                        .set('Authorization', 'Bearer ' + adminAuthToken)
                        .end(function (err, res) {

                            res.should.have.status(200);

                            res.body.should.have.property('id');
                            res.body.id.should.be.a('number');
                            res.body.id.should.be.eql(recId);


                            res.body.should.have.property('user_id');
                            res.body.user_id.should.be.a('number');
                            res.body.user_id.should.be.eql(1);

                            res.body.should.have.property('duration');
                            res.body.duration.should.be.a('number');
                            res.body.duration.should.be.eql(200);

                            res.body.should.have.property('note');
                            res.body.note.should.be.a('string');
                            res.body.note.should.be.eql("cool");

                            done();
                        });
                });
        });
    });

    describe('Delete Record', function () {
        it('should fail without auth token', function (done) {

            chai.request(url)
                .delete('/v1/work/' + 0)
                .send()
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail for invalid record', function (done) {


            chai.request(url)
                .delete('/v1/work/' + 0)
                .send()
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('rec_not_found');

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {

            // Add work for admin
            var record = new Work();
            record.setUserId(1);
            record.setDay(Date.now());
            record.setDuration(1);
            record.setNote("add");

            var records = new Works(db);
            records.add(record, function (err, rec) {

                should.not.exist(err);
                var recId = rec.getId();


                // A regular user
                var user = new User();
                user.setName("User name");
                user.setEmail("user_add_work4@domain.com");
                user.setPassword("password");
                user.setWorkingHours(8);
                user.setRoleId(1);

                // Create a dummy user through model
                var users = new Users(db);
                users.add(user, function (err, usr) {
                    should.not.exist(err);
                    usr.should.be.a('object');

                    // Try to login with it
                    var authRequest = {
                        email: 'user_add_work4@domain.com',
                        password: 'password'
                    };



                    chai.request(url)
                        .post('/v1/auth/signin')
                        .send(authRequest)
                        .end(function (err, res) {

                            res.should.have.status(200);
                            res.body.should.be.a('object');

                            res.body.should.have.property('token');
                            var userAuthToken = res.body.token;

                            chai.request(url)
                                .delete('/v1/work/' + recId)
                                .send()
                                .set('Authorization', 'Bearer ' + userAuthToken)
                                .end(function (err, res) {

                                    res.should.have.status(403);

                                    done();
                                });
                        });
                });
            });

        });
        
        it('should succeed', function (done) {
            var addRecordRequest = {
                missing_user_id: 1,
                when: "2016-08-24 01:40:12",
                duration: 7200,
                note: "Did something cool"
            };

            chai.request(url)
                .post('/v1/work')
                .send(addRecordRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(200);

                    res.body.should.have.property('id');
                    res.body.id.should.be.a('number');

                    var recId = res.body.id;
                    
                    chai.request(url)
                        .delete('/v1/work/' + recId)
                        .send()
                        .set('Authorization', 'Bearer ' + adminAuthToken)
                        .end(function (err, res) {

                            res.should.have.status(200);

                            var works = new Works(db);
                            works.get(recId, function (err, rec) {
                                should.not.exist(err);
                                should.not.exist(rec);
                                done();
                            });

                        });
                });
        });
    });
});