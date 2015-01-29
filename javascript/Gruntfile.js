/*jslint node: true */
"use strict";
module.exports = function (grunt) {
    grunt.initConfig({
        pkg: grunt.file.readJSON('package.json'),

        clean: {
            build: ['../olad/www/index.html', '../olad/www/js', '../olad/www/css', '../olad/www/img', '../olad/www/libs', '../olad/www/views']
        },

        copy: {
            build: {
                files: [
                    {
                        expand: true,
                        src: 'src/img/*',
                        dest: '../olad/www/img/',
                        flatten: true,
                        filter: 'isFile'
                    },
                    {
                        expand: true,
                        src: 'src/views/*',
                        dest: '../olad/www/views/',
                        flatten: true,
                        filter: 'isFile'
                    },
                  {
                    expand: true,
                    src: 'src/libs/*',
                    dest: '../olad/www/libs/',
                    flatten: true
                  }
                ]
            }
        },

        htmlrefs: {
            index: {
                src: 'src/index.html',
                dest: '../olad/www/index.html'
            }
        },

        bower: {
            dev: {
                options: {
                    targetDir: 'src/libs',
                    install: true,
                    copy: true,
                    cleanup: true,
                    layout: 'byComponent'
                }
            }
        },

        uglify: {
            default: {
                files: {
                    '../olad/www/js/app.min.js': ['../olad/www/js/app.js']
                },
                options: {
                    mangle: true,
                    sourceMap: true,
                    sourceMapName: '../olad/www/js/app.min.js.map',
                }
            },
            libInclude:{
                files:{
                    '../olad/www/js/ola.min.js': ['../olad/www/js/libs.js', '../olad/www/js/app.js']
                },
                options: {
                    mangle: true,
                    sourceMap: true,
                    sourceMapName: '../olad/www/js/ola.min.js.map'
                }
            }
        },

        concat: {
            options: {
                separator: '\n'
            },
            js: {
                src: ['src/js/*.js'],
                dest: '../olad/www/js/app.js'
            },
            css: {
                src: ['src/css/*.css'],
                dest: '../olad/www/css/style.css'
            },
            libs: {
                src: ['src/libs/jquery/js/jquery.js','src/libs/angular/js/angular.js', 'src/libs/angular-route/js/angular-route.js', 'src/libs/bootstrap/js/bootstrap.js'],
                dest: '../olad/www/js/libs.js'
            }
        },

        jshint: {
            dev: ['Gruntfile.js', 'src/js/*.js']
        },

        connect: {
            dev: {
                options: {
                    hostname: '*',
                    port: 8080,
                    base: 'src'
                }
            }
        },

        watch: {
            dev: {
                files: ['Gruntfile.js', 'src/js/*.js', 'src/*.html', 'src/css/*.css'],
                tasks: ['jshint:dev'],
                options: {
                    atBegin: true
                }
            },
            build: {
                files: ['Gruntfile.js', 'src/js/*.js', 'src/*.html', 'src/css/*.css', 'src/views/*.html'],
                tasks: ['jshint:dev', 'htmlrefs:index', 'copy:build', 'concat:js', 'uglify:default', 'concat:css', 'cssmin:default'],
                options: {
                    atBegin: true
                }
            }
        },

        cssmin: {
            default: {
                files: {
                    '../olad/www/css/style.min.css': ['../olad/www/css/style.css']
                }
            }
        }
    });

    grunt.loadNpmTasks('grunt-contrib-jshint');
    grunt.loadNpmTasks('grunt-contrib-clean');
    grunt.loadNpmTasks('grunt-contrib-connect');
    grunt.loadNpmTasks('grunt-contrib-concat');
    grunt.loadNpmTasks('grunt-contrib-uglify');
    grunt.loadNpmTasks('grunt-contrib-watch');
    grunt.loadNpmTasks('grunt-contrib-cssmin');
    grunt.loadNpmTasks('grunt-contrib-copy');
    grunt.loadNpmTasks('grunt-bower-task');
    grunt.loadNpmTasks('grunt-htmlrefs');

    grunt.registerTask('dev-server', ['bower:dev', 'connect:dev', 'watch:dev']);
    grunt.registerTask('build-server', ['connect:build', 'watch:build']);
    grunt.registerTask('build', ['clean:build', 'htmlrefs:index', 'copy:build', 'jshint:dev', 'concat:js', 'uglify:default', 'concat:css', 'cssmin:default']);
};
