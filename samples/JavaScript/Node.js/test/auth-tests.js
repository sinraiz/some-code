//Require the dev-dependencies
var chai = require('chai');
var chaiHttp = require('chai-http');
var should = chai.should();

// Inner app models
// Register requireLib() in the GLOBAL scope:
GLOBAL.requireLib = function (name) {
    return require('../lib/' + name);
};
var config = require('../config.js');
var db = require('../lib/db.js')(config.db.connectionString, config.db.poolSize); 
var Users = require("../models/users.js");
var Work = require("../models/record.js");
var Works = require("../models/records.js");

chai.use(chaiHttp);

var url = 'http://localhost:3000';

describe('Auth', function () {

    this.timeout(15000);

    // Clean up before we start
    before(function (done) {

        console.log('    Clearing users...');

        // Remove all users (leave only the system admin)
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

    describe('Sign Up', function () {
        it('should fail to sign up without email', function (done) {
            var authRequest = {
                username: 'admin@domain.com',
                name: 'Admin Duplicate',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_email');

                    done();
                });
        });

        it('should fail to sign up without name', function (done) {
            var authRequest = {
                email: 'admin@domain.com',
                missingname: 'Admin Duplicate',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_name');

                    done();
                });
        });

        it('should fail to sign up without password', function (done) {
            var authRequest = {
                email: 'admin@domain.com',
                name: 'Admin Duplicate',
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_password');

                    done();
                });
        });

        it('should fail to sign up with wrong email', function (done) {
            var authRequest = {
                email: 'admin-domain.com',
                name: 'Admin Duplicate',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('bad_email');

                    done();
                });
        });

        it('should fail to sign up with short password', function (done) {
            var authRequest = {
                email: 'admin@domain.com',
                name: 'Admin Duplicate',
                password: 'pass'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('bad_password');

                    done();
                });
        });

        it('should succeed to sign up', function (done) {
            var authRequest = {
                email: 'user@domain.com',
                name: 'Scott Tiger',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(200);
                    res.body.should.be.a('object');

                    res.body.should.have.property('token');
                        res.body.token.length.should.be.gt(50);

                    res.body.should.have.property('user');
                        res.body.user.should.have.property('id');
                        res.body.user.id.should.be.a('number');


                        res.body.user.should.have.property('email');
                        res.body.user.email.should.be.eql('user@domain.com');
                        res.body.user.should.have.property('name');
                        res.body.user.name.should.be.eql('Scott Tiger');
                        res.body.user.should.have.property('role');
                        res.body.user.role.should.be.a('number');
                        res.body.user.role.should.be.eql(1);


                    done();
                });
        });

        it('should fail to sign up with duplicate email', function (done) {
            var authRequest = {
                email: 'user@domain.com',
                name: 'Scott Tiger',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('email_in_use');

                    done();
                });
        });
    });

    describe('Sign In', function () {
        it('should fail to sign in without email', function (done) {
            var authRequest = {
                username: 'admin@domain.com',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signin')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_email');
                    done();
                });
        });

        it('should fail to sign in with unknown user', function (done) {
            var authRequest = {
                email: 'nobody@domain.com',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signin')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(401);
                    res.text.should.be.a('string');
                    res.text.should.equal('auth_error');
                    done();
                });
        });

        it('should fail to sign in without password', function (done) {
            var authRequest = {
                email: 'admin@domain.com',
            };

            chai.request(url)
                .post('/v1/auth/signin')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_password');
                    done();
                });
        });

        it('should fail to sign in with invalid credentials', function (done) {
            var authRequest = {
                email: 'admin@domain.com',
                password: 'invalid'
            };

            chai.request(url)
                .post('/v1/auth/signin')
                .send(authRequest)
                .end(function (err, res) {

                    res.should.have.status(401);
                    res.text.should.be.a('string');
                    res.text.should.equal('auth_error');
                    done();
                });
        });

        it('should succeed to sign in with valid credentials', function (done) {
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
                    res.body.token.length.should.be.gt(50);

                    res.body.should.have.property('user');
                    res.body.user.should.have.property('id');
                    res.body.user.id.should.be.a('number');


                    res.body.user.should.have.property('email');
                    res.body.user.email.should.be.eql('admin@domain.com');
                    res.body.user.should.have.property('role');
                    res.body.user.role.should.be.a('number');
                    res.body.user.role.should.be.eql(3);


                    done();
                });
        });
    });

    describe('Sign In By Token', function () {
        it('should fail to sign in without token', function (done) {

            var tokenRequest = { missing_token: "tokenvalue" };

            chai.request(url)
                .post('/v1/auth/signin-by-token')
                .send(tokenRequest)
                .end(function (err, res) {

                    res.should.have.status(401);
                    done();
                });
        });

        it('should fail to sign in with invalid token', function (done) {

            var tokenRequest = { token: "invalid tokenvalue" };

            chai.request(url)
                .post('/v1/auth/signin-by-token')
                .send(tokenRequest)
                .end(function (err, res) {

                    res.should.have.status(401);
                    done();
                });
        });        

        it('should succeed to sign in using the token', function (done) {
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
                    res.body.token.length.should.be.gt(50);

                    var tokenRequest = { token: res.body.token };

                    chai.request(url)
                        .post('/v1/auth/signin-by-token')
                        .send(tokenRequest)
                        .end(function (err, res) {

                            res.should.have.status(200);
                            res.body.should.be.a('object');

                            res.body.should.have.property('token');
                            res.body.token.length.should.be.gt(50);

                            var token = res.body.token;



                            done();
                        });
                });
        });
    });

    describe('Change own password', function () {

        var token = null;

        // Need to create an account first
        before(function (done) {
            var signupRequest = {
                email: 'user.test@domain.com',
                name: 'Scott Tiger',
                password: 'password'
            };

            chai.request(url)
                .post('/v1/auth/signup')
                .send(signupRequest)
                .end(function (err, res) {

                    res.should.have.status(200);
                    res.body.should.be.a('object');

                    res.body.should.have.property('token');
                    res.body.token.length.should.be.gt(50);

                    token = res.body.token;

                    done();
                });
        })
       
        it('should fail without a token', function (done) {

            var changePassRequest = { 
                password: "password",
                missing_token: token
            }

            chai.request(url)
                .put('/v1/auth/password')
                .send(changePassRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_token');
                    done();
                });
        });

        it('should fail without a new password', function (done) {

            var changePassRequest = {
                missing_password: "password",
                token: token
            }

            chai.request(url)
                .put('/v1/auth/password')
                .send(changePassRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_password');
                    done();
                });
        });

        it('should fail with invalid token', function (done) {

            var changePassRequest = {
                password: "password",
                token: "invalid token"
            }

            chai.request(url)
                .put('/v1/auth/password')
                .send(changePassRequest)
                .end(function (err, res) {

                    res.should.have.status(422);
                    done();
                });
        });

        it('should succeed with valid token', function (done) {
            var changePassRequest = {
                password: "new password",
                token: token
            };

            chai.request(url)
                .put('/v1/auth/password')
                .send(changePassRequest)
                .end(function (err, res) {

                    res.should.have.status(200);
                    res.body.should.be.a('object');

                    res.body.should.have.property('token');
                    res.body.token.length.should.be.gt(50);

                    // Now attempt to login using the new password
                    var authRequest = {
                        email: 'user.test@domain.com',
                        password: 'new password'
                    };

                    chai.request(url)
                        .post('/v1/auth/signin')
                        .send(authRequest)
                        .end(function (err, res) {

                            res.should.have.status(200);
                            res.body.should.be.a('object');

                            res.body.should.have.property('token');
                            res.body.token.length.should.be.gt(50);

                            res.body.should.have.property('user');
                            res.body.user.should.have.property('id');
                            res.body.user.id.should.be.a('number');


                            res.body.user.should.have.property('email');
                            res.body.user.email.should.be.eql('user.test@domain.com');
                            res.body.user.should.have.property('role');
                            res.body.user.role.should.be.a('number');
                            res.body.user.role.should.be.eql(1);


                            done();
                        });
                });
        }); 
    });
});