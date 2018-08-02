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
var User = require("../models/user.js");
var Work = require("../models/record.js");
var Works = require("../models/records.js");

chai.use(chaiHttp);

var url = 'http://localhost:3000';

describe('Users', function () {

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

    beforeEach(function (done) {        

        // Remove all users (leave only the system admin)
        var users = new Users(db);
        users._deleteAllButAdmin(function (err) {
            if (err) {
                done(err);
            }
            else {
                //console.log('    Cleared users');
                done();
            }
        });
    });

    describe('Add User', function () {

        it('should fail without auth token', function (done) {
            var addUserRequest = {
                email: "user@domain.com",
                name: "Steve Jobs",
                password: "change_me",
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail without email', function (done) {
            var addUserRequest = {
                //email: "user@domain.com",
                name: "Steve Jobs",
                password: "change_me",
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_email');

                    done();
                });
        });

        it('should fail with bad email', function (done) {
            var addUserRequest = {
                email: "user-domain.com",
                name: "Steve Jobs",
                password: "change_me",
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('bad_email');

                    done();
                });
        });

        it('should fail without name', function (done) {
            var addUserRequest = {
                email: "user_add@domain.com",
                //name: "Steve Jobs",
                password: "change_me",
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_name');
                    done();
                });
        });

        it('should fail without password', function (done) {
            var addUserRequest = {
                email: "user_add@domain.com",
                name: "Steve Jobs",
                //password: "change_me",
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('no_password');
                    done();
                });
        });

        it('should fail with weak password', function (done) {
            var addUserRequest = {
                email: "user_add_norm@domain.com",
                name: "Steve Jobs",
                password: "abc",
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('bad_password');
                    done();
                });
        });

        it('should fail with invalid role', function (done) {
            var addUserRequest = {
                email: "user_add_norm@domain.com",
                name: "Steve Jobs",
                password: "password",
                role: 666
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);
                    res.text.should.be.a('string');
                    res.text.should.equal('bad_role');
                    done();
                });
        });

        it('should add ok', function (done) {
            var addUserRequest = {
                email: "user_add_norm@domain.com",
                name: "Steve Jobs",
                password: "password",
                working_hours: 7,
                role: 1
            }

            chai.request(url)
                .post('/v1/users')
                .send(addUserRequest)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(200);
                    res.body.should.be.a('object');
                    
                    res.body.should.have.property('id');
                    res.body.id.should.be.a('number');
                    res.body.should.have.property('email');
                    res.body.email.should.be.eql('user_add_norm@domain.com');
                    res.body.should.have.property('name');
                    res.body.name.should.be.eql('Steve Jobs');
                    res.body.should.have.property('role');
                    res.body.role.should.be.a('number');
                    res.body.role.should.be.eql(1);
                    res.body.should.have.property('working_hours');
                    res.body.working_hours.should.be.a('number');
                    res.body.working_hours.should.be.eql(7);
                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {
            var addUserRequest = {
                email: "user_add_norm2@domain.com",
                name: "Steve Jobs",
                password: "password",
                role: 1
            }
            // A regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("regular@domain.com");
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
                    email: 'regular@domain.com',
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

                        // Try to add another user with this token
                        chai.request(url)
                            .post('/v1/users')
                            .send(addUserRequest)
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403);
                                done();
                            });

                    });
            });
        });
    });
    
    describe('Delete User', function () {

        it('should fail without auth token', function (done) {

            chai.request(url)
                .delete('/v1/users/' + adminUserId)
                .send()
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail for invalid user id', function (done) {

            chai.request(url)
                .put('/v1/users/' + "0")
                .send()
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {
            
            // Add a regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("check_delete_user@domain.com");
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
                    email: 'check_delete_user@domain.com',
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

                        // Try to update the admin user with this token
                        chai.request(url)
                            .delete('/v1/users/' + adminUserId)
                            .send()
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403); 
                                done();
                            });

                    });
            });
        });

        it('should fail to delete own account', function (done) {

            // Add a regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("check_delete_own@domain.com");
            user.setPassword("password");
            user.setWorkingHours(8);
            user.setRoleId(3); // Admin

            // Create a dummy user through model
            var users = new Users(db);
            users.add(user, function (err, usr) {
                should.not.exist(err);
                usr.should.be.a('object');

                // Try to login with it
                var authRequest = {
                    email: 'check_delete_own@domain.com',
                    password: 'password'
                };

                chai.request(url)
                    .post('/v1/auth/signin')
                    .send(authRequest)
                    .end(function (err, res) {

                        res.should.have.status(200);
                        res.body.should.be.a('object');

                        res.body.should.have.property('token');
                        var newAdminAuthToken = res.body.token;


                        res.body.should.have.property('user');
                        res.body.user.should.be.a('object');
                        res.body.user.should.have.property('id');
                        res.body.user.id.should.be.a('number');
                        var newAdminUserId = res.body.user.id;

                        
                        // Try to delete oneself
                        chai.request(url)
                            .delete('/v1/users/' + newAdminUserId)
                            .send()
                            .set('Authorization', 'Bearer ' + newAdminAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403);
                                done();
                            });

                    });
            });
        });

        it('should be able to delete others', function (done) {

            // Add a regular user
            var user = new User();
            user.setName("Admin name");
            user.setEmail("check_delete_other@domain.com");
            user.setPassword("password");
            user.setWorkingHours(8);
            user.setRoleId(3); // Admin

            // Create a dummy user through model
            var users = new Users(db);
            users.add(user, function (err, usr) {
                should.not.exist(err);
                usr.should.be.a('object');

                // Try to login with it
                var authRequest = {
                    email: 'check_delete_other@domain.com',
                    password: 'password'
                };

                chai.request(url)
                    .post('/v1/auth/signin')
                    .send(authRequest)
                    .end(function (err, res) {

                        res.should.have.status(200);
                        res.body.should.be.a('object');

                        res.body.should.have.property('token');
                        var newAdminAuthToken = res.body.token;


                        res.body.should.have.property('user');
                        res.body.user.should.be.a('object');
                        res.body.user.should.have.property('id');
                        res.body.user.id.should.be.a('number');
                        var newAdminUserId = res.body.user.id;
                        res.body.user.should.have.property('role');
                        res.body.user.role.should.be.a('number');
                        res.body.user.role.should.be.eql(3); // Admin
                        var newAdminUserId = res.body.user.id;


                        // Try to delete oneself
                        chai.request(url)
                            .delete('/v1/users/' + newAdminUserId)
                            .send()
                            .set('Authorization', 'Bearer ' + adminAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(200);

                                // Now try to get the deleted user
                                users.get(newAdminUserId, function (err, usr) {
                                    should.not.exist(err);
                                    should.not.exist(usr);
                                    done();
                                });
                            });

                    });
            });
        });
    });

    describe('Get User', function () {

        it('should fail without auth token', function (done) {

            chai.request(url)
                .get('/v1/users/' + adminUserId)
                .send()
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail for invalid user id', function (done) {

            chai.request(url)
                .get('/v1/users/' + "0")
                .send()
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {

            // Add a regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("check_get_user@domain.com");
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
                    email: 'check_get_user@domain.com',
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

                        // Try to add another user with this token
                        chai.request(url)
                            .get('/v1/users/' + adminUserId)
                            .send()
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403);
                                done();
                            });

                    });
            });
        });

        it('should fetch the user data', function (done) {

            chai.request(url)
                .get('/v1/users/' + adminUserId)
                .send()
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(200);
                    res.body.should.be.a('object');

                    res.body.should.have.property('id');
                    res.body.id.should.be.a('number');
                    res.body.should.have.property('email');
                    res.body.email.should.be.eql('admin@domain.com');
                    res.body.should.have.property('name');
                    res.body.name.should.be.eql('System Admin');
                    res.body.should.have.property('role');
                    res.body.role.should.be.a('number');
                    res.body.role.should.be.eql(3);
                    done();
                });
        });
    });

    describe('Update User', function () {

        it('should fail without auth token', function (done) {

            chai.request(url)
                .put('/v1/users/' + adminUserId)
                .send()
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail for invalid user id', function (done) {

            chai.request(url)
                .put('/v1/users/' + "0")
                .send()
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(422);

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {

            // New user data
            var updatedData = {
                email: "manager12@domain.com",
                name: "Scott Tiger",
                working_hours: 12,
                role: 2
            }

            // Add a regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("check_update_user12@domain.com");
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
                    email: 'check_update_user12@domain.com',
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

                        // Try to update the admin user with this token
                        chai.request(url) 
                            .put('/v1/users/' + adminUserId)
                            .send()
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403);
                                done();
                            });

                    });
            });
        });

        it('should fail to change own role', function (done) {

            // New user data
            var updatedData = {
                email: "admin@domain.com",
                name: "System Admin",
                working_hours: 12,
                role: 2
            }

            chai.request(url)
                .put('/v1/users/' + adminUserId)
                .send(updatedData)
                .set('Authorization', 'Bearer ' + adminAuthToken)
                .end(function (err, res) {

                    res.should.have.status(403);
                     
                    done(); 
                });
        });

        it('should allow users to change themselves', function (done) {

            // New user data
            var updatedData = {
                email: "updated_account@domain.com",
                name: "Scott Tiger",
                working_hours: 12
            }

            // Add a regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("check_update_own_account@domain.com");
            user.setPassword("password");
            user.setWorkingHours(8);
            user.setRoleId(1); // ordinary user

            // Create a dummy user through model
            var users = new Users(db);
            users.add(user, function (err, usr) {
                should.not.exist(err);
                usr.should.be.a('object');

                var userId = usr.getId();

                // Try to login with it
                var authRequest = {
                    email: 'check_update_own_account@domain.com',
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

                        // Try to update the admin user with this token
                        chai.request(url)
                            .put('/v1/users/' + userId)
                            .send(updatedData)
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(200);


                                res.body.should.have.property('id');
                                res.body.id.should.be.a('number');
                                res.body.id.should.be.eql(userId);
                                res.body.should.have.property('email');
                                res.body.email.should.be.eql('updated_account@domain.com');
                                res.body.should.have.property('name');
                                res.body.name.should.be.eql('Scott Tiger');
                                res.body.should.have.property('working_hours');
                                res.body.working_hours.should.be.a('number');
                                res.body.working_hours.should.be.eql(12);
                                res.body.should.have.property('role');
                                res.body.role.should.be.a('number');
                                res.body.role.should.be.eql(1);

                                done();
                            });

                    });
            });
        });
    });

    describe('Get All Users', function () {

        it('should fail without auth token', function (done) {

            chai.request(url)
                .get('/v1/users')
                .send()
                .end(function (err, res) {

                    res.should.have.status(401);

                    done();
                });
        });

        it('should fail with insufficient rights', function (done) {

            // Add a regular user
            var user = new User();
            user.setName("User name");
            user.setEmail("check_get_all@domain.com");
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
                    email: 'check_get_all@domain.com',
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

                        // Try to update the admin user with this token
                        chai.request(url)
                            .get('/v1/users')
                            .send()
                            .set('Authorization', 'Bearer ' + userAuthToken)
                            .end(function (err, res) {

                                res.should.have.status(403);
                                done();
                            });

                    });
            });
        });

        it('should be able to get 2 users', function (done) {

            // Cleat the DB
            var users = new Users(db);
            users._deleteAllButAdmin(function (err) {
                if (err) {
                    done(err);
                }
                else {
                    // Add a regular user
                    var user = new User();
                    user.setName("user1");
                    user.setEmail("user1@domain.com");
                    user.setPassword("password");
                    user.setWorkingHours(11);
                    user.setRoleId(3);

                    var users = new Users(db);
                    users.add(user, function (err, usr) {
                        should.not.exist(err);
                        usr.should.be.a('object');

                        // Now there must be two users
                        // Try to update the admin user with this token
                        chai.request(url)
                            .get('/v1/users')
                            .send()
                            .set('Authorization', 'Bearer ' + adminAuthToken)
                            .end(function (err, res) {
                                res.should.have.status(200);
                                res.body.should.be.a('array');
                                res.body.should.have.length(2);
                                res.body[0].should.be.a('object');
                                res.body[0].should.have.property('email');
                                res.body[0].email.should.be.eql('admin@domain.com');

                                res.body[1].should.be.a('object');
                                res.body[1].should.have.property('email');
                                res.body[1].email.should.be.eql('user1@domain.com');
                                
                                for (var i = 0; i < 2; i++) {
                                    res.body[i].should.have.property('id');
                                    res.body[i].id.should.be.a('number');
                                    res.body[i].should.have.property('name');
                                    res.body[i].name.should.be.a('string');
                                    //res.body[i].name.should.be.eql('user1').or.should.be.eql('System Admin');
                                    res.body[i].should.have.property('role');
                                    res.body[i].role.should.be.a('number');
                                    res.body[1].role.should.be.eql(3);
                                }
                                done();
                            });
                    });
                }
            });
        });
    });
});