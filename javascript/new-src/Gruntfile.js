module.exports = function (grunt) {
 'use strict';
 grunt.initConfig({
  pkg: grunt.file.readJSON('package.json'),
  bower: {
   dev: {
    options: {
     targetDir: 'javascript/new-src/src/libs',
     install: true,
     copy: true,
     cleanup: true,
     layout: 'byComponent'
    }
   }
  },
  uglify: {
   build: {
    files: {
     'olad/www/new/js/app.min.js': ['javascript/new-src/src/js/app.js']
    },
    options: {
     mangle: true,
     sourceMap: true,
     sourceMapName: 'olad/www/new/js/app.min.js.map'
    }
   }
  },
  jshint: {
   dev: ['javascript/new-src/Gruntfile.js', 'javascript/new-src/src/js/app.js'],
   options: {
    jshintrc: true
   }
  },
  watch: {
   build: {
    files: ['javascript/new-src/Gruntfile.js',
     'javascript/new-src/src/js/app.js', 'javascript/new-src/src/index.html',
     'javascript/new-src/src/css/style.css',
     'javascript/new-src/src/views/*.html'],
    tasks: ['jshint:dev', 'uglify:build', 'cssmin:build'],
    options: {
     atBegin: true
    }
   }
  },
  cssmin: {
   build: {
    files: {
     'olad/www/new/css/style.min.css': ['javascript/new-src/src/css/style.css']
    }
   }
  }
 });
 grunt.loadNpmTasks('grunt-contrib-jshint');
 grunt.loadNpmTasks('grunt-contrib-clean');
 grunt.loadNpmTasks('grunt-contrib-concat');
 grunt.loadNpmTasks('grunt-contrib-uglify');
 grunt.loadNpmTasks('grunt-contrib-watch');
 grunt.loadNpmTasks('grunt-contrib-cssmin');
 grunt.loadNpmTasks('grunt-contrib-copy');
 grunt.loadNpmTasks('grunt-bower-task');
 grunt.file.setBase('../../');
 grunt.registerTask('dev', ['watch:build']);
 grunt.registerTask('build', ['jshint:dev', 'uglify:build', 'cssmin:build']);
};
